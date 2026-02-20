#include "NetClient.h"
#include "NetSocket.h"

bool NetClient::Send(const VariantVector& data)
{
    if(socket < 0 || !pNetSocket) {
        return false;
    }

    uint32 size = 0;
    uint8* pData = SerializeVariantVectorForTCP(data, size);

    bool succeed = pNetSocket->Send(this, pData, size);

    SAFE_DELETE_ARRAY(pData);
    return succeed;
}

uint8* SerializeVariantVectorForTCP(const VariantVector& varVector, uint32& outSize)
{
    uint32 size = 5;
    for(auto& var : varVector) {
        size += 1 + var.GetSize();
        if(var.GetType() == VARIANT_TYPE_STRING) { size += 2; }
    }

    uint8* pData = new uint8[size];

    MemoryBuffer memBuffer(pData, size);
    memBuffer.Write(size - 4);

    uint8 varVecSize = varVector.size();
    memBuffer.Write(varVecSize);

    for(auto& var : varVector) {
        uint32 varSize = var.GetSize();
        memBuffer.Write(var.GetType());

        if(var.GetType() == VARIANT_TYPE_STRING) {
            memBuffer.WriteString<uint16>(var.GetString());
        }
        else {
            auto varValue = var.GetValue();
            memBuffer.WriteRaw(&varValue, varSize);
        }
    }

    outSize = size;
    return pData;
}

void DeSerializeVariantVectorForTCP(MemoryBuffer& memBuffer, VariantVector& out)
{
    out.clear();

    uint8 varCount = 0;
    memBuffer.Read(varCount);

    out.reserve(varCount);

    for(uint16 i = 0; i < varCount; ++i) {
        Variant var;

        uint8 type = 0;
        memBuffer.Read(type);

        switch(type) {
            case VARIANT_TYPE_STRING: {
                string str;
                memBuffer.ReadString<uint16>(str);
    
                var = str;
                break;
            }

            case VARIANT_TYPE_FLOAT: {
                float val;
                memBuffer.Read(val);

                var = val;
                break;
            }

            case VARIANT_TYPE_UINT: {
                uint32 val;
                memBuffer.Read(val);

                var = val;
                break;
            }

            case VARIANT_TYPE_BOOL: {
                bool val;
                memBuffer.Read(val);

                var = val;
                break;
            }

            case VARIANT_TYPE_INT: {
                int32 val;
                memBuffer.Read(val);

                var = val;
                break;
            }
        }

        out.push_back(std::move(var));
    }
}
