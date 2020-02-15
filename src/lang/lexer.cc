#include "lang/lexer.h"
#include "base/arena-utils.h"

namespace mai {

namespace lang {

Error Lexer::SwitchInputFile(const std::string &name, SequentialFile *file) {
    input_file_ = DCHECK_NOTNULL(file);
    DCHECK_NOTNULL(error_feedback_)->set_file_name(name);
    scratch_.clear();
    buffered_ = "";
    line_ = 1;
    row_ = 1;
    if (auto rs = input_file_->GetFileSize(&available_); rs.fail()) {
        return rs;
    }
    if (auto rs = input_file_->Read(std::min(static_cast<size_t>(available_), kBufferSize),
                                     &buffered_, &scratch_);
        rs.fail()) {
        return rs;
    }
    
    buffer_position_ = 0;
    while (!in_string_template_.empty()) {
        in_string_template_.pop();
    }
    in_string_template_.push(kNotTemplate);
    return Error::OK();
}

Token Lexer::Next() {
    
    for (;;) {
        int ch = Peek();
        if (in_string_template_.top() == kSimpleTemplate) {
            return MatchSimpleTemplateString();
        } else if (in_string_template_.top() == kExpressionTemplate) {
            if (ch == '}') {
                SourceLocation loc = SourceLocation::One(line_, row_);
                MoveNext();
                in_string_template_.pop();
                return Token(Token::kStringTempleteExpressEnd, loc);
            }
        }

        switch (ch) {
            case 0:
                return Token(Token::kEOF, SourceLocation::One(line_, row_));
                
            case ' ':
            case '\t':
                MoveNext();
                break;

            case '\r':
                MoveNext();
                if (ch = Peek(); ch == '\n') {
                    MoveNext();
                    line_++;
                    row_ = 1;
                }
                break;

            case '\n':
                MoveNext();
                line_++;
                row_ = 1;
                break;
                
            case '\"':
                return MatchString();
                
            case '(':
                return Token(Token::kLParen, SourceLocation::One(line_, row_));
                
            case ')':
                return Token(Token::kRParen, SourceLocation::One(line_, row_));

            case '[':
                return Token(Token::kLBrack, SourceLocation::One(line_, row_));
                
            case ']':
                return Token(Token::kRBrack, SourceLocation::One(line_, row_));
                
            case '{':
                return Token(Token::kLBrace, SourceLocation::One(line_, row_));
                
            case '}':
                return Token(Token::kRBrace, SourceLocation::One(line_, row_));
                
            case '*':
                return Token(Token::kStar, SourceLocation::One(line_, row_));
                
            case '_':
                return MatchIdentifier();

            case '+': {
                int begin_line = line_, begin_row = row_;
                ch = MoveNext();
                if (ch == '=') {
                    MoveNext();
                    return Token(Token::kPlusEqual, {begin_line, begin_row, line_, row_});
                } else if (ch == '+') {
                    MoveNext();
                    return Token(Token::kPlusPlus, {begin_line, begin_row, line_, row_});
                } else if (IsTerm(ch)) {
                    return Token(Token::kPlus, SourceLocation::One(begin_line, begin_row));
                } else {
                    SourceLocation loc = SourceLocation::One(line_, row_);
                    error_feedback_->Printf(loc, "Unexpected `+' postfix");
                    return Token(Token::kError, SourceLocation::One(line_, row_));
                }
            } break;
                
            case '-': {
                int begin_line = line_, begin_row = row_;
                ch = MoveNext();
                if (ch == '=') {
                    MoveNext();
                    return Token(Token::kMinusEqual, {begin_line, begin_row, line_, row_});
                } else if (ch == '-') {
                    MoveNext();
                    return Token(Token::kMinusMinus, {begin_line, begin_row, line_, row_});
                } else if (IsTerm(ch)) {
                    return Token(Token::kMinus, SourceLocation::One(begin_line, begin_row));
                } else {
                    SourceLocation loc = SourceLocation::One(line_, row_);
                    error_feedback_->Printf(loc, "Unexpected `-' postfix");
                    return Token(Token::kError, SourceLocation::One(line_, row_));
                }
            } break;
                
            default:
                if (ch < 0) {
                    return Token(Token::kError, SourceLocation::One(line_, row_));
                } else {
                    return MatchIdentifier();
                }
                break;
        }
    }
}

Token Lexer::MatchIdentifier() {
    SourceLocation loc{line_, row_};

    std::string buf;
    for (;;) {
        int ch = Peek();
        if (::isdigit(ch) || ::isalpha(ch) || ch == '_') {
            loc.end_line = line_;
            loc.end_row  = row_;
            MoveNext();
            buf.append(1, ch);
        } else if (ch < 0 || ch == '\r' || ch == '\n') {
            error_feedback_->Printf(loc, "Unexpected identifier character, expected %c", ch);
            return Token(Token::kError, loc);
        } else if (IsTerm(ch)) {
            Token::Kind keyword = Token::IsKeyword(buf);
            if (keyword != Token::kError) {
                return Token(keyword, loc);
            }
            const ASTString *asts = ASTString::New(arena_, buf.data(), buf.size());
            return Token(Token::kIdentifier, loc).With(asts);
        } else {
            loc.end_line = line_;
            loc.end_row  = row_;
            MoveNext();
            buf.append(1, ch);
        }
    }
}

Token Lexer::MatchSimpleTemplateString() {
    SourceLocation loc{line_, row_};
    std::string buf;
    for (;;) {
        int ch = Peek();
        if (ch == '\"') {
            loc.end_line = line_;
            loc.end_row  = row_;
            MoveNext();
            
            in_string_template_.pop();
            const ASTString *asts = ASTString::New(arena_, buf.data(), buf.size());
            return Token(Token::kStringTempleteSuffix, loc).With(asts);
        } else if (ch == '$') {
            loc.end_line = line_;
            loc.end_row  = row_;
            if (!buf.empty()) {
                const ASTString *asts = ASTString::New(arena_, buf.data(), buf.size());
                return Token(Token::kStringTempletePart, loc).With(asts);
            }
            ch = MoveNext();
            if (ch == '{') {
                loc.end_line = line_;
                loc.end_row  = row_;
                MoveNext();
                in_string_template_.push(kExpressionTemplate);
                DCHECK(buf.empty());
                return Token(Token::kStringTempleteExpressBegin, loc);
            } else {
                return MatchIdentifier();
            }
        } else if (ch == '\\') {
            // TODO:
        } else if (ch == '\r' || ch == '\n') {
            loc.end_line = line_;
            loc.end_row  = row_;
            error_feedback_->Printf(loc, "Unexpected string term, expected '\\r' '\\n'");
            return Token(Token::kError, loc);
        } else {
            buf.append(1, ch);
            ch = MoveNext();
        }
    }
    return Token(Token::kError, {});
}

Token Lexer::MatchString() {
    int begin_line = line_, begin_row = row_;
    MoveNext();
    
    std::string buf;
    for (;;) {
        int ch = Peek();
        if (ch == '\"') {
            MoveNext();
            const ASTString *asts = ASTString::New(arena_, buf.data(), buf.size());
            return Token(Token::kStringVal, {begin_line, begin_row, line_, row_}).With(asts);
        } else if (ch == '$') {
            in_string_template_.push(kSimpleTemplate);
            const ASTString *asts = ASTString::New(arena_, buf.data(), buf.size());
            return Token(Token::kStringTempletePrefix,
                         {begin_line, begin_row, line_, row_}).With(asts);
        } else if (ch == '\\') {
            // TODO
        } else if (ch == '\n' || ch == '\r') {
            SourceLocation loc = SourceLocation::One(line_, row_);
            error_feedback_->Printf(loc, "Unexpected string term, expected '\\r' '\\n'");
            return Token(Token::kError, SourceLocation::One(line_, row_));
        } else if (ch == 0) {
            SourceLocation loc{begin_line, begin_row, line_, row_};
            error_feedback_->Printf(loc, "Unexpected string term, expected EOF");
            return Token(Token::kError, loc);
        } else {
            MoveNext();
            buf.append(1, ch);
        }
    }
}

int Lexer::Peek() {
    if (buffer_position_ < buffered_.size()) {
        return buffered_[buffer_position_];
    }
    if (available_ == 0) {
        return 0;
    }
    
    size_t wanted = std::min(static_cast<size_t>(available_), kBufferSize);
    if (auto rs = input_file_->Read(wanted, &buffered_, &scratch_); rs.fail()) {
        error_feedback_->Printf(SourceLocation::One(line_, row_), "Read file error: %s",
                                rs.ToString().c_str());
        return -1;
    }
    available_ -= wanted;
    buffer_position_ = 0;
    return buffered_[buffer_position_];
}

/*static*/ bool Lexer::IsTerm(int ch) {
    switch (ch) {
        case 0:
        case '+':
        case '-':
        case '*':
        case '/':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case '.':
        case ';':
        case ':':
        case '%':
        case '\'':
        case '\"':
        case '`':
        case '#':
        case '!':
        case '^':
        case '&':
        case '<':
        case '>':
        case '=':
        case ',':
        case '?':
        case '\\':
        case '|':
        case '$':
        case ' ':
        case '\t':
            return true;
            
        default:
            if (::isalpha(ch) || ::isdigit(ch)) {
                return true;
            }
            break;
    }
    return false;
}

} // namespace lang

} // namespace mai
