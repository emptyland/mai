#include "lang/ast.h"
#include "lang/token.h"
#include "base/hash.h"
#include "base/lazy-instance.h"
#include <unordered_map>

namespace mai {

namespace lang {

// i8 toU8, toI16, toU16, toInt, toUInt, toI32, toU32, toI64, toU64, toF32, toF64
// u8 toI8, toI16, toU16, toInt, toUInt, toI32, toU32, toI64, toU64, toF32, toF64

namespace {

struct NumberCastHint {
    Token::Kind type;
    struct {
        const char *name;
        Token::Kind dest;
    } entries[12];
};

const NumberCastHint number_cast_hints[] = {
    {
        Token::kI8, {
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
        }
    }, {
        Token::kU8, {
            {"toI8", Token::kI8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
        }
    }, {
       Token::kI16, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
       }
    }, {
        Token::kU16, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
        }
    }, {
        Token::kI32, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
        }
    }, {
        Token::kU32, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
        }
    }, {
        Token::kInt, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
        }
    }, {
       Token::kUInt, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
       }
    }, {
      Token::kI64, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
        }
    }, {
        Token::kU64, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toF32", Token::kF32},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
        }
    }, {
        Token::kF32, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF64", Token::kF64},
            {nullptr, Token::kVoid}
        }
    }, {
        Token::kF64, {
            {"toI8", Token::kI8},
            {"toU8", Token::kU8},
            {"toI16", Token::kI16},
            {"toU16", Token::kU16},
            {"toI32", Token::kI32},
            {"toU32", Token::kU32},
            {"toInt", Token::kInt},
            {"toUInt", Token::kUInt},
            {"toI64", Token::kI64},
            {"toU64", Token::kU64},
            {"toF32", Token::kF32},
            {nullptr, Token::kVoid}
        }
    }
}; // const NumberCastHint number_cast_hints

class ASTNumberCastHintTable {
public:
    ASTNumberCastHintTable() {
        for (size_t i = 0; i < arraysize(number_cast_hints); i++) {
            size_t j = 0;
            while (number_cast_hints[i].entries[j].name) {
                ASTNumberCastPair key;
                key.name = number_cast_hints[i].entries[j].name;
                key.dest = number_cast_hints[i].type;
                DCHECK(table_.find(key) == table_.end());
                table_[key] = number_cast_hints[i].entries[j].dest;
                j++;
            }
        }
    }
    
    Token::Kind Find(std::string_view name, Token::Kind kind) const {
        ASTNumberCastPair key;
        key.name = name;
        key.dest = kind;
        auto iter = table_.find(key);
        return iter == table_.end() ? Token::kError : iter->second;
    }
private:
    struct ASTNumberCastPair {
        std::string_view name;
        Token::Kind dest;
    };

    struct ASTNumberCastHash : public std::unary_function<ASTNumberCastPair, size_t> {
        size_t operator () (ASTNumberCastPair key) const {
            return base::Hash::Js(key.name.data(), key.name.size()) ^ static_cast<int>(key.dest);
        }
    };

    struct ASTNumberCastEqualTo :
        public std::binary_function<ASTNumberCastPair, ASTNumberCastPair, bool> {
        bool operator () (ASTNumberCastPair lhs, ASTNumberCastPair rhs) const {
            return std::equal_to<std::string_view>{}(lhs.name, rhs.name) && lhs.dest == rhs.dest;
        }
    };
    
    using Table = std::unordered_map<ASTNumberCastPair, Token::Kind, ASTNumberCastHash,
                                     ASTNumberCastEqualTo>;
    Table table_;
}; // class ASTNumberCastHintTable


class ASTCompilingAttributesTable {
public:
    ASTCompilingAttributesTable() {
        #define DEFINE_TABLE_ENTRY(name, text, value) \
            table_[text] = value;
            DECLARE_ALL_ATTRIBUTES(DEFINE_TABLE_ENTRY)
        #undef DEFINE_TABLE_ENTRY
    }
    
    uint32_t Find(std::string_view attr) const {
        auto iter = table_.find(attr);
        return iter == table_.end() ? 0 : iter->second;
    }

private:
    std::unordered_map<std::string_view, uint32_t> table_;
}; // class ASTCompilingAttributesTable


