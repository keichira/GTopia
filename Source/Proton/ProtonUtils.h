#pragma once

/**
 * 
 * TAKEN FROM Seth A. Robinson's ProtonSDK
 * https://github.com/SethRobinson/proton
 * 
 */

#include "../Precompiled.h"
#include "../Utils/Variant.h"

#define C_RTFILE_PACKAGE_LATEST_VERSION 0
#define C_RTFILE_PACKAGE_HEADER "RTPACK"
#define C_RTFILE_PACKAGE_HEADER_BYTE_SIZE 6
#define RT_FORMAT_EMBEDDED_FILE 20000000
#define C_RTFILE_TEXTURE_HEADER "RTTXTR"

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

	struct RTFileHeader
    {
    	char fileTypeID[C_RTFILE_PACKAGE_HEADER_BYTE_SIZE];
    	uint8 version;
    	uint8 reserved[1];
    };
    
	struct rtpack_header
    {
    	RTFileHeader rtFileHeader;
    	unsigned int compressedSize;
    	unsigned int decompressedSize;
    	uint8 compressionType; //one of eCompressionType
    	uint8 reserved[15];
    };
    
#pragma pack(push,1)
    struct rttex_header
    {
    	RTFileHeader rtFileHeader;
    
    	//our custom header
    	int height;
    	int width;
    	int format; // probably GL_UNSIGNED_BYTE , or GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, or RT_FORMAT_EMBEDDED_FILE for jpgs, etc
    	int originalHeight; //before we padded to be a power of 2, if applicable.
    	int originalWidth; //before we padded to be a power of 2, if applicable.
    	unsigned char bUsesAlpha;
    	unsigned char bAlreadyCompressed; //if 1, it means we don't require additional compression
    	unsigned char reservedFlags[2]; //keep this struct packed right
    	int mipmapCount; //how many tex infos are followed...
    
    	int reserved[16];
    
    	//texture info to follow, 1 for each mip map
    };
#pragma pack(pop)
    
    struct rttex_mip_header
    {
    	int height;
    	int width;
    	int dataSize;
    	int mipLevel;
    	int reserved[2];
    };
    
    
    uint32 HashString(const char *str, int32 len);
    uint8 ConvertVariantType(eVariantTypes type);
    uint8 *SerializeToMem(const VariantVector &varVector, uint32 *pSizeOut, uint8 *pDest);
	uint32 GetMemEstiamte(const VariantVector& varVector);
}