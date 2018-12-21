#include "table/sst-table-reader.h"
#include "table/block-iterator.h"
#include "table/key-bloom-filter.h"
#include "table/block-cache.h"
#include "core/internal-key-comparator.h"
#include "core/key-boundle.h"
#include "base/slice.h"
#include "base/io-utils.h"
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
    
static void LRUHandleCleanup(void *arg0, void */*arg1*/) {
    static_cast<core::LRUHandle *>(arg0)->ReleaseRef();
}
    
class SstTableReader::IteratorImpl : public Iterator {
public:
    IteratorImpl(const core::InternalKeyComparator *ikcmp,
                 Iterator *index_iter, bool checksum_verify,
                 SstTableReader *owns)
        : ikcmp_(DCHECK_NOTNULL(ikcmp))
        , index_iter_(DCHECK_NOTNULL(index_iter))
        , checksum_verify_(checksum_verify)
        , owns_(DCHECK_NOTNULL(owns)) {
    }
    
    virtual ~IteratorImpl() {}
    
    virtual bool Valid() const override {
        return error_.ok() && index_iter_ &&
            index_iter_->Valid() && block_iter_ && block_iter_->Valid();
    }
    
    virtual void SeekToFirst() override {
        index_iter_->SeekToFirst();
        direction_ = kForward;
        
        if (index_iter_->Valid()) {
            BlockHandle bh;
            bh.Decode(index_iter_->value());
            Seek(bh, true);
        }
        SaveKeyIfNeed();
    }
    
    virtual void SeekToLast() override {
        index_iter_->SeekToLast();
        direction_ = kReserve;
        
        if (index_iter_->Valid()) {
            BlockHandle bh;
            bh.Decode(index_iter_->value());
            Seek(bh, false);
        }
        SaveKeyIfNeed();
    }
    
    virtual void Seek(std::string_view target) override {
        direction_ = kForward;
        index_iter_->Seek(target);
        if (!index_iter_->Valid()) {
            return;
        }
        
        BlockHandle bh;
        bh.Decode(index_iter_->value());
        Seek(bh, true);
        block_iter_->Seek(target);
        if (!block_iter_->Valid()) {
            error_ = MAI_NOT_FOUND("Seek()");
        }
        SaveKeyIfNeed();
    }
    
    virtual void Next() override {
        DCHECK(Valid());

        direction_ = kForward;
        block_iter_->Next();

        if (!block_iter_->Valid()) {
            index_iter_->Next();
            if (index_iter_->Valid()) {
                BlockHandle bh;
                bh.Decode(index_iter_->value());
                Seek(bh, true);
            }
        }
        SaveKeyIfNeed();
    }
    
    virtual void Prev() override {
        DCHECK(Valid());
        
        direction_ = kReserve;
        block_iter_->Prev();
        
        if (!block_iter_->Valid()) {
            index_iter_->Prev();
            if (index_iter_->Valid()) {
                BlockHandle bh;
                bh.Decode(index_iter_->value());
                Seek(bh, true);
            }
        }
        SaveKeyIfNeed();
    }
    
    virtual std::string_view key() const override {
        DCHECK(Valid());
        return owns_->table_props_->last_level ? saved_key_ : block_iter_->key();
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
            block_iter(owns_->NewBlockIterator(ikcmp_, handle, checksum_verify_));
        block_iter_.swap(block_iter);
        
        if (to_first) {
            block_iter_->SeekToFirst();
        } else {
            block_iter_->SeekToLast();
        }
    }
    
    void SaveKeyIfNeed() {
        if (Valid() && owns_->table_props_->last_level) {
            saved_key_ = KeyBoundle::MakeKey(block_iter_->key(), 0,
                                             Tag::kFlagValue);
        }
    }
    
    const core::InternalKeyComparator *const ikcmp_;
    const std::unique_ptr<Iterator> index_iter_;
    const bool checksum_verify_;
    SstTableReader *const owns_;
    
    std::unique_ptr<Iterator> block_iter_;
    std::string saved_key_; // For last level sst table.
    Error error_;
    Direction direction_ = kForward;
}; // class SstTableReader::IteratorImpl
    
    
SstTableReader::SstTableReader(RandomAccessFile *file, uint64_t file_number,
                               uint64_t file_size, bool checksum_verify,
                               BlockCache *cache)
    : file_(DCHECK_NOTNULL(file))
    , file_number_(file_number)
    , file_size_(file_size)
    , checksum_verify_(checksum_verify)
    , cache_(DCHECK_NOTNULL(cache)) {}
    
/*virtual*/ SstTableReader::~SstTableReader() {
    cache_->Purge(file_number_);
}
    
#define TRY_RUN0(expr) \
    (expr); \
    if (!reader.error()) { \
        return reader.error(); \
    } (void)0
    
#define TRY_RUN1(expr) \
    rs = (expr); \
    if (!rs) { \
        return rs; \
    } (void)0

