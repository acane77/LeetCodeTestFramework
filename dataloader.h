/*
 * Leetcode Sample Loader
 *
 * BY MIYUKI,  MARCH, 2021
 * Licensed under GNU General Public License (Version 3)
 *
 * Usage:
 *  ```c++
 *      DataLoader loader;
 *      loader.load(R"sample(
 *            a = [1,2,3, 4]
 *            b=[[1,2,3,4],[4,5,[6,7,8,9]],[]]
 *            c=2.5, d=66
 *            e=["I","love","you"]
 *      )sample");
 *      float c = loader["c"];
 *      int d = loader["d"].asInt();
 *      vector<int> a = loader["a"].asArray<int>();
 *      vector<string> e = loader["e"].asArray<string>();
 *      cout << "c=" << c << endl;
 *      cout << "d=" << d << endl;
 *      cout << "sizeof a=" << a.size() << endl;
 *      cout << "e[1]=" << e[1] << endl;
 *      cout << "b[1][2][3]=" << loader["b"][1][2][3].asInt() << endl;
 *  ```
 * */

#ifndef DP_DATALOADER_H
#define DP_DATALOADER_H

#include <cstdint>
#include <string>
#include <memory>
#include <map>
#include <deque>
#include <iostream>
#include <typeinfo>
#include <vector>
#include <sstream>
#include <functional>
#include <cassert>

using namespace std;

// ===============      Defines     =====================
#define ishexdigit(x) ( isdigit(x) || (x >= 'A' && x <= 'F') || (x >= 'a' && x <= 'f') )
#define isoctdigit(x) ( x >= '0' && x <= '8' )

typedef long double FloatingLiteralType;
typedef int64_t     IntegerLiteralType;

// =============== Type Pre-Definitions =================
#define DEFINE_SHARED_PTR(type) class type; typedef std::shared_ptr<type> type##Ptr;
#define DEFINE_WEAK_PTR(type)   class type; typedef std::weak_ptr<type> type##PtrW;
#define DEFINE_UNIQUE_PTR(type) class type; typedef std::unique_ptr<type> type##Ptr;
#define DEFINE_RAW_PTR(type) class type; typedef type* type##Ptr;

// ***** Source Manager *****
DEFINE_SHARED_PTR(FileRead)
DEFINE_SHARED_PTR(SourceManager)

// ***** Token *****
DEFINE_SHARED_PTR(Token)
DEFINE_SHARED_PTR(WordToken)
DEFINE_SHARED_PTR(IntToken)
DEFINE_SHARED_PTR(FloatToken)
DEFINE_SHARED_PTR(CharToken)
DEFINE_SHARED_PTR(StringToken)

DEFINE_SHARED_PTR(Lexer)
DEFINE_SHARED_PTR(IParser)

typedef deque<TokenPtr> TokenSequence;
typedef shared_ptr<TokenSequence> TokenSequencePtr;
typedef TokenSequence::iterator TokenSequenceIter;

// ===============    Stream Reader     ===================
class FileRead {
    int column = 0;
    int row = 0;
    int max_row = 0;

    deque<string> lines;
    string line;

    string  filename;
    std::iostream* M_Stream;

public:
    FileReadPtr includeFrom = nullptr;
    int includeFromLine = 0;

public:
    explicit FileRead(std::iostream& stream) {
        filename = "<string>";
        M_Stream = &stream;
    }

    bool nextLine() {
        char p;
        string line_save = line;

        if (max_row != row) {
            // Current row is not the last row
            if (row + 1 >= lines.size()) return false;
            column = 0;
            line = lines[++row];
            return true;
        }

        // Current row is last row
        bool enc_eof = false;

        line = "";

        if ((*M_Stream).eof())  return false;

        // Read and append char to string until new_line or EOF
        while (1) {
            (*M_Stream).get(p);
            if ((*M_Stream).eof()) { enc_eof = true; break; }
            if (p != '\r') line += p;  // ignore CR
            if (p == '\n') break;
        }
        column = 0;
        row++;
        max_row++;
        lines.push_back(line);

        if (enc_eof) {
            // For convince of retract, here still column add 1
            column++;
            return true;
        }
        //if (line.length())
        return true;
    }
    bool lastLine() {
        if (row == 0) return false;
        column = 0;
        line = lines[--row];
        return true;
    }
    int getColumn() { return column; }
    int getRow() { return row; }
    string getLine(int i) { return lines[i]; }
    string getLine() { return line; }
    void to(int r, int c = 0) { row = r; column = c; line = lines[r]; }

    int nextChar() {
        while (column >= line.length()) {
            if (!nextLine()) return -1;
            column = 0;
        }
        return line[column++];
    }
    int lastChar() {
        while (column - 1 < 0) {
            if (!lastLine()) return -1;
            column = line.length();
        }
        return line[--column];
    }
    const string& getFilename() const { return filename; }
};

// ===============    Source Manager    ===================
class SourceManager {
    // Current file
    FileReadPtr  currFile;
public:
    SourceManager() { currFile = nullptr; }

    // Operations for the file
    bool nextLine() { return currFile->nextLine(); }
    bool lastLine() { return currFile->lastLine(); }

    int getColumn() { return currFile->getColumn(); }
    int getRow() { return currFile->getRow(); }
    string getLine(int i) { return currFile->getLine(i); }
    string getLine() { return currFile->getLine(); }
    void to(int r, int c = 0) { currFile->to(r, c); }
    inline int _nextChar() { return currFile->nextChar(); }

    int nextChar() {
        int ch = _nextChar();
        // eat \r
        if (ch == '\r') return nextChar();
        // eat \\\n
        if (ch == '\\') {
            ch = _nextChar();
            if (ch == '\n') return nextChar();
            lastChar(); return '\\';
        }
        return ch;
    }
    int lastChar() { return currFile->lastChar(); }
    void openFile(std::iostream& stream) {
        FileReadPtr fr = make_shared<FileRead>(stream);
        currFile = fr;
    }
    const string& getCurrentFilename() const { return currFile->getFilename(); }
};

// =============== Token Tag Definitions =================
namespace Tag {
    enum : int32_t {
        EndOfFile = -1,
        ID = 1 << 8,
        Integer = 1 << 9,
        Floating = 1 << 10,
        Character = 1 << 12,
        StringLiteral = 1 << 13,
        Number = Integer | Floating,
        BaseFactor = Integer | Floating | Character | StringLiteral,
        Identifier = ID,

        // Properties
        PunctuatorStart = 0,
        PunctuatorEnd = 128,
        Punctuator = -2,
    };
}

// ===============     Util Classes     ===================
class IOException : public exception {
    string msg;
public:
    bool isWarning;
    IOException(string& _msg) { msg = std::move(_msg); }
    IOException(string&& _msg) { msg = _msg; }
    IOException() { msg = "No such file or directory."; }

    const char* what() const noexcept override {
        return msg.c_str();
    }
};

class InvalidToken : public exception {
public:
    const char* what() const noexcept override {
        return "invalid token found.";
    }
};

class SyntaxError : public exception {
    string msg;
public:
    SyntaxError(string& _msg) { msg = std::move(_msg); }
    SyntaxError(string&& _msg) { msg = _msg; }

    const char* what() const noexcept override {
        return msg.c_str();
    }
};

class ParseError : public exception {
    // normal error message
    string msg;
    // more info string
    string info;
    // suggest replace for invaid token
    string suggest;
    // error token
    TokenPtr tok;
    // show if this is a warning
    bool warning;

public:
    const string& getInfo() const { return info; }
    void setInfo(const string& info) { ParseError::info = info; }
    const string& getSuggest() const { return suggest; }
    void setSuggest(const string& suggest) { ParseError::suggest = suggest; }

public:
    ParseError(string&& _msg, TokenPtr _tok, bool _warning) { msg = std::move(_msg); tok = _tok; warning = _warning; }
    ParseError(string& _msg, TokenPtr _tok, bool _warning) { msg = std::move(_msg); tok = _tok; warning = _warning; }

