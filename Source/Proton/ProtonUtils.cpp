#include "ProtonUtils.h"

string XorCipherString(const string& str, const char* secret, int32 id)
{
    uint32 secretLen = strlen(secret);
    id %= secretLen;

    string out = str;

    for(uint32 i = 0; i < str.size(); ++i) {
        out[i] = (str[i] ^ secret[id++]);
        if(id >= secretLen) {
            id = 0;
        }
    }

    return out;
}

uint8 Proton::ConvertVariantType(eVariantTypes type)
{
    switch(type) {
		case VARIANT_TYPE_UINT:
		    return 5;
        case VARIANT_TYPE_INT: 
		    return 9;
        case VARIANT_TYPE_FLOAT:
            return 1;
        case VARIANT_TYPE_STRING:
            return 2;
        case VARIANT_TYPE_VECTOR2INT:
        case VARIANT_TYPE_VECTOR2FLOAT:
            return 3;

		default: 
		    return 0;
	}

	return 0;
}
