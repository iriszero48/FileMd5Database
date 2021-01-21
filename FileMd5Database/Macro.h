#pragma once

#pragma region private

#define __Macro_ToStringFunc__(x) #x

#pragma endregion private

#pragma region public

#define MacroToString(x) __Macro_ToStringFunc__(x)
#define MacroLine MacroToString(__LINE__)

#if (defined _WIN32 || _WIN64)
	#define MacroWindows
#endif

#ifdef _MSC_VER
	#define MacroFunctionName __FUNCSIG__
#elif defined __GNUC__
	#define MacroFunctionName __PRETTY_FUNCTION__
#else
	#define MacroFunctionName __func__
#endif

#pragma endregion public
