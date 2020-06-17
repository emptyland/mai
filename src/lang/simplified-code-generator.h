#ifndef MAI_LANG_SIMPLIFIED_CODE_GENERATOR_H_
#define MAI_LANG_SIMPLIFIED_CODE_GENERATOR_H_

namespace mai {
namespace base {
class Arena;
} // namespace base
namespace lang {

class Kode;
class CompilationInfo;

Kode *GenerateSimplifiedCode(const CompilationInfo *compilation_info, bool enable_debug,
                             bool enable_jit, base::Arena *arena);

} // namespace lang

} // namespace mai

#endif // MAI_LANG_SIMPLIFIED_CODE_GENERATOR_H_
