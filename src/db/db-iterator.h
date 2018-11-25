#ifndef MAI_DB_DB_ITERATOR_H_
#define MAI_DB_DB_ITERATOR_H_

#include "core/key-boundle.h"
#include "mai/iterator.h"
#include "glog/logging.h"

namespace mai {
class Comparator;
namespace db {
    
class DBIterator final : public Iterator {
public:
    DBIterator(const Comparator *ucmp, Iterator *iter,
               core::SequenceNumber last_sequence_number)
        : ucmp_(DCHECK_NOTNULL(ucmp))
        , iter_(DCHECK_NOTNULL(iter))
        , last_sequence_number_(last_sequence_number) {}
    
    virtual ~DBIterator();

    virtual bool Valid() const override;
    virtual void SeekToFirst() override;
    virtual void SeekToLast() override;
    virtual void Seek(std::string_view target) override;
    virtual void Next() override;
    virtual void Prev() override;
    virtual std::string_view key() const override;
    virtual std::string_view value() const override;
    virtual Error error() const override;
    
    void ClearSavedValue() {
        if (saved_value_.capacity() > 1048576) {
            std::string empty;
            swap(empty, saved_value_);
        } else {
            saved_value_.clear();
        }
    }

    static void SaveKey(std::string_view raw, std::string *key) {
        key->assign(raw.data(), raw.size());
    }
    
private:
    void FindNextUserEntry(bool skipping, std::string *skip);
    void FindPrevUserEntry();
    
    const Comparator *const ucmp_;
    std::unique_ptr<Iterator> iter_;
    const core::SequenceNumber last_sequence_number_;
    
    Error error_;
    std::string saved_key_;
    std::string saved_value_;
    Direction direction_ = kForward;
    bool valid_ = false;
}; // class DBIterator

    
} // namespace db
    
} // namespace mai


#endif // MAI_DB_DB_ITERATOR_H_
