#pragma once
#ifndef MAI_LANG_METADATA_H_
#define MAI_LANG_METADATA_H_

#include "lang/mm.h"
#include "base/base.h"
#include "mai/type-defs.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class Isolate;
class Method;
class Field;
class Class;
class ClassBuilder;
class Type;
class Closure;
class Code;
class Function;
class FunctionBuilder;
class SourceLineInfo;
class BytecodeArray;
class PrototypeDesc;
class MetadataSpace;

class MetadataObject {
public:
    //void *operator new (size_t) = delete;
    void operator delete(void *) = delete;

    DISALLOW_IMPLICIT_CONSTRUCTORS(MetadataObject);
protected:
    MetadataObject() = default;
}; // class MetadataObject

using MDStr = const char *;

// Metadata: type
class Type : public MetadataObject {
public:
    static constexpr uint32_t kPrimitiveTag = 1;
    static constexpr uint32_t kReferenceTag = 2;
    static constexpr uint32_t kBuiltinTag = 1u << 2;
    
    bool is_primitive() const { return tags_ & kPrimitiveTag; }
    bool is_reference() const { return tags_ & kReferenceTag; }
    bool is_void() const { return !(tags_ & (kReferenceTag | kPrimitiveTag)); }
    bool is_builtin() const { return tags_ & kBuiltinTag; }
    bool is_user() const { return !is_builtin(); }
    
    bool IsNumber() const { return IsIntegral() || IsFloating(); }
    bool IsIntegral() const { return IsUnsignedIntegral() || IsSignedIntegral(); }
    bool IsUnsignedIntegral() const;
    bool IsSignedIntegral() const;
    bool IsFloating() const { return id_ == kType_f32 || id_ == kType_f64; }
    bool IsArray() const {
        return id_ == kType_array8 || id_ == kType_array16 || id_ == kType_array32 ||
               id_ == kType_array64 || id_ == kType_array;
    }
    bool IsMap() const {
        return id_ == kType_map8 || id_ == kType_map16 || id_ == kType_map32 || id_ == kType_map64
            || id_ == kType_map;
    }

    DEF_VAL_GETTER(uint32_t, n_fields);
    DEF_VAL_GETTER(uint32_t, id);
    DEF_VAL_GETTER(uint32_t, tags);
    DEF_VAL_GETTER(uint32_t, reference_size);
    DEF_VAL_GETTER(uint32_t, instrance_size);
    DEF_VAL_GETTER(MDStr, name);
    
    inline const Field *field(uint32_t i) const;

    friend class ClassBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Type);
protected:
    Type() = default;

    uint32_t id_ = 0; // Type unique id
    uint32_t tags_ = 0; // Flags
    uint32_t reference_size_ = 0; // Reference size
    uint32_t instrance_size_ = 0; // Instrance size
    MDStr name_ = nullptr; // Name of type
    uint32_t n_fields_ = 0; // Number of fields
    Field *fields_ = nullptr; // All fields
}; // class Type

// Metadata: class
class Class : public Type {
public:
    using Builder = ClassBuilder;
    
    static const int32_t kOffsetNMethods;
    static const int32_t kOffsetMethods;

    DEF_PTR_GETTER(const Class, base);
    DEF_PTR_GETTER(Method, init);
    DEF_VAL_GETTER(uint32_t, n_methods);
    
    inline const Method *method(uint32_t i) const;
    inline void set_method_function(uint32_t i, Closure *fn);
    inline void set_init_function(Closure *fn);
    
    inline bool IsArray() const;
    
    bool IsSameOrBaseOf(const Class *base) const { return IsSameOf(base) || IsBaseOf(base); }
    bool IsSameOf(const Class *base) const { return this == DCHECK_NOTNULL(base); }
    inline bool IsBaseOf(const Class *base) const;
    inline int GetSameOrBaseOf(const Class *base) const;
        
    friend class ClassBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Class);
