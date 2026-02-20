#include "Event/TCP/TCPEventBase.h"

class TCPEventAuth : public TCPEventBase {
public:
    void Execute(NetClient* pClient, VariantVector& data) override;
};