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
                return MatchString(ch, true/*escape*/, false/*block*/);
                
            case '\'':
                return MatchString(ch, false/*escape*/, false/*block*/);
                
            case '`':
                return MatchString(ch, false/*escape*/, true/*block*/);

            case '(':
                return MatchOne(Token::kLParen);
                
            case ')':
                return MatchOne(Token::kRParen);

            case '[':
                return MatchOne(Token::kLBrack);
                
            case ']':
                return MatchOne(Token::kRBrack);
                
            case '{':
                return MatchOne(Token::kLBrace);
                
            case '}':
                return MatchOne(Token::kRBrace);
                
            case '*':
                return MatchOne(Token::kStar);
                
            case '/':
                return MatchOne(Token::kDiv);
                
            case '%':
                return MatchOne(Token::kPercent);
                
            case '~':
                return MatchOne(Token::kWave);
                
            case '^':
                return MatchOne(Token::kBitwiseXor);

            case '_':
                return MatchIdentifier();
                
            case '!':
                return MatchOne(Token::kNot);
                
            case '|': {
                SourceLocation loc{line_, row_};
                ch = MoveNext();
                if (ch == '|') {
                    loc.end_line = line_;
                    loc.end_row  = row_;
                    MoveNext();
                    return Token(Token::kOr, loc);
                } else {
                    loc.end_line = line_;
                    loc.end_row  = row_;
                    return Token(Token::kBitwiseOr, loc);
                }
            } break;
                
            case '&': {
                SourceLocation loc{line_, row_};
                ch = MoveNext();
                if (ch == '&') {
                    loc.end_line = line_;
                    loc.end_row  = row_;
                    MoveNext();
                    return Token(Token::kAnd, loc);
                } else {
                    loc.end_line = line_;
                    loc.end_row  = row_;
                    return Token(Token::kBitwiseAnd, loc);
                }
            } break;
                
            case '.': {
                SourceLocation loc{line_, row_};
                ch = MoveNext();
                if (ch == '.') {
                    for (int i = 0; i < 2; i++) {
                        loc.end_line = line_;
                        loc.end_row = row_;
                        ch = MoveNext();
                        if (ch != '.') {
                            error_feedback_->Printf(loc, "Unexpected `...', expected: %c", ch);
                            return Token(Token::kError, loc);
                        }
                    }
                } else {
                    return Token(Token::kDot, SourceLocation::One(loc.begin_line, loc.begin_row));
                }
            } break;
                
            case ':': {
                SourceLocation loc{line_, row_};
                ch = MoveNext();
                if (ch == ':') {
                    loc.end_line = line_;
                    loc.end_row = row_;
                    return Token(Token::k2Colon, loc);
                } else {
                    return Token(Token::kColon, SourceLocation::One(loc.begin_line, loc.begin_row));
                }
            } break;

            case '<': {
                int begin_line = line_, begin_row = row_;
                ch = MoveNext();
                if (ch == '=') { // <=
                    MoveNext();
                    return Token(Token::kLessEqual, {begin_line, begin_row, line_, row_});
                } else if (ch == '<') { // <<
                    MoveNext();
                    return Token(Token::kLShift, {begin_line, begin_row, line_, row_});
                } else if (ch == '>') { // <>
                    MoveNext();
                    return Token(Token::kNotEqual, {begin_line, begin_row, line_, row_});
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
                    return Token(Token::k2Plus, {begin_line, begin_row, line_, row_});
                } else if (::isdigit(ch)) {
                    return MatchNumber(1/*sign*/, line_, row_);
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
                    return Token(Token::k2Minus, {begin_line, begin_row, line_, row_});
                } else if (ch == '>') { // ->
                    MoveNext();
                    return Token(Token::kRArrow, {begin_line, begin_row, line_, row_});
                } else if (::isdigit(ch)) {
                    return MatchNumber(-1/*sign*/, line_, row_);
                } else {
                    return Token(Token::kMinus, SourceLocation::One(begin_line, begin_row));
                }
            } break;
                
            default:
                if (ch == -1) {
                    return Token(Token::kError, SourceLocation::One(line_, row_));
                }
                if (::isdigit(ch)) {
                    return MatchNumber(0/*sign*/, line_, row_);
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
        if (ch == -1) {
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

Token Lexer::MatchNumber(int sign, int line, int row) {
    SourceLocation loc{line, row};
    
    int ch = Peek();
    if (sign == 0) {
        if (ch == '-') {
            MoveNext();
            sign = -1;
        } else if (ch == '+') {
            MoveNext();
            sign = 1;
        } else {
            sign = 1;
        }
    }

    std::string buf;
    int base = 10;
    if (Peek() == '0') {
        ch = MoveNext();
        if (ch == 'x' || ch == 'X') {
            MoveNext();
            base = 16;
            buf.append("0x");
        } else {
            if (::isdigit(Peek())) {
                base = 8;
            }
            buf.append("0");
        }
    }

    int has_e = 0;
    Token::Kind type = Token::kIntVal;
    for (;;) {
        ch = Peek();
        if (ch == -1) {
            error_feedback_->Printf(loc, "I/O error");
            return Token(Token::kError, loc);
        } else if (::isdigit(ch)) {
            if (base == 8 && ch >= '8' && ch <= '9') {
                return OneCharacterError(line_, row_, "Unexpected number character: %c", ch);
            }
            MoveNext();
            buf.append(1, ch);
        } else if ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
            if (base != 16 || type == Token::kF32Val || type == Token::kF64Val) {
                return OneCharacterError(line_, row_, "Unexpected number character: %c", ch);
            }
            MoveNext();
            buf.append(1, ch);
        } else if (ch == 'e') {
            if (has_e++ > 0) {
                return OneCharacterError(line_, row_, "Duplicated exp `e' in floating literal");
            }
            if (base != 10) {
                return OneCharacterError(line_, row_, "Unexpected integral character: %c", ch);
            }
            MoveNext();
            buf.append(1, ch);
        } else if (ch == '.') {
            if (type == Token::kF32Val || type == Token::kF64Val) {
                return OneCharacterError(line_, row_, "Duplicated dot `.' in floating literal");
            }
            if (base != 10) {
                return OneCharacterError(line_, row_, "Unexpected integral character: %c", ch);
            }
            type = Token::kF32Val;
            buf.append(1, ch);
            MoveNext();
        } else if (ch == 'f') {
            if (base != 10) {
                return OneCharacterError(line_, row_, "Unexpected integral character: %c", ch);
            }
            ch = MoveNext();
            if (!IsTermChar(ch)) {
                return OneCharacterError(line_, row_, "Incorrect number literal suffix: f%x", ch);
            }
            type = Token::kF32Val;
        } else if (ch == 'd') {
            ch = MoveNext();
            if (!IsTermChar(ch)) {
                return OneCharacterError(line_, row_, "Incorrect number literal suffix: d%x", ch);
            }
            type = Token::kF64Val;
        } else if (ch == 'i' || ch == 'u') {
            int sign_ch = ch;
            if (type == Token::kF32Val || type == Token::kF64Val) {
                return OneCharacterError(line_, row_, "Incorrect integral suffix in floating number");
            }

            ch = MoveNext();
            if (ch == '8') {
                if (!IsTermChar(MoveNext())) {
                    return OneCharacterError(line_, row_, "Incorrect number literal suffix: %c8",
                                             sign_ch);
                }
                type = sign_ch == 'i' ? Token::kI8Val : Token::kU8Val;
            } else if (ch == '1') {
                if (MoveNext() != '6' || !IsTermChar(MoveNext())) {
                    return OneCharacterError(line_, row_, "Incorrect number literal suffix: %c16",
                                             sign_ch);
                }
                type = sign_ch == 'i' ? Token::kI16Val : Token::kU16Val;
            } else if (ch == '3') {
                if (MoveNext() != '2' || !IsTermChar(MoveNext())) {
                    return OneCharacterError(line_, row_, "Incorrect number literal suffix: %c32",
                                             sign_ch);
                }
                type = sign_ch == 'i' ? Token::kI32Val : Token::kU32Val;
            } else if (ch == '6') {
                if (MoveNext() != '4' || !IsTermChar(MoveNext())) {
                    return OneCharacterError(line_, row_, "Incorrect number literal suffix: %c64",
                                             sign_ch);
                }
                type = sign_ch == 'i' ? Token::kI64Val : Token::kU64Val;
            } else {
                type = sign_ch == 'i' ? Token::kIntVal : Token::kUIntVal;
            }
        } else if (IsTermChar(ch)) {
            loc.end_line = line_;
            loc.end_row  = row_;
            break;
        } else {
            return OneCharacterError(line_, row_, "Unexpected number character: %c", ch);
        }
    }
    
    uint64_t val = 0;
    switch (type) {
        case Token::kI8Val:
        case Token::kI16Val:
        case Token::kI32Val:
        case Token::kU8Val:
        case Token::kU16Val:
        case Token::kU32Val:
        case Token::kI64Val:
        case Token::kU64Val:
        case Token::kIntVal:
        case Token::kUIntVal: {
            int rv = 0;
            if (base == 8) {
                rv = base::Slice::ParseO64(buf.c_str(), &val);
            } else if (base == 10) {
                int64_t tmp;
                rv = base::Slice::ParseI64(buf.c_str(), &tmp);
                val = tmp;
            } else if (base == 16) {
                rv = base::Slice::ParseH64(buf.c_str(), &val);
            }
            DCHECK(rv >= 0);
            if (rv > 0) {
                error_feedback_->Printf(loc, "Integral number: %s overflow", buf.data());
                return Token(Token::kError, loc);
            }
        } break;

        case Token::kF32Val: {
            float f32 = ::atof(buf.data());
            return Token(Token::kF32Val, loc).With(f32);
        } break;

        case Token::kF64Val: {
            double f64 = ::atof(buf.data());
            return Token(Token::kF32Val, loc).With(f64);
        } break;
            
        default:
            NOREACHED();
            break;
    }

    switch (type) {
        case Token::kI8Val: {
            auto test = sign < 0 ? -static_cast<int64_t>(val) : static_cast<int64_t>(val);
            if (test < std::numeric_limits<int8_t>::min() ||
                test > std::numeric_limits<int8_t>::max()) {
                error_feedback_->Printf(loc, "Integral number: %s overflow of i8", buf.data());
                return Token(Token::kError, loc);
            }
            return Token(type, loc).With(static_cast<int32_t>(test));
        } break;
            
        case Token::kU8Val: {
            auto test = sign < 0 ? -static_cast<uint64_t>(val) : static_cast<uint64_t>(val);
            if (test < std::numeric_limits<uint8_t>::min() ||
                test > std::numeric_limits<uint8_t>::max()) {
                error_feedback_->Printf(loc, "Integral number: %s overflow of i8", buf.data());
                return Token(Token::kError, loc);
            }
            return Token(type, loc).With(static_cast<uint32_t>(test));
        } break;
            
        case Token::kI16Val: {
            auto test = sign < 0 ? -static_cast<int64_t>(val) : static_cast<int64_t>(val);
            if (test < std::numeric_limits<int16_t>::min() ||
                test > std::numeric_limits<int16_t>::max()) {
                error_feedback_->Printf(loc, "Integral number: %s overflow of i16", buf.data());
                return Token(Token::kError, loc);
            }
            return Token(type, loc).With(static_cast<int32_t>(test));
        } break;
            
        case Token::kU16Val: {
            auto test = sign < 0 ? -static_cast<uint64_t>(val) : static_cast<uint64_t>(val);
            if (test < std::numeric_limits<uint16_t>::min() ||
                test > std::numeric_limits<uint16_t>::max()) {
                error_feedback_->Printf(loc, "Integral number: %s overflow of u16", buf.data());
                return Token(Token::kError, loc);
            }
            return Token(type, loc).With(static_cast<uint32_t>(test));
        } break;
            
        case Token::kIntVal:
        case Token::kI32Val: {
            auto test = sign < 0 ? -static_cast<int64_t>(val) : static_cast<int64_t>(val);
            if (test < std::numeric_limits<int32_t>::min() ||
                test > std::numeric_limits<int32_t>::max()) {
                error_feedback_->Printf(loc, "Integral number: %s overflow of i32", buf.data());
                return Token(Token::kError, loc);
            }
            return Token(type, loc).With(static_cast<int32_t>(test));
        } break;
        
        case Token::kUIntVal:
        case Token::kU32Val: {
            auto test = sign < 0 ? -static_cast<uint64_t>(val) : static_cast<uint64_t>(val);
            if (test < std::numeric_limits<uint32_t>::min() ||
                test > std::numeric_limits<uint32_t>::max()) {
                error_feedback_->Printf(loc, "Integral number: %s overflow of u32", buf.data());
                return Token(Token::kError, loc);
            }
            return Token(type, loc).With(static_cast<uint32_t>(test));
        } break;
            
        case Token::kI64Val: {
            auto test = sign < 0 ? -static_cast<int64_t>(val) : static_cast<int64_t>(val);
            return Token(type, loc).With(test);
        } break;
            
        case Token::kU64Val: {
            auto test = sign < 0 ? -static_cast<uint64_t>(val) : static_cast<uint64_t>(val);
            return Token(type, loc).With(test);
        } break;

        default:
            NOREACHED();
            break;
    }
    return Token(Token::kError, loc);
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

Token Lexer::MatchString(int quote, bool escape, bool block) {
    SourceLocation loc{line_, row_};
    MoveNext();
    
    std::string buf;
    for (;;) {
        int ch = Peek();
        if (ch == quote) {
            loc.end_line = line_;
            loc.end_row  = row_;
            MoveNext();
            const ASTString *asts = ASTString::New(arena_, buf.data(), buf.size());
            return Token(Token::kStringLine, loc).With(asts);
        } else if (ch == '$') {
            if (escape) {
                loc.end_line = line_;
                loc.end_row  = row_;
                in_string_template_.push(kSimpleTemplate);
                const ASTString *asts = ASTString::New(arena_, buf.data(), buf.size());
                return Token(Token::kStringTempletePrefix, loc).With(asts);
            }
            MoveNext();
            buf.append(1, ch);
        } else if (ch == '\\') { // Escape
            if (escape) {
                if (!MatchEscapeCharacter(&buf)) {
                    return Token(Token::kError, SourceLocation::One(line_, row_));
                }
            } else {
                MoveNext();
                buf.append(1, ch);
            }
        } else if (ch == '\n' || ch == '\r') {
            if (!block) {
                SourceLocation loc = SourceLocation::One(line_, row_);
                error_feedback_->Printf(loc, "Unexpected string term, expected '\\r' '\\n'");
                return Token(Token::kError, SourceLocation::One(line_, row_));
            } else {
                MoveNext();
                line_++;
                row_ = 1;
                buf.append(1, ch);
            }
        } else if (ch == 0) {
            loc.end_line = line_;
            loc.end_row  = row_;
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

Token Lexer::OneCharacterError(int line, int row, const char *fmt, ...) {
    SourceLocation loc = SourceLocation::One(line_, row_);
    va_list ap;
    va_start(ap, fmt);
    std::string message(base::Vsprintf(fmt, ap));
    va_end(ap);
    error_feedback_->DidFeedback(loc, message.data(), message.size());
    return Token(Token::kError, loc);
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
        case '~':
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
        case '\r':
        case '\n':
            return true;
        default:
            break;
    }
    return false;
}

} // namespace lang

} // namespace mai
