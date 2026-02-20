#include "Variant.h"

Variant::Variant()
: m_type(VARIANT_TYPE_NONE)
{
}

uint32 Variant::GetSize() const
{
    switch(GetType()) {
        case VARIANT_TYPE_INT:
        case VARIANT_TYPE_UINT:
        case VARIANT_TYPE_FLOAT:
            return 4;

        case VARIANT_TYPE_BOOL:
            return 1;

        case VARIANT_TYPE_STRING:
            return GetString().size();

        case VARIANT_TYPE_VECTOR2INT:
        case VARIANT_TYPE_VECTOR2FLOAT:
            return 8;
    }

    return 0;
}
