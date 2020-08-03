#pragma once

#include <cstdio>
#include <string>
#include <cinttypes>

#if (defined _WIN32 || defined _WIN64)

#define StringFormatChar "%hs"
#define LogFunc wprintf
#define LogErrFunc fwprintf
#define FormatString L
#define NativeStringType std::wstring
#define FileOpenFunc _wfopen
#define OmpParallel __pragma(omp parallel)
#define StartWithFunc data._Starts_with(keyword)
#define PathPerfix L"\\\\?\\" +

#else

#define StringFormatChar "%s"
#define LogFunc printf
#define LogErrFunc fprintf
#define FormatString
#define NativeStringType std::string
#define FileOpenFunc fopen
#define OmpParallel _Pragma("omp parallel")
#define StartWithFunc data.find(keyword) == 0
#define PathPerfix

#endif

#define FileOpen(path, mode) FileOpenFunc(path, FormatString"" mode)

#define _ToString(x) #x
#define ToString(x) _ToString(x)

#define Line ToString(__LINE__)

#define Log(...) LogFunc(FormatString"<%s,<" StringFormatChar ",%" PRIu64 "," StringFormatChar ">>\n", __VA_ARGS__)
#define LogErr(path, message) LogErrFunc(stderr, FormatString"    %s(" Line "): " StringFormatChar "\n", path, message)