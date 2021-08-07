#ifndef ibkr_h
#define ibkr_h

#include <string>
#include "IbkrClient.h"

#define LOCAL_HOST "127.0.0.1"
#define DEFAULT_PORT 7497

class BotData
{
    std::string apiKey;

public:
    void setUpKeys();
} bot;

#endif