    const char* what() const noexcept override { return msg.c_str(); }
    const TokenPtr getToken() { return tok; }
    bool isWarning() { return warning; }
};

class PasreCannotRecoveryException : public exception {
public:
    const char* what() const noexcept override { return "compilation failed due to errors."; }
};

class Encoding {
private:
    uint32_t encoding;
    const char* encodingString;

public:
    enum Prefix { U = 0, u, u8, L, ASCII };
    explicit Encoding(uint32_t _encoding) { setEncoding(_encoding); }
    Encoding& operator=(Encoding& enc) { encoding = enc.encoding; encodingString = enc.encodingString; return *this; }
    Encoding& operator=(uint32_t _encoding) { setEncoding(_encoding); return *this; }
    uint32_t getEncoding() { return encoding; }
    const char* getEncodingString() { return encodingString; }
    void setEncoding(uint32_t encoding) {
        Encoding::encoding = encoding;
        if (encoding == U) encodingString = "U";
        else if (encoding == u) encodingString = "u";
        else if (encoding == u8) encodingString = "u8";
        else if (encoding == L) encodingString = "L";
        else if (encoding == ASCII) encodingString = "";
        else assert(false && "invalid encoding");
    };
    static uint32_t getEncodingFromString(std::string& enc) {
        if (enc == "U")  return U;
        else if (enc == "u") return u;
        else if (enc == "u8") return u8;
        else if (enc == "L") return L;
        else assert(false && "invalid encoding");
    }
};

// ===============        Tokens        ===================
class Token {
public:
    // information in source code
    int startCol;
    int column;
    int row;
    int chrlen;
    const char* filenam;

    // characteristic information
    int32_t tag;

    // use for getting know about more information
    inline static SourceManagerPtr flread;
    inline static int startColumn;

    explicit Token(int32_t _tag) {
        tag = _tag;
        if (flread) {
            column = flread->getColumn(); row = flread->getRow() - 1; //row returned by lexer is next row nuber so minus 1
            filenam = flread->getCurrentFilename().c_str(); chrlen = column - startColumn;
            startCol = startColumn;
        }
        else filenam = "";
    }

    virtual string toString() {
        if (tag > 31 && tag < 127)  return string("") + ((char)tag);
        return toSourceLiteral();
    }

    // Generate string in source code
    virtual string toSourceLiteral() {
        if (tag == Tag::EndOfFile)
            return "EOF";
        return "Tag";
    }

    // test if this is _tag
    bool is(int32_t _tag) {
        // if _tag passes special kind, then compare
        if (_tag == Tag::Punctuator)  return tag >= Tag::PunctuatorStart && tag <= Tag::PunctuatorEnd;
        // expected token is a punctuator or keyword or EOF - using literal value comparsion
        // Type comparsion - using bitwise-and
        return ((_tag >= Tag::PunctuatorStart && _tag <= Tag::PunctuatorEnd) || _tag == Tag::EndOfFile || tag == Tag::EndOfFile) ? (_tag == tag) : (_tag & tag);
    }

    bool isNot(int32_t _tag) { return !is(_tag); }

    // type conversion (used by AST eval)
    virtual FloatingLiteralType toFloat() { assert(false && "this token cannot convert to floating point"); return 0.0; }
    virtual IntegerLiteralType toInt() { assert(false && "this token cannot convert to integer"); return 0; }
    virtual void setValue(FloatingLiteralType f) { assert(false && "this token cannot set value of floating point"); }
    virtual void setValue(IntegerLiteralType i) { assert(false && "this token cannotset value of integer"); }

    // other
    void copyAdditionalInfo(const TokenPtr& tok) {
        startCol = tok->startCol;
        row = tok->row;
        column = tok->column;
        filenam = tok->filenam;
    }
};

// Identifier & Keyword & Enumration
class WordToken : public Token {
public:
    string name;

    // For Identifier
    explicit WordToken(string&& _name) :Token(Tag::ID) { name = _name; }
    explicit WordToken(string& _name) :Token(Tag::ID) { name = move(_name); }

    // For keywords
    WordToken(int32_t _tag, string&& _name) :Token(_tag) { name = _name; }
    WordToken(int32_t _tag, string& _name) :Token(_tag) { name = move(_name); }

    string toString() override { return name; }
    string toSourceLiteral() override { return name; }
};

// Integer Constant
class IntToken : public Token {
public:
    short bit;
    uint64_t value;
    bool  isSigned;
    IntegerLiteralType signedValue;

    IntToken(uint64_t _value, bool _isSigned = true, short _bit = 32) :Token(Tag::Integer) { value = _value; isSigned = _isSigned; bit = _bit; signedValue = value; }
    string toString() override { return "<Integer>"; }
    string toSourceLiteral() override { return "<Integer>"; }
    IntegerLiteralType toInt() override { return signedValue; } /// TODO: int to string
    FloatingLiteralType toFloat() override { return (FloatingLiteralType)signedValue; }
    void setValue(IntegerLiteralType v) override { value = (uint64_t)v; signedValue = v; }
};

// Floating Constant
class FloatToken : public Token {
public:
    short bit;
    FloatingLiteralType value;

    FloatToken(FloatingLiteralType _value, short _bit) :Token(Tag::Floating) { value = _value; bit = _bit; }
    string toString() override { return "<Floating>"; }
    string toSourceLiteral() override { return "<Floating>"; }

    FloatingLiteralType toFloat() override { return value; }
    IntegerLiteralType toInt() override { return (IntegerLiteralType)value; }
    void setValue(FloatingLiteralType f) override { value = f; }
    void setValue(IntegerLiteralType i) override { value = (FloatingLiteralType)i; }
};

// Charater Constant
class CharToken : public Token {
    void _setIntValue() {
        if (!charseq.length()) throw SyntaxError("empty character constant");
        value = 0;
        for (int i = charseq.length() - 1; i >= 0; i--)
            value = (value << 8) | charseq[i];
    }
public:
    uint64_t value;
    Encoding encoding;
    string   charseq;

    explicit CharToken(string&& _charseq, int32_t enc) :Token(Tag::Character), encoding(enc), charseq(std::move(_charseq)) {
        _setIntValue();
    }
    explicit CharToken(string& _charseq, int32_t enc) :Token(Tag::Character), encoding(enc), charseq(std::move(_charseq)) {
        _setIntValue();
    }
    string toString() override { return "Character"; }
    string toSourceLiteral() override { return "Character"; }

    IntegerLiteralType toInt() override { return (IntegerLiteralType)value; }
    FloatingLiteralType toFloat() override { return (FloatingLiteralType)value; }
};

// String Literals
class StringToken : public Token {
public:
    string value;
    Encoding encoding;

    explicit StringToken(string&& _value, int32_t enc) : Token(Tag::StringLiteral), encoding(enc) { value = _value; }
    explicit StringToken(string& _value, int32_t enc) : Token(Tag::StringLiteral), encoding(enc) { value = move(_value); }
    string toString() override { return value; }
    string toSourceLiteral() override { return value; }
};

// ================ Lexer =================
class Lexer {
protected:
    SourceManagerPtr M_sm;
    int peak;

