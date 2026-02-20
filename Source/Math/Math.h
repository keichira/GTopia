#pragma once

template<typename T, typename U>
inline T Min(T lhs, U rhs) { return lhs < rhs ? lhs : rhs; }

template <typename T, typename U>
inline T Max(T lhs, U rhs) { return lhs > rhs ? lhs : rhs; }