#include "yjson.h"

namespace yph {
/* TOOL */
inline bool isDigit09(char c) { return c >= '0' && c <= '9'; }
inline bool isDigit19(char c) { return c >= '1' && c <= '9'; }

/* CLASS */
string TypeStr[] = {
    "NULL", "FALSE", "TRUE", "NUMBER", "STRING", "ARRAY", "OBJECT",
};

std::ostream& operator<<(std::ostream& os, Type t) {
  os << "Type::" << TypeStr[castEnum(t)];
  return os;
}

string StatusStr[] = {
    "PARSE_OK",
    "PARSE_EXPECT_VALUE",
    "PARSE_INVALID_VALUE",
    "PARSE_ROOT_NOT_SINGULAR",
    "PARSE_NULL_POINTER",
    "PARSE_NUMBER_TOO_BIG",
    "PARSE_MISS_QUOTATION_MARK",
    "PARSE_INVALID_STRING_ESCAPE",
    "PARSE_INVALID_STRING_CHAR",
    "PARSE_INVALID_UNICODE_HEX",
    "PARSE_INVALID_UNICODE_SURROGATE",
    "PARSE_MISS_COMMA_OR_SQUARE_BRACKET",
    "PARSE_MISS_KEY",
    "PARSE_MISS_COLON",
    "PARSE_MISS_COMMA_OR_CURLY_BRACKET",
    "STRINGIFY_OK",
};

std::ostream& operator<<(std::ostream& os, Status s) {
  os << "Status::" << StatusStr[castEnum(s)];
  return os;
}

Value::Value() : type(Type::NVLL), data(nullptr) {}

Value::Value(Type t) : type(t), data(nullptr) {}

std::pair<bool, unsigned int> parseHex4(StringPtr s, size_t& pos) {
  unsigned int u = 0;
  pos++;
  for (int i = 0; i < 4; pos++, i++) {
    auto ch = (*s)[pos];
    u <<= 4;
    if (ch >= '0' && ch <= '9') {
      u += (ch - '0');
    } else if (ch >= 'A' && ch <= 'F') {
      u += (ch - 'A' + 10);
    } else if (ch >= 'a' && ch <= 'f') {
      u += (ch - 'a' + 10);
    } else {
      return std::make_pair(true, 0);
    }
  }
  pos--;
  return std::make_pair(false, u);
}

void encodeUtf8(ValuePtr v, unsigned int u) {
  if (u <= 0x7F) {
    // std::get return a reference
    std::get<string>(v->data) += static_cast<char>(u & 0xFF);
  } else if (u <= 0x7FF) {
    std::get<string>(v->data) += static_cast<char>(0xC0 | ((u >> 6) & 0xFF));
    std::get<string>(v->data) += static_cast<char>(0x80 | (u & 0x3F));
  } else if (u <= 0xFFFF) {
    std::get<string>(v->data) += static_cast<char>(0xE0 | ((u >> 12) & 0xFF));

    std::get<string>(v->data) += static_cast<char>(0x80 | ((u >> 6) & 0x3F));
    std::get<string>(v->data) += static_cast<char>(0x80 | (u & 0x3F));
  } else {
    assert(u <= 0x10FFFF);
    std::get<string>(v->data) += static_cast<char>(0xF0 | ((u >> 18) & 0xFF));
    std::get<string>(v->data) += static_cast<char>(0x80 | ((u >> 12) & 0x3F));

    std::get<string>(v->data) += static_cast<char>(0x80 | ((u >> 6) & 0x3F));
    std::get<string>(v->data) += static_cast<char>(0x80 | (u & 0x3F));
  }
}

/*YJSON PARSER*/
void parseWhitespace(StringPtr s) noexcept {
  auto it = s->begin();
  for (; it < s->end(); ++it) {
    if ((*it) != ' ' && (*it) != '\t' && (*it) != '\n' && (*it) != '\r') {
      break;
    }
  }
  s->erase(s->begin(), it);
  return;
}

inline Status parseNull(StringPtr s, ValuePtr v) {
  return parseLiteral<Type::NVLL>(s, v, "null");
}

inline Status parseTrue(StringPtr s, ValuePtr v) {
  return parseLiteral<Type::TRUE>(s, v, "true");
}

inline Status parseFalse(StringPtr s, ValuePtr v) {
  return parseLiteral<Type::FALSE>(s, v, "false");
}

Status parseNumber(StringPtr s, ValuePtr v) {
  // only validate
  string::size_type pos = 0;
  if ((*s)[pos] == '-') {  // check negtive
    pos++;
  }

  if ((*s)[pos] == '0') {  // check integer
    pos++;
  } else {
    if (!isDigit19((*s)[pos])) {
      return Status::PARSE_INVALID_VALUE;
    }
    for (pos++; isDigit09((*s)[pos]); pos++) {  // simply neglect
    }
  }

  if ((*s)[pos] == '.') {  // check demical
    pos++;
    if (!isDigit09((*s)[pos])) {
      return Status::PARSE_INVALID_VALUE;
    }
    for (pos++; isDigit09((*s)[pos]); pos++) {
    }
  }

  if ((*s)[pos] == 'e' || (*s)[pos] == 'E') {  // check exp
    pos++;
    if ((*s)[pos] == '+' || (*s)[pos] == '-') {
      pos++;
    }
    for (pos++; isDigit09((*s)[pos]); pos++) {
    }
  }
  Status status = Status::PARSE_OK;
  // in c++ std::stod throw errors instead of returning error codes
  try {
    v->data = std::stod((*s), nullptr);  // parse it by std lib
    v->type = Type::NUMBER;
  } catch (std::out_of_range e) {
    v->type = Type::NVLL;
    status = Status::PARSE_NUMBER_TOO_BIG;
  } catch (...) {
    v->type = Type::NVLL;
    status = Status::PARSE_INVALID_VALUE;
  }
  s->erase(0, pos);
  return status;
}

Status parseString(StringPtr s, ValuePtr v) {
  size_t pos = 1;
  v->data = "";
  for (; pos < s->length(); pos++) {
    auto ch = (*s)[pos];
    switch (ch) {
      case '\"': {
        s->erase(0, pos + 1);
        v->type = Type::STRING;
        return Status::PARSE_OK;
      }
      case '\\': {
        auto ch = (*s)[++pos];
        switch (ch) {
          case '\"':
          case '\\':
          case '/': {
            std::get<string>(v->data) += ch;
            break;
          }
          case 'b': {
            std::get<string>(v->data) += '\b';
            break;
          }
          case 'f': {
            std::get<string>(v->data) += '\f';
            break;
          }
          case 'n': {
            std::get<string>(v->data) += '\n';
            break;
          }
          case 'r': {
            std::get<string>(v->data) += '\r';
            break;
          }
          case 't': {
            std::get<string>(v->data) += '\t';
            break;
          }
          case 'u': {
            unsigned int unicode = 0;
            auto [err1, u1] = parseHex4(s, pos);
            if (err1) {
              return Status::PARSE_INVALID_UNICODE_HEX;
            }
            if (u1 >= 0xD800 && u1 <= 0xDBFF) {
              if ((*s)[++pos] != '\\') {
                return Status::PARSE_INVALID_UNICODE_SURROGATE;
              }
              if ((*s)[++pos] != 'u') {
                return Status::PARSE_INVALID_UNICODE_SURROGATE;
              }
              auto [err2, u2] = parseHex4(s, pos);
              if (err2) {
                return Status::PARSE_INVALID_UNICODE_HEX;
              }
              if (u2 < 0xDC00 || u2 > 0xDFFF) {
                return Status::PARSE_INVALID_UNICODE_SURROGATE;
              }
              unicode = (((u1 - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
            } else {
              unicode = u1;
            }
            if (unicode == 0) {
              for (pos++; pos < s->length(); pos++) {
                if ((*s)[pos] == '\"') {
                  s->erase(0, pos + 1);
                  v->type = Type::STRING;
                  return Status::PARSE_OK;
                }
              }
              return Status::PARSE_MISS_QUOTATION_MARK;
            }
            encodeUtf8(v, unicode);
            break;
          }
          default: { return Status::PARSE_INVALID_STRING_ESCAPE; }
        }
        break;
      }
      default: {
        // cast to unsigned char, incase of misjudgement
        if (static_cast<unsigned char>(ch) < 0x20) {
          return Status::PARSE_INVALID_STRING_CHAR;
        }
        std::get<string>(v->data) += ch;
      }
    }
  }
  return Status::PARSE_MISS_QUOTATION_MARK;
}

Status parseArray(StringPtr s, ValuePtr v) {
  v->data = vector<Value>();
  s->erase(0, 1);
  parseWhitespace(s);
  while (s->length() > 0) {
    if ((*s)[0] == ']') {
      s->erase(0, 1);
      v->type = Type::ARRAY;
      return Status::PARSE_OK;
    }
    if ((*s)[0] == '}') {
      // don't treat it as PARSE_INVALID_VALUE
      return Status::PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
    }
    auto el = std::make_shared<Value>();
    if (Status status = parseValue(s, el); status != Status::PARSE_OK) {
      return status;
    }
    std::get<vector<Value>>(v->data).push_back(*el);
    parseWhitespace(s);
    if ((*s)[0] == ',') {
      s->erase(0, 1);
      parseWhitespace(s);
      if ((*s)[0] == ']') {
        v->type = Type::NVLL;
        return Status::PARSE_INVALID_VALUE;
      }
    }
  }
  return Status::PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
}

Status parseObject(StringPtr s, ValuePtr v) {
  v->data = vector<Entry>();
  s->erase(0, 1);
  parseWhitespace(s);
  while (s->length() > 0) {
    if ((*s)[0] == '}') {
      s->erase(0, 1);
      v->type = Type::OBJECT;
      return Status::PARSE_OK;
    }
    if ((*s)[0] == ']') {
      return Status::PARSE_MISS_COMMA_OR_CURLY_BRACKET;
    }
    auto el = std::make_shared<Entry>();
    if ((*s)[0] != '\"') {
      return Status::PARSE_MISS_KEY;
    }
    auto elKey = std::make_shared<Value>();
    if (Status status = parseValue(s, elKey); status != Status::PARSE_OK) {
      return status;
    }
    el->key = getString(elKey);
    parseWhitespace(s);
    if ((*s)[0] != ':') {
      return Status::PARSE_MISS_COLON;
    }
    s->erase(0, 1);
    parseWhitespace(s);
    auto elVal = std::make_shared<Value>();
    if (Status status = parseValue(s, elVal); status != Status::PARSE_OK) {
      return status;
    }
    el->val = *elVal;
    std::get<vector<Entry>>(v->data).push_back(*el);
    parseWhitespace(s);
    if ((*s)[0] == ',') {
      s->erase(0, 1);
      parseWhitespace(s);
      if (s->length() == 0) {
        v->type = Type::NVLL;
        return Status::PARSE_MISS_KEY;
      }
      if ((*s)[0] == '}') {
        v->type = Type::NVLL;
        return Status::PARSE_INVALID_VALUE;
      }
    } else if ((*s)[0] != '}') {
      v->type = Type::NVLL;
      return Status::PARSE_MISS_COMMA_OR_CURLY_BRACKET;
    }
  }
  return Status::PARSE_MISS_COMMA_OR_CURLY_BRACKET;
}

Status parseValue(StringPtr s, ValuePtr v) {
  switch ((*s)[0]) {
    case 'n': {
      return parseNull(s, v);
    }
    case 't': {
      return parseTrue(s, v);
    }
    case 'f': {
      return parseFalse(s, v);
    }
    case '\"': {
      return parseString(s, v);
    }
    case '[': {
      return parseArray(s, v);
    }
    case '{': {
      return parseObject(s, v);
    }
    case '\0': {
      return Status::PARSE_EXPECT_VALUE;
    }
    default: { return parseNumber(s, v); }
  }
  return Status::PARSE_OK;
}

Status parse(ValuePtr v, const string& json) {
  auto s = std::make_shared<string>(json);
  if (v) {
    parseWhitespace(s);
    if (s->length() == 0) {
      return Status::PARSE_EXPECT_VALUE;
    } else {
      Status ret = parseValue(s, v);
      if (ret == Status::PARSE_OK) {
        parseWhitespace(s);
        if (s->length() != 0) {
          v->type = Type::NVLL;
          return Status::PARSE_ROOT_NOT_SINGULAR;
        }
      }
      return ret;
    }
  } else {
    return Status::PARSE_NULL_POINTER;
  }
}

/*YJSON ACCESSOR*/
Type getType(const ValuePtr v) {
  assert(v != nullptr);
  return v->type;
}

Data getValue(const ValuePtr v) {
  assert(v != nullptr);
  switch (v->type) {
    case Type::FALSE: {
      return false;
    }
    case Type::TRUE: {
      return true;
    }
    case Type::NUMBER: {
      return std::get<double>(v->data);
    }
    case Type::STRING: {
      return std::get<string>(v->data);
    }
    default: { throw std::bad_variant_access(); }
  }
}

double getNumber(const ValuePtr v) {
  assert(v != nullptr && v->type == Type::NUMBER);
  return std::get<double>(v->data);
}

bool getBoolean(const ValuePtr v) {
  assert(v != nullptr && (v->type == Type::TRUE || v->type == Type::FALSE));
  return v->type == Type::TRUE ? true : false;
}

string getString(const ValuePtr v) {
  assert(v != nullptr && v->type == Type::STRING);
  return std::get<string>(v->data);
}

auto getStringLength(const ValuePtr v)
    -> decltype(std::get<string>(v->data).length()) {
  assert(v != nullptr && v->type == Type::STRING);
  return std::get<string>(v->data).length();
}

auto getArraySize(const ValuePtr v)
    -> decltype(std::get<vector<Value>>(v->data).size()) {
  return std::get<vector<Value>>(v->data).size();
}

ValuePtr getArrayElement(const ValuePtr v, const size_t& i) {
  return std::make_shared<Value>(std::get<vector<Value>>(v->data)[i]);
}

size_t getObjectSize(const ValuePtr v) {
  return std::get<vector<Entry>>(v->data).size();
}

const string getObjectKey(const ValuePtr v, const size_t& index) {
  return std::get<vector<Entry>>(v->data)[index].key;
}

size_t getObjectKeyLength(const ValuePtr v, const size_t& index) {
  return std::get<vector<Entry>>(v->data)[index].key.length();
}

ValuePtr getObjectValue(const ValuePtr v, const size_t& index) {
  return std::make_shared<Value>(std::get<vector<Entry>>(v->data)[index].val);
}

/*YJSON SETTER*/
void setNull(const ValuePtr v) {
  assert(v != nullptr);
  v->data = nullptr;
  v->type = Type::NVLL;
}

void setNumber(const ValuePtr v, const double& num) {
  assert(v != nullptr);
  v->data = num;
  v->type = Type::NUMBER;
}

void setBoolean(const ValuePtr v, const bool& bl) {
  assert(v != nullptr);
  v->data = nullptr;
  v->type = bl ? Type::TRUE : Type::FALSE;
}

void setString(const ValuePtr v, const string& str) {
  assert(v != nullptr);
  v->data = str;
  v->type = Type::STRING;
}

Status stringify(const ValuePtr v, std::shared_ptr<string> s) {
  if (Status status = stringifyValue(v, s); status != Status::STRINGIFY_OK) {
    s.reset();
    return status;
  }
  return Status::STRINGIFY_OK;
}

Status stringifyValue(const ValuePtr v, std::shared_ptr<string> s) {
  switch (v->type) {
    case Type::NVLL: {
      s->append("null");
      break;
    }
    case Type::TRUE: {
      s->append("true");
      break;
    }
    case Type::FALSE: {
      s->append("false");
      break;
    }
    case Type::NUMBER: {
      char buffer[32];
      sprintf(buffer, "%.17g", std::get<double>(v->data));
      s->append(buffer);
      break;
    }
    case Type::STRING: {
      s->append("\"");
      for (auto ch : std::get<string>(v->data)) {
        switch (ch) {
          case '\"': {
            s->append("\\\"");
            break;
          }
          case '\\': {
            s->append("\\\\");
            break;
          }
          case '\b': {
            s->append("\\b");
            break;
          }
          case '\f': {
            s->append("\\f");
            break;
          }
          case '\n': {
            s->append("\\n");
            break;
          }
          case '\r': {
            s->append("\\r");
            break;
          }
          case '\t': {
            s->append("\\t");
            break;
          }
          case '\0': {
            s->append("\\u0000");
            break;
          }
          default: {
            if (ch < 0x20) {
              std::stringstream ss;
              string unicode;
              ss << std::setw(4) << std::setfill('0') << ch;
              ss >> unicode;
              *s = (*s) + "\\u" + unicode + "x";
            } else {
              s->push_back(ch);
            }
          }
        }
      }
      s->append("\"");
      break;
    }
    case Type::ARRAY: {
      s->append("[");
      for (auto x : std::get<vector<Value>>(v->data)) {
        stringifyValue(std::make_shared<Value>(x), s);
        s->append(",");
      }
      if (s->length() > 2) {
        s->pop_back();
      }
      s->append("]");
      break;
    }
    case Type::OBJECT: {
      s->append("{");
      for (auto x : std::get<vector<Entry>>(v->data)) {
        auto key = std::make_shared<Value>();
        setString(key, x.key);
        stringifyValue(key, s);
        s->append(":");
        stringifyValue(std::make_shared<Value>(x.val), s);
        s->append(",");
      }
      if (s->length() > 2) {
        s->pop_back();
      }
      s->append("}");
      break;
    }
  }
  return Status::STRINGIFY_OK;
}

}  // namespace yph