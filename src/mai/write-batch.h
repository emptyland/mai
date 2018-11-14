#ifndef MAI_WRITE_BATCH_H_
#define MAI_WRITE_BATCH_H_

#include "mai/error.h"
#include <string>
#include <string_view>

namespace mai {
    
class ColumnFamily;
    
class WriteBatch final {
public:
    WriteBatch() {}
    ~WriteBatch();
    
    void Put(ColumnFamily *cf, std::string_view key, std::string_view value);
    void Delete(ColumnFamily *cf, std::string_view key);
    void Clear() { redo_.clear(); }
    
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
        return Iterate(redo_.data(), redo_.size(), handler);
    }
    
    static Error Iterate(const char *buf, size_t len, Stub *handler);
    
    std::string_view redo() const { return redo_; }
    
    WriteBatch(const WriteBatch &) = delete;
    WriteBatch(WriteBatch &&) = delete;
    void operator = (const WriteBatch &) = delete;
private:
    std::string redo_;
};

} // namespace mai


#endif // MAI_WRITE_BATCH_H_