    // Get character value with a slash
    char _getCharFromSlash() {
        short chr;
        if (peak == '\\') {
            readch();
            if (peak == '\'' || peak == '\"' || peak == '?' || peak == '\\') chr = peak;
            else if (peak == 'a') chr = '\a';
            else if (peak == 'b') chr = '\b';
            else if (peak == 'f') chr = '\f';
            else if (peak == 'n') chr = '\n';
            else if (peak == 'r') chr = '\r';
            else if (peak == 't') chr = '\t';
            else if (peak == 'v') chr = '\v';
            else if (peak >= '0' && peak <= '7') {
                // octal-digit
                short x = peak - '0'; readch();
                if (peak >= '0' && peak <= '7') {
                    x = x * 8 + peak - '0'; readch();
                    if (peak >= '0' && peak <= '7') {
                        x = x * 8 + peak - '0';
                    }
                    else retract();
                }
                else retract();
                chr = x;
            }
            else if (peak == 'x') {
                // hexadecimal-escape-sequence
                short hex = 0;
                readch();
                if (!(isdigit(peak) || (peak >= 'a' && peak <= 'f') || (peak >= 'A' && peak <= 'D')))
                    diagError("Invalid hexadecimal-escape-sequence");
                for (; isdigit(peak) || (peak >= 'a' && peak <= 'f') || (peak >= 'A' && peak <= 'D'); readch()) {
                    hex = hex * 16 + peak - '0';
                }
                retract();
                chr = hex;
            }
            else diagError("Invalid escape sequence");
        }
        else chr = peak;
        return chr;
    }
    // Skip commemt and spaces, returns no token.
    // Return value: wheather program meets EOF.
    virtual bool eatCommentAndSpaces() {
        for (readch(); ; readch()) {
            if (peak == Tag::EndOfFile) return true;
            else if (peak == ' ' || peak == '\t' || peak == '\r' || peak == '\n') continue;
            else if (peak == '/') {
                readch();
                if (peak == '/') {
                    for (; ; readch()) {
                        if (peak == '\n')  break;
                        if (peak == Tag::EndOfFile)   return true;
                    }
                }
                else if (peak == '*') {
                    readch();
                    for (; ; readch()) {
                        if (peak == Tag::EndOfFile)   diagError("Comment not closed.");
                        if (peak == '*') {
                            readch();
                            if (peak == '/') break;
                            else             retract();
                        }
                    }
                }
                else {
                    // if code runs here, it represent that the '/' is as a divide instead of a comment
                    // So break here
                    retract(); peak = '/';
                    break;
                }
            }
            else break;
        }
        return false;
    }
    // Scan punctuators
    virtual TokenPtr scanPunctuators() {
        int singelPunc = peak;
        if (singelPunc == '-')
            return scanIntegerAndFloating();
        return make_shared<Token>(singelPunc);
    }
    virtual TokenPtr scanKeywordOrIdentifier(string& word) {
        if (word == "true")  return make_shared<IntToken>(1);
        if (word == "false") return make_shared<IntToken>(0);
        return make_shared<WordToken>(word);
    }
    virtual TokenPtr scanIdentifierAndStringCharacterLiteral() {
        // Identifiers and Keywords, string-literal, character
        uint32_t encoding = Encoding::ASCII;
        if (isalpha(peak) || peak == '_' || peak == '\\') {
            string word = "";
            for (; isalnum(peak) || peak == '_' || peak == '\\'; readch()) {
                if (peak == '\\') {
                    diagError("My implementation does not support universal character name.");
                }
                word += peak;
            }
            // Test if it is prefix of character
            if (peak == '\"' || peak == '\'') {
                bool thisTokenIsAIdentifier = false;
                if (word == "u") encoding = Encoding::u;
                else if (word == "L") encoding = Encoding::L;
                else if (word == "U") encoding = Encoding::U;
                else if (word == "u8" && peak == '\"') encoding = Encoding::u8; // for string-literal only
                else thisTokenIsAIdentifier = true;
                if (!thisTokenIsAIdentifier)
                    // scan string-literal and character
                    return scanStringCharacterLiteral(encoding);
            }
            retract();
            return scanKeywordOrIdentifier(word);
        }
        else if (peak == '\"' || peak == '\'')
            return scanStringCharacterLiteral(encoding);

        assert(false && "This is not a (Identifiers and Keywords, string-literal, character)");
    }
    virtual TokenPtr scanStringCharacterLiteral(uint32_t encoding) {
        if (peak == '\'' || peak == '"') {
            int quotationMark = peak;
            string chrseq;
            readch();
            for (; ; readch()) {
                if (peak == quotationMark) break;
                else if (peak == '\r' || peak == '\n') diagError("Unexpected new-line");
                else if (peak == -1) diagError("Unexpected end-of-file");
                else chrseq += _getCharFromSlash();
            }

            return quotationMark == '\"' ?
                static_pointer_cast<Token>(make_shared<StringToken>(chrseq, encoding)) :
                static_pointer_cast<Token>(make_shared<CharToken>(chrseq, encoding));
        }

        assert(false && "this is not a string-literal nor character");
    }
    virtual TokenPtr scanIntegerAndFloating() {
        // Integer and floating constant
        FloatingLiteralType floating;
        // Integer of integer part of floating
        uint64_t intValue = 0;
        // Ary of the integer (or integer part of floating)
        int ary = 10;
        // Integer has unsigbed suffix (u)
        bool hasUnsignedSuffix = false;
        // Integer or floating has long suffix (L)
        bool hasLongSuffix = false;
        // Integer has long long suffix (LL)
        bool hasLongLongSuffix = false;
        // Test if there's a floating suffix (F)
        bool hasFloatingSuffix = false;
        // Floating or Integer suffix is vilid
        bool suffixInvalid = false;
        // Represent if code goes to floating test directly instead of enterring to integer
        bool gotoFloatingDirectly = true;
        // Store suffix for feature use
        string suffix;
        // Is negative number
        bool isNegativeNumber = false;

        if (peak == '-') {
            isNegativeNumber = true; readch();
        }

        if (isdigit(peak)) {
            gotoFloatingDirectly = false;
            if (peak != '0') {
                // decimal-constant
                ary = 10;
                for (; isdigit(peak); readch())
                    intValue = intValue * ary + peak - '0';
            }
            else {
                readch();
                if (peak == 'x' || peak == 'X') {
                    // hexadecimal-constant
                    ary = 16;
                    for (readch(); isdigit(peak) || (peak >= 'A' && peak <= 'F') || (peak >= 'a' && peak <= 'f'); readch())
                        intValue = intValue * ary + (isdigit(peak) ? peak - '0' : 10 + (peak >= 'A' && peak <= 'F' ? peak - 'A' : peak - 'a'));
                }
                else {
                    // octal-constant
                    ary = 8;
                    for (; isdigit(peak); readch())
                        intValue = intValue * ary + peak - '0';
                }
            }

            // if next char is dot it is a floting-constant
            if (peak == '.'); // do nothing
                // floating (hexadecimal)
            else if (peak == 'e') {
                floating = intValue;
            add_exponment:
                int exponment = 0;  //faction * 1/16
                bool positive = true;
                if (readch(); peak == '-' || peak == '+') { positive = peak == '+'; readch(); }
                if (!isdigit(peak)) diagError("exponent has no digits");
                for (; isdigit(peak); readch())
                    exponment = exponment * 10 + peak - '0';
                for (int i = 0; i < exponment; i++) {
                    if (positive) floating *= ary;
                    else floating /= ary;
                }
                // Add floating-constant suffix below
                goto add_floating_suffix;
            }
            // integer-suffix
            else {
                for (; isalnum(peak); readch()) {
                    suffix += (char)peak;
                    if (suffixInvalid) continue;
                    if ((peak == 'u' || peak == 'U') && !hasUnsignedSuffix)
                        hasUnsignedSuffix = true;
                    else if ((peak == 'l' || peak == 'L') && !(hasLongLongSuffix || hasLongSuffix)) {
                        char save = (char)peak; readch();
                        if (peak == save) { suffix += (char)peak; hasLongLongSuffix = true; }
                        else { retract(); hasLongSuffix = true; }
                    }
                    else if ((peak == 'f' || peak == 'F') && !hasFloatingSuffix)
                        hasFloatingSuffix = true;
                    else suffixInvalid = true;
                }

                if (suffixInvalid) diagError("Invalid integer suffix '{0}'.");
                if (hasFloatingSuffix) {
                    if (hasUnsignedSuffix || hasLongLongSuffix || hasLongSuffix) diagError("Invalid floating constant suffix '{0}'.");
                    floating = intValue;
                    goto convert_to_floating_here;
                }
                retract();
            it_must_be_an_integer:
                short bit;
                if (hasLongLongSuffix) bit = 64;
                else if (hasLongSuffix) bit = 32;
                //else if ((hasUnsignedSuffix && intValue < INT16_MAX) || (!hasUnsignedSuffix && intValue < INT16_MAX / 2) ) bit = 16;
                else bit = 32;
                return make_shared<IntToken>(isNegativeNumber ? -(int64_t)intValue : intValue, hasUnsignedSuffix, bit);
            }

        }

        // Floating-constant (decimal)
        if (peak == '.') {
            readch(); FloatingLiteralType decimalPart;
            if (peak == '.') {
                // why retract here? if here is .. it reads two more characters than
                // nessesarry, so retract is required.
                // if !gotoFloatingDirectly source like '123...', integer + ...
                if (!gotoFloatingDirectly) { retract(); retract(); goto it_must_be_an_integer; }
                // it may be ..., .. is also possible
                // retract is also required but only once.
                // we can manually set peak = '.' because we know what peak should be
                else { retract(); peak = '.'; return nullptr; };
            }
            if (gotoFloatingDirectly && !(isdigit(peak))) {
                // floating like .12 = 0.12, it must be a decimal number.
                // assign directly just becsuse peak must be dot
                retract(); peak = '.'; return nullptr;
            }

            if (intValue == 0 && ary == 8) ary = 10;
            if (ary == 8)  diagError("Invalid number.");
            else if (ary == 10) {
                FloatingLiteralType fraction = 0.0, factor = 0.1;
                int exponment = 0;
                for (; isdigit(peak); readch()) {
                    fraction = fraction + (peak - '0') * factor;
                    factor = factor / 10;
                }
                if (peak == '.') diagError("too many decimal points in number");
                if (peak == 'e' || peak == 'E') { floating = intValue + fraction;  goto add_exponment; }
                decimalPart = fraction;
            }
            else if (ary == 16) {
                int fraction = 0, exponment = 0;  //faction * 1/16
                bool positive = true;
                for (; isdigit(peak) || (peak >= 'A' && peak <= 'F') || (peak >= 'a' && peak <= 'f'); readch())
                    fraction = fraction * 16 + (isdigit(peak) ? peak - '0' : 10 + (peak >= 'A' && peak <= 'F' ? peak - 'A' : peak - 'a'));
                if (peak == '.') diagError("too many decimal points in number");
                if (peak != 'p' && peak != 'P') diagError("hexadecimal floating constants require an exponent");
                if (readch(); peak == '-' || peak == '+') { positive = peak == '+'; readch(); }
                for (; isdigit(peak); readch())
                    exponment = exponment * 10 + peak - '0';
                if (positive) decimalPart = (FloatingLiteralType)fraction / 16 * (1 << exponment);
                else          decimalPart = (FloatingLiteralType)fraction / 16 / (1 << exponment);
            }
            floating = ((FloatingLiteralType)intValue) + decimalPart;
        add_floating_suffix:
            // floting-constant suffix
            for (; isalnum(peak); readch()) {
                suffix += peak;
                if (suffixInvalid) continue;
                if ((peak == 'f' || peak == 'F') && !(hasFloatingSuffix || hasLongSuffix))
                    hasFloatingSuffix = true;
                else if ((peak == 'l' || peak == 'L') && !(hasFloatingSuffix || hasLongSuffix))
                    hasLongSuffix = true;
                else suffixInvalid = true;
            }
        convert_to_floating_here:
            if (suffixInvalid) diagError("Invalid floating constant suffix.");

            short bit;
            if (hasFloatingSuffix) bit = 32;
            else if (hasLongSuffix) bit = 64;
            else bit = (*((uint64_t*)(&floating)) >> 32) ? 32 : 64;

            if (peak == '.') diagError("too many decimal points in number");

            retract();
            return make_shared<FloatToken>(isNegativeNumber ? -floating : floating, bit);
        }

        diagError("A negative operator without a number is not allowed.");
        return nullptr;
    }
    // error recovery
    bool  detectAnError = false;
    void eatAnyNonBlankChar() {
        for (; peak != ' ' && peak != '\n' && peak != '\r' && peak != '\t' && peak != -1; readch());
    }
    inline void diagError(string e) { detectAnError = true; eatAnyNonBlankChar(); throw SyntaxError(e); }

public:
    explicit Lexer() {
        M_sm = make_shared<SourceManager>();
    }
    explicit Lexer(bool doNotInitSM) { }

