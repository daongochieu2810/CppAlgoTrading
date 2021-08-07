#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <chrono>
#include <thread>

#include "StdAfx.h"
#include "Ibkr.h"
#include "Strategy.h"

int main()
{
    Strategy strategy;
    IbkrClient client;
    if (client.connect(LOCAL_HOST, DEFAULT_PORT, 0))
    {
        while (client.isConnected())
        {
            printf("Waiting for next iteration...\n");
            client.processMessages();
            //client.m_state = ST_PNL;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    return 0;
}