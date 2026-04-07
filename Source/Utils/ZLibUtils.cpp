#include "ZLibUtils.h"
#include <zlib.h>

uint8* zLibInflateToMemory(uint8 *pCompData, uint32 compSize, uint32 decompSize)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    uint8* pOut = new uint8[decompSize + 1];
    strm.avail_in = compSize;
    strm.next_in = pCompData;
    strm.avail_out = decompSize;
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