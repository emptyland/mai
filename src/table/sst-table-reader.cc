#include "table/sst-table-reader.h"
#include "table/block-iterator.h"
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
    
class SstTableReader::IteratorImpl : public Iterator {
public:
    IteratorImpl(const core::InternalKeyComparator *ikcmp, bool verify_checksums,
                 SstTableReader *reader)
    : ikcmp_(DCHECK_NOTNULL(ikcmp))
    , verify_checksums_(verify_checksums)
    , reader_(DCHECK_NOTNULL(reader)) {}
    
    virtual ~IteratorImpl() {}
    
    virtual bool Valid() const override {
//        return error_.ok() &&
//        (block_idx_ >= 0 && block_idx_ < reader_->indexs_.size()) &&
//        block_iter_ && block_iter_->Valid();
        return false;
    }
    
    virtual void SeekToFirst() override {
//        block_idx_ = 0;
//        direction_ = kForward;
//        Seek(reader_->indexs_[block_idx_], true);
    }
    
    virtual void SeekToLast() override {
//        block_idx_ = reader_->indexs_.size() - 1;
//        direction_ = kReserve;
//        if (block_idx_ >= 0) {
//            Seek(reader_->indexs_[block_idx_], false);
//        }
    }
    
    virtual void Seek(std::string_view target) override {
//        direction_ = kForward;
//        block_idx_ = -1;
//
//        int64_t found_idx = 0;
//        std::string scratch;
//        std::string_view result;
//        for (int64_t i = reader_->indexs_.size() - 1; i >= 0; --i) {
//            BlockHandle entry = reader_->indexs_[i];
//            error_ = reader_->GetFirstKey(entry, &result, &scratch);
//            if (error_.fail()) {
//                return;
//            }
//            int rv = ikcmp_->Compare(result, target);
//            if (rv < 0) {
//                found_idx = (i <= 0) ? 0 : i - 1;
//                break;
//            }
//        }
//
//        for (int64_t i = found_idx; i < reader_->indexs_.size(); ++i) {
//
//            BlockHandle entry = reader_->indexs_[i];
//            error_ = reader_->GetFirstKey(entry, &result, &scratch);
//            if (error_.fail()) {
//                return;
//            }
//            Seek(entry, true);
//            block_idx_ = i;
//            block_iter_->Seek(target);
//            if (block_iter_->Valid()) {
//                return;
//            }
//        }
//        error_ = MAI_NOT_FOUND("Seek()");
    }
    
    virtual void Next() override {
//        DCHECK(Valid());
//
//        direction_ = kForward;
//        block_iter_->Next();
//
//        if (!block_iter_->Valid()) {
//
//            // test not eof
//            if (block_idx_ < reader_->indexs_.size() - 1) {
//                BlockHandle handle = reader_->indexs_[++block_idx_];
//                Seek(handle, true);
//            } else {
//                block_idx_++;
//            }
//        }
    }
    
    virtual void Prev() override {
//        DCHECK(Valid());
//        
//        direction_ = kReserve;
//        block_iter_->Prev();
//        
//        if (!block_iter_->Valid()) {
//            
//            if (block_idx_ > 0) {
//                BlockHandle handle = reader_->indexs_[--block_idx_];
//                Seek(handle, false);
//            } else {
//                block_idx_--;
//            }
//        }
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
        
        std::unique_ptr<Iterator>
        block_iter(new BlockIterator(ikcmp_, reader_->file_, handle.offset() + 4,
                                     handle.size() - 4));
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
    BlockHandle index_handle(table_props_->index_position + 4,
                             table_props_->index_count - 4);
    const uint32_t index_checksum = Slice::SetU32(result);
    if (checksum_verify_) {
        rs = file_->Read(index_handle.offset(), index_handle.size(),
                         &result, &scratch);
        if (!rs) {
            return rs;
        }
        uint32_t checksum = ::crc32(0, result.data(), result.size());
        if (checksum != index_checksum) {
            return MAI_IO_ERROR("Incorrect index block checksum!");
        }
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
        uint32_t checksum = ::crc32(0, result.data(), result.size());
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
    EnsureIndexReady(ikcmp);
    
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
    EnsureIndexReady(ikcmp);
    
    index_iter_->Seek(target);
    if (!index_iter_->Valid()) {
        return MAI_NOT_FOUND("Index Seek()");
    }
    
    BlockHandle bh;
    bh.Decode(index_iter_->value());

    BlockIterator iter(ikcmp, file_, bh.offset() + 4, bh.size() - 4);
    iter.Seek(target);
    if (!iter.Valid()) {
        return MAI_NOT_FOUND("Data block Seek()");
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

void SstTableReader::EnsureIndexReady(const core::InternalKeyComparator *ikcmp) {
    if (!index_iter_) {
        index_iter_.reset(new BlockIterator(ikcmp, file_,
                                            table_props_->index_position + 4,
                                            table_props_->index_count - 4));
    } else {
        index_iter_->set_ikcmp(ikcmp);
    }
}
    
void SstTableReader::TEST_PrintAll(const core::InternalKeyComparator *ikcmp) {
    EnsureIndexReady(ikcmp);
    
    int index = 0;
    for (index_iter_->SeekToFirst(); index_iter_->Valid(); index_iter_->Next()) {
        printf("----------index:[%d]---------\n", index++);
        
        BlockHandle bh;
        bh.Decode(index_iter_->value());
        
        BlockIterator iter(ikcmp, file_, bh.offset() + 4, bh.size() - 4);
        for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
            std::string key(KeyBoundle::ExtractUserKey(iter.key()));
            printf("key: %s\n", key.c_str());
        }
    }
}

} // namespace table

} // namespace mai