    void openFile(iostream& stream) { M_sm->openFile(stream); }
    const string& getFileName() { return M_sm->getCurrentFilename(); }

    virtual void readch() { peak = M_sm->nextChar(); }
    virtual void retract() { peak = M_sm->lastChar(); }
    string getLine(int i) { return M_sm->getLine(i); }
    string getLine() { return M_sm->getLine(); }
    string getCurrentLine() { return M_sm->getLine(); }
    void backToPos(int row, int col) { M_sm->to(row, col); }
    virtual int getColumn() { return M_sm->getColumn(); }
    virtual int getRow() { return M_sm->getRow(); }
    SourceManagerPtr getSourceManager() { return M_sm; }

    virtual TokenPtr scan() {
        // Eat comments and wite spaces
        if (eatCommentAndSpaces()) return make_shared<Token>(Tag::EndOfFile);

        Token::startColumn = getColumn();

        /// Integer & Floating constant
        if (isdigit(peak) || peak == '.') {
            TokenPtr ptr = scanIntegerAndFloating();
            if (ptr) return ptr;
        }

        /// Identifier, Keywords, String & Character Literal
        if (isalpha(peak) || peak == '_' || peak == '\\' || peak == '\'' || peak == '"')
            return scanIdentifierAndStringCharacterLiteral();

        // Punctuators
        return scanPunctuators();
    }

    // returns invalid token, for error reporting, not for parsing
    TokenPtr getLexedInvalidToken() {
        assert(detectAnError && "invalid call of Lexer::getLexedInvalidToken()");
        detectAnError = false;
        return make_shared<Token>(-1);
    }

    ~Lexer() { }
};

// ========== Parser ============
class IParser {
protected:
    LexerPtr M_lex;
    // lookahead token
    TokenPtr look;
    // thrown exceptions (common used by all instance)
    deque<ParseError> errors;
    // error count (Note: errorCount != length of error as warning items is also in errors)
    size_t errorCount = 0;

    struct CommonParserState {
        // if compiler meet error
        bool encountErrors = false;

    } commonParserState;

    enum {
        // max number of retractable tokens
        MaxRetractSize = 10
    };

    // Saved and retracted tokens
    TokenPtr tokens[MaxRetractSize];
    // Pointer to read token
    int     m_tsptr_r = -1;
    // Pointer to write token
    int     m_tsptr_w = -1;

    // Cache token
    void    cacheToken(TokenPtr tok);

    /////  token matching //////
    // test match, if not, throw exception with msg
    void match(uint32_t term, string&& errmsg, TokenPtr& ptr);

    // get next token from lexer
    virtual TokenPtr next();
    // Note: retract is 'put back' 1 token , not read the value of
    //       previous token.
    virtual TokenPtr retract();
public:
    ///// error recovery //////
    // error-recovery flag
    enum RecoveryFlag : uint32_t {
        SkipUntilSemi = 1 >> 0,  // Stop at ';'
        KeepSpecifiedToken = 1 >> 1,  // When find specified token ,keep it
        ConsumeSpecifiedToken = 1 >> 2, // When find specified token , consume it
        SkipUntilEndOfFile = 1 >> 3 // Stop at EOF, if not found specified
    };

    // test recovery flag
    bool hasRecoveryFlag(uint32_t a, uint32_t b) { return a & b; }

    // skip tokens
    // if returns false means we do not find token we want
    // if returns true means we've found tokens we need
    // skipUntil usage:  if next token isn't what we want, then specify tokens and skipping
    //    flags, if return true, then let's continue parsing, otherwise bail out
    virtual bool skipUntil(const deque<int32_t>& toks, uint32_t flag);

    // add error statement prepare for output
    inline void diagError(string&& errmsg, TokenPtr ptr) { errors.push_back(ParseError(errmsg, ptr, false)); errorCount++; }

    // add warning statement prepare for output
    // warning information can be disabled by flag
    inline void diagWarning(string&& errmsg, TokenPtr ptr) { errors.push_back(ParseError(errmsg, ptr, true)); }

