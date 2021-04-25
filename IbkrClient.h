#pragma once
#ifndef IbkrClient_h
#define IbkrClient_h

#include "EWrapper.h"
#include "EReaderOSSignal.h"
#include "EReader.h"
#include "StdAfx.h"

#include "EClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include "Execution.h"
#include "CommissionReport.h"
#include "ContractSamples.h"
#include "OrderSamples.h"
#include "ScannerSubscription.h"
#include "ScannerSubscriptionSamples.h"
#include "executioncondition.h"
#include "PriceCondition.h"
#include "MarginCondition.h"
#include "PercentChangeCondition.h"
#include "TimeCondition.h"
#include "VolumeCondition.h"
#include "AvailableAlgoParams.h"
#include "FAMethodSamples.h"
#include "CommonDefs.h"
#include "AccountSummaryTags.h"
#include "Utils.h"

#include <memory>
#include <vector>
#include <stdio.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <ctime>
#include <fstream>
#include <cstdint>

class IbkrClient : public EWrapper
{
public:
    IbkrClient();
    ~IbkrClient();
    void processMessages();
    bool connect(const char *host, int port, int clientId = 0);
    void disconnect() const;
    bool isConnected() const;

#include "EWrapper_prototypes.h"

private:
    EReaderOSSignal m_osSignal;
    EClientSocket *const m_pClient;
    State m_state;
    time_t m_sleepDeadline;
    OrderId m_orderId;
    EReader *m_pReader;
    bool m_extraAuth;
    std::string m_bboExchange;

    void pnlOperation();
    void pnlSingleOperation();
    void connectAck();
};

enum State
{
    ST_CONNECT,
    ST_TICKDATAOPERATION,
    ST_TICKDATAOPERATION_ACK,
    ST_TICKOPTIONCOMPUTATIONOPERATION,
    ST_TICKOPTIONCOMPUTATIONOPERATION_ACK,
    ST_DELAYEDTICKDATAOPERATION,
    ST_DELAYEDTICKDATAOPERATION_ACK,
    ST_MARKETDEPTHOPERATION,
    ST_MARKETDEPTHOPERATION_ACK,
    ST_REALTIMEBARS,
    ST_REALTIMEBARS_ACK,
    ST_MARKETDATATYPE,
    ST_MARKETDATATYPE_ACK,
    ST_HISTORICALDATAREQUESTS,
    ST_HISTORICALDATAREQUESTS_ACK,
    ST_OPTIONSOPERATIONS,
    ST_OPTIONSOPERATIONS_ACK,
    ST_CONTRACTOPERATION,
    ST_CONTRACTOPERATION_ACK,
    ST_MARKETSCANNERS,
    ST_MARKETSCANNERS_ACK,
    ST_FUNDAMENTALS,
    ST_FUNDAMENTALS_ACK,
    ST_BULLETINS,
    ST_BULLETINS_ACK,
    ST_ACCOUNTOPERATIONS,
    ST_ACCOUNTOPERATIONS_ACK,
    ST_ORDEROPERATIONS,
    ST_ORDEROPERATIONS_ACK,
    ST_OCASAMPLES,
    ST_OCASAMPLES_ACK,
    ST_CONDITIONSAMPLES,
    ST_CONDITIONSAMPLES_ACK,
    ST_BRACKETSAMPLES,
    ST_BRACKETSAMPLES_ACK,
    ST_HEDGESAMPLES,
    ST_HEDGESAMPLES_ACK,
    ST_TESTALGOSAMPLES,
    ST_TESTALGOSAMPLES_ACK,
    ST_FAORDERSAMPLES,
    ST_FAORDERSAMPLES_ACK,
    ST_FAOPERATIONS,
    ST_FAOPERATIONS_ACK,
    ST_DISPLAYGROUPS,
    ST_DISPLAYGROUPS_ACK,
    ST_MISCELANEOUS,
    ST_MISCELANEOUS_ACK,
    ST_CANCELORDER,
    ST_CANCELORDER_ACK,
    ST_FAMILYCODES,
    ST_FAMILYCODES_ACK,
    ST_SYMBOLSAMPLES,
    ST_SYMBOLSAMPLES_ACK,
    ST_REQMKTDEPTHEXCHANGES,
    ST_REQMKTDEPTHEXCHANGES_ACK,
    ST_REQNEWSTICKS,
    ST_REQNEWSTICKS_ACK,
    ST_REQSMARTCOMPONENTS,
    ST_REQSMARTCOMPONENTS_ACK,
    ST_NEWSPROVIDERS,
    ST_NEWSPROVIDERS_ACK,
    ST_REQNEWSARTICLE,
    ST_REQNEWSARTICLE_ACK,
    ST_REQHISTORICALNEWS,
    ST_REQHISTORICALNEWS_ACK,
    ST_REQHEADTIMESTAMP,
    ST_REQHEADTIMESTAMP_ACK,
    ST_REQHISTOGRAMDATA,
    ST_REQHISTOGRAMDATA_ACK,
    ST_REROUTECFD,
    ST_REROUTECFD_ACK,
    ST_MARKETRULE,
    ST_MARKETRULE_ACK,
    ST_PNL,
    ST_PNL_ACK,
    ST_PNLSINGLE,
    ST_PNLSINGLE_ACK,
    ST_CONTFUT,
    ST_CONTFUT_ACK,
    ST_PING,
    ST_PING_ACK,
    ST_REQHISTORICALTICKS,
    ST_REQHISTORICALTICKS_ACK,
    ST_REQTICKBYTICKDATA,
    ST_REQTICKBYTICKDATA_ACK,
    ST_WHATIFSAMPLES,
    ST_WHATIFSAMPLES_ACK,
    ST_IDLE
};

#endif