private:
    Class() = default;
    
    const Class *base_ = nullptr; // Base class
    Method *init_ = nullptr; // [nested strong ref] Constructor method
    uint32_t n_methods_ = 0; // Number of method
    Method *methods_ = nullptr; // [nested strong ref] All methods
}; // class Class

class Field : public MetadataObject {
public:
    static constexpr uint32_t kPublic = 0;
    static constexpr uint32_t kProtected = 1;
    static constexpr uint32_t kPrivate = 2;
    static constexpr uint32_t kPermMask = 0x3;
    
    static constexpr uint32_t kRead = 1u << 2;
    static constexpr uint32_t kWrite = 1u << 3;
    static constexpr uint32_t kVolatile = 1u << 4;
    static constexpr uint32_t kRdWr = kRead | kWrite;
    
    static constexpr uint32_t kArray = 1u << 5;

    DEF_VAL_GETTER(MDStr, name);
    DEF_PTR_GETTER(const Class, type);
    DEF_VAL_GETTER(uint32_t, flags);
    DEF_VAL_GETTER(uint32_t, tag);
    DEF_VAL_GETTER(uint32_t, offset);
    
    uint32_t perm() const { return flags_ & kPermMask; }
    
    bool is_public() const { return perm() == kPublic; }
    bool is_protected() const { return perm() == kProtected; }
    bool is_private() const { return perm() == kPrivate; }
    
    bool can_read() const { return flags_ & kRead; }
    bool can_write() const { return flags_ & kWrite; }
    bool is_readonly() const { return can_read() && !can_write(); }
    bool is_readwrite() const { return flags_ & kRdWr; }
    bool is_volatile() const { return flags_ & kVolatile; }

    friend class ClassBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Field);
private:
    Field() = default;
    
    MDStr name_ = nullptr; // Field name
    const Class *type_ = nullptr; // Type of field
    uint32_t flags_ = 0; // Flags
    uint32_t tag_ = 0; // Tag for serializition
    uint32_t offset_ = 0; // Offset of object address
}; // class Field


// The method function of class
class Method : public MetadataObject {
public:
    static const int32_t kOffsetFunction;
    
    static constexpr uint32_t kKindMask = 3;
    static constexpr uint32_t kNative = 1;
    static constexpr uint32_t kBytecode = 0;
    static constexpr uint32_t kTailAlignBit = 4; // Should alignment tail argument?
    
    DEF_VAL_GETTER(MDStr, name);
    DEF_VAL_GETTER(uint32_t, tags);
    DEF_PTR_GETTER(Closure, fn);
    
    bool is_native() const { return (tags_ & kKindMask) == kNative; }
    bool is_bytecode() const { return (tags_ & kKindMask) == kBytecode; }
    bool should_tail_align() const { return tags_ & kTailAlignBit; }
    
    friend class Class;
    friend class ClassBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Method);
private:
    Method() = default;

    MDStr name_ = nullptr; // Method name
    uint32_t tags_ = 0; // Tag for serializition
    Closure *fn_ = nullptr; // [strong ref]
}; // class Method


class MDStrHeader {
public:
    DEF_VAL_GETTER(uint32_t, length);
    MDStr data() const { return data_; }
    
    static const MDStrHeader *FromStr(MDStr z) {
        return reinterpret_cast<const MDStrHeader *>(z - sizeof(MDStrHeader));
    }
    
    static std::string_view ToStringView(MDStr z) {
        const MDStrHeader *header = FromStr(z);
        return std::string_view(header->data(), header->length());
    }
    
    friend class ClassBuilder;
    friend class MetadataSpace;
private:
    MDStrHeader(const char *z, size_t n)
        : length_(static_cast<uint32_t>(n)){
        ::memcpy(data_, z, n);
        data_[n] = '\0';
    }

    uint32_t length_;
    char data_[0];
}; // class MDStrHeader


// Mai compiled function protocol
class Function : public MetadataObject {
public:
    enum CapturedVarKind {
        IN_STACK,
        IN_CAPTURED,
    }; // enum CpaturedVarKind

