#include "lang/token.h"
#include "base/lazy-instance.h"
#include "glog/logging.h"
#include <atomic>
#include <thread>
#include <unordered_map>

namespace mai {

namespace lang {

namespace { // internal

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

//std::unordered_map<std::string, Token::Kind> *all_keywords = nullptr;

class KeyWordTable {
public:
    KeyWordTable() {
        const KeywordEntry *entry = &kKeywordEntries[0];
        while (entry->kind != Token::kEOF) {
            table_[entry->literal] = entry->kind;
            entry++;
        }
    }
    
    Token::Kind Find(std::string_view text) const {
        auto iter = table_.find(text);
        return iter == table_.end() ? Token::kError : iter->second;
    }
private:
    std::unordered_map<std::string_view, Token::Kind> table_;
}; // class KeyWordTable

base::LazyInstance<KeyWordTable> all_keywords;

} // namespace

/*static*/ Token::Kind Token::IsKeyword(std::string_view text) {
    return all_keywords->Find(text);
}

Token::NamePair Token::kNameTable[kMax] = {
#define DEFINE_NAME_PAIR(name, literal, ...) { #name, literal, },
    DECLARE_ALL_TOKEN(DEFINE_NAME_PAIR)
#undef DEFINE_NAME_PAIR
}; // Token::kNameTable[kMax]

} // namespace lang

} // namespace mai
