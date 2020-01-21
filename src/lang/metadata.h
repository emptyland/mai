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
    void *operator new (size_t) = delete;
    void operator delete(void *) = delete;

    DISALLOW_IMPLICIT_CONSTRUCTORS(MetadataObject);
}; // class MetadataObject

using MDStr = const char *;

// Metadata: type
class Type : public MetadataObject {
public:
    DEF_VAL_GETTER(uint32_t, n_fields);
    DEF_VAL_GETTER(uint32_t, id);
    DEF_VAL_GETTER(uint32_t, tags);
    DEF_VAL_GETTER(MDStr, name);
    
    inline const Field *field(uint32_t i) const;

    friend class ClassBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Type);
private:
    uint32_t id_; // Type unique id
    uint32_t tags_; // flags
    MDStr name_; // Name of type
    uint32_t n_fields_; // Number of fields
    Field *fields_; // All fields
}; // class Type

// Metadata: class
class Class : public Type {
public:
    DEF_PTR_GETTER(Class, base);
    DEF_PTR_GETTER(Method, init);
    DEF_VAL_GETTER(uint32_t, n_methods);
    
    inline const Method *method(uint32_t i) const;
    
    friend class ClassBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Class);
private:
    Class *base_; // Base class
    Method *init_; // Constructor method
    uint32_t n_methods_; // Number of method
    Method *methods_; // All methods
}; // class Class

class Field : public MetadataObject {
public:
    DEF_VAL_GETTER(MDStr, name);
    DEF_PTR_GETTER(Class, type);
    DEF_VAL_GETTER(uint32_t, flags);
    DEF_VAL_GETTER(uint32_t, tag);
    DEF_VAL_GETTER(uint32_t, offset);

    friend class ClassBuilder;
    friend class MetadataSpace;
    DISALLOW_IMPLICIT_CONSTRUCTORS(Field);
private:
    MDStr name_; // Field name
    Class *type_; // Type of field
    uint32_t flags_; // Flags
    uint32_t tag_; // Tag for serializition
    uint32_t offset_; // Offset of object address
}; // class Field

class Method : public MetadataObject {
public:
    DEF_VAL_GETTER(MDStr, name);
    DEF_VAL_GETTER(uint32_t, tag);
    
private:
    MDStr name_; // Method name
    uint32_t tag_; // Tag for serializition
    Closure *fn_; // [strong ref]
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