base::LazyInstance<ASTNumberCastHintTable> ast_number_cast_hint_table;

base::LazyInstance<ASTCompilingAttributesTable> ast_compiling_attributes_table;

} // namespace

uint32_t FindCompilingAttribute(std::string_view attr) {
    return ast_compiling_attributes_table->Find(attr);
}

TypeSign::TypeSign(int position, const ASTString *prefix, const ASTString *name)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kIdentifier)
    , prefix_(prefix)
    , name_(name) {
}

TypeSign::TypeSign(int position, FunctionPrototype *prototype)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kFun)
    , prototype_(DCHECK_NOTNULL(prototype)) {
}

TypeSign::TypeSign(int position, ClassDefinition *clazz)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kClass)
    , clazz_(DCHECK_NOTNULL(clazz)) {
}

TypeSign::TypeSign(int position, ObjectDefinition *object)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kObject)
    , object_(DCHECK_NOTNULL(object)) {
}

TypeSign::TypeSign(int position, InterfaceDefinition *interface)
    : ASTNode(position, kTypeSign)
    , kind_(Token::kInterface)
    , interface_(DCHECK_NOTNULL(interface)) {
}

TypeSign::TypeSign(int position, int kind, Definition *def)
    : ASTNode(position, kTypeSign)
    , kind_(kind)
    , definition_(DCHECK_NOTNULL(def)) {
    DCHECK((kind == Token::kInterface && def->IsInterfaceDefinition()) ||
           (kind == Token::kClass && def->IsClassDefinition()) ||
           (kind == Token::kObject && def->IsObjectDefinition()) ||
           kind == Token::kRef);
}

#if defined(DEBUG) || defined(_DEBUG)
StructureDefinition *TypeSign::structure() const {
    DCHECK(kind_ == Token::kClass ||
           kind_ == Token::kObject ||
           kind_ == Token::kRef)
        << Token::ToString(static_cast<Token::Kind>(kind_));
    DCHECK(structure_->IsClassDefinition() || structure_->IsObjectDefinition());
    return structure_;
}

Definition *TypeSign::definition() const {
    DCHECK(kind_ == Token::kClass ||
           kind_ == Token::kObject ||
           kind_ == Token::kInterface ||
           kind_ == Token::kRef) << Token::ToString(static_cast<Token::Kind>(kind_));
    DCHECK(definition_->IsClassDefinition() || definition_->IsObjectDefinition() ||
           definition_->IsInterfaceDefinition());
    return definition_;
}
#endif // defined(DEBUG) || defined(_DEBUG)

bool TypeSign::IsSignedIntegral() const {
    switch (id()) {
        case Token::kI8:
        case Token::kI16:
        case Token::kI32:
        case Token::kInt:
        case Token::kI64:
            return true;
            
        default:
            return false;
    }
}

bool TypeSign::IsUnsignedIntegral() const {
    switch (id()) {
        case Token::kU8:
        case Token::kU16:
        case Token::kU32:
        case Token::kUInt:
        case Token::kU64:
            return true;
            
        default:
            return false;
    }
}

bool TypeSign::IsFloating() const { return id() == Token::kF32 || id() == Token::kF64; }