    struct CapturedVarDesc {
        MDStr name;
        const Class *type;
        CapturedVarKind kind;
        int32_t index;
    }; // struct CpaturedVarDesc
    
    struct ExceptionHandlerDesc {
        const Class *expected_type = nullptr;
        intptr_t start_pc = -1;
        intptr_t stop_pc = -1;
        intptr_t handler_pc = -1;
    }; // struct ExceptionHandlerDesc
    
    struct InvokingHint {
        intptr_t pc;
        uint32_t flags;
        int stack_ref_top; // for gc
    }; // struct InvokingHint
    
    enum Kind {
        BYTECODE,
        COMPILED,
    }; // enum Kind
    
    using Builder = FunctionBuilder;
    
    static const int32_t kOffsetStackSize;
    static const int32_t kOffsetBytecode;
    static const int32_t kOffsetConstPool;
    static const int32_t kOffsetExceptionTableSize;

    DEF_VAL_GETTER(MDStr, name);
    DEF_PTR_GETTER(PrototypeDesc, prototype);
    DEF_VAL_GETTER(Kind, kind);
    DEF_PTR_GETTER(Code, code);
    DEF_PTR_GETTER(BytecodeArray, bytecode);
    DEF_VAL_GETTER(uint32_t, stack_size);
    DEF_PTR_GETTER(uint32_t, stack_bitmap);
    DEF_PTR_GETTER(SourceLineInfo, source_line_info);
    DEF_PTR_GETTER(Span32, const_pool);
    DEF_VAL_GETTER(uint32_t, captured_var_size);
    DEF_VAL_GETTER(uint32_t, exception_table_size);
    DEF_VAL_GETTER(uint32_t, invoking_hint_size);

    const CapturedVarDesc *captured_var(uint32_t i) const {
        DCHECK_LT(i, captured_var_size());
        return captured_vars_ + i;
    }

    const InvokingHint *invoking_hint(uint32_t i) const {
        DCHECK_LT(i, invoking_hint_size());
        return invoking_hint_ + i;
    }

    uint32_t const_pool_spans_size() const { return const_pool_size_ / sizeof(Span32); }

    uint32_t stack_bitmap_size() const;

    int32_t DispatchException(Any *exception, int32_t pc);
    
    int32_t FindStackRefTop(intptr_t pc) const {
        for (uint32_t i = 0; i < invoking_hint_size_; i++) {
            if (invoking_hint_[i].pc == pc) {
                return invoking_hint_[i].stack_ref_top;
            }
        }
        return -1;
    }
    
    bool TestStackBitmap(int offset) const {
        DCHECK_LT(offset, stack_size_);
        int index = offset / sizeof(Span16);
        return stack_bitmap_[index / 32] & (1u << (index % 32));
    }
    
    bool TestConstBitmap(int index) const {
        DCHECK_LT(index, const_pool_spans_size());
        return const_pool_bitmap_[index / 32] & (1u << (index % 32));
    }

    void Print(base::AbstractPrinter *output) const;
    
    //bool Verify(base::AbstractPrinter *feedback, Isolate *isolate) const;

    friend class FunctionBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Function);
private:
    Function() = default;

    MDStr name_ = nullptr; // Function full name
    PrototypeDesc *prototype_ = nullptr; // Function prototype description
    Kind kind_ = BYTECODE; // Kind of function
    uint32_t stack_size_ = 0; // Stack size for invoking frame
    uint32_t *stack_bitmap_ = nullptr; // Stack spans bitmap for GC
    uint32_t const_pool_size_ = 0; // Bytes of constants value pool
    Span32 *const_pool_ = nullptr; // [nested strong ref] Constants value pool
    uint32_t *const_pool_bitmap_ = nullptr; // Constants value pool spans bitmap for GC
    SourceLineInfo *source_line_info_ = nullptr; // Source info for debug and backtrace
    Code *code_ = nullptr; // Native code
    BytecodeArray *bytecode_ = nullptr; // Bytecodes or native code
    uint32_t captured_var_size_ = 0;
    CapturedVarDesc *captured_vars_ = nullptr; // Capture variable for closure
    uint32_t exception_table_size_ = 0;
    ExceptionHandlerDesc *exception_table_ = nullptr; // Exception handler table
    uint32_t invoking_hint_size_ = 0;
    InvokingHint *invoking_hint_ = nullptr; // Invoking hint information
}; // class Function


