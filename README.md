# yjson

![Language](https://img.shields.io/badge/language-C++-green.svg)
![License](https://img.shields.io/badge/license-MIT-green)

Yet another json library in modern C++. 

Based on the original C tutorial from [miloyip](https://github.com/miloyip/json-tutorial).

Use `run.sh` to test. 

Example:

```C++
ValuePtr v = make_shared<Value>();
parse(v, "[ null , false , true , 123 , \"abc\" ]");
getArraySize(v); // 5
getType(getArrayElement(v, 2)); // Type::TRUE
getString(getArrayElement(v, 4)); // "abc"
```

A minimum g++ version>7 may be [enough](https://en.cppreference.com/w/cpp/compiler_support). During implementation, I use g++ version 7.5.0 and cmake version 3.10.2.

Todo:
OOP implementations like [MiniJson](https://github.com/zsmj2017/MiniJson) or [Json](https://github.com/Yuan-Hang/Json) 

Attention: **Some float boundary testcases are not fully coverred due to the unknown issue for `std::stod()`.**
