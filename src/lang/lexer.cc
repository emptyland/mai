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
                
            case '<': {
                int begin_line = line_, begin_row = row_;
                ch = MoveNext();
                if (ch == '=') { // <=
                    MoveNext();
                    return Token(Token::kLessEqual, {begin_line, begin_row, line_, row_});
                } else if (ch == '<') { // <<
                    MoveNext();
                    return Token(Token::kLShift, {begin_line, begin_row, line_, row_});
                } else if (ch == '-') { // <-
                    MoveNext();
                    return Token(Token::kLArrow, {begin_line, begin_row, line_, row_});
                } else {
                    return Token(Token::kLess, SourceLocation::One(begin_line, begin_row));
                }
            } break;
                
            case '>': {
                int begin_line = line_, begin_row = row_;
                ch = MoveNext();
                if (ch == '=') { // >=
                    MoveNext();
                    return Token(Token::kGreaterEqual, {begin_line, begin_row, line_, row_});
                } else if (ch == '>') { // >>
                    MoveNext();
                    return Token(Token::kRShift, {begin_line, begin_row, line_, row_});
                } else {
                    return Token(Token::kGreater, SourceLocation::One(begin_line, begin_row));
                }
            } break;
                
            case '=': {
                int begin_line = line_, begin_row = row_;
                ch = MoveNext();
                if (ch == '=') {
                    MoveNext();
                    return Token(Token::kEqual, {begin_line, begin_row, line_, row_});
                } else {
                    return Token(Token::kAssign, SourceLocation::One(begin_line, begin_row));
                }
            } break;

            case '+': {
                int begin_line = line_, begin_row = row_;
                ch = MoveNext();
                if (ch == '=') {
                    MoveNext();
                    return Token(Token::kPlusEqual, {begin_line, begin_row, line_, row_});
                } else if (ch == '+') {
                    MoveNext();
                    return Token(Token::kPlusPlus, {begin_line, begin_row, line_, row_});
                } else {
                    return Token(Token::kPlus, SourceLocation::One(begin_line, begin_row));
                }
            } break;
                
            case '-': {
                int begin_line = line_, begin_row = row_;
                ch = MoveNext();
                if (ch == '=') { // -=
                    MoveNext();
                    return Token(Token::kMinusEqual, {begin_line, begin_row, line_, row_});
                } else if (ch == '-') { // --
                    MoveNext();
                    return Token(Token::kMinusMinus, {begin_line, begin_row, line_, row_});
                } else if (ch == '>') { // ->
                    MoveNext();
                    return Token(Token::kRArrow, {begin_line, begin_row, line_, row_});
                } else {
                    return Token(Token::kMinus, SourceLocation::One(begin_line, begin_row));
                }
            } break;
                
            default:
                if (ch == -1) {
                    return Token(Token::kError, SourceLocation::One(line_, row_));
                }
                if (IsUtf8Prefix(ch) && !IsTermChar(ch)) {
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
        if (ch == -1 || ch == '\r' || ch == '\n') {
            error_feedback_->Printf(loc, "Unexpected identifier character, expected %x", ch);
            return Token(Token::kError, loc);
        } else if (IsTermChar(ch)) {
            Token::Kind keyword = Token::IsKeyword(buf);
            if (keyword != Token::kError) {
                return Token(keyword, loc);
            }
            const ASTString *asts = ASTString::New(arena_, buf.data(), buf.size());
            return Token(Token::kIdentifier, loc).With(asts);
        } else {
            loc.end_line = line_;
            loc.end_row  = row_;
            if (!MatchUtf8Character(&buf)) {
                return Token(Token::kError, SourceLocation::One(line_, row_));
            }
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
            if (!MatchEscapeCharacter(&buf)) {
                return Token(Token::kError, SourceLocation::One(line_, row_));
            }
        } else if (ch == '\r' || ch == '\n') {
            loc.end_line = line_;
            loc.end_row  = row_;
            error_feedback_->Printf(loc, "Unexpected string term, expected '\\r' '\\n'");
            return Token(Token::kError, loc);
        } else {
            if (!MatchUtf8Character(&buf)) {
                return Token(Token::kError, SourceLocation::One(line_, row_));
            }
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
        } else if (ch == '\\') { // Escape
            if (!MatchEscapeCharacter(&buf)) {
                return Token(Token::kError, SourceLocation::One(line_, row_));
            }
        } else if (ch == '\n' || ch == '\r') {
            SourceLocation loc = SourceLocation::One(line_, row_);
            error_feedback_->Printf(loc, "Unexpected string term, expected '\\r' '\\n'");
            return Token(Token::kError, SourceLocation::One(line_, row_));
        } else if (ch == 0) {
            SourceLocation loc{begin_line, begin_row, line_, row_};
            error_feedback_->Printf(loc, "Unexpected string term, expected EOF");
            return Token(Token::kError, loc);
        } else {
            if (!MatchUtf8Character(&buf)) {
                return Token(Token::kError, SourceLocation::One(line_, row_));
            }
        }
    }
}

bool Lexer::MatchUtf8Character(std::string *buf) {
    int ch = Peek();
    int remain = IsUtf8Prefix(ch) - 1;
    if (remain < 0) {
        error_feedback_->Printf(SourceLocation::One(line_, row_), "Incorrect UTF8 character: '%x'",
                                ch);
        return false;
    }
    MoveNext();
    buf->append(1, ch);
    for (int i = 0; i < remain; i++) {
        ch = Peek();
        if (ch == 0) {
            error_feedback_->Printf(SourceLocation::One(line_, row_), "Unexpected EOF");
            return false;
        } else if (ch == -1) {
            error_feedback_->Printf(SourceLocation::One(line_, row_), "I/O Error");
            return false;
        }

        if ((ch & 0xc0) != 0x80) {
            error_feedback_->Printf(SourceLocation::One(line_, row_),
                                    "Incorrect UTF8 character: '%x'", ch);
            return false;
        }
        MoveNext();
        buf->append(1, ch);
    }
    return true;
}

bool Lexer::MatchEscapeCharacter(std::string *buf) {
    int ch = MoveNext();
    switch (ch) {
        case '$':
            MoveNext();
            buf->append(1, '$');
            break;
        case 'r':
            MoveNext();
            buf->append(1, '\r');
            break;
        case 'n':
            MoveNext();
            buf->append(1, '\n');
            break;
        case 't':
            MoveNext();
            buf->append(1, '\t');
            break;
        case 'b':
            MoveNext();
            buf->append(1, '\b');
            break;
        case 'x':
        case 'X': {
            uint8_t byte = 0;
            ch = MoveNext();
            if (!IsHexChar(ch)) {
                error_feedback_->Printf(SourceLocation::One(line_, row_),
                                        "Incorrect hex character: %c", ch);
                return false;
            }
            byte = static_cast<uint8_t>(HexCharToInt(ch)) << 4;
            ch = MoveNext();
            if (!IsHexChar(ch)) {
                error_feedback_->Printf(SourceLocation::One(line_, row_),
                                        "Incorrect hex character: %c", ch);
                return false;
            }
            byte |= static_cast<uint8_t>(HexCharToInt(ch));
            buf->append(1, byte);
            MoveNext();
        } break;
        case 'u':
        case 'U': {
            TODO();
        } break;
        default:
            break;
    }
    return true;
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
    return bit_cast<int>(static_cast<uint32_t>(buffered_[buffer_position_]));
}

/*static*/ bool Lexer::IsTermChar(int ch) {
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
            break;
    }
    return false;
}

} // namespace lang

} // namespace mai