    ////// parser state //////
    // called when parse done
    virtual void parseDone();

    // report error from exception
    virtual void reportError(std::ostream& os);

    // parse source file
    virtual void parse() = 0;
};

void IParser::match(uint32_t term, string&& errmsg, TokenPtr& ptr) {
    if (look->isNot(term)) {
        diagError(std::move(errmsg), ptr);
        if (look->is(-1))  parseDone();
        if (!skipUntil({ ';' }, RecoveryFlag::KeepSpecifiedToken | RecoveryFlag::SkipUntilSemi))
            parseDone();
        return;
    }
    next();
}

TokenPtr IParser::next() {
reget_token:
    if (m_tsptr_r == m_tsptr_w) {
        try {
            look = M_lex->scan();
            tokens[(++m_tsptr_w) % MaxRetractSize] = look;
            m_tsptr_r++;
        }
        catch (SyntaxError& err) {
            // notice: we cannot pass look token, because lexer do not generate
            //   token at all if there's lexical error, so generate a token for
            //   error mamually.
            diagError(err.what(), M_lex->getLexedInvalidToken());
            goto reget_token;
        }
        //cout << " -- GET TOKEN FROM STREAM: " << look->toString() << endl;
        return look;
    }
    else if (m_tsptr_r < m_tsptr_w) {
        look = tokens[(++m_tsptr_r) % MaxRetractSize];
        //cout << " -- GET TOKEN FROM CACHE: " << look->toString() << endl;
        return look;
    }
    else assert(false && "stack overflow.");
}

TokenPtr IParser::retract() {
    if (m_tsptr_w - m_tsptr_r > MaxRetractSize || m_tsptr_r <= 0)
        assert(false && "Invaild retract operation.");
    look = tokens[(--m_tsptr_r) % MaxRetractSize];
    return look;
}

void IParser::reportError(std::ostream& os) {
    for (ParseError& e : errors) {
        TokenPtr tok = e.getToken();
        if (!tok) {
            cout << "<Unknown source>\n^~~~~~~~~~~~~~~~\n";
            os << "<unknown location>: " << e.what() << endl << endl;
            continue;
        }
        string s = M_lex->getSourceManager()->getLine(tok->row);
        for (int i = 0; i < s.length(); i++) {
            if (s[i] == '\t') os << "    ";
            else if (s[i] != '\n') os << s[i];
        }
        os << endl;
        // output blank before token
        for (int i = 0; i < tok->startCol - 1; i++) {
            if (s[i] == '\t') os << "    ";
            else os << ' ';
        }
        os << "^";
        for (int i = tok->startCol; i < tok->column; i++) {
            if (s[i] == '\t') os << "~~~~";
            else os << '~';
        }
        os << endl;
        os << tok->filenam << ":" << tok->row << ":" << tok->column << ": " << e.what() << endl << endl;
    }
}

void IParser::parseDone() {
    reportError(cout);
    if (errorCount != 0)  throw PasreCannotRecoveryException();
}

bool IParser::skipUntil(const deque<int32_t>& toks, uint32_t flag) {
    // First we find token we want
    for (; ; next()) {
        for (int32_t expect : toks) {
            if (look->is(expect)) {
                if (hasRecoveryFlag(flag, RecoveryFlag::KeepSpecifiedToken));// do nothing
                else next();
                return true;
            }
        }

        if (hasRecoveryFlag(flag, RecoveryFlag::SkipUntilSemi) && look->is(';'))
            return false;

        // meet EOF and is required
        if (look->is(-1) && toks.size() == 1 && toks[0] == -1)
            return true;

        // token runs out
        if (look->is(-1))
            return false;

        // TODO: add_special_skip_rules
    }
}

void IParser::cacheToken(TokenPtr tok) {
    assert(m_tsptr_r == m_tsptr_w && "read pointer is not at top of stack");
    tokens[(++m_tsptr_w) % MaxRetractSize] = tok;
    m_tsptr_r++;
}

std::ostream& operator<<(std::ostream& os, const TokenSequence& tokenSeq) {
    for (int i = 0; i < tokenSeq.size(); i++) {
        os << "TokenSequence: " << tokenSeq[i]->toString() << endl;
    }
    return os;
}

// ================= AST Def =================
class ErrorReportSupport {
public:
    TokenPtr  tok = nullptr;

    TokenPtr getErrorToken() { return tok; }
    void setErroToken(const TokenPtr& _tok) {
        tok = _tok;
    }

    // global value, provide token hold support, directly use it when wang to set err_token for
    // a symbol
    static inline TokenPtr holdToken;
    static void holdErrorToken(TokenPtr tok) {
        holdToken = tok;
    }

    ErrorReportSupport() {
        tok = holdToken;
        holdToken = nullptr;
    }
};

DEFINE_SHARED_PTR(Symbol)

class Symbol : public std::enable_shared_from_this<Symbol>, public ErrorReportSupport {
public:
    enum Kind {
        SYMBOL = 0,

        decls, decl, factor, arrayKind
    };

    virtual int getKind() { return Kind::SYMBOL; }
    virtual void printAST(int hierarchy) {
        printHierarchy(hierarchy, "Symbol");
    };
    void printHierarchy(int hierarchy, string name) {
        cout << endl;
        for (int i = 0; i < hierarchy; i++) {
            cout << "  |";
        }
        cout << "- " << name;
    };
};

DEFINE_SHARED_PTR(DeclsSymbol)
DEFINE_SHARED_PTR(DeclSymbol)
DEFINE_SHARED_PTR(FactorSymbol)
DEFINE_SHARED_PTR(ArraySymbol)
DEFINE_SHARED_PTR(ArrayItemSymbol)

class DeclsSymbol : public Symbol {
public:
    deque<DeclSymbolPtr> decls;
    virtual int getKind() { return Kind::decls; }
    virtual void printAST(int hierarchy);
};

class DeclSymbol : public Symbol {
public:
    WordTokenPtr id;
    FactorSymbolPtr factor;

    DeclSymbol(WordTokenPtr _id, FactorSymbolPtr _f) : id(_id), factor(_f) { }
    virtual int getKind() { return Kind::decl; }
    virtual void printAST(int hierarchy);
};

class FactorSymbol : public Symbol {
public:
    IntTokenPtr integer = nullptr;
    FloatTokenPtr floating = nullptr;
    CharTokenPtr character = nullptr;
    StringTokenPtr stringLiteral = nullptr;
    ArraySymbolPtr array = nullptr;

    enum FactorType { INT = 0, FLOAT, CHAR, STRING, ARRAY };
    int factorType = 0;

    explicit FactorSymbol(IntTokenPtr i) : factorType(FactorType::INT), integer(i) { }
    explicit FactorSymbol(FloatTokenPtr f) : factorType(FactorType::FLOAT), floating(f) { }
    explicit FactorSymbol(CharTokenPtr c) : factorType(FactorType::CHAR), character(c) { }
    explicit FactorSymbol(StringTokenPtr s) : factorType(FactorType::STRING), stringLiteral(s) { }
    explicit FactorSymbol(ArraySymbolPtr a) : factorType(FactorType::ARRAY), array(a) { }

    virtual int getKind() { return Kind::factor; }
    virtual void printAST(int hierarchy);
};

class ArraySymbol : public Symbol {
public:
    deque<FactorSymbolPtr> arrayItems;

    virtual int getKind() { return Kind::arrayKind; }
    virtual void printAST(int hierarchy);
};

void DeclsSymbol::printAST(int hierarchy) {
    printHierarchy(hierarchy, "DeclsSymbol");
    for (DeclSymbolPtr decl : decls) {
        decl->printAST(hierarchy + 1);
    }
}

void DeclSymbol::printAST(int hierarchy) {
    printHierarchy(hierarchy, "DeclSymbol");
    printHierarchy(hierarchy + 1, "id: ");
    cout << (id ? id->name : "<Unnamed>");
    factor->printAST(hierarchy + 1);
}

