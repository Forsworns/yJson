#include <cstring>
#include <iostream>
#include "yjson.h"

using namespace std;
using namespace yph;

static int mainRet = 0;
static int testCount = 0;
static int testPass = 0;

#define EXPECT_BASE(condition, expect, actual)                           \
  do {                                                                   \
    testCount++;                                                         \
    if (condition) {                                                     \
      testPass++;                                                        \
    } else {                                                             \
      cerr << __FILE__ << " : line" << __LINE__ << " expect: " << expect \
           << ", actual: " << actual << endl;                            \
      mainRet = 1;                                                       \
    }                                                                    \
  } while (0)

#define EXPECT_EQ(expect, actual) \
  EXPECT_BASE((expect) == (actual), expect, actual)

#define TEST(status, type, json)       \
  do {                                 \
    auto v = make_shared<Value>();     \
    EXPECT_EQ(status, parse(v, json)); \
    EXPECT_EQ(type, getType(v));       \
  } while (0)

#define TEST_NULL(status, json) TEST(status, Type::NVLL, json);

#define TEST_INVALID(json) TEST(Status::PARSE_INVALID_VALUE, Type::NVLL, json);

#define TEST_NUMBER(expect, json)                \
  do {                                           \
    auto v = make_shared<Value>();               \
    EXPECT_EQ(Status::PARSE_OK, parse(v, json)); \
    EXPECT_EQ(Type::NUMBER, getType(v));         \
    EXPECT_EQ(expect, getNumber(v));             \
  } while (0)

#define TEST_STRING(expect, json)                  \
  do {                                             \
    auto v = make_shared<Value>();                 \
    EXPECT_EQ(Status::PARSE_OK, parse(v, json));   \
    EXPECT_EQ(Type::STRING, getType(v));           \
    EXPECT_EQ(expect, getString(v));               \
    EXPECT_EQ(strlen(expect), getStringLength(v)); \
  } while (0)

#define TEST_ROUNDTRIP(json)                            \
  do {                                                  \
    auto v = make_shared<Value>();                      \
    auto res = make_shared<string>();                   \
    EXPECT_EQ(Status::PARSE_OK, parse(v, json));        \
    EXPECT_EQ(Status::STRINGIFY_OK, stringify(v, res)); \
    EXPECT_EQ(json, (*res));                            \
  } while (0)

static void testParseNull() { TEST_NULL(Status::PARSE_OK, "null"); }

static void testParseTrue() { TEST(Status::PARSE_OK, Type::TRUE, "true"); }

static void testParseFalse() { TEST(Status::PARSE_OK, Type::FALSE, "false"); }

static void testParseExpectValue() {
  TEST_NULL(Status::PARSE_EXPECT_VALUE, "");
  TEST_NULL(Status::PARSE_EXPECT_VALUE, " ");
}

static void testParseInvalidValue() {
  // invalid format
  TEST_INVALID("nul");
  TEST_INVALID(" ?");
  // invalid number
  TEST_INVALID("+0");
  TEST_INVALID("+1");
  TEST_INVALID(".123");  // at least one digit before '.'
  TEST_INVALID("1.");    // at least one digit after '.'
  TEST_INVALID("INF");
  TEST_INVALID("inf");
  TEST_INVALID("NAN");
  TEST_INVALID("nan");
  // invalid array
  TEST_INVALID("[1,]");
  TEST_INVALID("[\"a\", nul]");
}