bool TypeSign::Convertible(const TypeSign *rhs) const {
    switch (static_cast<Token::Kind>(id())) {
        case Token::kNil:
            return !rhs->IsNumber() && rhs->id() != Token::kString && rhs->id() != Token::kFun;
        case Token::kAny:
            return true; // Alaways ok
        case Token::kString:
            return rhs->id() == Token::kString;
        case Token::kIdentifier:
            return id() == rhs->id() && ToSymbolString() == rhs->ToSymbolString();
        case Token::kInterface:
            if ((id() == rhs->id() && interface_ == rhs->interface_) || rhs->id() == Token::kNil) {
                return true;
            }
            return (rhs->id() == Token::kObject || rhs->id() == Token::kRef) &&
                   interface_->HasImplement(rhs->structure());
        case Token::kObject:
            return (id() == rhs->id() && object_ == rhs->object_) || rhs->id() == Token::kNil;
        case Token::kRef:
        case Token::kClass:
            return clazz()->SameOrBaseOf(rhs->clazz()) || rhs->id() == Token::kNil;
        case Token::kFun:
            return (id() == rhs->id() && prototype()->IsAccept(rhs->prototype())) ||
                   rhs->id() == Token::kNil;
        case Token::kArray:
            return (rhs->id() == id() && parameter(0)->Convertible(rhs->parameter(0))) ||
                   rhs->id() == Token::kNil;
        case Token::kMap:
        case Token::kMutableMap:
            return (rhs->id() == id() && parameter(0)->Convertible(rhs->parameter(0)) &&
                   parameter(1)->Convertible(rhs->parameter(1))) || rhs->id() == Token::kNil;
        case Token::kU16:
            return rhs->id() == Token::kU16 || rhs->id() == Token::kU8;
        case Token::kInt:
        case Token::kI32:
            return rhs->id() == Token::kInt || rhs->id() == Token::kI32;
        case Token::kUInt:
        case Token::kU32:
            return rhs->id() == Token::kUInt || rhs->id() == Token::kU32;
        case Token::kU64:
            return rhs->id() == Token::kU64 || rhs->id() == Token::kUInt || rhs->id() == Token::kU32;
        default:
            return id() == rhs->id();
    }
}

bool TypeSign::Castable(const TypeSign *rhs) const {
    if (rhs->id() == Token::kAny || rhs->id() == Token::kVoid) {
        return true;
    }
    switch (static_cast<Token::Kind>(id())) {
        case Token::kNil:
            return !rhs->IsNumber() && rhs->id() != Token::kString && rhs->id() != Token::kFun;
        case Token::kAny:
            return true; // Alaways ok
        case Token::kString:
            return rhs->id() == Token::kArray && (rhs->parameter(0)->id() == Token::kU8 ||
                                                  rhs->parameter(0)->id() == Token::kI8);
        case Token::kIdentifier:
            return id() == rhs->id() && ToSymbolString() == rhs->ToSymbolString();
        case Token::kInterface:
            if ((id() == rhs->id() && interface_ == rhs->interface_)) {
                return true;
            }
            return (rhs->id() == Token::kObject || rhs->id() == Token::kRef) &&
                   interface_->HasImplement(rhs->structure());
        case Token::kObject:
            return id() == rhs->id() && object_ == rhs->object_;
        case Token::kRef:
        case Token::kClass:
            if (rhs->id() == Token::kRef || rhs->id() == Token::kClass) {
                return clazz()->SameOrBaseOf(rhs->clazz()) || rhs->clazz()->SameOrBaseOf(clazz());
            }
            return false;
        case Token::kFun:
            return (id() == rhs->id() && prototype()->IsAccept(rhs->prototype()));
        case Token::kArray:
            return (rhs->id() == id() && parameter(0)->Convertible(rhs->parameter(0)));
        case Token::kMap:
        case Token::kMutableMap:
            return (rhs->id() == id() && parameter(0)->Convertible(rhs->parameter(0)) &&
                    parameter(1)->Convertible(rhs->parameter(1)));
        case Token::kBool:
            return rhs->id() == Token::kBool;
        default:
            return IsNumber() == rhs->IsNumber();
    }
}

std::string TypeSign::ToSymbolString() const {
    DCHECK_EQ(Token::kIdentifier, kind_);
    std::string buf(!prefix_ ? "" : prefix_->ToString());
    if (prefix_) { buf.append(1, '.'); }
    return buf.append(DCHECK_NOTNULL(name_)->ToString());
}

BuiltinType TypeSign::ToBuiltinType() const {
    switch (static_cast<Token::Kind>(id())) {
    #define DEFINE_PRIMITIVE_TYPE(token, type, ...) case Token::k##token: return kType_##type;
        DECLARE_BOX_NUMBER_TYPES(DEFINE_PRIMITIVE_TYPE)
    #undef DEFINE_PRIMITIVE_TYPE
        case Token::kVoid:
            return kType_void;
        case Token::kString:
            return kType_string;
        case Token::kArray:
            return GetArrayBuiltinType(false/*is_mutable*/, parameter(0));
        case Token::kMap:
            return GetMapBuiltinType(false/*is_mutable*/, parameter(0));
        case Token::kMutableMap:
            return GetMapBuiltinType(true/*is_mutable*/, parameter(0));
        case Token::kFun:
            return kType_closure;
        case Token::kChannel:
            return kType_channel;
        case Token::kInterface:
        case Token::kObject:
        case Token::kRef:
            return kType_any;
        case Token::kAny:
            return kType_any;
        case Token::kClass:
        default:
            NOREACHED() << ToString();
            return static_cast<BuiltinType>(-1);
    }
}

