#pragma once
#ifndef MAI_LANG_HIR_H_
#define MAI_LANG_HIR_H_

#include "base/base.h"
#include "base/arenas.h"

namespace mai {

namespace lang {

class Class;
class Used;
class HValue;
class String;

class HType {
public:
    static constexpr uint32_t kFlagFloating = 1;
    static constexpr uint32_t kFlagUnsinged = 2;
    static constexpr uint32_t kFlagHeapType = 4;

    constexpr HType(const char *name, uint32_t reference_size, uint32_t instance_size, uint32_t flags)
        : name_(name)
        , reference_size_(reference_size)
        , instance_size_(instance_size)
        , flags_(flags) {}

private:
    const char *name_;
    uint32_t reference_size_;
    uint32_t instance_size_;
    uint32_t flags_;
    const Class *clazz_ = nullptr;
}; // class HType

struct HTypes {
    static constexpr HType boolean{"bool", 1, 1, 0};
    static constexpr HType int8{"int8", 1, 1, 0};
    static constexpr HType int16{"int16", 2, 2, 0};
    static constexpr HType int32{"int32", 4, 4, 0};
    static constexpr HType int64{"int64", 8, 8, 0};

    static constexpr HType i8{"i8", 1, 1, 0};
    static constexpr HType u8{"u8", 1, 1, HType::kFlagUnsinged};
    static constexpr HType i16{"i16", 2, 2, 0};
    static constexpr HType u16{"u16", 2, 2, HType::kFlagUnsinged};

    static constexpr HType float32{"float32", 4, 4, HType::kFlagFloating};
    static constexpr HType float64{"float64", 8, 8, HType::kFlagFloating};

    static constexpr HType any{"any", kPointerSize, 0, HType::kFlagHeapType};
}; // struct HTypes


class Used {
public:

private:
    Used *prev_;
    Used *next_;
    HValue *value_;
}; // class Used

class HValue {
public:
    enum Kind {
        kConstant,
        kLoadGlobal,
        kStoreGlobal,
    }; // enum Kind

    static HValue *New(base::Arena *arena, Kind kind, const HType *type, int slot);

    static HValue *New(base::Arena *arena, Kind kind, const HType *type, int slot, HValue *use);

    static HValue *New(base::Arena *arena, Kind kind, const HType *type, int slot, HValue *use1, HValue *user2);
private:
    HValue *prev_;
    HValue *next_;
    Kind kind_;
    int slot_;
    uint32_t flags_;
    int use_size_;
    const HType *type_;
    Used used_dummy_;
    union {
        int32_t int32_value_;
        int64_t int64_value_;
        float   float32_value_;
        double  float64_value_;
        String *string_value_;
        HValue *use_[4];
    };
}; // class HValue

} // namespace lang
    
} // namespace mai


#endif // MAI_LANG_HIR_H_