void FactorSymbol::printAST(int hierarchy) {
    printHierarchy(hierarchy, "FactorSymbol");
    cout << "  ";
    if (factorType == FactorType::INT)
        cout << integer->toInt();
    else if (factorType == FactorType::FLOAT)
        cout << floating->toFloat();
    else if (factorType == FactorType::STRING)
        cout << "\"" << stringLiteral->value << "\"";
    else if (factorType == FactorType::CHAR)
        cout << "'" << (char)character->value << "'";
    else array->printAST(hierarchy + 1);
}

void ArraySymbol::printAST(int hierarchy) {
    printHierarchy(hierarchy, "ArraySymbol");
    for (FactorSymbolPtr f : arrayItems) {
        f->printAST(hierarchy + 1);
    }
}

// =============== AST Builder ===============
#define SYMBOL_UNCHECKED

#define SKIP_TO_SEMI_AND_END_THIS_SYMBOL  { skipUntil({ ';' }, SkipUntilSemi); return nullptr; }
#define MATCH(x)  { if ( look->isNot(x) ) {\
        diagError(#x " expected.", look);\
        SKIP_TO_SEMI_AND_END_THIS_SYMBOL\
    } else next(); }
#define match_tag(x) { if ( look->isNot(Tag::x) ) {\
        diagError("'" #x "' expected.", look);\
        SKIP_TO_SEMI_AND_END_THIS_SYMBOL\
    } else next(); }
#define REPORT_ERROR(msg) { diagError(msg, look); SKIP_TO_SEMI_AND_END_THIS_SYMBOL; }
#define REPORT_ERROR_1(msg, skipTo) { diagError(msg, look); skipUntil(skipTo, SkipUntilSemi | KeepSpecifiedToken); return nullptr; }

class ASTBuilder : public IParser {
public:
    inline DeclsSymbolPtr  root();
    DeclsSymbolPtr  decls() SYMBOL_UNCHECKED;
    DeclSymbolPtr   decl();
    FactorSymbolPtr factor();
    ArraySymbolPtr  array();
};

DeclsSymbolPtr ASTBuilder::root() {
    return decls();
}

DeclsSymbolPtr ASTBuilder::decls() {
    DeclsSymbolPtr declsSym = make_shared<DeclsSymbol>();
    while (look && look->isNot(Tag::EndOfFile) && look->isNot(0)) {
        DeclSymbolPtr declSym = decl();
        if (declSym)
            declsSym->decls.push_back(declSym);
    }
    return declsSym;
}

DeclSymbolPtr ASTBuilder::decl() {
    if (look->is(Tag::ID)) {
        WordTokenPtr id = static_pointer_cast<WordToken>(look);
        next(); MATCH('=');
        FactorSymbolPtr factorSym = factor();
        if (look->is(';') || look->is(','))
            next();
        return make_shared<DeclSymbol>(id, factorSym);
    }
    else if (look->is(';') || look->is(',')) {
        next();
        return nullptr;
    }
    else {
        FactorSymbolPtr factorSym = factor();
        if (look->is(';') || look->is(','))
            next();
        return make_shared<DeclSymbol>(nullptr, factorSym);
    }
}

FactorSymbolPtr ASTBuilder::factor() {
    if (look->is(Tag::Integer)) {
        TokenPtr tok = look; next();
        return make_shared<FactorSymbol>(static_pointer_cast<IntToken>(tok));
    }
    if (look->is(Tag::Floating)) {
        TokenPtr tok = look; next();
        return make_shared<FactorSymbol>(static_pointer_cast<FloatToken>(tok));
    }
    if (look->is(Tag::Character)) {
        TokenPtr tok = look; next();
        return make_shared<FactorSymbol>(static_pointer_cast<CharToken>(tok));
    }
    if (look->is(Tag::StringLiteral)) {
        TokenPtr tok = look; next();
        return make_shared<FactorSymbol>(static_pointer_cast<StringToken>(tok));
    }
    if (look->is('[')) {
        next();
        if (look->is(']')) {
            next();
            // empty array
            return make_shared<FactorSymbol>(make_shared<ArraySymbol>());
        }
        ArraySymbolPtr arr = array();
        MATCH(']');
        return make_shared<FactorSymbol>(arr);
    }
    REPORT_ERROR("unexpected token");
}

ArraySymbolPtr ASTBuilder::array() {
    ArraySymbolPtr arraySym = make_shared<ArraySymbol>();
    while (look->is(Tag::BaseFactor) || look->is('[')) {
        FactorSymbolPtr factorSym = factor();
        if (look->is(','))
            next();
        arraySym->arrayItems.push_back(factorSym);
    }
    return arraySym;
}

// =================== Parser ======================

class Parser : public ASTBuilder {
private:
    // Root of AST
    DeclsSymbolPtr M_ASTRoot = nullptr;
public:
    explicit Parser(iostream& stream) {
        M_lex = make_shared<Lexer>();
        M_lex->openFile(stream);
        Token::flread = M_lex->getSourceManager();
    }

    // parse source code
    //   returns: root of AST
    void parse() override {
        next();
        M_ASTRoot = root();
    }

    // get AST
    DeclsSymbolPtr getAST() { return M_ASTRoot; }
};

// =============== Link List Helper ===============
template <class ValTy>
class LinkListConstructor {
public:
    virtual ValTy  getValue() { assert(!"getValue() not implemented"); };
    virtual void   setValue(ValTy value) = 0;
    virtual LinkListConstructor<ValTy>* getNextItem() = 0;
    virtual LinkListConstructor<ValTy>* getPreviousItem() { assert(!"getPreviousItem() not implemented"); }
    virtual void setNextItem(LinkListConstructor<ValTy>* _next) = 0;
    virtual void setPreviousItem(LinkListConstructor<ValTy>* _prev) { assert(!"setPreviousItem() not implemented"); }

    template <class ContainerTy>
    static LinkListConstructor* constructLinkList(vector<ValTy>& init) {
        LinkListConstructor* head = new ContainerTy();
        LinkListConstructor* curr = head;
        for (ValTy& e : init) {
            LinkListConstructor* _new = new ContainerTy();
            _new->setNextItem(nullptr);
            _new->setValue(e);
            curr->setNextItem(_new);
            curr = _new;
        }
        return head->getNextItem();
    }

    template <class ContainerTy>
    static LinkListConstructor* constructDualLinkList(vector<ValTy>& init) {
        LinkListConstructor* head = new ContainerTy();
        LinkListConstructor* curr = head;
        for (ValTy& e : init) {
            LinkListConstructor* _new = new ContainerTy();
            _new->setNextItem(nullptr);
            _new->setPreviousItem(curr);
            _new->setValue(e);
            curr->setNextItem(_new);
            curr = _new;
        }
        auto _hptr = head;
        head = head->getNextItem();
        head->setPreviousItem(nullptr);
        delete _hptr;
        return head;
    }

    template <class ContainerTy>
    static LinkListConstructor* constructLoopLinkList(vector<ValTy>& init) {
        LinkListConstructor* head = new ContainerTy();
        LinkListConstructor* curr = head;
        for (ValTy& e : init) {
            LinkListConstructor* _new = new ContainerTy();
            _new->setNextItem(nullptr);
            _new->setValue(e);
            curr->setNextItem(_new);
            curr = _new;
        }
        curr->setNextItem(head->getNextItem());
        return head->getNextItem();
    }

    template <class ContainerTy>
    static LinkListConstructor* constructLoopDualLinkList(vector<ValTy>& init) {
        LinkListConstructor* head = new ContainerTy();
        LinkListConstructor* curr = head;
        for (ValTy& e : init) {
            LinkListConstructor* _new = new ContainerTy();
            _new->setNextItem(nullptr);
            _new->setPreviousItem(curr);
            _new->setValue(e);
            curr->setNextItem(_new);
            curr = _new;
        }
        auto _hptr = head;
        head = head->getNextItem();
        head->setPreviousItem(curr);
        curr->setNextItem(head);
        //delete _hptr;
        return head;
    }

