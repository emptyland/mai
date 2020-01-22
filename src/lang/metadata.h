#ifndef MAI_LANG_METADATA_H_
#define MAI_LANG_METADATA_H_

#include "lang/type-defs.h"
#include "base/base.h"
#include "glog/logging.h"

namespace mai {

namespace lang {

class Method;
class Field;
class Class;
class Type;
class Closure;

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
    static constexpr uint32_t kUserTag = 0;
    static constexpr uint32_t kPrimitiveTag = 1;
    static constexpr uint32_t kBuiltinTag = 1u << 1;

    DEF_VAL_GETTER(uint32_t, n_fields);
    DEF_VAL_GETTER(uint32_t, id);
    DEF_VAL_GETTER(uint32_t, tags);
    DEF_VAL_GETTER(MDStr, name);
    
    inline const Field *field(uint32_t i) const;

    friend class ClassBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Type);
protected:
    Type() = default;

    uint32_t id_ = 0; // Type unique id
    uint32_t tags_ = 0; // flags
    MDStr name_ = nullptr; // Name of type
    uint32_t n_fields_ = 0; // Number of fields
    Field *fields_ = nullptr; // All fields
}; // class Type

// Metadata: class
class Class : public Type {
public:
    DEF_PTR_GETTER(const Class, base);
    DEF_PTR_GETTER(Method, init);
    DEF_VAL_GETTER(uint32_t, n_methods);
    
    inline const Method *method(uint32_t i) const;
    
    friend class ClassBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Class);
private:
    Class() = default;
    
    const Class *base_ = nullptr; // Base class
    Method *init_ = nullptr; // Constructor method
    uint32_t n_methods_ = 0; // Number of method
    Method *methods_ = nullptr; // All methods
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

class Method : public MetadataObject {
public:
    DEF_VAL_GETTER(MDStr, name);
    DEF_VAL_GETTER(uint32_t, tags);
    
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


// Machine code object
class Code : public MetadataObject {
public:
    enum Kind {
        kEntry,
        kStub,
        kHandler,
    };
    
    DEF_VAL_GETTER(Kind, kind);
    DEF_VAL_GETTER(uint32_t, size);

    Address entry() { return instructions_; }
    
    friend class MetadataSpace;
private:
    Code(Kind kind, Address instructions, uint32_t size)
        : kind_(kind)
        , size_(size) {
        ::memcpy(instructions_, instructions, size);
    }
    
    Kind kind_;
    uint32_t size_;
    uint8_t instructions_[0];
}; // class Code

inline const Field *Type::field(uint32_t i) const {
    DCHECK_LT(i, n_fields_);
    return fields_ + i;
}

inline const Method *Class::method(uint32_t i) const {
    DCHECK_LT(i, n_methods_);
    return methods_ + i;
}

} // namespace lang

} // namespace mai

#endif // MAI_LANG_METADATA_H_
