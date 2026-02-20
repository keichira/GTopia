#pragma once

#include "../Precompiled.h"
#include <mysql.h>
#include <cassert>

// iphone kupi

template<typename T>
struct MySQLINTType;

template<> struct MySQLINTType<int8> { static constexpr enum_field_types Type = MYSQL_TYPE_TINY; };
template<> struct MySQLINTType<uint8> { static constexpr enum_field_types Type = MYSQL_TYPE_TINY; };
template<> struct MySQLINTType<int16> { static constexpr enum_field_types Type = MYSQL_TYPE_SHORT; };
template<> struct MySQLINTType<uint16> { static constexpr enum_field_types Type = MYSQL_TYPE_SHORT; };
template<> struct MySQLINTType<int32> { static constexpr enum_field_types Type = MYSQL_TYPE_LONG; };
template<> struct MySQLINTType<uint32> { static constexpr enum_field_types Type = MYSQL_TYPE_LONG; };

class PreparedParam {
public:
    typedef std::vector<MYSQL_BIND> BindVector;
    typedef std::vector<std::vector<uint8>> BufferVector;
    typedef std::vector<uint8> NullVecotr;
    typedef std::vector<unsigned long> LengthVector;

public:
    PreparedParam(const uint8 paramSize);
    ~PreparedParam();

public:
    void Reset();
    void SetString(const uint8 index, const string& value);

    template<typename T>
    void SetInt(const uint8 index, const T& value) {
        assert(index < m_binds.size()); 

        m_binds[index].buffer_type = MySQLINTType<T>::Type;

        m_buffers[index].resize(sizeof(T));
        memcpy(m_buffers[index].data(), &value, sizeof(T));

        m_binds[index].buffer = (void*)m_buffers[index].data();
        m_binds[index].buffer_length = sizeof(T);

        m_binds[index].is_unsigned = std::is_unsigned<T>::value;

        m_nulls[index] = false;
        m_binds[index].is_null = (bool*)&m_nulls[index];
    }

    void Load(MYSQL_FIELD* pFields);
    uint32 GetParamCount() { return m_binds.size(); }

    BindVector& GetBinds() { return m_binds; }
    BufferVector& GetBuffers() { return m_buffers; }
    NullVecotr& GetNulls() { return m_nulls; }
    LengthVector& GetLengths() { return m_lengths; }

private:
    std::vector<MYSQL_BIND> m_binds;
    std::vector<std::vector<uint8>> m_buffers;
    std::vector<uint8> m_nulls;
    std::vector<unsigned long> m_lengths;
};