#pragma once
#ifndef MAI_LANG_SYNTAX_MOCK_TEST_H_
#define MAI_LANG_SYNTAX_MOCK_TEST_H_

#include "lang/syntax.h"
#include "mai/env.h"

namespace mai {

namespace lang {

class MockFeedback : public SyntaxFeedback {
public:
    void DidFeedback(const SourceLocation &location, const char *z, size_t n) override {
        ::printf("%s:%d:%d-%d:%d %s\n", file_name_.c_str(), location.begin_line, location.begin_row,
                 location.end_line, location.end_row, z);
    }
};

class MockFile : public SequentialFile {
public:
    MockFile(const std::string &s)
        : in_memory_(s)
        , used_(0)
        , available_(s.size()) {}

    Error Read(size_t n, std::string_view *result, std::string */*scratch*/) override {
        size_t wanted = std::min(n, available_);
        *result = std::string_view(in_memory_.data() + used_, wanted);
        used_ += wanted;
        available_ -= wanted;
        return Error::OK();
    }

    Error Skip(size_t n) override {
        size_t wanted = std::min(n, available_);
        used_ += wanted;
        available_ -= wanted;
        return Error::OK();
    }

    Error GetFileSize(uint64_t *size) override {
        *size = in_memory_.size();
        return Error::OK();
    }
    
private:
    const std::string in_memory_;
    size_t used_;
    size_t available_;
};

} // namespace lang

} // namespace mai


#endif // MAI_LANG_SYNTAX_MOCK_TEST_H_
