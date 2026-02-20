#pragma once

#include "../Precompiled.h"
#include "Vector2.h"

template <typename T>
class Rect {
public:
    Rect() : left(0), top(0), right(0), bottom(0) {}
    Rect(T left_, T top_, T right_, T bottom_) :
        left(left_), top(top_), right(right_), bottom(bottom_) {}

    T Width() const { return right - left; }
    T Height() const { return bottom - top; }

    bool IsInside(const Vector2<T>& other) {
        if (other.x < left || other.y < top || other.x >= right || other.y >= bottom)
            return false;
        
        return true;
    }

    bool operator==(const Rect& other) {
        return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
    }

public:
    T left, top;
    T right, bottom; 
};

typedef Rect<int32> RectInt;
typedef Rect<float> RectFloat;