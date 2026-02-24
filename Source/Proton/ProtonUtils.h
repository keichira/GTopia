#pragma once

/**
 * 
 * TAKEN FROM Seth A. Robinson's ProtonSDK
 * https://github.com/SethRobinson/proton
 * 
 */

#include "../Precompiled.h"
#include "../Utils/Variant.h"

string XorCipherString(const string& str, const char* secret, int32 id);

namespace Proton
{
    enum ePlatformID
    {
    	PLATFORM_ID_UNKNOWN = -1,
    	PLATFORM_ID_WINDOWS,
    	PLATFORM_ID_IOS, //iPhone/iPad etc
    	PLATFORM_ID_OSX,
    	PLATFORM_ID_LINUX,
    	PLATFORM_ID_ANDROID,
    	PLATFORM_ID_WINDOWS_MOBILE, //yeah, right.  Doesn't look like we'll be porting here anytime soon.
    	PLATFORM_ID_WEBOS,
    	PLATFORM_ID_BBX, //RIM Playbook
    	PLATFORM_ID_FLASH,
    	PLATFORM_ID_HTML5, //javascript output via emscripten for web
    	PLATFORM_ID_PSVITA,
    	
    	//new platforms will be added above here.  Don't count on PLATFORM_ID_COUNT not changing!
    	PLATFORM_ID_COUNT
    };
    
    uint32 HashString(const char *str, int32 len);
    uint8 ConvertVariantType(eVariantTypes type);
    uint8 *SerializeToMem(VariantVector &varVector, uint32 *pSizeOut, uint8 *pDest);
}
