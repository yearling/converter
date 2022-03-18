#pragma once
#ifdef __clang__
template <typename T>
auto ArrayCountHelper(T& t) -> typename TEnableIf<__is_array(T), char(&)[sizeof(t) / sizeof(t[0]) + 1]>::Type;
#else
template <typename T, uint32_t N>
char(&ArrayCountHelper(const T(&)[N]))[N + 1];
#endif
#define ARRAY_COUNT( array ) (sizeof(ArrayCountHelper(array)) - 1)