size_t TypeSign::GetReferenceSize() const {
    switch (static_cast<Token::Kind>(id())) {
    #define DEFINE_PRIMITIVE_TYPE(token, type, kind) case Token::k##token: \
        return sizeof(kind);
        DECLARE_BOX_NUMBER_TYPES(DEFINE_PRIMITIVE_TYPE)
    #undef DEFINE_PRIMITIVE_TYPE
        case Token::kAny:
        case Token::kString:
        case Token::kArray:
        case Token::kMap:
        case Token::kMutableMap:
        case Token::kInterface:
        case Token::kObject:
        case Token::kRef:
        case Token::kFun:
        case Token::kChannel:
            return kPointerSize;
        case Token::kClass:
        default:
            NOREACHED() << Token::ToString(static_cast<Token::Kind>(id()));
            return 0;
    }
}

/*static*/ BuiltinType TypeSign::GetArrayBuiltinType(bool is_mutable, TypeSign *element) {
    switch (static_cast<Token::Kind>(element->id())) {
        case Token::kBool:
        case Token::kI8:
        case Token::kU8:
            return kType_array8;
        case Token::kI16:
        case Token::kU16:
            return kType_array16;
        case Token::kI32:
        case Token::kU32:
        case Token::kInt:
        case Token::kUInt:
        case Token::kF32:
            return kType_array32;
        case Token::kI64:
        case Token::kU64:
        case Token::kF64:
            return kType_array64;
        default:
            return kType_array;
    }
}

/*static*/ BuiltinType TypeSign::GetMapBuiltinType(bool is_mutable, TypeSign *key) {
//    switch (static_cast<Token::Kind>(key->id())) {
//        case Token::kBool:
//        case Token::kI8:
//        case Token::kU8:
//            return is_mutable ? kType_mutable_map : kType_map8;
//        case Token::kI16:
//        case Token::kU16:
//            return is_mutable ? kType_mutable_array16 : kType_array16;
//        case Token::kI32:
//        case Token::kU32:
//        case Token::kInt:
//        case Token::kUInt:
//        case Token::kF32:
//            return is_mutable ? kType_mutable_array32 : kType_array32;
//        case Token::kI64:
//        case Token::kU64:
//        case Token::kF64:
//            return is_mutable ? kType_mutable_array64 : kType_array64;
//        default:
//            return is_mutable ? kType_mutable_array : kType_array;
//    }
    // TODO:
    TODO();
    return kType_any;
}

int TypeSign::FindNumberCastHint(std::string_view name) const {
    return ast_number_cast_hint_table->Find(name, static_cast<Token::Kind>(id()));
}

