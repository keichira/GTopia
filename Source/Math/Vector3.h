#pragma once

#include "../Precompiled.h"

template<typename T>
class Vector3 {
public:
    T x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

    bool operator==(const Vector3& rhs) 
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    bool operator!=(const Vector3<T>& rhs)
    {
        return !(*this == rhs);
    }

    bool operator<(const Vector3& rhs) const {
        return (x < rhs.x) || (x == rhs.x && y < rhs.y) || (x == rhs.x && y == rhs.y && z < rhs.z);
    }

    bool operator>(const Vector3& rhs) const {
        return rhs < *this;
    }

    Vector3<T>& operator+=(const Vector3<T>& rhs)
    {
        x += rhs.x; y += rhs.y; z += rhs.z;
        return *this;
    }

    Vector3<T>& operator-=(const Vector3<T>& rhs)
    {
        x -= rhs.x; y -= rhs.y; z -= rhs.z;
        return *this;
    }

    Vector3<T>& operator=(const Vector3<T>& rhs) 
    {
        x = rhs.x; y = rhs.y; z = rhs.z;
        return *this;
    }

    Vector3<T> operator+(const Vector3<T>& rhs) 
    {
        return Vector3<T>(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vector3<T> operator-(const Vector3<T>& rhs) 
    {
        return Vector3<T>(x - rhs.x, y - rhs.y, z - rhs.z);
    }
};

typedef Vector3<int32> Vector3Int;
typedef Vector3<float> Vector3Float;