    static void printList(LinkListConstructor* head) {
        bool headAllow = true;
        cout << "[";
        for (LinkListConstructor* curr = head;
            (curr != head || headAllow) && curr; curr = curr->getNextItem()) {
            headAllow = false;
            cout << curr->getValue() << ", ";
        }
        cout << "] ";
    }
    void printList() {
        printList(this);
    }

    static void printReversedList(LinkListConstructor* head) {
        head = head->getPreviousItem();
        bool headAllow = true;
        for (LinkListConstructor* curr = head;
            (curr != head || headAllow) && curr; curr = curr->getPreviousItem()) {
            headAllow = false;
            cout << curr->getValue() << ", ";
        }
        cout << endl;
    }
    void printReversedList() {
        printReversedList(this);
    }
};

#define LL_DEFINE_NEXT_PTR(nextPropName, typeName) \
    LinkListConstructor<int> * getNextItem() override {\
        return nextPropName;\
    }                                          \
    void setNextItem(LinkListConstructor<int> *_next) override {\
        nextPropName = (typeName*)_next;\
    }

#define LL_DEFINE_PREV_PTR(prevPropName, typeName) \
    LinkListConstructor<int> * getPreviousItem() override {\
        return prevPropName;\
    }                                          \
    void setPreviousItem(LinkListConstructor<int> *_prev) override {\
        prevPropName = (typeName*)_prev;\
    }

#define LL_DEFINE_VALUE(valuePropName, valueType) \
    valueType getValue() override {\
        return valuePropName;\
    }\
    void setValue(valueType value) override {\
        valuePropName = value;\
    }

// ================== Data Loader =================
DEFINE_SHARED_PTR(DataResult);
DEFINE_SHARED_PTR(DataLoader);

template <class T, int N>
struct vector_helper {
    using value_type = vector<typename vector_helper<T, N - 1>::value_type>;
};

template <class T>
struct vector_helper<T, 1> {
    using value_type = vector<T>;
};

template <class T, int N>
using vector_helper_t = typename vector_helper<T, N>::value_type;

class DataResult {
protected:
    FactorSymbolPtr factorSym = nullptr;
public:
#define DEFINE_OPERATORS(op, baseFunc) operator op() { return as##baseFunc<op>(); }
    template <class T = int64_t>
    T asInt() { return (T)(factorSym->integer ? factorSym->integer->toInt() : (factorSym->floating ? factorSym->floating->toInt() : 0)); }
#define INT_OPERATORS(x) DEFINE_OPERATORS(x, Int)
    INT_OPERATORS(int8_t)    INT_OPERATORS(uint8_t)
        INT_OPERATORS(int16_t)    INT_OPERATORS(uint16_t)
        INT_OPERATORS(int32_t)    INT_OPERATORS(uint32_t)
        INT_OPERATORS(int64_t)    INT_OPERATORS(uint64_t)

        template <class T = double>
    T asFloat() { return (T)(factorSym->integer ? factorSym->integer->toFloat() : (factorSym->floating ? factorSym->floating->toFloat() : 0)); }
    DEFINE_OPERATORS(float, Float)
        DEFINE_OPERATORS(double, Float)

        template <class T, int N>
    typename std::enable_if_t<N == 1, typename vector_helper<T, 1>::value_type> asNDArray() {
        typename vector_helper<T, 1>::value_type vec = asArray<T>();
        return vec;
    }

    template <class T, int N>
    typename std::enable_if_t<N != 1, typename vector_helper<T, N>::value_type> asNDArray() {
        typename vector_helper<T, N>::value_type vec;
        if (!factorSym->array) return vec;
        for (FactorSymbolPtr fact : factorSym->array->arrayItems) {
            DataResult dr; dr.factorSym = fact;
            if (!fact->array) continue; // Nested array only
            vec.push_back(dr.asNDArray<T, N - 1>());
        }
        return vec;
    }

    template <class T>
    vector<T> asArray() {
        vector<T> vec;
        if (!factorSym->array)
            throw runtime_error("Object is not an array");
        for (FactorSymbolPtr fact : factorSym->array->arrayItems) {
            DataResult dr; dr.factorSym = fact;
            if (fact->array) continue; // Do not support nested array
            else if (fact->integer) vec.push_back((T)dr);
            else if (fact->floating) vec.push_back((T)dr);
            else if (fact->character) vec.push_back((int8_t)dr);
        }
        return vec;
    }

    template <>
    vector<string> asArray<string>() {
        vector<string> vec;
        if (!factorSym->array) return vec;
        for (FactorSymbolPtr fact : factorSym->array->arrayItems) {
            DataResult dr; dr.factorSym = fact;
            if (fact->array) continue; // Do not support nested array
            vec.push_back(dr.asString());
        }
        return vec;
    }

    template <class T>
    typename vector_helper<T, 2>::value_type as2DArray() { return asNDArray<T, 2>(); }

    template <class T>
    typename vector_helper<T, 3>::value_type as3DArray() { return asNDArray<T, 3>(); }

    template <class T>
    operator vector<T>() { return asArray<T>(); }

    template <class T>
    operator vector<vector<T>>() { return asNDArray<T, 2>(); }

    template <class T>
    operator vector<vector<vector<T>>>() { return asNDArray<T, 3>(); }

    template <class ValTy, class ContainerTy>
    ContainerTy* asLinkedList() {
        vector<ValTy> init = asArray<ValTy>();
        LinkListConstructor<ValTy>* llist = LinkListConstructor<ValTy>::template constructLinkList<ContainerTy>(init);
        return (ContainerTy*)llist;
    }

    template <class ValTy, class ContainerTy>
    ContainerTy* asDualLinkedList() {
        vector<ValTy> init = asArray<ValTy>();
        LinkListConstructor<ValTy>* llist = LinkListConstructor<ValTy>::template constructDualLinkList<ContainerTy>(init);
        return (ContainerTy*)llist;
    }

    template <class ValTy, class ContainerTy>
    ContainerTy* asLoopLinkedList() {
        vector<ValTy> init = asArray<ValTy>();
        LinkListConstructor<ValTy>* llist = LinkListConstructor<ValTy>::template constructLoopLinkList<ContainerTy>(init);
        return (ContainerTy*)llist;
    }

    template <class ValTy, class ContainerTy>
    ContainerTy* asLoopDualLinkedList() {
        vector<ValTy> init = asArray<ValTy>();
        LinkListConstructor<ValTy>* llist = LinkListConstructor<ValTy>::template constructLoopDualLinkList<ContainerTy>(init);
        return (ContainerTy*)llist;
    }

    DataResult operator[](int idx) {
        if (!factorSym->array)
            throw runtime_error("Object is not subscriptable");
        DataResult dr; dr.factorSym = factorSym->array->arrayItems[idx];
        return move(dr);
    }

    friend class DataLoader;

    string asString() { return factorSym->stringLiteral ? factorSym->stringLiteral->value : ""; }
    char asChar() { return factorSym->character ? factorSym->character->value : 0; }
};

class NoSuchNameException : exception {
    const char* msg;
public:
    NoSuchNameException(const char* _msg) { msg = _msg; }
    const char* what() const noexcept override { return msg; }
};

class DataLoader : public DataResult {
    DeclsSymbolPtr AST;
public:
    DataLoader() { }
    bool load(string str) {
        AST = nullptr;
        stringstream ss;
        ss << str;
        Parser parser(ss);
        try {
            parser.parse();
            parser.parseDone();
            AST = parser.getAST();
            if (AST->decls.size() > 0)
                factorSym = AST->decls[0]->factor;
            return true;
        }
        catch (exception e) {
            cout << "Parse teminated due to errors.\n";
            parser.parseDone();
            return false;
        }
    }

    void printAST() { if (AST) AST->printAST(1); }

