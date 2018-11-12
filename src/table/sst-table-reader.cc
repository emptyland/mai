#include "table/sst-table-reader.h"
#include "table/key-bloom-filter.h"
#include "core/internal-key-comparator.h"
#include "core/key-boundle.h"
#include "base/slice.h"
#include "mai/env.h"
#include "mai/iterator.h"
#include "mai/options.h"
#include "glog/logging.h"

namespace mai {

namespace table {
    
using base::Slice;
using base::Varint32;
using base::Varint64;
using core::KeyBoundle;
using core::ParsedTaggedKey;
using core::Tag;
    
class BlockIterator : public Iterator {
public:
    BlockIterator(const core::InternalKeyComparator *ikcmp,
                  RandomAccessFile *file,
                  BlockHandle handle)
        : ikcmp_(DCHECK_NOTNULL(ikcmp))
        , file_(DCHECK_NOTNULL(file))
        , data_base_(handle.offset()) {
        std::string_view result;
        std::string scratch;
            
        uint64_t offset = handle.offset() + handle.size() - 4;
        error_ = file_->Read(offset, 4, &result, &scratch);
        if (error_.fail()) {
            return;
        }
        n_restarts_ = Slice::SetU32(result);
        DCHECK_GT(n_restarts_, 0);
        
        restarts_.reset(new uint32_t[n_restarts_]);
        offset -= n_restarts_ * 4;
        error_ = file_->Read(offset, n_restarts_ * 4, &result, &scratch);
        if (error_.fail()) {
            return;
        }
        data_end_ = offset;
        memcpy(restarts_.get(), result.data(), result.size());
    }
    
    virtual ~BlockIterator() {}
    
    virtual bool Valid() const override {
        return error_.ok() &&
            (curr_restart_ >= 0 && curr_restart_ < n_restarts_) &&
            (curr_local_ >= 0 && curr_local_ < local_.size());
    }

    virtual void SeekToFirst() override {
        PrepareRead(0);
        curr_local_   = 0;
        curr_restart_ = 0;
    }
    
    virtual void SeekToLast() override {
        PrepareRead(n_restarts_ - 1);
        curr_local_   = static_cast<int64_t>(local_.size()) - 1;
        curr_restart_ = static_cast<int64_t>(n_restarts_) - 1;
    }
    
