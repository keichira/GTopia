#pragma once

#include "../Precompiled.h"
#include "../Math/Vector2.h"

enum eVariantTypes 
{
    VARIANT_TYPE_NONE,
    VARIANT_TYPE_INT,
    VARIANT_TYPE_UINT,
    VARIANT_TYPE_FLOAT,
    VARIANT_TYPE_BOOL,
    VARIANT_TYPE_STRING,
    VARIANT_TYPE_VECTOR2INT,
    VARIANT_TYPE_VECTOR2FLOAT,
};

struct VariantValue
{
    union { int32 i_0; uint32 u_0; float f_0; bool b_0; void* ptr_0; };
    union { int32 i_1; uint32 u_1; float f_1; void* ptr_1; };
    union { int32 i_2; uint32 u_2; float f_2; void* ptr_2; };
    union { int32 i_3; uint32 u_3; float f_3; void* ptr_3; };
};

class Variant {
public:
    Variant();
    Variant(const int32 rhs) { *this = rhs; }
    Variant(const uint32 rhs) { *this = rhs; }
    Variant(const bool rhs) { *this = rhs; }
    Variant(const string& rhs) { *this = rhs; }
    Variant(const Vector2Int& rhs) { *this = rhs; }
    Variant(const Vector2Float& rhs) { *this = rhs; }

public:
    void operator=(const int32 rhs) 
    {
        m_type = VARIANT_TYPE_INT;
        m_value.i_0 = rhs;
    }

    void operator=(const uint32 rhs)
    {
        m_type = VARIANT_TYPE_UINT;
        m_value.u_0 = rhs;
    }

    void operator=(const float rhs)
    {
        m_type = VARIANT_TYPE_FLOAT;
        m_value.f_0 = rhs;
    }

    void operator=(const bool rhs)
    {
        m_type = VARIANT_TYPE_BOOL;
        m_value.b_0 = rhs;
    }

    void operator=(const string& rhs)
    {
        m_type = VARIANT_TYPE_STRING;
        m_str = rhs;
    }

    void operator=(const char* rhs)
    {
        m_type = VARIANT_TYPE_STRING;
        m_str = rhs ? rhs : "";
    }

    void operator=(const Vector2Int& rhs)
    {
        m_type = VARIANT_TYPE_VECTOR2INT;
        *reinterpret_cast<Vector2Int*>(&m_value) = rhs;
    }

    void operator=(const Vector2Float& rhs)
    {
        m_type = VARIANT_TYPE_VECTOR2FLOAT;
        *reinterpret_cast<Vector2Float*>(&m_value) = rhs;
    }

    int32 GetINT() const
    {
        if(m_type == VARIANT_TYPE_INT)
            return m_value.i_0;
        return 0;
    }

    uint32 GetUINT() const
    {
        if(m_type == VARIANT_TYPE_UINT)
            return m_value.i_0;
        return 0;
    }

    float GetFloat() const
    {
        if(m_type == VARIANT_TYPE_FLOAT)
            return m_value.f_0;
        return 0.0f;
    }

    bool GetBool() const
    {
        if(m_type == VARIANT_TYPE_BOOL)
            return m_value.b_0;
        return false;
    }

    const string& GetString() const
    {
        return m_str;
    }

    const Vector2Int& GetVector2Int() const
    {
        if(m_type == VARIANT_TYPE_VECTOR2INT)
            return *reinterpret_cast<const Vector2Int*>(&m_value);
        return Vector2Int::New;
    }

    const Vector2Float& GetVector2Float() const
    {
        if(m_type == VARIANT_TYPE_VECTOR2FLOAT)
            return *reinterpret_cast<const Vector2Float*>(&m_value);
        return Vector2Float::New;
    }

public:
    eVariantTypes GetType() const { return m_type; }
    VariantValue GetValue() const { return m_value; }

    uint32 GetSize() const;

private:
    VariantValue m_value;
    eVariantTypes m_type;
    string m_str;
};

typedef std::vector<Variant> VariantVector;
typedef std::unordered_map<string, Variant> VariantMap;