#include "CrashContext.h"
#include "../Utils/StringUtils.h"

enum class ValueType : uint8
{
    Empty,
    Int,
    Float,
    Bool,
    String
};

struct ContextValue
{
    ValueType type = ValueType::Empty;
    union
    {
        int64 i64;
        double f64;
        bool b;
        char str[128];
    };
};

struct ContextEntry
{
    char key[32]{};
    ContextValue val;
    bool active = false;
};

constexpr uint32 MAX_ENTRIES = 32;

// check thread local, needed?
static thread_local ContextEntry gEntries[MAX_ENTRIES];

static ContextEntry* GetOrCreateEntry(const char* key)
{
    if (!key || key[0] == '\0')
        return nullptr;

    ContextEntry* pFirstFree = nullptr;

    for (uint32 i = 0; i < MAX_ENTRIES; ++i)
    {
        if (gEntries[i].active)
        {
            if (strcmp(gEntries[i].key, key) == 0)
                return &gEntries[i];
        }
        else if (!pFirstFree)
        {
            pFirstFree = &gEntries[i];
        }
    }

    if (pFirstFree)
    {
        StrCopyFast(pFirstFree->key, key, sizeof(pFirstFree->key));
        pFirstFree->active = true;
        return pFirstFree;
    }

    return nullptr;
}

void CrashContext::SetInt64(const char* key, int64 value)
{
    if (auto* pEntry = GetOrCreateEntry(key))
    {
        pEntry->val.type = ValueType::Int;
        pEntry->val.i64 = value;
    }
}

void CrashContext::SetDouble(const char* key, double value)
{
    if (auto* pEntry = GetOrCreateEntry(key))
    {
        pEntry->val.type = ValueType::Float;
        pEntry->val.f64 = value;
    }
}

void CrashContext::Set(const char* key, bool value)
{
    if (auto* pEntry = GetOrCreateEntry(key))
    {
        pEntry->val.type = ValueType::Bool;
        pEntry->val.b = value;
    }
}

void CrashContext::Set(const char* key, const char* value)
{
    if (!key || !value)
        return;

    if (value[0] == '\0')
    {
        for (uint32 i = 0; i < MAX_ENTRIES; ++i)
        {
            if (gEntries[i].active && strcmp(gEntries[i].key, key) == 0)
            {
                gEntries[i].active = false;
                return;
            }
        }
        return;
    }

    if (auto* pEntry = GetOrCreateEntry(key))
    {
        pEntry->val.type = ValueType::String;
        StrCopyFast(pEntry->val.str, value, sizeof(pEntry->val.str));
    }
}

usize CrashContext::Dump(char* buf, usize bufSize)
{
    if (!buf || bufSize == 0)
        return 0;

    int32 written = std::snprintf(buf, bufSize, "\n========== LAST CRASH CONTEXT ==========\n");
    usize offset = (written > 0 && written < (int32)(bufSize)) ? written : 0;

    for (uint32 i = 0; i < MAX_ENTRIES; ++i)
    {
        if (!gEntries[i].active)
            continue;

        ContextValue& val = gEntries[i].val;
        int32 lineLen = 0;

        switch (val.type)
        {
            case ValueType::Int:
                lineLen =
                    std::snprintf(buf + offset, bufSize - offset, "  %s: %lld\n", gEntries[i].key, (long long)val.i64);
                break;
            case ValueType::Float:
                lineLen = std::snprintf(buf + offset, bufSize - offset, "  %s: %.3f\n", gEntries[i].key, val.f64);
                break;
            case ValueType::Bool:
                lineLen = std::snprintf(buf + offset, bufSize - offset, "  %s: %s\n", gEntries[i].key,
                                        val.b ? "true" : "false");
                break;
            case ValueType::String:
                lineLen = std::snprintf(buf + offset, bufSize - offset, "  %s: %s\n", gEntries[i].key, val.str);
                break;
            default:
                lineLen = std::snprintf(buf + offset, bufSize - offset, "  %s: N/A\n", gEntries[i].key);
                break;
        }

        if (lineLen > 0 && (offset + lineLen) < bufSize)
            offset += lineLen;
        else
            break;
    }

    written = std::snprintf(buf + offset, bufSize - offset, "==============================================\n");
    if (written > 0 && (offset + written) < bufSize)
    {
        offset += written;
    }

    return offset;
}