    virtual void Seek(std::string_view target) override {
        bool found = false;
        int32_t i;
        std::tuple<std::string, std::string> kv;
        for (i = static_cast<int32_t>(n_restarts_) - 1; i >= 0; i--) {
            uint64_t offset = data_base_ + restarts_[i];
            
            Read("", offset, &kv);
            if (ikcmp_->Compare(target, std::get<0>(kv)) >= 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            i = 0;
        }
        
        for (; i < static_cast<int32_t>(n_restarts_); i++) {
            PrepareRead(i);
            for (auto j = 0; j < local_.size(); j++) {
                if (ikcmp_->Compare(target, std::get<0>(local_[j])) <= 0) {
                    curr_local_   = j;
                    curr_restart_ = i;
                    return;
                }
            }
        }
        
        error_ = MAI_NOT_FOUND("Seek()");
    }
    
    virtual void Next() override {
        if (curr_local_ >= local_.size() - 1) {
            if (curr_restart_ < n_restarts_ - 1) {
                PrepareRead(++curr_restart_);
            } else {
                ++curr_restart_;
            }
            curr_local_ = 0;
            return;
        }
        
        curr_local_++;
    }

    virtual void Prev() override {
        if (curr_local_ == 0) {
            if (curr_restart_ > 0) {
                PrepareRead(--curr_restart_);
            } else {
                --curr_restart_;
            }
            curr_local_ = static_cast<int64_t>(local_.size()) - 1;
            return;
        }
        
        --curr_local_;
    }
    
    virtual std::string_view key() const override {
        return std::get<0>(local_[curr_local_]);
    }
    virtual std::string_view value() const override {
        return std::get<1>(local_[curr_local_]);
    }
    
    virtual Error error() const override { return error_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(BlockIterator);
private:
    uint64_t PrepareRead(uint64_t i) {
        DCHECK_LT(i, n_restarts_);
        
        uint64_t offset = data_base_ + restarts_[i];
        uint64_t end    = (i == n_restarts_ - 1) ? data_end_ : data_base_ + restarts_[i + 1];
        
        std::tuple<std::string, std::string> kv;
        std::string last_key;
        local_.clear();
        while (offset < end) {
            offset = Read(last_key, offset, &kv);
            if (error_.fail()) {
                return 0;
            }
            last_key = std::get<0>(kv);
            local_.push_back(kv);
        }
        return offset;
    }
    
    uint64_t Read(std::string_view prev_key, uint64_t offset,
                  std::tuple<std::string, std::string> *kv) {
        std::string_view result;
        std::string scratch;
        
        // shared len
        error_ = file_->Read(offset, Varint64::kMaxLen, &result, &scratch);
        if (error_.fail()) {
            return 0;
        }
        size_t varint_len;
        uint64_t shared_len = Varint64::Decode(result.data(), &varint_len);
        offset += varint_len;
        
        std::string key(prev_key.substr(0, shared_len));
        
        // private len
        error_ = file_->Read(offset, Varint64::kMaxLen, &result, &scratch);
        if (error_.fail()) {
            return 0;
        }
        uint64_t private_len = Varint64::Decode(result.data(), &varint_len);
        offset += varint_len;
        
        // key body
        error_ = file_->Read(offset, private_len, &result, &scratch);
        if (error_.fail()) {
            return 0;
        }
        offset += result.size();
        key.append(result);
        
        // value len
        error_ = file_->Read(offset, Varint64::kMaxLen, &result, &scratch);
        if (error_.fail()) {
            return 0;
        }
        uint64_t value_len = Varint64::Decode(result.data(), &varint_len);
        offset += varint_len;
        
        // value body
        error_ = file_->Read(offset, value_len, &result, &scratch);
        if (error_.fail()) {
            return 0;
        }
        offset += result.size();
        
        std::get<0>(*kv) = key;
        std::get<1>(*kv) = result;
        return offset;
    }
    
    const core::InternalKeyComparator *const ikcmp_;
    RandomAccessFile *file_;
    uint64_t data_base_;
    uint64_t data_end_;
    std::unique_ptr<uint32_t[]> restarts_;
    size_t n_restarts_;
    int64_t curr_restart_;
    int64_t curr_local_;
    std::vector<std::tuple<std::string, std::string>> local_;
    Error error_;
}; // class BlockIterator
    

class SstTableReader::IteratorImpl : public Iterator {
public:
    IteratorImpl(const core::InternalKeyComparator *ikcmp, bool verify_checksums,
                 SstTableReader *reader)
        : ikcmp_(DCHECK_NOTNULL(ikcmp))
        , verify_checksums_(verify_checksums)
        , reader_(DCHECK_NOTNULL(reader)) {}

    virtual ~IteratorImpl() {}
    
    virtual bool Valid() const override {
        return error_.ok() &&
            (block_idx_ >= 0 && block_idx_ < reader_->indexs_.size()) &&
            block_iter_ && block_iter_->Valid();
    }

    virtual void SeekToFirst() override {
        block_idx_ = 0;
        direction_ = kForward;
        Seek(reader_->indexs_[block_idx_], true);
    }
    
    virtual void SeekToLast() override {
        block_idx_ = reader_->indexs_.size() - 1;
        direction_ = kReserve;
        if (block_idx_ >= 0) {
            Seek(reader_->indexs_[block_idx_], false);
        }
    }
    
    virtual void Seek(std::string_view target) override {
        direction_ = kForward;
        block_idx_ = -1;
        
        int64_t found_idx = 0;
        std::string scratch;
        std::string_view result;
        for (int64_t i = reader_->indexs_.size() - 1; i >= 0; --i) {
            BlockHandle entry = reader_->indexs_[i];
            error_ = reader_->GetFirstKey(entry, &result, &scratch);
            if (error_.fail()) {
                break;
            }
            int rv = ikcmp_->Compare(result, target);
            if (rv < 0) {
                found_idx = (i <= 0) ? 0 : i - 1;
                break;
            }
        }
        
        for (int64_t i = 0; i < reader_->indexs_.size(); ++i) {
            
            BlockHandle entry = reader_->indexs_[i];
            error_ = reader_->GetFirstKey(entry, &result, &scratch);
            if (error_.fail()) {
                return;
            }
            Seek(entry, true);
            block_idx_ = i;
            block_iter_->Seek(target);
            if (block_iter_->Valid()) {
                return;
            }
        }
        error_ = MAI_NOT_FOUND("Seek()");
    }
    
    virtual void Next() override {
        DCHECK(Valid());
        
        direction_ = kForward;
        block_iter_->Next();
        
        if (!block_iter_->Valid()) {
            
            // test not eof
            if (block_idx_ < reader_->indexs_.size() - 1) {
                BlockHandle handle = reader_->indexs_[++block_idx_];
                Seek(handle, true);
            } else {
                block_idx_++;
            }
        }
    }

    virtual void Prev() override {
        DCHECK(Valid());
        
        direction_ = kReserve;
        block_iter_->Prev();
        
        if (!block_iter_->Valid()) {
            
            if (block_idx_ > 0) {
                BlockHandle handle = reader_->indexs_[--block_idx_];
                Seek(handle, false);
            } else {
                block_idx_--;
            }
        }
    }
    
    virtual std::string_view key() const override {
        DCHECK(Valid());
        return block_iter_->key();
    }

    virtual std::string_view value() const override {
        DCHECK(Valid());
        return block_iter_->value();
    }
    
    virtual Error error() const override { return error_; }
    
    DISALLOW_IMPLICIT_CONSTRUCTORS(IteratorImpl);
private:
    void Seek(BlockHandle handle, bool to_first) {
        
        BlockHandle bh(handle.offset() + 4, handle.size() - 4);
        std::unique_ptr<Iterator> block_iter(new BlockIterator(ikcmp_,
                                                               reader_->file_,
                                                               bh));
        block_iter_.swap(block_iter);
        
        if (to_first) {
            block_iter_->SeekToFirst();
        } else {
            block_iter_->SeekToLast();
        }
    }
    
    
    const core::InternalKeyComparator *const ikcmp_;
    const bool verify_checksums_;
    SstTableReader *reader_;
    std::unique_ptr<Iterator> block_iter_;
    int64_t block_idx_;
    Error error_;
    Direction direction_ = kForward;
}; // class SstTableReader::IteratorImpl
    
    
SstTableReader::SstTableReader(RandomAccessFile *file, uint64_t file_size,
                               bool checksum_verify)
    : file_(DCHECK_NOTNULL(file))
    , file_size_(file_size)
    , checksum_verify_(checksum_verify) {}
    
/*virtual*/ SstTableReader::~SstTableReader() {}

Error SstTableReader::Prepare() {
    if (file_size_ < 12) {
        return MAI_CORRUPTION("Incorrect file type. File too small.");
    }
    
    std::string_view result;
    std::string scratch;
    
    Error rs = file_->Read(file_size_ - 4, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    if (Slice::SetU32(result) != Table::kSstMagicNumber) {
        return MAI_CORRUPTION("Incorrect file type, required: sst");
    }
    
    rs = file_->Read(file_size_ - 12, 8, &result, &scratch);
    if (!rs) {
        return rs;
    }
    uint64_t position = Slice::SetU64(result);
    if (position >= file_size_ - 12) {
        return MAI_CORRUPTION("Incorrect table properties position.");
    }
    
    table_props_.reset(new TableProperties{});
    rs = Table::ReadProperties(file_, &position, table_props_.get());
    if (!rs) {
        return rs;
    }
    
    // Indexs:
    rs = file_->Read(table_props_->index_position, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    const uint32_t index_checksum = Slice::SetU32(result);
    
    uint32_t checksum = 0;
    int64_t count = table_props_->index_count;
    position = table_props_->index_position + 4;
    while (count--) {
        rs = file_->Read(position, Varint64::kMaxLen, &result, &scratch);
        if (!rs) {
            return rs;
        }
        size_t varint_len;
        uint64_t offset = Varint64::Decode(result.data(), &varint_len);
        position += varint_len;
        checksum = ::crc32(checksum, result.data(), varint_len);
        
        rs = file_->Read(position, Varint64::kMaxLen, &result, &scratch);
        if (!rs) {
            return rs;
        }
        uint64_t size = Varint64::Decode(result.data(), &varint_len);
        position += varint_len;
        checksum = ::crc32(checksum, result.data(), varint_len);
        
        indexs_.push_back(BlockHandle{offset, size});
    }
    
    if (checksum_verify_ && index_checksum != checksum) {
        return MAI_IO_ERROR("Incorrect index block checksum!");
    }
    
    // Filter:
    rs = file_->Read(table_props_->filter_position, 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    const uint32_t filter_checksum = Slice::SetU32(result);
    rs = file_->Read(table_props_->filter_position + 4,
                     table_props_->filter_size - 4, &result, &scratch);
    if (!rs) {
        return rs;
    }
    if (checksum_verify_) {
        checksum = ::crc32(0, result.data(), result.size());
        if (filter_checksum != checksum) {
            return MAI_IO_ERROR("Incorrect filter block checksum!");
        }
    }
    core::KeyFilter *filter
        = new KeyBloomFilter(reinterpret_cast<const uint32_t *>(result.data()),
                             result.size() / 4,
                             base::Hash::kBloomFilterHashs,
                             base::Hash::kNumberBloomFilterHashs);
    bloom_filter_.reset(filter);
    return Error::OK();
}

/*virtual*/ Iterator *
SstTableReader::NewIterator(const ReadOptions &read_opts,
            const core::InternalKeyComparator *ikcmp) {
    if (!table_props_) {
        return Iterator::AsError(MAI_CORRUPTION("Table reader not prepared!"));
    }
    if (indexs_.empty()) {
        return Iterator::AsError(MAI_CORRUPTION("Table reader not prepared! Can not find any index."));
    }
    
    return new IteratorImpl(ikcmp, read_opts.verify_checksums, this);
}

/*virtual*/ Error SstTableReader::Get(const ReadOptions &read_opts,
                  const core::InternalKeyComparator *ikcmp,
                  std::string_view target,
                  core::Tag *tag,
                  std::string_view *value,
                  std::string *scratch) {
    if (!table_props_) {
        return MAI_CORRUPTION("Table reader not prepared!");
    }
    if (indexs_.empty()) {
        return MAI_CORRUPTION("Table reader not prepared! Can not find any index.");
    }
    
    IteratorImpl iter(ikcmp, read_opts.verify_checksums, this);
    iter.Seek(target);
    if (!iter.Valid()) {
        return MAI_NOT_FOUND("Seek()");
    }
    ParsedTaggedKey ikey;
    KeyBoundle::ParseTaggedKey(iter.key(), &ikey);
    if (!ikcmp->ucmp()->Equals(ikey.user_key,
                               KeyBoundle::ExtractUserKey(target))) {
        return MAI_NOT_FOUND("Key not seeked!");
    }
    if (tag) {
        *tag = ikey.tag;
    }
    *scratch = iter.value();
    *value = *scratch;
    
//    std::string_view result;
//    int64_t left = 0, right = indexs_.size() - 1, middle = 0;
//    int rv = 0;
//    while (left <= right) {
//        middle = (right + left) / 2;
//
//        Error rs = GetFirstKey(indexs_[middle], &result, scratch);
//        if (!rs) {
//            return rs;
//        }
//
//        rv = ikcmp->Compare(result, target);
//        if (rv > 0) {
//            right = middle - 1;
//        } else if (rv < 0) {
//            left  = middle + 1;
//        } else {
//            break;
//        }
//    }
//    BlockHandle bh(indexs_[middle].offset() + 4, indexs_[middle].size() - 4);
//    BlockIterator block_iter(ikcmp, file_, bh);
//    if (block_iter.error().fail()) {
//        return block_iter.error();
//    }
//    block_iter.Seek(target);
//    if (!block_iter.Valid()) {
//        return MAI_NOT_FOUND("Get()");
//    }
//
//    ParsedTaggedKey ikey;
//    KeyBoundle::ParseTaggedKey(block_iter.key(), &ikey);
//    if (!ikcmp->ucmp()->Equals(ikey.user_key,
//                               KeyBoundle::ExtractUserKey(target))) {
//        return MAI_NOT_FOUND("Key not seeked!");
//    }
//    if (tag) {
//        *tag = ikey.tag;
//    }
//    *scratch = block_iter.value();
//    *value = *scratch;

    return Error::OK();
}

/*virtual*/ size_t SstTableReader::ApproximateMemoryUsage() const {
    size_t usage = sizeof(*this);
    usage += (table_props_ ? sizeof(TableProperties) : 0);
    usage += (bloom_filter_ ? bloom_filter_->memory_usage() : 0);
    // TODO:
    return usage;
}

/*virtual*/ std::shared_ptr<TableProperties>
SstTableReader::GetTableProperties() const { return table_props_; }

/*virtual*/ std::shared_ptr<core::KeyFilter>
SstTableReader::GetKeyFilter() const { return bloom_filter_; }
    
    
Error SstTableReader::GetFirstKey(BlockHandle handle, std::string_view *result,
                                  std::string *scratch) {
    BlockHandle range(handle.offset() + 4, handle.size() - 4); // Ignore crc32 checksum
    uint64_t offset = range.offset();
    
    Error rs = GetKey("", &offset, result, scratch);
    if (!rs) {
        return rs;
    }
    // TODO: check offset;
    return Error::OK();
}
    
Error SstTableReader::GetKey(std::string_view prev_key, uint64_t *offset,
                             std::string_view *result, std::string *scratch) {
    std::string buf;
    Error rs = file_->Read(*offset, Varint64::kMaxLen, result, &buf);
    if (!rs) {
        return rs;
    }
    size_t varint_len;
    uint64_t shared_len = Varint64::Decode(result->data(), &varint_len);
    (*offset) += varint_len;
    
    if (shared_len > 0) {
        if (prev_key.size() < shared_len) {
            return MAI_CORRUPTION("Prev key length too large or shared length too small.");
        }
        scratch->assign(prev_key.substr(0, shared_len));
    }
    
    rs = file_->Read(*offset, Varint64::kMaxLen, result, &buf);
    if (!rs) {
        return rs;
    }
    uint64_t private_len = Varint64::Decode(result->data(), &varint_len);
    (*offset) += varint_len;
    if (shared_len == 0) {
        rs = file_->Read(*offset, private_len, result, scratch);
    } else {
        rs = file_->Read(*offset, private_len, result, &buf);
    }
    if (!rs) {
        return rs;
    }
    if (shared_len > 0) {
        scratch->append(*result);
        *result = *scratch;
    }
    return Error::OK();
}
    
void SstTableReader::TEST_PrintAll(const core::InternalKeyComparator *ikcmp) {
    for (size_t i = 0; i < indexs_.size(); ++i) {
        printf("------------[%lu]---------\n", i);
        
        BlockHandle bh(indexs_[i].offset() + 4, indexs_[i].size() - 4);
        BlockIterator iter(ikcmp, file_, bh);
        
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            std::string key(KeyBoundle::ExtractUserKey(iter.key()));
            printf("key: %s\n", key.c_str());
        }
    }
}

} // namespace table

} // namespace mai
