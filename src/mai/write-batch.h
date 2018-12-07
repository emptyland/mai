#ifndef MAI_WRITE_BATCH_H_
#define MAI_WRITE_BATCH_H_

#include "mai/error.h"
#include <string>
#include <string_view>

namespace mai {
    
class ColumnFamily;
    
class WriteBatch final {
public:
    // Header: sequence number + number of entries
    static const int kHeaderSize = sizeof(uint64_t) + sizeof(uint32_t);
    
    WriteBatch() { redo_.resize(kHeaderSize, 0); }
    ~WriteBatch();
    
    void Put(ColumnFamily *cf, std::string_view key, std::string_view value);
    void Delete(ColumnFamily *cf, std::string_view key);
    void Clear() {
        redo_.resize(kHeaderSize, 0);
        n_entries_ = 0;
    }
    
    class Stub {
    public:
        Stub() {}
        virtual ~Stub() {}

        virtual void Put(uint32_t cfid, std::string_view key,
                         std::string_view value) = 0;
        virtual void Delete(uint32_t cfid, std::string_view key) = 0;
        
        Stub(const Stub &) = delete;
        Stub(Stub &&) = delete;
        void operator = (const Stub &) = delete;
    }; // class Stub
    
    Error Iterate(Stub *handler) const {
        return Iterate(redo_.data() + kHeaderSize, // Ignore header
                       redo_.size() - kHeaderSize, handler);
    }
    
    static Error Iterate(const char *buf, size_t len, Stub *handler);
    
    std::string_view redo(uint64_t sn = 0) {
        *reinterpret_cast<uint64_t *>(&redo_[0]) = sn;
        *reinterpret_cast<uint32_t *>(&redo_[0] + sizeof(sn)) = n_entries_;
        return redo_;
    }
    uint32_t n_entries() const { return n_entries_; }
    
    WriteBatch(const WriteBatch &) = delete;
    WriteBatch(WriteBatch &&) = delete;
    void operator = (const WriteBatch &) = delete;
private:
    std::string redo_;
    uint32_t n_entries_ = 0;
};

} // namespace mai


#endif // MAI_WRITE_BATCH_H_
