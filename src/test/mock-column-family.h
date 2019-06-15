#ifndef MAI_TEST_MOCK_COLUMN_FAMILY_H_
#define MAI_TEST_MOCK_COLUMN_FAMILY_H_

#include "base/base.h"
#include "mai/db.h"

namespace mai {
    
namespace test {
    
class MockColumnFamily final : public ColumnFamily {
public:
    MockColumnFamily(const std::string &name, uint32_t id, const Comparator *cmp)
        : name_(name)
        , id_(id)
        , cmp_(cmp) {}
    
    virtual ~MockColumnFamily() override {}
    virtual std::string name() const override { return name_; }
    virtual uint32_t id() const override { return id_; }
    virtual const Comparator *comparator() const override { return cmp_; }
    virtual Error GetDescriptor(ColumnFamilyDescriptor *desc) const override {
        return MAI_NOT_SUPPORTED("");
    }
    
private:
    std::string const name_;
    uint32_t const id_;
    const Comparator *const cmp_;
};
    
} // namespace test
    
} // namespace mai

#endif // MAI_TEST_MOCK_COLUMN_FAMILY_H_
