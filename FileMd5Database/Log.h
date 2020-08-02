#pragma once

#include <cinttypes>

#define _ToString(x) #x
#define ToString(x) _ToString(x)

#define Line ToString(__LINE__)

#define Log(...) LogFunc(FormatString"<%s,<" StringFormatChar ",%" PRIu64 "," StringFormatChar ">>\n", __VA_ARGS__)
#define LogErr(path, message) LogErrFunc(stderr, FormatString"    %s(" Line "): " StringFormatChar "\n", path, message)