// Machine code object
class Code : public MetadataObject {
public:
    enum Kind {
        ENTRY,
        STUB,
        BUILTIN,
        HANDLER,
    };
    
    static const int32_t kOffsetEntry;
    
    DEF_VAL_GETTER(Kind, kind);
    DEF_VAL_GETTER(uint32_t, size);
    DEF_PTR_GETTER(PrototypeDesc, prototype);

    Address entry() { return instructions_; }
    
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Code);
private:
    Code(Kind kind, Address instructions, uint32_t size, PrototypeDesc *prototype)
        : kind_(kind)
        , size_(size)
        , prototype_(prototype) {
        ::memcpy(instructions_, instructions, size);
    }
    
    Kind kind_; // Kind of code
    uint32_t size_; // Bytes size of instructions
    PrototypeDesc *prototype_; // Only STUB kind has prototype_ field.
    uint8_t instructions_[0]; // Native instructions
}; // class Code


// Interpreter bytecode array object
class BytecodeArray : public MetadataObject {
public:
    static const int32_t kOffsetEntry;
    
    DEF_VAL_GETTER(uint32_t, size);
    
    BytecodeInstruction *entry() { return instructions_; }

    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(BytecodeArray);
private:
    BytecodeArray(const BytecodeInstruction *instructions, uint32_t size)
        : size_(size) {
        ::memcpy(instructions_, instructions, sizeof(BytecodeInstruction) * size);
    }

    uint32_t size_; // Number of instructions
    BytecodeInstruction instructions_[0]; // Bytecode instructions
}; // class BytecodeArray


class SourceLineInfo : public MetadataObject {
public:
    struct Range {
        intptr_t start_pc; // Start pc (contains)
        intptr_t stop_pc; // Stop pc (not contains)
        int line; // Source line number
    }; // struct Range
    
    enum Kind {
        RANGE,
        SIMPLE,
    };
    
    DEF_VAL_GETTER(MDStr, file_name);
    DEF_VAL_GETTER(Kind, kind);
    DEF_VAL_GETTER(uint32_t, length);
    
    bool range_format() const { return kind() == RANGE; }
    bool simple_format() const { return kind() == SIMPLE; }
    
    const Range *range(uint32_t i) const {
        DCHECK_LT(i, length_);
        return ranges() + i;
    }
    
    const Range *ranges() const {
        DCHECK(range_format());
        return reinterpret_cast<const Range *>(this + 1);
    }
    
    int source_line(intptr_t pc) const {
        DCHECK_GE(pc, 0);
        DCHECK_LT(pc, length_);
        return source_lines()[pc];
    }

    const int *source_lines() const {
        DCHECK(simple_format());
        return reinterpret_cast<const int *>(this + 1);
    }
    
    inline int FindSourceLine(intptr_t pc) const;
    
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(SourceLineInfo);
private:
    SourceLineInfo(MDStr file_name, const Range *ranges, size_t n)
        : file_name_(file_name)
        , kind_(RANGE)
        , length_(static_cast<uint32_t>(n)) {
        ::memcpy(this + 1, ranges, sizeof(ranges[0]) * length_);
    }

    SourceLineInfo(MDStr file_name, const int *lines, size_t n)
        : file_name_(file_name)
        , kind_(SIMPLE)
        , length_(static_cast<uint32_t>(n)) {
        ::memcpy(this + 1, lines, sizeof(lines[0]) * length_);
    }

