#include "sql/ast.h"

namespace mai {

namespace sql {
    
namespace ast {
    
#define DECL_OP_CASE(name, op, fmt) case SQL_##name:

/*static*/ bool UnaryExpression::IsUnaryOperator(SQLOperator op) {
    switch (op) {
        DEFINE_SQL_UNARY_OPS(DECL_OP_CASE)
            return true;

        case SQL_MAX_OP:
            NOREACHED();
            break;
            
        default:
            break;
    }
    return false;
}
    
/*static*/ bool BinaryExpression::IsBinaryOperator(SQLOperator op) {
    switch (op) {
        DEFINE_SQL_BINARY_OPS(DECL_OP_CASE)
            return true;

        case SQL_MAX_OP:
            NOREACHED();
            break;
            
        default:
            break;
    }
    return false;
}
    
/*static*/ bool Comparison::IsComparisonOperator(SQLOperator op) {
    switch (op) {
        DEFINE_SQL_CMP_OPS(DECL_OP_CASE)
            return true;
            
        case SQL_MAX_OP:
            NOREACHED();
            break;
            
        default:
            break;
    }
    return false;
}
    
} // namespace ast

} // namespace sql
    
} // namespace mai
