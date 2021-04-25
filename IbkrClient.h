#pragma once
#ifndef IbkrClient_h
#define IbkrClient_h

#include "EWrapper.h"
#include "EReaderOSSignal.h"
#include "EReader.h"

#include <memory>
#include <vector>

class IbkrClient : public EWrapper
{
public:
    void processMessages();
    bool connect(const char *host, int port, int clientId = 0);
    void disconnect() const;
    bool isConnected() const;

private:
    void pnlOperation();
};

#endif