    MDStr file_name_; // Source code file name
    Kind kind_; // Kind of source-line-info format
    uint32_t length_; // Length of line info
}; // class SourceLineInfo


// Function prototype description
class PrototypeDesc : public MetadataObject {
public:
    DEF_VAL_GETTER(uint32_t, return_type);
    DEF_VAL_GETTER(uint32_t, parameter_size);
    
    uint32_t parameter(uint32_t i) const {
        DCHECK_LT(i, parameter_size_);
        return parameters_[i];
    }

    // Has variable arguments?
    bool has_vargs() const { return vargs_; }
    
    size_t GetParametersPlacedSize() const;
    
    size_t GetParametersPlacedSize(const MetadataSpace *space) const {
        return RoundUp(GetParametersRealSize(space), kStackAligmentSize);
    }
    
    size_t GetParametersRealSize(const MetadataSpace *space) const;
    
    uint32_t HashCode() const;
    
    bool IsSameOf(const PrototypeDesc *proto) const;
    
    bool IsEqualOf(const PrototypeDesc *proto) const;

    // To readable string
    std::string ToString() const;

    std::string ToString(const MetadataSpace *space) const;

    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(PrototypeDesc);
private:
    PrototypeDesc(const uint32_t *parameters, uint32_t parameter_size, int vargs,
                  uint32_t return_type)
        : return_type_(return_type)
        , parameter_size_(parameter_size)
        , vargs_(vargs) {
        ::memcpy(parameters_, parameters, sizeof(parameters_[0]) * parameter_size);
    }

    uint32_t return_type_; // Return type id
    uint32_t parameter_size_; // Number of parameters
    int vargs_; // Has variable arguments?
    uint32_t parameters_[0]; // Type id of parameters
}; // class PrototypeDesc


template <class T, class E = Code>
class CallStub {
public:
    CallStub(E *code): code_(DCHECK_NOTNULL(code)) {}
    
    T *entry() const { return reinterpret_cast<T *>(code_->entry()); }
private:
    E *const code_;
};

class Coroutine;

using TrampolineStub = CallStub<intptr_t(Coroutine *)>;

inline const Field *Type::field(uint32_t i) const {
    DCHECK_LT(i, n_fields_);
    return fields_ + i;
}

inline const Method *Class::method(uint32_t i) const {
    DCHECK_LT(i, n_methods_);
    return methods_ + i;
}

inline void Class::set_method_function(uint32_t i, Closure *fn) {
    DCHECK_LT(i, n_methods_);
    methods_->fn_ = DCHECK_NOTNULL(fn);
}

inline void Class::set_init_function(Closure *fn) {
    DCHECK_NOTNULL(init_)->fn_ = DCHECK_NOTNULL(fn);
}

inline bool Class::IsArray() const {
    switch (static_cast<BuiltinType>(id())) {
        case kType_array:
        case kType_array8:
        case kType_array16:
        case kType_array32:
        case kType_array64:
            return true;
            
        default:
            return false;
    }
}

inline bool Class::IsBaseOf(const Class *base) const {
    DCHECK(base != nullptr);
    for (const Class *clazz = this->base_; clazz; clazz = clazz->base()) {
        if (clazz == base) {
            return true;
        }
    }
    return false;
}

inline int Class::GetSameOrBaseOf(const Class *base) const {
    int level = 1;
    for (const Class *clazz = this; clazz; clazz = clazz->base(), level++) {
        if (clazz == base) {
            return level;
        }
    }
    return 0;
}

inline int SourceLineInfo::FindSourceLine(intptr_t pc) const {
    if (simple_format()) {
        return source_line(pc);
    }
    for (uint32_t i = 0; i < length_; i++) {
        if (pc >= range(i)->start_pc && pc < range(i)->stop_pc) {
            return range(i)->line;
        }
    }
    NOREACHED();
    return -1;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_METADATA_H_
