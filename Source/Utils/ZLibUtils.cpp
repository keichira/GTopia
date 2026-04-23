#include "ZLibUtils.h"
#include <zlib.h>

uint8* zLibInflateToMemory(uint8* pCompData, uint32 compSize, uint32 decompSize)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    uint8* pOut = new uint8[decompSize + 1];
    strm.avail_in = compSize;
    strm.next_in = pCompData;
    strm.avail_out = decompSize + 1;
    strm.next_out = pOut;

    int32 ret = inflateInit(&strm);
    if(ret != Z_OK) {
        SAFE_DELETE_ARRAY(pOut);
        return nullptr;
    }

    ret = inflate(&strm, Z_FINISH);
    if(ret != Z_STREAM_END) {
        SAFE_DELETE_ARRAY(pOut);
        inflateEnd(&strm);
        return nullptr;
    }

    ret = inflateEnd(&strm);
    return pOut;
}

uint8* zLibDefalteToMemory(uint8* pData, uint32 size, uint32& compSize)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    uint8* pOut = new uint8[size + 1];
    strm.avail_in = size;
    strm.next_in = pData;
    strm.avail_out = size + 1;
    strm.next_out = pOut;

    int32 ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if(ret != Z_OK) {
        SAFE_DELETE_ARRAY(pOut);
        return nullptr;
    }

    ret = deflate(&strm, Z_FINISH);
    if(ret != Z_STREAM_END) {
        SAFE_DELETE_ARRAY(pOut);
        deflateEnd(&strm);
        return nullptr;
    }

    compSize = strm.total_out;
    ret = deflateEnd(&strm);
    return pOut;
}
