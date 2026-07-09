#include "OSPrecompiled.h"

string GetFileNameFromPath(const string& path)
{
    usize pos = path.find_last_of("/\\");
    if (pos == string::npos)
        return path;

    return path.substr(pos + 1);
}