std::string TypeSign::ToString() const {
    // Token::kIdentifier                -> prefix_, name_
    // Token::kRef                       -> definition_
    // Token::kClass                     -> clazz_
    // Token::kObject                    -> object_
    // Token::kInterface                 -> interface_
    // Token::kChannel                   -> parameters_[0]
    // Token::kArray/Token::MutableArray -> parameters_[0]
    // Token::kMap/Token::MutableMap     -> parameters_[0], parameters_[1]
    // Token::kPair                      -> parameters_[0], parameters_[1]
    // Token::kFun                       -> prototype_
    switch (static_cast<Token::Kind>(id())) {
        case Token::kIdentifier:
            return base::Sprintf("identifier('%s')", ToSymbolString().c_str());
        case Token::kRef:
            return base::Sprintf("udo('%s')", clazz_->identifier()->data());
        case Token::kClass:
            return base::Sprintf("class('%s')", clazz_->identifier()->data());
        case Token::kObject:
            return base::Sprintf("object('%s')", object_->identifier()->data());
        case Token::kInterface:
            return base::Sprintf("interface('%s')", interface_->identifier()->data());
        case Token::kChannel:
            return base::Sprintf("channel[%s]", parameter(0)->ToString().c_str());
        case Token::kArray:
            return base::Sprintf("array[%s]", parameter(0)->ToString().c_str());
        case Token::kMap:
            return base::Sprintf("map[%s,%s]", parameter(0)->ToString().c_str(),
                                 parameter(1)->ToString().c_str());
        case Token::kMutableMap:
            return base::Sprintf("mutable_map[%s,%s]", parameter(0)->ToString().c_str(),
                                 parameter(1)->ToString().c_str());
        case Token::kPair:
            return base::Sprintf("(%s->%s)", parameter(0)->ToString().c_str(),
                                 parameter(1)->ToString().c_str());
        case Token::kFun:
            return base::Sprintf("fun %s", prototype_->ToString().c_str());
        case Token::kNil:
            return "nil";
        case Token::kBool:
            return "bool";
        case Token::kI8:
            return "i8";
        case Token::kU8:
            return "u8";
        case Token::kI16:
            return "i16";
        case Token::kU16:
            return "u16";
        case Token::kI32:
            return "i32";
        case Token::kU32:
            return "u32";
        case Token::kInt:
            return "int";
        case Token::kUInt:
            return "uint";
        case Token::kI64:
            return "i64";
        case Token::kU64:
            return "u64";
        case Token::kF32:
            return "f32";
        case Token::kF64:
            return "f64";
        case Token::kAny:
            return "any";
        case Token::kString:
            return "string";
        case Token::kVoid:
            return "void";
        default:
            NOREACHED();
            break;
    }
    return "TODO";
}

std::string FunctionPrototype::ToString() const {
    std::string buf("(");
    bool need_comma = false;
    if (owner_) {
        buf.append("self: ").append(owner_->identifier()->ToString());
        need_comma = true;
    }
    
    for (auto param : parameters_) {
        if (need_comma) {
            buf.append(",");
        }
        if (param.name) {
            buf.append(param.name->ToString()).append(": ");
        }
        buf.append(param.type->ToString());
        need_comma = true;
    }
    
    if (vargs_) {
        if (need_comma) {
            buf.append(",");
        }
        buf.append("...");
    }
    buf.append("): ");
    if (return_type_) {
        buf.append(return_type_->ToString());
    } else {
        buf.append("[NONE]");
    }
    return buf;
}

ClassDefinition::ClassDefinition(base::Arena *arena,
                                 int position,
                                 const ASTString *name,
                                 base::ArenaVector<Parameter> &&constructor,
                                 base::ArenaVector<Field> &&fields,
                                 const ASTString *base_prefix,
                                 const ASTString *base_name,
                                 base::ArenaVector<Expression *> &&arguments)
    : StructureDefinition(arena, position, kClassDefinition, name, std::move(fields))
    , parameters_(constructor)
    , named_parameters_(arena)
    , base_prefix_(base_prefix)
    , base_name_(base_name)
    , base_(nullptr)
    , arguments_(arguments) {
}

int ClassDefinition::MakeParameterLookupTable() {
    for (size_t i = 0; i < parameters_size(); i++) {
        Parameter param = parameter(i);
        const ASTString *name = nullptr;
        if (param.field_declaration) {
            name = field(param.as_field).declaration->identifier();
        } else {
            name = param.as_parameter->identifier();
        }
        if (auto iter = named_parameters_.find(name->ToSlice()); iter != named_parameters_.end()) {
            return -static_cast<int>(i + 1);
        }
        named_parameters_[name->ToSlice()] = i;
    }
    return 0;
}

TryCatchFinallyBlock::TryCatchFinallyBlock(int position,
                                           base::ArenaVector<Statement *> &&try_statements,
                                           base::ArenaVector<CatchBlock *> &&catch_blocks,
                                           base::ArenaVector<Statement *> &&finally_statements)
    : Statement(position, kTryCatchFinallyBlock)
    , try_statements_(std::move(try_statements))
    , catch_blocks_(std::move(catch_blocks))
    , finally_statements_(std::move(finally_statements)) {
    for (auto block : catch_blocks_) { block->set_owner(this); }
}

} // namespace lang

} // namespace mai
