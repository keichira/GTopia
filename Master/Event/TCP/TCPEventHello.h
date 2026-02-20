#include "Event/TCP/TCPEventBase.h"

class TCPEventHello : public TCPEventBase {
public:
    void Execute(NetClient* pClient, VariantVector& data) override;
};