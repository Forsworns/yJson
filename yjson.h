#ifndef YJSON_H__
#define YJSON_H__

#include <cassert>
#include <cstdio>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace yph {
class Value;
class Entry;
class Context;

using std::string;
using std::vector;
// c++17 only allows incomplete class usage in certain containers
using Data = std::variant<double, string, vector<Value>, vector<Entry>, void*>;
using ValuePtr = std::shared_ptr<Value>;
using StringPtr = std::shared_ptr<string>;

/*TOOL*/
template <typename E>
constexpr auto castEnum(const E v) -> std::underlying_type_t<E> {
  return static_cast<std::underlying_type_t<E>>(v);
}
// template implementations must be in header or included in "xx.tpp" file

inline bool isDigit09(char c);
inline bool isDigit19(char c);

/*CLASS*/
enum class Type : std::uint8_t {
  NVLL,
  FALSE,
  TRUE,
  NUMBER,
  STRING,
  ARRAY,
  OBJECT,
};
extern string TypeStr[];
std::ostream& operator<<(std::ostream& os, Type t);

// #include <system_error>?
enum class Status : std::uint8_t {
  PARSE_OK = 0,
  PARSE_EXPECT_VALUE,
  PARSE_INVALID_VALUE,
  PARSE_ROOT_NOT_SINGULAR,
  PARSE_NULL_POINTER,
  PARSE_NUMBER_TOO_BIG,
  PARSE_MISS_QUOTATION_MARK,
  PARSE_INVALID_STRING_ESCAPE,
  PARSE_INVALID_STRING_CHAR,
  PARSE_INVALID_UNICODE_HEX,
  PARSE_INVALID_UNICODE_SURROGATE,
  PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
  PARSE_MISS_KEY,
  PARSE_MISS_COLON,
  PARSE_MISS_COMMA_OR_CURLY_BRACKET,
  STRINGIFY_OK,
};
extern string StatusStr[];
std::ostream& operator<<(std::ostream& os, Status s);

// std unicode
std::pair<bool, unsigned int> parseHex4(StringPtr, size_t& pos);
void encodeUtf8(StringPtr, unsigned int u);

// incomplete class is valid in certain c++17 STL containers
class Value {
 public:
  Type type;
  Data data;
  Value();
  Value(Type t);
};

class Entry {
 public:
  string key;
  Value val;
};

/*YJSON PARSER*/
void parseWhitespace(StringPtr s) noexcept;

template <Type type>
Status parseLiteral(StringPtr s, ValuePtr v, string&& typeStr) {
  if (s->length() < typeStr.length() ||
      s->compare(0, typeStr.length(), typeStr) != 0) {
    return Status::PARSE_INVALID_VALUE;
  }
  s->erase(0, typeStr.length());
  v->type = type;
  return Status::PARSE_OK;
}

Status parseNull(StringPtr s, ValuePtr v);
Status parseTrue(StringPtr s, ValuePtr v);
Status parseFalse(StringPtr s, ValuePtr v);
Status parseNumber(StringPtr s, ValuePtr v);
Status parseString(StringPtr s, ValuePtr v);
Status parseArray(StringPtr s, ValuePtr v);
Status parseObject(StringPtr s, ValuePtr v);
Status parseValue(StringPtr s, ValuePtr v);
Status parse(ValuePtr v, const string& json);

/*YJSON ACCESSOR*/
Type getType(const ValuePtr v);
double getNumber(const ValuePtr v);
bool getBoolean(const ValuePtr v);
string getString(const ValuePtr v);
Data getValue(const ValuePtr v);
auto getStringLength(const ValuePtr v)
    -> decltype(std::get<string>(v->data).length());
auto getArraySize(const ValuePtr v)
    -> decltype(std::get<vector<Value>>(v->data).size());
ValuePtr getArrayElement(const ValuePtr v, const size_t& i);
size_t getObjectSize(const ValuePtr v);
const string getObjectKey(const ValuePtr v, const size_t& index);
size_t getObjectKeyLength(const ValuePtr v, const size_t& index);
ValuePtr getObjectValue(const ValuePtr v, const size_t& index);

/*YJSON SETTER*/
void setNull(const ValuePtr v);
void setNumber(const ValuePtr v, const double& num);
void setBoolean(const ValuePtr v, const bool& bl);
void setString(const ValuePtr v, const string& str);

/*YJSON GENERATOR*/
Status stringify(const ValuePtr v, std::shared_ptr<string> s);
Status stringifyValue(const ValuePtr v, std::shared_ptr<string> s);

}  // namespace yph

#endif /*YJSON*/
