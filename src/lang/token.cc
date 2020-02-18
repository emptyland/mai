#include "lang/token.h"
#include "glog/logging.h"
#include <atomic>
#include <thread>
#include <unordered_map>

namespace mai {

namespace lang {

struct KeywordEntry {
    Token::Kind kind;
    const char *literal;
}; // struct KeywordEntry

const KeywordEntry kKeywordEntries[] = {
#define DEFINE_KEYWORD(name, literal, ...) { Token::k##name, literal },
    DECLARE_KEYWORDS_TOKEN(DEFINE_KEYWORD)
#undef DEFINE_KEYWORD
    {Token::kEOF, nullptr},
}; // const KeywordEntry kKeywordEntries

std::unordered_map<std::string, Token::Kind> *all_keywords = nullptr;

/*static*/ Token::Kind Token::IsKeyword(const std::string &text) {
    auto iter = DCHECK_NOTNULL(all_keywords)->find(text);
    return iter == all_keywords->end() ? kError : iter->second;
}

Token::NamePair Token::kNameTable[kMax] = {
#define DEFINE_NAME_PAIR(name, literal, ...) { #name, literal, },
    DECLARE_ALL_TOKEN(DEFINE_NAME_PAIR)
#undef DEFINE_NAME_PAIR
}; // Token::kNameTable[kMax]

void InitializeSyntaxLibrary() {
    all_keywords = new std::unordered_map<std::string, Token::Kind>();

    const KeywordEntry *entry = &kKeywordEntries[0];
    while (entry->kind != Token::kEOF) {
        (*all_keywords)[entry->literal] = entry->kind;
        entry++;
    }
}

void FreeSyntaxLibrary() {
    delete all_keywords;
    all_keywords = nullptr;
}


} // namespace lang

} // namespace mai
