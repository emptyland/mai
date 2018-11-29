#ifndef MAI_TABLE_BLOCK_ITERATOR_H_
#define MAI_TABLE_BLOCK_ITERATOR_H_

#include "table/table.h"
#include "base/io-utils.h"
#include "mai/iterator.h"
#include <vector>
#include <tuple>

namespace mai {
class RandomAccessFile;
namespace core {
class InternalKeyComparator;
} // namespace core
namespace table {

class BlockIterator : public Iterator {
public:
    BlockIterator(const core::InternalKeyComparator *ikcmp,
                  RandomAccessFile *file, uint64_t block_offset,
                  uint64_t block_size);
    virtual ~BlockIterator();
    
    virtual bool Valid() const override;
    virtual void SeekToFirst() override;
    virtual void SeekToLast() override;
    virtual void Seek(std::string_view target) override;
    virtual void Next() override;
    virtual void Prev() override;
    virtual std::string_view key() const override;
    virtual std::string_view value() const override;
    virtual Error error() const override;
    
    DEF_PTR_SETTER(const core::InternalKeyComparator, ikcmp);

    DISALLOW_IMPLICIT_CONSTRUCTORS(BlockIterator);
private:
    uint64_t PrepareRead(uint64_t i);
    uint64_t Read(std::string_view prev_key, uint64_t offset,
                  std::tuple<std::string, std::string> *kv);
    
    const core::InternalKeyComparator *ikcmp_;
    //RandomAccessFile *file_;
    base::RandomAccessFileReader reader_;
    uint64_t data_base_;
    uint64_t data_end_;
    std::unique_ptr<uint32_t[]> restarts_;
    size_t n_restarts_;
    int64_t curr_restart_;
    int64_t curr_local_;
    std::vector<std::tuple<std::string, std::string>> local_;
    Error error_;
}; // class BlockIterator


    
} // namespace table

    
} // namespace mai


#endif // MAI_TABLE_BLOCK_ITERATOR_H_