    DataResult getData(const char* name) {
        DeclSymbolPtr declSym = findDeclSymbolByID(name);
        if (!declSym)
            throw NoSuchNameException(name);
        DataResult ds;
        ds.factorSym = declSym->factor;
        return move(ds);
    }
    DataResult getData(int index) {
        DeclSymbolPtr declSym = findDeclSymbolByIndex(index);
        if (!declSym)
            throw NoSuchNameException("<Unnamed>");
        DataResult ds;
        ds.factorSym = declSym->factor;
        return move(ds);
    }
    DataResult operator[] (const char* name) {
        return move(getData(name));
    }
    DataResult operator[] (int index) {
        return move(getData(index));
    }

private:
    DeclSymbolPtr findDeclSymbolByID(const char* name) {
        if (!AST) return nullptr;
        for (DeclSymbolPtr decl : AST->decls) {
            if (!decl->id)
                continue;
            if (decl->id->name == name)
                return decl;
        }
        return nullptr;
    }
    DeclSymbolPtr findDeclSymbolByIndex(int idx) {
        if (!AST) return nullptr;
        if (idx >= AST->decls.size()) return nullptr;
        return AST->decls[idx];
    }
};

class SolutionTester {
protected:
    vector<DataLoader> loaders;
    function<bool(DataLoader& loader)> checkFn;
public:
    void addTestCase(string case_) {
        DataLoader loader;
        try {
            loader.load(case_);
            loaders.push_back(move(loader));
        }
        catch (exception e) {
            cout << "Error while loading test case: " << e.what() << endl;
        }
    }
    void setCheckFn(function<bool(DataLoader& loader)> _checkFn) {
        checkFn = _checkFn;
    }
    void test() {
        int index = 0;
        int success = 0, failed = 0;
        for (DataLoader& loader : loaders) {
            cout << "Checking case #" << index << endl;
            cout << "----------------------------------------------------\n";
            if (!checkFn(loader)) {
                cout << "\n----------------------------------------------------\n";
                cout << "  Error while checking case #" << index << endl;
                failed++;
            }
            else {
                cout << "\n----------------------------------------------------\n";
                cout << "  Test case # " << index << ": Passed" << endl;
                success++;
            }
            index++;
            cout << "====================================================\n";
        }
        cout << " TESTING COMPLETED: " << (success + failed) << " total, " << success << " passed, " << failed << "failed.\n";
    }
};

template <class T>
struct is_std_function : public false_type { };

template <class T>
struct is_std_function<std::function<T>> : public true_type { };

template <class T, class ...Args>
struct is_std_function<T(Args...)> : public true_type { };

template <class T, class ...Args>
struct is_std_function<std::function<T(Args...)>> : public true_type { };

template <class T>
struct function_helper;

template <class T, class ...Args>
struct function_helper<T(*)(Args...)> {
    using return_type = T;
    using arguments = std::tuple<Args...>;
};

template <class T, class ...Args>
struct function_helper<std::function<T(Args...)>> {
    using return_type = T;
    using arguments = std::tuple<Args...>;
};

template <class T, class C, class ...Args>
struct function_helper<T (C::*)(Args...)> {
    using return_type = T;
    using arguments = std::tuple<Args...>;
    using class_type = C;
    using functional_type = std::function<T (Args...)>;
};

template <int i, class TupleTyDst, class TupleTySrc>
typename std::enable_if_t < i<0, void>
    initialize_tuple_with_another(TupleTyDst& tuple, const TupleTySrc& another, DataLoader& loader) {   }

template <int i, class TupleTyDst, class TupleTySrc>
typename std::enable_if_t< i >= 0, void>
initialize_tuple_with_another(TupleTyDst& tuple, const TupleTySrc& another, DataLoader& loader) {
    auto another_v = std::get<i>(another);
    static_assert(is_same_v<decltype(another_v), const char*> || is_integral_v<decltype(another_v)>);
    std::get<i>(tuple) = loader[another_v];
    initialize_tuple_with_another<i - 1>(tuple, another, loader);
}

template <class AnswerTy, template<class> class Hash = std::hash>
class EnhancedSoultionTester : protected SolutionTester {
    vector_helper_t<AnswerTy, 1> answers;
    Hash<AnswerTy> answer_hash_func;
public:
    void addTestCase(string case_, AnswerTy answer_) {
        SolutionTester::addTestCase(case_);
        answers.push_back(answer_);
    }

    void setCheckFn(function<bool(DataLoader& loader)> _checkFn) = delete;

    template <class FuncTy, class... IndexType>
    typename std::enable_if<std::tuple_size<typename function_helper<FuncTy>::arguments>::value == sizeof...(IndexType)
        and std::is_same_v<typename function_helper<FuncTy>::return_type, AnswerTy>, void>::type
        test(FuncTy func, IndexType... arg_indexes) {
        using function_args_tuple = typename function_helper<FuncTy>::arguments;
        using function_return_type = typename function_helper<FuncTy>::return_type;
        function_args_tuple params;
        constexpr int max_index_of_indexes = sizeof...(IndexType) - 1;

        int index = 0;
        int success = 0, failed = 0;
        for (DataLoader& loader : loaders) {
            cout << "Checking case #" << index << endl;
            cout << "----------------------------------------------------\n";
            int is_passed = false;
            initialize_tuple_with_another<max_index_of_indexes>(params, forward_as_tuple(arg_indexes...), loader);
            function_return_type ret_value = std::apply(func, params);
            is_passed = answer_hash_func(ret_value) == answer_hash_func(answers[index]);
            if (!is_passed) {
                cout << "\n----------------------------------------------------\n";
                cout << "  Error while checking case #" << index << endl;
                failed++;
            }
            else {
                cout << "\n----------------------------------------------------\n";
                cout << "  Test case # " << index << ": Passed" << endl;
                success++;
            }
            index++;
            cout << "====================================================\n";
        }
        cout << " TESTING COMPLETED: " << (success + failed) << " total, " << success << " passed, " << failed << " failed.\n";
    }

};

template <class FuncTy, class... IndexTy>
requires requires { is_std_function<FuncTy>::value; }
class EnhancedTesterWrapper {
public:
    EnhancedSoultionTester<typename function_helper<FuncTy>::return_type> ST;
    tuple<IndexTy...> indexes;
    FuncTy func;

    EnhancedTesterWrapper(FuncTy _func, IndexTy... idx) {
        indexes = forward_as_tuple(idx...);
        func = _func;
    }

    template <class... Args>
    EnhancedTesterWrapper<FuncTy, IndexTy...>& addTestCase(Args... args) {
        ST.addTestCase(std::forward<Args>(args)...);
        return *this;
    }

    void test() {
        tuple<FuncTy> funcTuple = make_tuple(func);
        tuple<FuncTy, IndexTy...> paramsTuple = tuple_cat(funcTuple, indexes);
        std::apply([=]<class... Args>(Args... args) {
            ST.template test(std::forward<Args>(args)...);
            }, paramsTuple);
    }
};

template <class FuncTy, class... IndexTy>
typename std::enable_if<!is_member_function_pointer_v<FuncTy> and
                        std::tuple_size<typename function_helper<FuncTy>::arguments>::value == sizeof...(IndexTy)
        , EnhancedTesterWrapper<FuncTy, IndexTy...>>::type
createSolutionTester(FuncTy _func, IndexTy... idx) {
    return std::move(EnhancedTesterWrapper<FuncTy, IndexTy...>(_func, std::forward<IndexTy>(idx)...));
}

template <class F>
typename function_helper<F>::functional_type get_non_member_function(F func) {
    using ret_t = typename function_helper<F>::return_type;
    typename function_helper<F>::functional_type f = [=]<class...Args> (Args... args) -> ret_t {
        using class_t = typename function_helper<F>::class_type;
        class_t _c;
        auto mem_fn_v = std::mem_fn(func);
        return mem_fn_v(&_c, forward<Args>(args)...);
    };
    return f;
}

template <class FuncTy, class... IndexTy>
typename std::enable_if<is_member_function_pointer_v<FuncTy> and
                        std::tuple_size<typename function_helper<FuncTy>::arguments>::value == sizeof...(IndexTy)
        , EnhancedTesterWrapper<typename function_helper<FuncTy>::functional_type, IndexTy...>>::type
createSolutionTester(FuncTy _func, IndexTy... idx) {
    return std::move(createSolutionTester(get_non_member_function(_func), idx...));
}

#endif //DP_DATALOADER_H

