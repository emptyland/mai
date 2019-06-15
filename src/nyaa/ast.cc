#include "nyaa/ast.h"
#include "nyaa/parser-ctx.h"
#include "nyaa/syntax.hh"

namespace mai {
    
namespace nyaa {

namespace ast {

Location::Location(const NYAA_YYLTYPE &yyl)
    : begin_line(yyl.first_line)
    , end_line(yyl.last_line)
    , begin_column(yyl.first_column)
    , end_column(yyl.last_column) {
}

/*static*/ Location Location::Concat(const NYAA_YYLTYPE &first, const NYAA_YYLTYPE &last) {
    Location loc;
    loc.begin_line = first.first_line;
    loc.end_line = last.last_line;
    loc.begin_column = first.first_column;
    loc.end_column = last.last_column;
    return loc;
}
    
} //namespace ast

} // namespace nyaa
    
} // namespace mai