Error SstTableReader::Prepare() {
    if (file_size_ < 12) {
        return MAI_CORRUPTION("Incorrect file type. File too small.");
    }

    base::RandomAccessFileReader reader(file_);
    uint32_t magic_num = 0;
    TRY_RUN0(magic_num = reader.ReadFixed32(file_size_ - 4));
    if (magic_num != Table::kSstMagicNumber) {
        return MAI_CORRUPTION("Incorrect file type, required: sst");
    }

    uint64_t position = 0;
    TRY_RUN0(position = reader.ReadFixed64(file_size_ - 12));
    if (position >= file_size_ - 12) {
        return MAI_CORRUPTION("Incorrect table properties position.");
    }
    
    std::string_view result;
    std::string scatch;
    Error rs;
    TRY_RUN0(ReadBlock({position, file_size_ - 12 - position}, &result, &scatch));
    
    table_props_boundle_.reset(new TablePropsBoundle);
    TRY_RUN1(Table::ReadProperties(result, table_props_boundle_->mutable_data()));
    if (table_props_boundle_->data().unordered) {
        return MAI_CORRUPTION("SST table can not be unordered.");
    }
    table_props_ = table_props_boundle_->mutable_data();

    // Indexs:
    if (checksum_verify_) {
        TRY_RUN1(ReadBlock({table_props_->index_position,
                            table_props_->index_size }, &result, &scatch));
    }
    
    // Filter:
    TRY_RUN1(ReadBlock({table_props_->filter_position,
                        table_props_->filter_size}, &result, &scatch));
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
    return new IteratorImpl(ikcmp, NewIndexIterator(ikcmp),
                            read_opts.verify_checksums, this);
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
    std::unique_ptr<Iterator> index_iter(NewIndexIterator(ikcmp));
    
    index_iter->Seek(target);
    if (!index_iter->Valid()) {
        return MAI_NOT_FOUND("Index Seek()");
    }
    
    BlockHandle bh;
    bh.Decode(index_iter->value());

    std::unique_ptr<Iterator> iter(NewBlockIterator(ikcmp, bh,
                                                    read_opts.verify_checksums));
    iter->Seek(target);
    if (!iter->Valid()) {
        return MAI_NOT_FOUND("Data block Seek()");
    }

    ParsedTaggedKey ikey;
    KeyBoundle::ParseTaggedKey(iter->key(), &ikey);
    if (!ikcmp->ucmp()->Equals(ikey.user_key,
                               KeyBoundle::ExtractUserKey(target))) {
        return MAI_NOT_FOUND("Key not seeked!");
    }
    if (tag) {
        *tag = ikey.tag;
    }
    *scratch = iter->value();
    *value = *scratch;

    return Error::OK();
}

/*virtual*/ size_t SstTableReader::ApproximateMemoryUsage() const {
    size_t usage = sizeof(*this);
    usage += (!table_props_boundle_.is_null() ? sizeof(TablePropsBoundle) : 0);
    usage += (!bloom_filter_.is_null() ? bloom_filter_->memory_usage() : 0);
    // TODO:
    return usage;
}

/*virtual*/ base::intrusive_ptr<TablePropsBoundle>
SstTableReader::GetTableProperties() const { return table_props_boundle_; }

/*virtual*/ base::intrusive_ptr<core::KeyFilter>
SstTableReader::GetKeyFilter() const { return bloom_filter_; }
    
Iterator *
SstTableReader::NewIndexIterator(const core::InternalKeyComparator *ikcmp) {
    if (!table_props_) {
        return Iterator::AsError(MAI_CORRUPTION("Table reader not prepared!"));
    }
    
    base::intrusive_ptr<core::LRUHandle> handle;
    Error rs = cache_->GetOrLoad(file_, file_number_,
                                 table_props_->index_position,
                                 table_props_->index_size , checksum_verify_,
                                 &handle);
    if (!rs) {
        return Iterator::AsError(rs);
    }
    
    auto iter = new BlockIterator(ikcmp, handle->value,
                                  table_props_->index_size  - 4);
    handle->AddRef();
    iter->RegisterCleanup(LRUHandleCleanup, handle.get());
    return iter;
}
    
Iterator *
SstTableReader::NewBlockIterator(const core::InternalKeyComparator *ikcmp,
                                 BlockHandle bh, bool checksum_verify) {
    if (!table_props_) {
        return Iterator::AsError(MAI_CORRUPTION("Table reader not prepared!"));
    }
    
    base::intrusive_ptr<core::LRUHandle> handle;
    Error rs = cache_->GetOrLoad(file_, file_number_, bh.offset(), bh.size(),
                                 checksum_verify, &handle);
    if (!rs) {
        return Iterator::AsError(rs);
    }
    
    auto iter = new BlockIterator(ikcmp, handle->value, bh.size() - 4);
    handle->AddRef();
    iter->RegisterCleanup(LRUHandleCleanup, handle.get());
    return iter;
}
    
void SstTableReader::TEST_PrintAll(const core::InternalKeyComparator *ikcmp) {
    std::unique_ptr<Iterator> index_iter(NewIndexIterator(ikcmp));
    
    int index = 0;
    for (index_iter->SeekToFirst(); index_iter->Valid(); index_iter->Next()) {
        printf("----------index:[%d]---------\n", index++);
        
        BlockHandle bh;
        bh.Decode(index_iter->value());
        
        Iterator *iter = NewBlockIterator(ikcmp, bh, true);
        for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
            std::string key(KeyBoundle::ExtractUserKey(iter->key()));
            printf("key: %s\n", key.c_str());
        }
        delete iter;
    }
}
    
Error SstTableReader::ReadBlock(const BlockHandle &bh, std::string_view *result,
                                std::string *scatch) const {
    Error rs = file_->Read(bh.offset(), bh.size(), result, scatch);
    if (!rs) {
        return rs;
    }
    if (checksum_verify_) {
        uint32_t checksum = *reinterpret_cast<const uint32_t *>(result->data());
        
        if (checksum != ::crc32(0, result->data() + 4, result->size() - 4)) {
            return MAI_IO_ERROR("CRC32 checksum fail!");
        }
    }
    result->remove_prefix(4); // remove crc32 checksum.
    return Error::OK();
}

} // namespace table

} // namespace mai