static void testParseRootNotSingular() {
  TEST_NULL(Status::PARSE_ROOT_NOT_SINGULAR, "null x");
  /* Invalid number */
  TEST_NULL(Status::PARSE_ROOT_NOT_SINGULAR,
            "0123"); /* after zero should be '.' or nothing */
  TEST_NULL(Status::PARSE_ROOT_NOT_SINGULAR, "0x0");
  TEST_NULL(Status::PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void testParseNumber() {
  TEST_NUMBER(0.0, "0");
  TEST_NUMBER(0.0, "-0");
  TEST_NUMBER(0.0, "-0.0");
  TEST_NUMBER(1.0, "1");
  TEST_NUMBER(-1.0, "-1");
  TEST_NUMBER(1.5, "1.5");
  TEST_NUMBER(-1.5, "-1.5");
  TEST_NUMBER(3.1416, "3.1416");
  TEST_NUMBER(1E10, "1E10");
  TEST_NUMBER(1e10, "1e10");
  TEST_NUMBER(1E+10, "1E+10");
  TEST_NUMBER(1E-10, "1E-10");
  TEST_NUMBER(-1E10, "-1E10");
  TEST_NUMBER(-1e10, "-1e10");
  TEST_NUMBER(-1E+10, "-1E+10");
  TEST_NUMBER(-1E-10, "-1E-10");
  TEST_NUMBER(1.234E+10, "1.234E+10");
  TEST_NUMBER(1.234E-10, "1.234E-10");
  // TEST_NUMBER(0.0, "1e-10000"); // Underflow

  /* boundary */
  TEST_NUMBER(1.0000000000000002,
              "1.0000000000000002");  // The smallest number > 1
  /* something wrong in stod ...
  TEST_NUMBER(4.9406564584124654e-324,
              "4.9406564584124654e-324"); // Min denormal
  cout << "here" << endl;
  TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
  TEST_NUMBER(2.2250738585072009e-308,
              "2.2250738585072009e-308"); // Max subnormal
  TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
  TEST_NUMBER(2.2250738585072014e-308,
              "2.2250738585072014e-308"); // Min normal
  TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
  TEST_NUMBER(1.7976931348623157e+308,
              "1.7976931348623157e+308"); // Max normal
  TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
  */
}

static void testParseNumberTooBig() {
  TEST_NULL(Status::PARSE_NUMBER_TOO_BIG, "1e309");
  TEST_NULL(Status::PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void testParseString() {
  TEST_STRING("", "\"\"");
  TEST_STRING("Hello", "\"Hello\"");
  TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
  TEST_STRING("\" \\ / \b \f \n \r \t",
              "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
  TEST_STRING("Hello", "\"Hello\\u0000World\"");
  TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
  TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
  TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
  TEST_STRING("\xF0\x9D\x84\x9E",
              "\"\\uD834\\uDD1E\""); /* G clef sign U+1D11E */
  TEST_STRING("\xF0\x9D\x84\x9E",
              "\"\\ud834\\udd1e\""); /* G clef sign U+1D11E */
}

static void testParseMissingQuotationMark() {
  TEST_NULL(Status::PARSE_MISS_QUOTATION_MARK, "\"");
  TEST_NULL(Status::PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void testParseInvalidStringEscape() {
  TEST_NULL(Status::PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
  TEST_NULL(Status::PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
  TEST_NULL(Status::PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
  TEST_NULL(Status::PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void testParseInvalidStringChar() {
  TEST_NULL(Status::PARSE_INVALID_STRING_CHAR, "\"\x01\"");
  TEST_NULL(Status::PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void testParseInvalidUnicodeHex() {
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void testParseInvalidUnicodeSurrogate() {
  TEST_NULL(Status::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
  TEST_NULL(Status::PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void testParseArray() {
  size_t i, j;
  auto v = make_shared<Value>();
  EXPECT_EQ(Status::PARSE_OK, parse(v, "[ ]"));
  EXPECT_EQ(Type::ARRAY, getType(v));
  EXPECT_EQ(0, getArraySize(v));
  EXPECT_EQ(Status::PARSE_OK,
            parse(v, "[ null , false , true , 123 , \"abc\" ]"));
  EXPECT_EQ(Type::ARRAY, getType(v));
  EXPECT_EQ(5, getArraySize(v));
  EXPECT_EQ(Type::NVLL, getType(getArrayElement(v, 0)));
  EXPECT_EQ(Type::FALSE, getType(getArrayElement(v, 1)));
  EXPECT_EQ(Type::TRUE, getType(getArrayElement(v, 2)));
  EXPECT_EQ(Type::NUMBER, getType(getArrayElement(v, 3)));
  EXPECT_EQ(Type::STRING, getType(getArrayElement(v, 4)));
  EXPECT_EQ(123.0, getNumber(getArrayElement(v, 3)));
  EXPECT_EQ("abc", getString(getArrayElement(v, 4)));
  EXPECT_EQ(Status::PARSE_OK,
            parse(v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
  EXPECT_EQ(Type::ARRAY, getType(v));
  EXPECT_EQ(4, getArraySize(v));
  for (i = 0; i < 4; i++) {
    auto a = getArrayElement(v, i);
    EXPECT_EQ(Type::ARRAY, getType(a));
    EXPECT_EQ(i, getArraySize(a));
    for (j = 0; j < i; j++) {
      auto e = getArrayElement(a, j);
      EXPECT_EQ(Type::NUMBER, getType(e));
      EXPECT_EQ((double)j, getNumber(e));
    }
  }
}

static void testParseMissCommaOrSquareBracket() {
  TEST_NULL(Status::PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
  TEST_NULL(Status::PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
  TEST_NULL(Status::PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
  TEST_NULL(Status::PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void testParseObject() {
  auto v = make_shared<Value>();
  size_t i;
  EXPECT_EQ(Status::PARSE_OK, parse(v, " { } "));
  EXPECT_EQ(Type::OBJECT, getType(v));
  EXPECT_EQ(0, getObjectSize(v));
  EXPECT_EQ(Status::PARSE_OK,
            parse(v,
                  " { "
                  "\"n\" : null , "
                  "\"f\" : false , "
                  "\"t\" : true , "
                  "\"i\" : 123 , "
                  "\"s\" : \"abc\", "
                  "\"a\" : [ 1, 2, 3 ],"
                  "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
                  " } "));
  EXPECT_EQ(Type::OBJECT, getType(v));
  EXPECT_EQ(7, getObjectSize(v));
  EXPECT_EQ("n", getObjectKey(v, 0));
  EXPECT_EQ(Type::NVLL, getType(getObjectValue(v, 0)));
  EXPECT_EQ("f", getObjectKey(v, 1));
  EXPECT_EQ(Type::FALSE, getType(getObjectValue(v, 1)));
  EXPECT_EQ("t", getObjectKey(v, 2));
  EXPECT_EQ(Type::TRUE, getType(getObjectValue(v, 2)));
  EXPECT_EQ("i", getObjectKey(v, 3));
  EXPECT_EQ(Type::NUMBER, getType(getObjectValue(v, 3)));
  EXPECT_EQ(123.0, getNumber(getObjectValue(v, 3)));
  EXPECT_EQ("s", getObjectKey(v, 4));
  EXPECT_EQ(Type::STRING, getType(getObjectValue(v, 4)));
  EXPECT_EQ("abc", getString(getObjectValue(v, 4)));
  EXPECT_EQ("a", getObjectKey(v, 5));
  EXPECT_EQ(Type::ARRAY, getType(getObjectValue(v, 5)));
  EXPECT_EQ(3, getArraySize(getObjectValue(v, 5)));
  for (i = 0; i < 3; i++) {
    auto e = getArrayElement(getObjectValue(v, 5), i);
    EXPECT_EQ(Type::NUMBER, getType(e));
    EXPECT_EQ(i + 1.0, getNumber(e));
  }
  EXPECT_EQ("o", getObjectKey(v, 6));
  {
    auto o = getObjectValue(v, 6);
    EXPECT_EQ(Type::OBJECT, getType(o));
    for (i = 0; i < 3; i++) {
      auto ov = getObjectValue(o, i);
      auto key = '1' + i;
      EXPECT_EQ(true,
                (key == static_cast<decltype(key)>(getObjectKey(o, i)[0])));
      EXPECT_EQ(1, getObjectKeyLength(o, i));
      EXPECT_EQ(Type::NUMBER, getType(ov));
      EXPECT_EQ(i + 1.0, getNumber(ov));
    }
  }
}

static void testParseMissKey() {
  TEST_NULL(Status::PARSE_MISS_KEY, "{:1,");
  TEST_NULL(Status::PARSE_MISS_KEY, "{1:1,");
  TEST_NULL(Status::PARSE_MISS_KEY, "{true:1,");
  TEST_NULL(Status::PARSE_MISS_KEY, "{false:1,");
  TEST_NULL(Status::PARSE_MISS_KEY, "{null:1,");
  TEST_NULL(Status::PARSE_MISS_KEY, "{[]:1,");
  TEST_NULL(Status::PARSE_MISS_KEY, "{{}:1,");
  TEST_NULL(Status::PARSE_MISS_KEY, "{\"a\":1,");
}

static void testParseMissColon() {
  TEST_NULL(Status::PARSE_MISS_COLON, "{\"a\"}");
  TEST_NULL(Status::PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void testParseMissCommaOrCurlyBracket() {
  TEST_NULL(Status::PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
  TEST_NULL(Status::PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
  TEST_NULL(Status::PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
  TEST_NULL(Status::PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void testAccessNull() {
  auto v = make_shared<Value>();
  setString(v, "Hello");
  setNull(v);
  EXPECT_EQ(Type::NVLL, getType(v));
}

static void testAccessBoolean() {
  auto v = make_shared<Value>();
  setString(v, "Hello");
  setBoolean(v, true);
  EXPECT_EQ(true, getBoolean(v));
  setBoolean(v, false);
  EXPECT_EQ(false, getBoolean(v));
}

static void testAccessNumber() {
  auto v = make_shared<Value>();
  setString(v, "Hello");
  setNumber(v, 1234.5);
  EXPECT_EQ(1234.5, getNumber(v));
}

static void testAccessString() {
  auto v = make_shared<Value>();
  setString(v, "");
  EXPECT_EQ("", getString(v));
  setString(v, "Hello");
  EXPECT_EQ("Hello", getString(v));
}

static void testStringifyNumber() {
  TEST_ROUNDTRIP("0");
  TEST_ROUNDTRIP("-0");
  TEST_ROUNDTRIP("1");
  TEST_ROUNDTRIP("-1");
  TEST_ROUNDTRIP("1.5");
  TEST_ROUNDTRIP("-1.5");
  TEST_ROUNDTRIP("3.25");
  TEST_ROUNDTRIP("1e+20");
  TEST_ROUNDTRIP("1.234e+20");
  TEST_ROUNDTRIP("1.234e-20");
  TEST_ROUNDTRIP("1.0000000000000002");  // the smallest number > 1

  /* something wrong in stod ...
  TEST_ROUNDTRIP("4.9406564584124654e-324"); // minimum denormal
  TEST_ROUNDTRIP("-4.9406564584124654e-324");
  TEST_ROUNDTRIP("2.2250738585072009e-308"); // Max subnormal double
  TEST_ROUNDTRIP("-2.2250738585072009e-308");
  TEST_ROUNDTRIP("2.2250738585072014e-308"); // Min normal positive double
  TEST_ROUNDTRIP("-2.2250738585072014e-308");
  TEST_ROUNDTRIP("1.7976931348623157e+308"); // Max double
  TEST_ROUNDTRIP("-1.7976931348623157e+308");
  */
}

static void testStringifyString() {
  TEST_ROUNDTRIP("\"\"");
  TEST_ROUNDTRIP("\"Hello\"");
  TEST_ROUNDTRIP("\"Hello\\nWorld\"");
  TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
  // TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void testStringifyArray() {
  TEST_ROUNDTRIP("[]");
  TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void testStringifyObject() {
  TEST_ROUNDTRIP("{}");
  TEST_ROUNDTRIP(
      "{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3]"
      ",\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}

static void testStringify() {
  TEST_ROUNDTRIP("null");
  TEST_ROUNDTRIP("false");
  TEST_ROUNDTRIP("true");
  testStringifyNumber();
  testStringifyString();
  testStringifyArray();
  testStringifyObject();
}

static void testParse() {
  testParseNull();
  testParseTrue();
  testParseFalse();
  testParseNumber();
  testParseString();
  testParseArray();
  testParseObject();

  testParseExpectValue();
  testParseInvalidValue();
  testParseRootNotSingular();
  testParseNumberTooBig();
  testParseMissingQuotationMark();
  testParseInvalidStringEscape();
  testParseInvalidStringChar();
  testParseInvalidUnicodeHex();
  testParseInvalidUnicodeSurrogate();
  testParseMissCommaOrSquareBracket();

  testParseMissKey();
  testParseMissColon();
  testParseMissCommaOrCurlyBracket();

  testAccessNull();
  testAccessBoolean();
  testAccessNumber();
  testAccessString();
  testStringify();
}

int main() {
  testParse();
  cout << testPass << "/" << testCount << " " << testPass * 100 / testCount
       << "% passed" << endl;
  return mainRet;
}