/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"

#include "EPosixClientSocketPlatform.h"

#include "EClient.h"

#include "EWrapper.h"
#include "TwsSocketClientErrors.h"
#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include "Execution.h"
#include "ScannerSubscription.h"
#include "CommissionReport.h"
#include "EDecoder.h"
#include "EMessage.h"
#include "ETransport.h"
#include "FamilyCode.h"

#include <sstream>
#include <iomanip>
#include <algorithm>

#include <stdio.h>
#include <string.h>
#include <assert.h>


using namespace ibapi::client_constants;

///////////////////////////////////////////////////////////
// encoders
template<>
void EClient::EncodeField<bool>(std::ostream& os, bool boolValue)
{
    EncodeField<int>(os, boolValue ? 1 : 0);
}

template<>
void EClient::EncodeField<double>(std::ostream& os, double doubleValue)
{
    char str[128];

    snprintf(str, sizeof(str), "%.10g", doubleValue);

    EncodeField<const char*>(os, str);
}

void EClient::EncodeContract(std::ostream& os, const Contract &contract)
{
    EncodeField(os, contract.conId);
    EncodeField(os, contract.symbol);
    EncodeField(os, contract.secType);
    EncodeField(os, contract.lastTradeDateOrContractMonth);
    EncodeField(os, contract.strike);
    EncodeField(os, contract.right);
    EncodeField(os, contract.multiplier);
    EncodeField(os, contract.exchange);
    EncodeField(os, contract.primaryExchange);
    EncodeField(os, contract.currency);
    EncodeField(os, contract.localSymbol);
    EncodeField(os, contract.tradingClass);
    EncodeField(os, contract.includeExpired);
}

void EClient::EncodeTagValueList(std::ostream& os, const TagValueListSPtr &tagValueList) 
{
    std::string tagValueListStr("");
    const int tagValueListCount = tagValueList.get() ? tagValueList->size() : 0;

    if (tagValueListCount > 0) {
        for (int i = 0; i < tagValueListCount; ++i) {
            const TagValue* tagValue = ((*tagValueList)[i]).get();

            tagValueListStr += tagValue->tag;
            tagValueListStr += "=";
            tagValueListStr += tagValue->value;
            tagValueListStr += ";";
        }
    }

    EncodeField(os, tagValueListStr);
}

///////////////////////////////////////////////////////////
// "max" encoders
void EClient::EncodeFieldMax(std::ostream& os, int intValue)
{
    if( intValue == INT_MAX) {
        EncodeField(os, "");
        return;
    }
    EncodeField(os, intValue);
}

void EClient::EncodeFieldMax(std::ostream& os, double doubleValue)
{
    if( doubleValue == DBL_MAX) {
        EncodeField(os, "");
        return;
    }
    EncodeField(os, doubleValue);
}


///////////////////////////////////////////////////////////
// member funcs
EClient::EClient( EWrapper *ptr, ETransport *pTransport)
    : m_pEWrapper(ptr)
    , m_transport(pTransport)
    , m_clientId(-1)
    , m_connState(CS_DISCONNECTED)
    , m_extraAuth(false)
    , m_serverVersion(0)
    , m_useV100Plus(true)
{
}

EClient::~EClient()
{
}

EClient::ConnState EClient::connState() const
{
    return m_connState;
}

bool EClient::isConnected() const
{
    return m_connState == CS_CONNECTED;
}

bool EClient::isConnecting() const
{
    return m_connState == CS_CONNECTING;
}

void EClient::eConnectBase()
{
}

void EClient::eDisconnectBase()
{
    m_TwsTime.clear();
    m_serverVersion = 0;
    m_connState = CS_DISCONNECTED;
    m_extraAuth = false;
    m_clientId = -1;
}

int EClient::serverVersion()
{
    return m_serverVersion;
}

std::string EClient::TwsConnectionTime()
{
    return m_TwsTime;
}

const std::string& EClient::optionalCapabilities() const
{
    return m_optionalCapabilities;
}

void EClient::setOptionalCapabilities(const std::string& optCapts)
{
    m_optionalCapabilities = optCapts;
}

void EClient::setConnectOptions(const std::string& connectOptions)
{
    if( isSocketOK()) {
        m_pEWrapper->error( NO_VALID_ID, ALREADY_CONNECTED.code(), ALREADY_CONNECTED.msg());
        return;
    }

    m_connectOptions = connectOptions;
}

void EClient::disableUseV100Plus()
{
    if( isSocketOK()) {
        m_pEWrapper->error( NO_VALID_ID, ALREADY_CONNECTED.code(), ALREADY_CONNECTED.msg());
        return;
    }

    m_useV100Plus = false;
    m_connectOptions = "";
}

bool EClient::usingV100Plus() {
    return m_useV100Plus;
}

int EClient::bufferedSend(const std::string& msg) {
    EMessage emsg(std::vector<char>(msg.begin(), msg.end()));

    return m_transport->send(&emsg);
}

void EClient::reqMktData(TickerId tickerId, const Contract& contract,
                         const std::string& genericTicks, bool snapshot, bool regulatorySnaphsot, const TagValueListSPtr& mktDataOptions)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // not needed anymore validation
    //if( m_serverVersion < MIN_SERVER_VER_SNAPSHOT_MKT_DATA && snapshot) {
    //	m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //		"  It does not support snapshot market data requests.");
    //	return;
    //}

    if( m_serverVersion < MIN_SERVER_VER_DELTA_NEUTRAL) {
        if( contract.deltaNeutralContract) {
            m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support delta-neutral orders.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_REQ_MKT_DATA_CONID) {
        if( contract.conId > 0) {
            m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support conId parameter.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
        if( !contract.tradingClass.empty() ) {
            m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support tradingClass parameter in reqMktData.");
            return;
        }
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 11;

    // send req mkt data msg
    ENCODE_FIELD( REQ_MKT_DATA);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( tickerId);

    // send contract fields
    if( m_serverVersion >= MIN_SERVER_VER_REQ_MKT_DATA_CONID) {
        ENCODE_FIELD( contract.conId);
    }
    ENCODE_FIELD( contract.symbol);
    ENCODE_FIELD( contract.secType);
    ENCODE_FIELD( contract.lastTradeDateOrContractMonth);
    ENCODE_FIELD( contract.strike);
    ENCODE_FIELD( contract.right);
    ENCODE_FIELD( contract.multiplier); // srv v15 and above

    ENCODE_FIELD( contract.exchange);
    ENCODE_FIELD( contract.primaryExchange); // srv v14 and above
    ENCODE_FIELD( contract.currency);

    ENCODE_FIELD( contract.localSymbol); // srv v2 and above

    if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.tradingClass);
    }

    // Send combo legs for BAG requests (srv v8 and above)
    if( contract.secType == "BAG")
    {
        const Contract::ComboLegList* const comboLegs = contract.comboLegs.get();
        const int comboLegsCount = comboLegs ? comboLegs->size() : 0;
        ENCODE_FIELD( comboLegsCount);
        if( comboLegsCount > 0) {
            for( int i = 0; i < comboLegsCount; ++i) {
                const ComboLeg* comboLeg = ((*comboLegs)[i]).get();
                assert( comboLeg);
                ENCODE_FIELD( comboLeg->conId);
                ENCODE_FIELD( comboLeg->ratio);
                ENCODE_FIELD( comboLeg->action);
                ENCODE_FIELD( comboLeg->exchange);
            }
        }
    }

    if( m_serverVersion >= MIN_SERVER_VER_DELTA_NEUTRAL) {
        if( contract.deltaNeutralContract) {
            const DeltaNeutralContract& deltaNeutralContract = *contract.deltaNeutralContract;
            ENCODE_FIELD( true);
            ENCODE_FIELD( deltaNeutralContract.conId);
            ENCODE_FIELD( deltaNeutralContract.delta);
            ENCODE_FIELD( deltaNeutralContract.price);
        }
        else {
            ENCODE_FIELD( false);
        }
    }

    ENCODE_FIELD( genericTicks); // srv v31 and above
    ENCODE_FIELD( snapshot); // srv v35 and above

    if (m_serverVersion >= MIN_SERVER_VER_REQ_SMART_COMPONENTS) {
        ENCODE_FIELD(regulatorySnaphsot);
    }

    // send mktDataOptions parameter
    if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
        ENCODE_TAGVALUELIST(mktDataOptions);
    }

    closeAndSend( msg.str());
}

void EClient::cancelMktData(TickerId tickerId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 2;

    // send cancel mkt data msg
    ENCODE_FIELD( CANCEL_MKT_DATA);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( tickerId);

    closeAndSend( msg.str());
}

void EClient::reqMktDepth( TickerId tickerId, const Contract& contract, int numRows, bool isSmartDepth, const TagValueListSPtr& mktDepthOptions)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    // This feature is only available for versions of TWS >=6
    //if( m_serverVersion < 6) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
    //	return;
    //}

    if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
        if( !contract.tradingClass.empty() || (contract.conId > 0)) {
            m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support conId and tradingClass parameters in reqMktDepth.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_SMART_DEPTH && isSmartDepth) {
        m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support SMART depth request.");
        return;
    }

    if (m_serverVersion < MIN_SERVER_VER_MKT_DEPTH_PRIM_EXCHANGE && !contract.primaryExchange.empty()) {
        m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support primaryExchange parameter in reqMktDepth.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 5;

    // send req mkt data msg
    ENCODE_FIELD( REQ_MKT_DEPTH);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( tickerId);

    // send contract fields
    if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.conId);
    }
    ENCODE_FIELD( contract.symbol);
    ENCODE_FIELD( contract.secType);
    ENCODE_FIELD( contract.lastTradeDateOrContractMonth);
    ENCODE_FIELD( contract.strike);
    ENCODE_FIELD( contract.right);
    ENCODE_FIELD( contract.multiplier); // srv v15 and above
    ENCODE_FIELD( contract.exchange);
    if( m_serverVersion >= MIN_SERVER_VER_MKT_DEPTH_PRIM_EXCHANGE) {
        ENCODE_FIELD( contract.primaryExchange);
    }
    ENCODE_FIELD( contract.currency);
    ENCODE_FIELD( contract.localSymbol);
    if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.tradingClass);
    }

    ENCODE_FIELD( numRows); // srv v19 and above

    if( m_serverVersion >= MIN_SERVER_VER_SMART_DEPTH) {
        ENCODE_FIELD( isSmartDepth);
    }

    // send mktDepthOptions parameter
    if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
        ENCODE_TAGVALUELIST(mktDepthOptions);
    }

    closeAndSend( msg.str());
}


void EClient::cancelMktDepth( TickerId tickerId, bool isSmartDepth)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if (m_serverVersion < MIN_SERVER_VER_SMART_DEPTH && isSmartDepth) {
        m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support SMART depth cancel.");
        return;
    }

    // Not needed anymore validation
    // This feature is only available for versions of TWS >=6
    //if( m_serverVersion < 6) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
    //	return;
    //}

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send cancel mkt data msg
    ENCODE_FIELD( CANCEL_MKT_DEPTH);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( tickerId);

    if( m_serverVersion >= MIN_SERVER_VER_SMART_DEPTH) {
        ENCODE_FIELD( isSmartDepth);
    }

    closeAndSend( msg.str());
}

void EClient::reqHistoricalData(TickerId tickerId, const Contract& contract,
                                const std::string& endDateTime, const std::string& durationStr,
                                const std::string&  barSizeSetting, const std::string& whatToShow,
                                int useRTH, int formatDate, bool keepUpToDate, const TagValueListSPtr& chartOptions)
{
    // not connected?
    if (!isConnected()) {
        m_pEWrapper->error(tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if (m_serverVersion < 16) {
    //	m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
    //	return;
    //}

    if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
        if (!contract.tradingClass.empty() || (contract.conId > 0)) {
            m_pEWrapper->error(tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support conId and tradingClass parameters in reqHistoricalData.");
            return;
        }
    }

    std::stringstream msg;
    prepareBuffer(msg);

    const int VERSION = 6;

    ENCODE_FIELD(REQ_HISTORICAL_DATA);

    if (m_serverVersion < MIN_SERVER_VER_SYNT_REALTIME_BARS) {
        ENCODE_FIELD(VERSION);
    }

    ENCODE_FIELD(tickerId);

    // send contract fields
    if (m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD(contract.conId);
    }
    ENCODE_FIELD(contract.symbol);
    ENCODE_FIELD(contract.secType);
    ENCODE_FIELD(contract.lastTradeDateOrContractMonth);
    ENCODE_FIELD(contract.strike);
    ENCODE_FIELD(contract.right);
    ENCODE_FIELD(contract.multiplier);
    ENCODE_FIELD(contract.exchange);
    ENCODE_FIELD(contract.primaryExchange);
    ENCODE_FIELD(contract.currency);
    ENCODE_FIELD(contract.localSymbol);
    if (m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD(contract.tradingClass);
    }
    ENCODE_FIELD(contract.includeExpired); // srv v31 and above

    ENCODE_FIELD(endDateTime); // srv v20 and above
    ENCODE_FIELD(barSizeSetting); // srv v20 and above

    ENCODE_FIELD(durationStr);
    ENCODE_FIELD(useRTH);
    ENCODE_FIELD(whatToShow);
    ENCODE_FIELD(formatDate); // srv v16 and above

    // Send combo legs for BAG requests
    if (contract.secType == "BAG")
    {
        const Contract::ComboLegList* const comboLegs = contract.comboLegs.get();
        const int comboLegsCount = comboLegs ? comboLegs->size() : 0;
        ENCODE_FIELD(comboLegsCount);
        if (comboLegsCount > 0) {
            for(int i = 0; i < comboLegsCount; ++i) {
                const ComboLeg* comboLeg = ((*comboLegs)[i]).get();
                assert(comboLeg);
                ENCODE_FIELD(comboLeg->conId);
                ENCODE_FIELD(comboLeg->ratio);
                ENCODE_FIELD(comboLeg->action);
                ENCODE_FIELD(comboLeg->exchange);
            }
        }
    }

    if (m_serverVersion >= MIN_SERVER_VER_SYNT_REALTIME_BARS) {
        ENCODE_FIELD(keepUpToDate);
    }

    // send chartOptions parameter
    if (m_serverVersion >= MIN_SERVER_VER_LINKING) {
        ENCODE_TAGVALUELIST(chartOptions);
    }

    closeAndSend(msg.str());
}

void EClient::cancelHistoricalData(TickerId tickerId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < 24) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //		"  It does not support historical data query cancellation.");
    //	return;
    //}

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_HISTORICAL_DATA);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( tickerId);

    closeAndSend( msg.str());
}

void EClient::reqRealTimeBars(TickerId tickerId, const Contract& contract,
                              int barSize, const std::string& whatToShow, bool useRTH,
                              const TagValueListSPtr& realTimeBarsOptions)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < MIN_SERVER_VER_REAL_TIME_BARS) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //		"  It does not support real time bars.");
    //	return;
    //}

    if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
        if( !contract.tradingClass.empty() || (contract.conId > 0)) {
            m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support conId and tradingClass parameters in reqRealTimeBars.");
            return;
        }
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 3;

    ENCODE_FIELD( REQ_REAL_TIME_BARS);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( tickerId);

    // send contract fields
    if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.conId);
    }
    ENCODE_FIELD( contract.symbol);
    ENCODE_FIELD( contract.secType);
    ENCODE_FIELD( contract.lastTradeDateOrContractMonth);
    ENCODE_FIELD( contract.strike);
    ENCODE_FIELD( contract.right);
    ENCODE_FIELD( contract.multiplier);
    ENCODE_FIELD( contract.exchange);
    ENCODE_FIELD( contract.primaryExchange);
    ENCODE_FIELD( contract.currency);
    ENCODE_FIELD( contract.localSymbol);
    if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.tradingClass);
    }
    ENCODE_FIELD( barSize);
    ENCODE_FIELD( whatToShow);
    ENCODE_FIELD( useRTH);

    // send realTimeBarsOptions parameter
    if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
        ENCODE_TAGVALUELIST(realTimeBarsOptions);
    }

    closeAndSend( msg.str());
}


void EClient::cancelRealTimeBars(TickerId tickerId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < MIN_SERVER_VER_REAL_TIME_BARS) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //		"  It does not support realtime bar data query cancellation.");
    //	return;
    //}

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_REAL_TIME_BARS);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( tickerId);

    closeAndSend( msg.str());
}


void EClient::reqScannerParameters()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < 24) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //		"  It does not support API scanner subscription.");
    //	return;
    //}

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( REQ_SCANNER_PARAMETERS);
    ENCODE_FIELD( VERSION);

    closeAndSend( msg.str());
}


void EClient::reqScannerSubscription(int tickerId,
                                     const ScannerSubscription& subscription, const TagValueListSPtr& scannerSubscriptionOptions, const TagValueListSPtr& scannerSubscriptionFilterOptions)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < 24) {
    //	m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //		"  It does not support API scanner subscription.");
    //	return;
    //}

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 4;

    ENCODE_FIELD( REQ_SCANNER_SUBSCRIPTION);

    if (m_serverVersion < MIN_SERVER_VER_SCANNER_GENERIC_OPTS) {
        ENCODE_FIELD(VERSION);
    }

    ENCODE_FIELD( tickerId);
    ENCODE_FIELD_MAX( subscription.numberOfRows);
    ENCODE_FIELD( subscription.instrument);
    ENCODE_FIELD( subscription.locationCode);
    ENCODE_FIELD( subscription.scanCode);
    ENCODE_FIELD_MAX( subscription.abovePrice);
    ENCODE_FIELD_MAX( subscription.belowPrice);
    ENCODE_FIELD_MAX( subscription.aboveVolume);
    ENCODE_FIELD_MAX( subscription.marketCapAbove);
    ENCODE_FIELD_MAX( subscription.marketCapBelow);
    ENCODE_FIELD( subscription.moodyRatingAbove);
    ENCODE_FIELD( subscription.moodyRatingBelow);
    ENCODE_FIELD( subscription.spRatingAbove);
    ENCODE_FIELD( subscription.spRatingBelow);
    ENCODE_FIELD( subscription.maturityDateAbove);
    ENCODE_FIELD( subscription.maturityDateBelow);
    ENCODE_FIELD_MAX( subscription.couponRateAbove);
    ENCODE_FIELD_MAX( subscription.couponRateBelow);
    ENCODE_FIELD_MAX( subscription.excludeConvertible);
    ENCODE_FIELD_MAX( subscription.averageOptionVolumeAbove); // srv v25 and above
    ENCODE_FIELD( subscription.scannerSettingPairs); // srv v25 and above
    ENCODE_FIELD( subscription.stockTypeFilter); // srv v27 and above

    if (m_serverVersion >= MIN_SERVER_VER_SCANNER_GENERIC_OPTS) {
        ENCODE_TAGVALUELIST(scannerSubscriptionFilterOptions);
    }

    // send scannerSubscriptionOptions parameter
    if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
        ENCODE_TAGVALUELIST(scannerSubscriptionOptions);
    }

    closeAndSend( msg.str());
}

void EClient::cancelScannerSubscription(int tickerId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < 24) {
    //	m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //		"  It does not support API scanner subscription.");
    //	return;
    //}

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_SCANNER_SUBSCRIPTION);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( tickerId);

    closeAndSend( msg.str());
}

void EClient::reqFundamentalData(TickerId reqId, const Contract& contract, 
                                 const std::string& reportType,
                                 //reserved for future use, must be blank
                                 const TagValueListSPtr& fundamentalDataOptions)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( reqId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_FUNDAMENTAL_DATA) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support fundamental data requests.");
        return;
    }

    if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
        if( contract.conId > 0) {
            m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support conId parameter in reqFundamentalData.");
            return;
        }
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 2;

    ENCODE_FIELD(REQ_FUNDAMENTAL_DATA);
    ENCODE_FIELD(VERSION);
    ENCODE_FIELD(reqId);

    // send contract fields
    if (m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.conId);
    }

    ENCODE_FIELD(contract.symbol);
    ENCODE_FIELD(contract.secType);
    ENCODE_FIELD(contract.exchange);
    ENCODE_FIELD(contract.primaryExchange);
    ENCODE_FIELD(contract.currency);
    ENCODE_FIELD(contract.localSymbol);

    ENCODE_FIELD(reportType);

    if (m_serverVersion >= MIN_SERVER_VER_LINKING) {
        ENCODE_TAGVALUELIST(fundamentalDataOptions);
    }

    closeAndSend(msg.str());
}

void EClient::cancelFundamentalData( TickerId reqId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( reqId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_FUNDAMENTAL_DATA) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support fundamental data requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_FUNDAMENTAL_DATA);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);

    closeAndSend( msg.str());
}

void EClient::calculateImpliedVolatility(TickerId reqId, const Contract& contract, double optionPrice, double underPrice,
                                         //reserved for future use, must be blank
                                         const TagValueListSPtr& miscOptions) {

                                             // not connected?
                                             if (!isConnected()) {
                                                 m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
                                                 return;
                                             }

                                             if (m_serverVersion < MIN_SERVER_VER_REQ_CALC_IMPLIED_VOLAT) {
                                                 m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                                                     "  It does not support calculate implied volatility requests.");
                                                 return;
                                             }

                                             if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
                                                 if( !contract.tradingClass.empty()) {
                                                     m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                                                         "  It does not support tradingClass parameter in calculateImpliedVolatility.");
                                                     return;
                                                 }
                                             }

                                             std::stringstream msg;

                                             prepareBuffer(msg);

                                             const int VERSION = 2;

                                             ENCODE_FIELD(REQ_CALC_IMPLIED_VOLAT);
                                             ENCODE_FIELD(VERSION);
                                             ENCODE_FIELD(reqId);

                                             // send contract fields
                                             ENCODE_FIELD(contract.conId);
                                             ENCODE_FIELD(contract.symbol);
                                             ENCODE_FIELD(contract.secType);
                                             ENCODE_FIELD(contract.lastTradeDateOrContractMonth);
                                             ENCODE_FIELD(contract.strike);
                                             ENCODE_FIELD(contract.right);
                                             ENCODE_FIELD(contract.multiplier);
                                             ENCODE_FIELD(contract.exchange);
                                             ENCODE_FIELD(contract.primaryExchange);
                                             ENCODE_FIELD(contract.currency);
                                             ENCODE_FIELD(contract.localSymbol);

                                             if (m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
                                                 ENCODE_FIELD(contract.tradingClass);
                                             }

                                             ENCODE_FIELD(optionPrice);
                                             ENCODE_FIELD(underPrice);

                                             if (m_serverVersion >= MIN_SERVER_VER_LINKING) {
                                                 ENCODE_TAGVALUELIST(miscOptions);
                                             }

                                             closeAndSend( msg.str());
}

void EClient::cancelCalculateImpliedVolatility(TickerId reqId) {

    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if (m_serverVersion < MIN_SERVER_VER_CANCEL_CALC_IMPLIED_VOLAT) {
        m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support calculate implied volatility cancellation.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_CALC_IMPLIED_VOLAT);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);

    closeAndSend( msg.str());
}

void EClient::calculateOptionPrice(TickerId reqId, const Contract& contract, double volatility, double underPrice, 
                                   //reserved for future use, must be blank
                                   const TagValueListSPtr& miscOptions) {

                                       // not connected?
                                       if( !isConnected()) {
                                           m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
                                           return;
                                       }

                                       if (m_serverVersion < MIN_SERVER_VER_REQ_CALC_OPTION_PRICE) {
                                           m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                                               "  It does not support calculate option price requests.");
                                           return;
                                       }

                                       if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
                                           if( !contract.tradingClass.empty()) {
                                               m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                                                   "  It does not support tradingClass parameter in calculateOptionPrice.");
                                               return;
                                           }
                                       }

                                       std::stringstream msg;

                                       prepareBuffer(msg);

                                       const int VERSION = 2;

                                       ENCODE_FIELD( REQ_CALC_OPTION_PRICE);
                                       ENCODE_FIELD( VERSION);
                                       ENCODE_FIELD( reqId);

                                       // send contract fields
                                       ENCODE_FIELD( contract.conId);
                                       ENCODE_FIELD( contract.symbol);
                                       ENCODE_FIELD( contract.secType);
                                       ENCODE_FIELD( contract.lastTradeDateOrContractMonth);
                                       ENCODE_FIELD( contract.strike);
                                       ENCODE_FIELD( contract.right);
                                       ENCODE_FIELD( contract.multiplier);
                                       ENCODE_FIELD( contract.exchange);
                                       ENCODE_FIELD( contract.primaryExchange);
                                       ENCODE_FIELD( contract.currency);
                                       ENCODE_FIELD( contract.localSymbol);

                                       if (m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
                                           ENCODE_FIELD( contract.tradingClass);
                                       }

                                       ENCODE_FIELD( volatility);
                                       ENCODE_FIELD( underPrice);

                                       if (m_serverVersion >= MIN_SERVER_VER_LINKING) {
                                           ENCODE_TAGVALUELIST(miscOptions);
                                       }

                                       closeAndSend( msg.str());
}

void EClient::cancelCalculateOptionPrice(TickerId reqId) {

    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if (m_serverVersion < MIN_SERVER_VER_CANCEL_CALC_OPTION_PRICE) {
        m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support calculate option price cancellation.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_CALC_OPTION_PRICE);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);

    closeAndSend( msg.str());
}

void EClient::reqContractDetails( int reqId, const Contract& contract)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    // This feature is only available for versions of TWS >=4
    //if( m_serverVersion < 4) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
    //	return;
    //}
    if (m_serverVersion < MIN_SERVER_VER_SEC_ID_TYPE) {
        if( !contract.secIdType.empty() || !contract.secId.empty()) {
            m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support secIdType and secId parameters.");
            return;
        }
    }
    if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
        if( !contract.tradingClass.empty()) {
            m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support tradingClass parameter in reqContractDetails.");
            return;
        }
    }
    if (m_serverVersion < MIN_SERVER_VER_LINKING) {
        if (!contract.primaryExchange.empty()) {
            m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support primaryExchange parameter in reqContractDetails.");
            return;
        }
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 8;

    // send req mkt data msg
    ENCODE_FIELD( REQ_CONTRACT_DATA);
    ENCODE_FIELD( VERSION);

    if( m_serverVersion >= MIN_SERVER_VER_CONTRACT_DATA_CHAIN) {
        ENCODE_FIELD( reqId);
    }

    // send contract fields
    ENCODE_FIELD( contract.conId); // srv v37 and above
    ENCODE_FIELD( contract.symbol);
    ENCODE_FIELD( contract.secType);
    ENCODE_FIELD( contract.lastTradeDateOrContractMonth);
    ENCODE_FIELD( contract.strike);
    ENCODE_FIELD( contract.right);
    ENCODE_FIELD( contract.multiplier); // srv v15 and above

    if (m_serverVersion >= MIN_SERVER_VER_PRIMARYEXCH)
    {
        ENCODE_FIELD(contract.exchange);
        ENCODE_FIELD(contract.primaryExchange);
    }
    else if (m_serverVersion >= MIN_SERVER_VER_LINKING)
    {
        if (!contract.primaryExchange.empty() && (contract.exchange == "BEST" || contract.exchange == "SMART"))
        {
            ENCODE_FIELD( contract.exchange + ":" + contract.primaryExchange);
        }
        else
        {
            ENCODE_FIELD(contract.exchange);
        }
    }

    ENCODE_FIELD( contract.currency);
    ENCODE_FIELD( contract.localSymbol);
    if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.tradingClass);
    }
    ENCODE_FIELD( contract.includeExpired); // srv v31 and above

    if( m_serverVersion >= MIN_SERVER_VER_SEC_ID_TYPE){
        ENCODE_FIELD( contract.secIdType);
        ENCODE_FIELD( contract.secId);
    }

    closeAndSend( msg.str());
}

void EClient::reqCurrentTime()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    // This feature is only available for versions of TWS >= 33
    //if( m_serverVersion < 33) {
    //	m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //		"  It does not support current time requests.");
    //	return;
    //}

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send current time req
    ENCODE_FIELD( REQ_CURRENT_TIME);
    ENCODE_FIELD( VERSION);

    closeAndSend( msg.str());
}

void EClient::placeOrder( OrderId id, const Contract& contract, const Order& order)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( id, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < MIN_SERVER_VER_SCALE_ORDERS) {
    //	if( order.scaleNumComponents != UNSET_INTEGER ||
    //		order.scaleComponentSize != UNSET_INTEGER ||
    //		order.scalePriceIncrement != UNSET_DOUBLE) {
    //		m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //			"  It does not support Scale orders.");
    //		return;
    //	}
    //}
    //
    //if( m_serverVersion < MIN_SERVER_VER_SSHORT_COMBO_LEGS) {
    //	if( contract.comboLegs && !contract.comboLegs->empty()) {
    //		typedef Contract::ComboLegList ComboLegList;
    //		const ComboLegList& comboLegs = *contract.comboLegs;
    //		ComboLegList::const_iterator iter = comboLegs.begin();
    //		const ComboLegList::const_iterator iterEnd = comboLegs.end();
    //		for( ; iter != iterEnd; ++iter) {
    //			const ComboLeg* comboLeg = *iter;
    //			assert( comboLeg);
    //			if( comboLeg->shortSaleSlot != 0 ||
    //				!comboLeg->designatedLocation.IsEmpty()) {
    //				m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //					"  It does not support SSHORT flag for combo legs.");
    //				return;
    //			}
    //		}
    //	}
    //}
    //
    //if( m_serverVersion < MIN_SERVER_VER_WHAT_IF_ORDERS) {
    //	if( order.whatIf) {
    //		m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
    //			"  It does not support what-if orders.");
    //		return;
    //	}
    //}

    if( m_serverVersion < MIN_SERVER_VER_DELTA_NEUTRAL) {
        if( contract.deltaNeutralContract) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support delta-neutral orders.");
            return;
        }
    }

    if( m_serverVersion < MIN_SERVER_VER_SCALE_ORDERS2) {
        if( order.scaleSubsLevelSize != UNSET_INTEGER) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support Subsequent Level Size for Scale orders.");
            return;
        }
    }

    if( m_serverVersion < MIN_SERVER_VER_ALGO_ORDERS) {

        if( !order.algoStrategy.empty()) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support algo orders.");
            return;
        }
    }

    if( m_serverVersion < MIN_SERVER_VER_NOT_HELD) {
        if (order.notHeld) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support notHeld parameter.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_SEC_ID_TYPE) {
        if( !contract.secIdType.empty() || !contract.secId.empty()) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support secIdType and secId parameters.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_PLACE_ORDER_CONID) {
        if( contract.conId > 0) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support conId parameter.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_SSHORTX) {
        if( order.exemptCode != -1) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support exemptCode parameter.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_SSHORTX) {
        const Contract::ComboLegList* const comboLegs = contract.comboLegs.get();
        const int comboLegsCount = comboLegs ? comboLegs->size() : 0;
        for( int i = 0; i < comboLegsCount; ++i) {
            const ComboLeg* comboLeg = ((*comboLegs)[i]).get();
            assert( comboLeg);
            if( comboLeg->exemptCode != -1 ){
                m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                    "  It does not support exemptCode parameter.");
                return;
            }
        }
    }

    if( m_serverVersion < MIN_SERVER_VER_HEDGE_ORDERS) {
        if( !order.hedgeType.empty()) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support hedge orders.");
            return;
        }
    }

    if( m_serverVersion < MIN_SERVER_VER_OPT_OUT_SMART_ROUTING) {
        if (order.optOutSmartRouting) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support optOutSmartRouting parameter.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_DELTA_NEUTRAL_CONID) {
        if (order.deltaNeutralConId > 0 
            || !order.deltaNeutralSettlingFirm.empty()
            || !order.deltaNeutralClearingAccount.empty()
            || !order.deltaNeutralClearingIntent.empty()
            ) {
                m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                    "  It does not support deltaNeutral parameters: ConId, SettlingFirm, ClearingAccount, ClearingIntent.");
                return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_DELTA_NEUTRAL_OPEN_CLOSE) {
        if (!order.deltaNeutralOpenClose.empty()
            || order.deltaNeutralShortSale
            || order.deltaNeutralShortSaleSlot > 0 
            || !order.deltaNeutralDesignatedLocation.empty()
            ) {
                m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() + 
                    "  It does not support deltaNeutral parameters: OpenClose, ShortSale, ShortSaleSlot, DesignatedLocation.");
                return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_SCALE_ORDERS3) {
        if (order.scalePriceIncrement > 0 && order.scalePriceIncrement != UNSET_DOUBLE) {
            if (order.scalePriceAdjustValue != UNSET_DOUBLE 
                || order.scalePriceAdjustInterval != UNSET_INTEGER 
                || order.scaleProfitOffset != UNSET_DOUBLE 
                || order.scaleAutoReset 
                || order.scaleInitPosition != UNSET_INTEGER 
                || order.scaleInitFillQty != UNSET_INTEGER 
                || order.scaleRandomPercent) {
                    m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                        "  It does not support Scale order parameters: PriceAdjustValue, PriceAdjustInterval, " +
                        "ProfitOffset, AutoReset, InitPosition, InitFillQty and RandomPercent");
                    return;
            }
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_ORDER_COMBO_LEGS_PRICE && contract.secType == "BAG") {
        const Order::OrderComboLegList* const orderComboLegs = order.orderComboLegs.get();
        const int orderComboLegsCount = orderComboLegs ? orderComboLegs->size() : 0;
        for( int i = 0; i < orderComboLegsCount; ++i) {
            const OrderComboLeg* orderComboLeg = ((*orderComboLegs)[i]).get();
            assert( orderComboLeg);
            if( orderComboLeg->price != UNSET_DOUBLE) {
                m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                    "  It does not support per-leg prices for order combo legs.");
                return;
            }
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_TRAILING_PERCENT) {
        if (order.trailingPercent != UNSET_DOUBLE) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support trailing percent parameter");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
        if( !contract.tradingClass.empty()) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support tradingClass parameter in placeOrder.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_SCALE_TABLE) {
        if( !order.scaleTable.empty() || !order.activeStartTime.empty() || !order.activeStopTime.empty()) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support scaleTable, activeStartTime and activeStopTime parameters");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_ALGO_ID) {
        if( !order.algoId.empty()) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support algoId parameter");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_ORDER_SOLICITED) {
        if (order.solicited) {
            m_pEWrapper->error(id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support order solicited parameter.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_MODELS_SUPPORT) {
        if( !order.modelCode.empty()) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support model code parameter.");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_EXT_OPERATOR) {
        if( !order.extOperator.empty()) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support ext operator parameter");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_SOFT_DOLLAR_TIER) 
    {
        if (!order.softDollarTier.name().empty() || !order.softDollarTier.val().empty())
        {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                " It does not support soft dollar tier");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_CASH_QTY) {
        if (order.cashQty != UNSET_DOUBLE) {
            m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support cash quantity parameter");
            return;
        }
    }

    if (m_serverVersion < MIN_SERVER_VER_DECISION_MAKER
        && (!order.mifid2DecisionMaker.empty()
        || !order.mifid2DecisionAlgo.empty())) {
            m_pEWrapper->error(id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                " It does not support MIFID II decision maker parameters");
            return;
    }

    if (m_serverVersion < MIN_SERVER_VER_MIFID_EXECUTION
        && (!order.mifid2ExecutionTrader.empty()
        || !order.mifid2ExecutionAlgo.empty())) {
            m_pEWrapper->error(id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                " It does not support MIFID II execution parameters");
            return;
    }

    if (m_serverVersion < MIN_SERVER_VER_AUTO_PRICE_FOR_HEDGE
        && order.dontUseAutoPriceForHedge) {
            m_pEWrapper->error(id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                " It does not support don't use auto price for hedge parameter");
            return;
    }

    if (m_serverVersion < MIN_SERVER_VER_ORDER_CONTAINER 
        && order.isOmsContainer) {
            m_pEWrapper->error(id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                " It does not support oms container parameter");
            return;
    }

    if (m_serverVersion < MIN_SERVER_VER_D_PEG_ORDERS 
        && order.discretionaryUpToLimitPrice) {
            m_pEWrapper->error(id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                " It does not support D-Peg orders");
            return;
    }

    if (m_serverVersion < MIN_SERVER_VER_PRICE_MGMT_ALGO
        && order.usePriceMgmtAlgo != UsePriceMmgtAlgo::DEFAULT) {
            m_pEWrapper->error(id, UPDATE_TWS.code(), UPDATE_TWS.msg() + " It does not support Use Price Management Algo requests");

            return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    int VERSION = (m_serverVersion < MIN_SERVER_VER_NOT_HELD) ? 27 : 45;

    // send place order msg
    ENCODE_FIELD( PLACE_ORDER);

    if (m_serverVersion < MIN_SERVER_VER_ORDER_CONTAINER) {
        ENCODE_FIELD( VERSION);
    }

    ENCODE_FIELD( id);

    // send contract fields
    if( m_serverVersion >= MIN_SERVER_VER_PLACE_ORDER_CONID) {
        ENCODE_FIELD( contract.conId);
    }
    ENCODE_FIELD( contract.symbol);
    ENCODE_FIELD( contract.secType);
    ENCODE_FIELD( contract.lastTradeDateOrContractMonth);
    ENCODE_FIELD( contract.strike);
    ENCODE_FIELD( contract.right);
    ENCODE_FIELD( contract.multiplier); // srv v15 and above
    ENCODE_FIELD( contract.exchange);
    ENCODE_FIELD( contract.primaryExchange); // srv v14 and above
    ENCODE_FIELD( contract.currency);
    ENCODE_FIELD( contract.localSymbol); // srv v2 and above
    if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.tradingClass);
    }

    if( m_serverVersion >= MIN_SERVER_VER_SEC_ID_TYPE){
        ENCODE_FIELD( contract.secIdType);
        ENCODE_FIELD( contract.secId);
    }

    // send main order fields
    ENCODE_FIELD( order.action);

    if (m_serverVersion >= MIN_SERVER_VER_FRACTIONAL_POSITIONS)
        ENCODE_FIELD(order.totalQuantity)
    else
    ENCODE_FIELD((long)order.totalQuantity)

    ENCODE_FIELD( order.orderType);
    if( m_serverVersion < MIN_SERVER_VER_ORDER_COMBO_LEGS_PRICE) {
        ENCODE_FIELD( order.lmtPrice == UNSET_DOUBLE ? 0 : order.lmtPrice);
    }
    else {
        ENCODE_FIELD_MAX( order.lmtPrice);
    }
    if( m_serverVersion < MIN_SERVER_VER_TRAILING_PERCENT) {
        ENCODE_FIELD( order.auxPrice == UNSET_DOUBLE ? 0 : order.auxPrice);
    }
    else {
        ENCODE_FIELD_MAX( order.auxPrice);
    }

    // send extended order fields
    ENCODE_FIELD( order.tif);
    ENCODE_FIELD( order.ocaGroup);
    ENCODE_FIELD( order.account);
    ENCODE_FIELD( order.openClose);
    ENCODE_FIELD( order.origin);
    ENCODE_FIELD( order.orderRef);
    ENCODE_FIELD( order.transmit);
    ENCODE_FIELD( order.parentId); // srv v4 and above

    ENCODE_FIELD( order.blockOrder); // srv v5 and above
    ENCODE_FIELD( order.sweepToFill); // srv v5 and above
    ENCODE_FIELD( order.displaySize); // srv v5 and above
    ENCODE_FIELD( order.triggerMethod); // srv v5 and above

    //if( m_serverVersion < 38) {
    // will never happen
    //	ENCODE_FIELD(/* order.ignoreRth */ false);
    //}
    //else {
    ENCODE_FIELD( order.outsideRth); // srv v5 and above
    //}

    ENCODE_FIELD( order.hidden); // srv v7 and above

    // Send combo legs for BAG requests (srv v8 and above)
    if( contract.secType == "BAG")
    {
        const Contract::ComboLegList* const comboLegs = contract.comboLegs.get();
        const int comboLegsCount = comboLegs ? comboLegs->size() : 0;
        ENCODE_FIELD( comboLegsCount);
        if( comboLegsCount > 0) {
            for( int i = 0; i < comboLegsCount; ++i) {
                const ComboLeg* comboLeg = ((*comboLegs)[i]).get();
                assert( comboLeg);
                ENCODE_FIELD( comboLeg->conId);
                ENCODE_FIELD( comboLeg->ratio);
                ENCODE_FIELD( comboLeg->action);
                ENCODE_FIELD( comboLeg->exchange);
                ENCODE_FIELD( comboLeg->openClose);

                ENCODE_FIELD( comboLeg->shortSaleSlot); // srv v35 and above
                ENCODE_FIELD( comboLeg->designatedLocation); // srv v35 and above
                if (m_serverVersion >= MIN_SERVER_VER_SSHORTX_OLD) { 
                    ENCODE_FIELD( comboLeg->exemptCode);
                }
            }
        }
    }

    // Send order combo legs for BAG requests
    if( m_serverVersion >= MIN_SERVER_VER_ORDER_COMBO_LEGS_PRICE && contract.secType == "BAG")
    {
        const Order::OrderComboLegList* const orderComboLegs = order.orderComboLegs.get();
        const int orderComboLegsCount = orderComboLegs ? orderComboLegs->size() : 0;
        ENCODE_FIELD( orderComboLegsCount);
        if( orderComboLegsCount > 0) {
            for( int i = 0; i < orderComboLegsCount; ++i) {
                const OrderComboLeg* orderComboLeg = ((*orderComboLegs)[i]).get();
                assert( orderComboLeg);
                ENCODE_FIELD_MAX( orderComboLeg->price);
            }
        }
    }	

    if( m_serverVersion >= MIN_SERVER_VER_SMART_COMBO_ROUTING_PARAMS && contract.secType == "BAG") {
        const TagValueList* const smartComboRoutingParams = order.smartComboRoutingParams.get();
        const int smartComboRoutingParamsCount = smartComboRoutingParams ? smartComboRoutingParams->size() : 0;
        ENCODE_FIELD( smartComboRoutingParamsCount);
        if( smartComboRoutingParamsCount > 0) {
            for( int i = 0; i < smartComboRoutingParamsCount; ++i) {
                const TagValue* tagValue = ((*smartComboRoutingParams)[i]).get();
                ENCODE_FIELD( tagValue->tag);
                ENCODE_FIELD( tagValue->value);
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    // Send the shares allocation.
    //
    // This specifies the number of order shares allocated to each Financial
    // Advisor managed account. The format of the allocation string is as
    // follows:
    //			<account_code1>/<number_shares1>,<account_code2>/<number_shares2>,...N
    // E.g.
    //		To allocate 20 shares of a 100 share order to account 'U101' and the
    //      residual 80 to account 'U203' enter the following share allocation string:
    //          U101/20,U203/80
    /////////////////////////////////////////////////////////////////////////////
    {
        // send deprecated sharesAllocation field
        ENCODE_FIELD( ""); // srv v9 and above
    }

    ENCODE_FIELD( order.discretionaryAmt); // srv v10 and above
    ENCODE_FIELD( order.goodAfterTime); // srv v11 and above
    ENCODE_FIELD( order.goodTillDate); // srv v12 and above

    ENCODE_FIELD( order.faGroup); // srv v13 and above
    ENCODE_FIELD( order.faMethod); // srv v13 and above
    ENCODE_FIELD( order.faPercentage); // srv v13 and above
    ENCODE_FIELD( order.faProfile); // srv v13 and above

    if (m_serverVersion >= MIN_SERVER_VER_MODELS_SUPPORT) {
        ENCODE_FIELD( order.modelCode);
    }

    // institutional short saleslot data (srv v18 and above)
    ENCODE_FIELD( order.shortSaleSlot);      // 0 for retail, 1 or 2 for institutions
    ENCODE_FIELD( order.designatedLocation); // populate only when shortSaleSlot = 2.
    if (m_serverVersion >= MIN_SERVER_VER_SSHORTX_OLD) { 
        ENCODE_FIELD( order.exemptCode);
    }

    // not needed anymore
    //bool isVolOrder = (order.orderType.CompareNoCase("VOL") == 0);

    // srv v19 and above fields
    ENCODE_FIELD( order.ocaType);
    //if( m_serverVersion < 38) {
    // will never happen
    //	send( /* order.rthOnly */ false);
    //}
    ENCODE_FIELD( order.rule80A);
    ENCODE_FIELD( order.settlingFirm);
    ENCODE_FIELD( order.allOrNone);
    ENCODE_FIELD_MAX( order.minQty);
    ENCODE_FIELD_MAX( order.percentOffset);
    ENCODE_FIELD( order.eTradeOnly);
    ENCODE_FIELD( order.firmQuoteOnly);
    ENCODE_FIELD_MAX( order.nbboPriceCap);
    ENCODE_FIELD( order.auctionStrategy); // AUCTION_MATCH, AUCTION_IMPROVEMENT, AUCTION_TRANSPARENT
    ENCODE_FIELD_MAX( order.startingPrice);
    ENCODE_FIELD_MAX( order.stockRefPrice);
    ENCODE_FIELD_MAX( order.delta);
    // Volatility orders had specific watermark price attribs in server version 26
    //double lower = (m_serverVersion == 26 && isVolOrder) ? DBL_MAX : order.stockRangeLower;
    //double upper = (m_serverVersion == 26 && isVolOrder) ? DBL_MAX : order.stockRangeUpper;
    ENCODE_FIELD_MAX( order.stockRangeLower);
    ENCODE_FIELD_MAX( order.stockRangeUpper);

    ENCODE_FIELD( order.overridePercentageConstraints); // srv v22 and above

    // Volatility orders (srv v26 and above)
    ENCODE_FIELD_MAX( order.volatility);
    ENCODE_FIELD_MAX( order.volatilityType);
    // will never happen
    //if( m_serverVersion < 28) {
    //	send( order.deltaNeutralOrderType.CompareNoCase("MKT") == 0);
    //}
    //else {
    ENCODE_FIELD( order.deltaNeutralOrderType); // srv v28 and above
    ENCODE_FIELD_MAX( order.deltaNeutralAuxPrice); // srv v28 and above

    if (m_serverVersion >= MIN_SERVER_VER_DELTA_NEUTRAL_CONID && !order.deltaNeutralOrderType.empty()){
        ENCODE_FIELD( order.deltaNeutralConId);
        ENCODE_FIELD( order.deltaNeutralSettlingFirm);
        ENCODE_FIELD( order.deltaNeutralClearingAccount);
        ENCODE_FIELD( order.deltaNeutralClearingIntent);
    }

    if (m_serverVersion >= MIN_SERVER_VER_DELTA_NEUTRAL_OPEN_CLOSE && !order.deltaNeutralOrderType.empty()){
        ENCODE_FIELD( order.deltaNeutralOpenClose);
        ENCODE_FIELD( order.deltaNeutralShortSale);
        ENCODE_FIELD( order.deltaNeutralShortSaleSlot);
        ENCODE_FIELD( order.deltaNeutralDesignatedLocation);
    }

    //}
    ENCODE_FIELD( order.continuousUpdate);
    //if( m_serverVersion == 26) {
    //	// Volatility orders had specific watermark price attribs in server version 26
    //	double lower = (isVolOrder ? order.stockRangeLower : DBL_MAX);
    //	double upper = (isVolOrder ? order.stockRangeUpper : DBL_MAX);
    //	ENCODE_FIELD_MAX( lower);
    //	ENCODE_FIELD_MAX( upper);
    //}
    ENCODE_FIELD_MAX( order.referencePriceType);

    ENCODE_FIELD_MAX( order.trailStopPrice); // srv v30 and above

    if( m_serverVersion >= MIN_SERVER_VER_TRAILING_PERCENT) {
        ENCODE_FIELD_MAX( order.trailingPercent);
    }

    // SCALE orders
    if( m_serverVersion >= MIN_SERVER_VER_SCALE_ORDERS2) {
        ENCODE_FIELD_MAX( order.scaleInitLevelSize);
        ENCODE_FIELD_MAX( order.scaleSubsLevelSize);
    }
    else {
        // srv v35 and above)
        ENCODE_FIELD( ""); // for not supported scaleNumComponents
        ENCODE_FIELD_MAX( order.scaleInitLevelSize); // for scaleComponentSize
    }

    ENCODE_FIELD_MAX( order.scalePriceIncrement);

    if( m_serverVersion >= MIN_SERVER_VER_SCALE_ORDERS3 
        && order.scalePriceIncrement > 0.0 && order.scalePriceIncrement != UNSET_DOUBLE) {
            ENCODE_FIELD_MAX( order.scalePriceAdjustValue);
            ENCODE_FIELD_MAX( order.scalePriceAdjustInterval);
            ENCODE_FIELD_MAX( order.scaleProfitOffset);
            ENCODE_FIELD( order.scaleAutoReset);
            ENCODE_FIELD_MAX( order.scaleInitPosition);
            ENCODE_FIELD_MAX( order.scaleInitFillQty);
            ENCODE_FIELD( order.scaleRandomPercent);
    }

    if( m_serverVersion >= MIN_SERVER_VER_SCALE_TABLE) {
        ENCODE_FIELD( order.scaleTable);
        ENCODE_FIELD( order.activeStartTime);
        ENCODE_FIELD( order.activeStopTime);
    }

    // HEDGE orders
    if( m_serverVersion >= MIN_SERVER_VER_HEDGE_ORDERS) {
        ENCODE_FIELD( order.hedgeType);
        if ( !order.hedgeType.empty()) {
            ENCODE_FIELD( order.hedgeParam);
        }
    }

    if( m_serverVersion >= MIN_SERVER_VER_OPT_OUT_SMART_ROUTING){
        ENCODE_FIELD( order.optOutSmartRouting);
    }

    if( m_serverVersion >= MIN_SERVER_VER_PTA_ORDERS) {
        ENCODE_FIELD( order.clearingAccount);
        ENCODE_FIELD( order.clearingIntent);
    }

    if( m_serverVersion >= MIN_SERVER_VER_NOT_HELD){
        ENCODE_FIELD( order.notHeld);
    }

    if( m_serverVersion >= MIN_SERVER_VER_DELTA_NEUTRAL) {
        if( contract.deltaNeutralContract) {
            const DeltaNeutralContract& deltaNeutralContract = *contract.deltaNeutralContract;
            ENCODE_FIELD( true);
            ENCODE_FIELD( deltaNeutralContract.conId);
            ENCODE_FIELD( deltaNeutralContract.delta);
            ENCODE_FIELD( deltaNeutralContract.price);
        }
        else {
            ENCODE_FIELD( false);
        }
    }

    if( m_serverVersion >= MIN_SERVER_VER_ALGO_ORDERS) {
        ENCODE_FIELD( order.algoStrategy);

        if( !order.algoStrategy.empty()) {
            const TagValueList* const algoParams = order.algoParams.get();
            const int algoParamsCount = algoParams ? algoParams->size() : 0;
            ENCODE_FIELD( algoParamsCount);
            if( algoParamsCount > 0) {
                for( int i = 0; i < algoParamsCount; ++i) {
                    const TagValue* tagValue = ((*algoParams)[i]).get();
                    ENCODE_FIELD( tagValue->tag);
                    ENCODE_FIELD( tagValue->value);
                }
            }
        }

    }

    if( m_serverVersion >= MIN_SERVER_VER_ALGO_ID) {
        ENCODE_FIELD( order.algoId);
    }

    ENCODE_FIELD( order.whatIf); // srv v36 and above

    // send miscOptions parameter
    if (m_serverVersion >= MIN_SERVER_VER_LINKING) {
        ENCODE_TAGVALUELIST(order.orderMiscOptions);
    }

    if (m_serverVersion >= MIN_SERVER_VER_ORDER_SOLICITED) {
        ENCODE_FIELD(order.solicited);
    }

    if (m_serverVersion >= MIN_SERVER_VER_RANDOMIZE_SIZE_AND_PRICE) {
        ENCODE_FIELD(order.randomizeSize);
        ENCODE_FIELD(order.randomizePrice);
    }

    if (m_serverVersion >= MIN_SERVER_VER_PEGGED_TO_BENCHMARK) {
        if (order.orderType == "PEG BENCH") {
            ENCODE_FIELD(order.referenceContractId);
            ENCODE_FIELD(order.isPeggedChangeAmountDecrease);
            ENCODE_FIELD(order.peggedChangeAmount);
            ENCODE_FIELD(order.referenceChangeAmount);
            ENCODE_FIELD(order.referenceExchangeId);
        }

        ENCODE_FIELD(order.conditions.size());

        if (order.conditions.size() > 0) {
            for (std::shared_ptr<OrderCondition> item : order.conditions) {
                ENCODE_FIELD(item->type());
                item->writeExternal(msg);
            }

            ENCODE_FIELD(order.conditionsIgnoreRth);
            ENCODE_FIELD(order.conditionsCancelOrder);
        }

        ENCODE_FIELD(order.adjustedOrderType);
        ENCODE_FIELD(order.triggerPrice);
        ENCODE_FIELD(order.lmtPriceOffset);
        ENCODE_FIELD(order.adjustedStopPrice);
        ENCODE_FIELD(order.adjustedStopLimitPrice);
        ENCODE_FIELD(order.adjustedTrailingAmount);
        ENCODE_FIELD(order.adjustableTrailingUnit);
    }

    if( m_serverVersion >= MIN_SERVER_VER_EXT_OPERATOR) {
        ENCODE_FIELD( order.extOperator);
    }

    if (m_serverVersion >= MIN_SERVER_VER_SOFT_DOLLAR_TIER) {
        ENCODE_FIELD(order.softDollarTier.name());
        ENCODE_FIELD(order.softDollarTier.val());
    }

    if (m_serverVersion >= MIN_SERVER_VER_CASH_QTY) {
        ENCODE_FIELD_MAX( order.cashQty);
    }

    if (m_serverVersion >= MIN_SERVER_VER_DECISION_MAKER) {
        ENCODE_FIELD(order.mifid2DecisionMaker);
        ENCODE_FIELD(order.mifid2DecisionAlgo);
    }

    if (m_serverVersion >= MIN_SERVER_VER_MIFID_EXECUTION) {
        ENCODE_FIELD(order.mifid2ExecutionTrader);
        ENCODE_FIELD(order.mifid2ExecutionAlgo);
    }

    if (m_serverVersion >= MIN_SERVER_VER_AUTO_PRICE_FOR_HEDGE) {
        ENCODE_FIELD(order.dontUseAutoPriceForHedge);
    }

    if (m_serverVersion >= MIN_SERVER_VER_ORDER_CONTAINER) {
        ENCODE_FIELD(order.isOmsContainer);
    }

    if (m_serverVersion >= MIN_SERVER_VER_D_PEG_ORDERS) {
        ENCODE_FIELD(order.discretionaryUpToLimitPrice);
    }

    if (m_serverVersion >= MIN_SERVER_VER_PRICE_MGMT_ALGO) {
        ENCODE_FIELD_MAX(order.usePriceMgmtAlgo);
    }

    closeAndSend( msg.str());
}

void EClient::cancelOrder( OrderId id)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( id, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    const int VERSION = 1;

    // send cancel order msg
    std::stringstream msg;
    prepareBuffer( msg);

    ENCODE_FIELD( CANCEL_ORDER);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( id);

    closeAndSend( msg.str());
}

void EClient::reqAccountUpdates(bool subscribe, const std::string& acctCode)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 2;

    // send req acct msg
    ENCODE_FIELD( REQ_ACCT_DATA);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( subscribe);  // TRUE = subscribe, FALSE = unsubscribe.

    // Send the account code. This will only be used for FA clients
    ENCODE_FIELD( acctCode); // srv v9 and above

    closeAndSend( msg.str());
}

void EClient::reqOpenOrders()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send req open orders msg
    ENCODE_FIELD( REQ_OPEN_ORDERS);
    ENCODE_FIELD( VERSION);

    closeAndSend( msg.str());
}

void EClient::reqAutoOpenOrders(bool bAutoBind)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send req open orders msg
    ENCODE_FIELD( REQ_AUTO_OPEN_ORDERS);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( bAutoBind);

    closeAndSend( msg.str());
}

void EClient::reqAllOpenOrders()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send req open orders msg
    ENCODE_FIELD( REQ_ALL_OPEN_ORDERS);
    ENCODE_FIELD( VERSION);

    closeAndSend( msg.str());
}

void EClient::reqExecutions(int reqId, const ExecutionFilter& filter)
{
    //NOTE: Time format must be 'yyyymmdd-hh:mm:ss' E.g. '20030702-14:55'

    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 3;

    // send req open orders msg
    ENCODE_FIELD( REQ_EXECUTIONS);
    ENCODE_FIELD( VERSION);

    if( m_serverVersion >= MIN_SERVER_VER_EXECUTION_DATA_CHAIN) {
        ENCODE_FIELD( reqId);
    }

    // Send the execution rpt filter data (srv v9 and above)
    ENCODE_FIELD( filter.m_clientId);
    ENCODE_FIELD( filter.m_acctCode);
    ENCODE_FIELD( filter.m_time);
    ENCODE_FIELD( filter.m_symbol);
    ENCODE_FIELD( filter.m_secType);
    ENCODE_FIELD( filter.m_exchange);
    ENCODE_FIELD( filter.m_side);

    closeAndSend( msg.str());
}

void EClient::reqIds( int numIds)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( numIds, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send req open orders msg
    ENCODE_FIELD( REQ_IDS);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( numIds);

    closeAndSend( msg.str());
}

void EClient::reqNewsBulletins(bool allMsgs)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send req news bulletins msg
    ENCODE_FIELD( REQ_NEWS_BULLETINS);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( allMsgs);

    closeAndSend( msg.str());
}

void EClient::cancelNewsBulletins()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send req news bulletins msg
    ENCODE_FIELD( CANCEL_NEWS_BULLETINS);
    ENCODE_FIELD( VERSION);

    closeAndSend( msg.str());
}

void EClient::setServerLogLevel(int logLevel)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send the set server logging level message
    ENCODE_FIELD( SET_SERVER_LOGLEVEL);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( logLevel);

    closeAndSend( msg.str());
}

void EClient::reqManagedAccts()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send req FA managed accounts msg
    ENCODE_FIELD( REQ_MANAGED_ACCTS);
    ENCODE_FIELD( VERSION);

    closeAndSend( msg.str());
}


void EClient::requestFA(faDataType pFaDataType)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < 13) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
    //	return;
    //}

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( REQ_FA);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( (int)pFaDataType);

    closeAndSend( msg.str());
}

void EClient::replaceFA(faDataType pFaDataType, const std::string& cxml)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < 13) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
    //	return;
    //}

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( REPLACE_FA);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( (int)pFaDataType);
    ENCODE_FIELD( cxml);

    closeAndSend( msg.str());
}



void EClient::exerciseOptions( TickerId tickerId, const Contract& contract,
                              int exerciseAction, int exerciseQuantity,
                              const std::string& account, int override)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    // Not needed anymore validation
    //if( m_serverVersion < 21) {
    //	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
    //	return;
    //}

    if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
        if( !contract.tradingClass.empty() || (contract.conId > 0)) {
            m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support conId, multiplier and tradingClass parameters in exerciseOptions.");
            return;
        }
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 2;

    ENCODE_FIELD( EXERCISE_OPTIONS);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( tickerId);

    // send contract fields
    if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.conId);
    }
    ENCODE_FIELD( contract.symbol);
    ENCODE_FIELD( contract.secType);
    ENCODE_FIELD( contract.lastTradeDateOrContractMonth);
    ENCODE_FIELD( contract.strike);
    ENCODE_FIELD( contract.right);
    ENCODE_FIELD( contract.multiplier);
    ENCODE_FIELD( contract.exchange);
    ENCODE_FIELD( contract.currency);
    ENCODE_FIELD( contract.localSymbol);
    if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
        ENCODE_FIELD( contract.tradingClass);
    }
    ENCODE_FIELD( exerciseAction);
    ENCODE_FIELD( exerciseQuantity);
    ENCODE_FIELD( account);
    ENCODE_FIELD( override);

    closeAndSend( msg.str());
}

void EClient::reqGlobalCancel()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if (m_serverVersion < MIN_SERVER_VER_REQ_GLOBAL_CANCEL) {
        m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support globalCancel requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    // send current time req
    ENCODE_FIELD( REQ_GLOBAL_CANCEL);
    ENCODE_FIELD( VERSION);

    closeAndSend( msg.str());
}

void EClient::reqMarketDataType( int marketDataType)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_MARKET_DATA_TYPE) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support market data type requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( REQ_MARKET_DATA_TYPE);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( marketDataType);

    closeAndSend( msg.str());
}

void EClient::reqPositions()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_POSITIONS) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support positions request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( REQ_POSITIONS);
    ENCODE_FIELD( VERSION);

    closeAndSend( msg.str());
}

void EClient::cancelPositions()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_POSITIONS) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support positions cancellation.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_POSITIONS);
    ENCODE_FIELD( VERSION);

    closeAndSend( msg.str());
}

void EClient::reqAccountSummary( int reqId, const std::string& groupName, const std::string& tags)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_ACCOUNT_SUMMARY) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support account summary request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( REQ_ACCOUNT_SUMMARY);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);
    ENCODE_FIELD( groupName);
    ENCODE_FIELD( tags);

    closeAndSend( msg.str());
}

void EClient::cancelAccountSummary( int reqId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_ACCOUNT_SUMMARY) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support account summary cancellation.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_ACCOUNT_SUMMARY);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);

    closeAndSend( msg.str());
}

void EClient::verifyRequest(const std::string& apiName, const std::string& apiVersion)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_LINKING) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support verification request.");
        return;
    }

    if( !m_extraAuth) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  Intent to authenticate needs to be expressed during initial connect request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( VERIFY_REQUEST);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( apiName);
    ENCODE_FIELD( apiVersion);

    closeAndSend( msg.str());
}

void EClient::verifyMessage(const std::string& apiData)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_LINKING) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support verification message sending.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( VERIFY_MESSAGE);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( apiData);

    closeAndSend( msg.str());
}

void EClient::verifyAndAuthRequest(const std::string& apiName, const std::string& apiVersion, const std::string& opaqueIsvKey)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_LINKING_AUTH) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support verification request.");
        return;
    }

    if( !m_extraAuth) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  Intent to authenticate needs to be expressed during initial connect request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( VERIFY_AND_AUTH_REQUEST);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( apiName);
    ENCODE_FIELD( apiVersion);
    ENCODE_FIELD( opaqueIsvKey);

    closeAndSend( msg.str());
}

void EClient::verifyAndAuthMessage(const std::string& apiData, const std::string& xyzResponse)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_LINKING_AUTH) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support verification message sending.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( VERIFY_AND_AUTH_MESSAGE);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( apiData);
    ENCODE_FIELD( xyzResponse);

    closeAndSend( msg.str());
}

void EClient::queryDisplayGroups( int reqId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_LINKING) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support queryDisplayGroups request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( QUERY_DISPLAY_GROUPS);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);

    closeAndSend( msg.str());
}

void EClient::subscribeToGroupEvents( int reqId, int groupId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_LINKING) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support subscribeToGroupEvents request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( SUBSCRIBE_TO_GROUP_EVENTS);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);
    ENCODE_FIELD( groupId);

    closeAndSend( msg.str());
}

void EClient::updateDisplayGroup( int reqId, const std::string& contractInfo)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_LINKING) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support updateDisplayGroup request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( UPDATE_DISPLAY_GROUP);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);
    ENCODE_FIELD( contractInfo);

    closeAndSend( msg.str());
}

void EClient::startApi()
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion >= 3) {
        if( m_serverVersion < MIN_SERVER_VER_LINKING) {
            std::stringstream msg;
            ENCODE_FIELD( m_clientId);
            bufferedSend( msg.str());
        }
        else
        {
            std::stringstream msg;
            prepareBuffer( msg);

            const int VERSION = 2;

            ENCODE_FIELD( START_API);
            ENCODE_FIELD( VERSION);
            ENCODE_FIELD( m_clientId);

            if (m_serverVersion >= MIN_SERVER_VER_OPTIONAL_CAPABILITIES)
                ENCODE_FIELD(m_optionalCapabilities);

            closeAndSend( msg.str());
        }
    }
}

void EClient::unsubscribeFromGroupEvents( int reqId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_LINKING) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support unsubscribeFromGroupEvents request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( UNSUBSCRIBE_FROM_GROUP_EVENTS);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);

    closeAndSend( msg.str());
}

void EClient::reqPositionsMulti( int reqId, const std::string& account, const std::string& modelCode)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_MODELS_SUPPORT) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support positions multi request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( REQ_POSITIONS_MULTI);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);
    ENCODE_FIELD( account);
    ENCODE_FIELD( modelCode);

    closeAndSend( msg.str());
}

void EClient::cancelPositionsMulti( int reqId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_MODELS_SUPPORT) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support positions multi cancellation.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_POSITIONS_MULTI);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);

    closeAndSend( msg.str());
}

void EClient::reqAccountUpdatesMulti( int reqId, const std::string& account, const std::string& modelCode, bool ledgerAndNLV)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_MODELS_SUPPORT) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support account updates multi request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( REQ_ACCOUNT_UPDATES_MULTI);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);
    ENCODE_FIELD( account);
    ENCODE_FIELD( modelCode);
    ENCODE_FIELD( ledgerAndNLV);

    closeAndSend( msg.str());
}

void EClient::cancelAccountUpdatesMulti( int reqId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_MODELS_SUPPORT) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support account updates multi cancellation.");
        return;
    }

    std::stringstream msg;
    prepareBuffer( msg);

    const int VERSION = 1;

    ENCODE_FIELD( CANCEL_ACCOUNT_UPDATES_MULTI);
    ENCODE_FIELD( VERSION);
    ENCODE_FIELD( reqId);

    closeAndSend( msg.str());
}

void EClient::reqSecDefOptParams(int reqId, const std::string& underlyingSymbol, const std::string& futFopExchange, const std::string& underlyingSecType, int underlyingConId)
{
    // not connected?
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_SEC_DEF_OPT_PARAMS_REQ) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support security definiton option requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);


    ENCODE_FIELD(REQ_SEC_DEF_OPT_PARAMS);
    ENCODE_FIELD(reqId);
    ENCODE_FIELD(underlyingSymbol); 
    ENCODE_FIELD(futFopExchange);
    ENCODE_FIELD(underlyingSecType);
    ENCODE_FIELD(underlyingConId);

    closeAndSend(msg.str());
}

void EClient::reqSoftDollarTiers(int reqId)
{
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);


    ENCODE_FIELD(REQ_SOFT_DOLLAR_TIERS);
    ENCODE_FIELD(reqId);

    closeAndSend(msg.str());
}

void EClient::reqFamilyCodes()
{
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_FAMILY_CODES) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support family codes requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_FAMILY_CODES);

    closeAndSend(msg.str());
}

void EClient::reqMatchingSymbols(int reqId, const std::string& pattern)
{
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_MATCHING_SYMBOLS) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support matching symbols requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_MATCHING_SYMBOLS);
    ENCODE_FIELD(reqId);
    ENCODE_FIELD(pattern);

    closeAndSend(msg.str());
}

void EClient::reqMktDepthExchanges()
{
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_MKT_DEPTH_EXCHANGES) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support market depth exchanges requests.");
        return;
    }


    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_MKT_DEPTH_EXCHANGES);

    closeAndSend(msg.str());
}

void EClient::reqSmartComponents(int reqId, std::string bboExchange) 
{
    if (!isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if (m_serverVersion < MIN_SERVER_VER_REQ_SMART_COMPONENTS) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support smart components request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_SMART_COMPONENTS);
    ENCODE_FIELD(reqId);
    ENCODE_FIELD(bboExchange);

    closeAndSend(msg.str());
}

void EClient::reqNewsProviders()
{
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_NEWS_PROVIDERS) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support news providers requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_NEWS_PROVIDERS);

    closeAndSend(msg.str());
}

void EClient::reqNewsArticle(int requestId, const std::string& providerCode, const std::string& articleId, const TagValueListSPtr& newsArticleOptions)
{
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_NEWS_ARTICLE) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support news article requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_NEWS_ARTICLE);
    ENCODE_FIELD(requestId);
    ENCODE_FIELD(providerCode);
    ENCODE_FIELD(articleId);


    // send newsArticleOptions parameter
    if( m_serverVersion >= MIN_SERVER_VER_NEWS_QUERY_ORIGINS) {
        ENCODE_TAGVALUELIST(newsArticleOptions);
    }

    closeAndSend(msg.str());
}

void EClient::reqHistoricalNews(int requestId, int conId, const std::string& providerCodes, const std::string& startDateTime, const std::string& endDateTime, int totalResults,
                                const TagValueListSPtr& historicalNewsOptions)
{
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_HISTORICAL_NEWS) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support historical news requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_HISTORICAL_NEWS);
    ENCODE_FIELD(requestId);
    ENCODE_FIELD(conId);
    ENCODE_FIELD(providerCodes);
    ENCODE_FIELD(startDateTime);
    ENCODE_FIELD(endDateTime);
    ENCODE_FIELD(totalResults);

    // send historicalNewsOptions parameter
    if( m_serverVersion >= MIN_SERVER_VER_NEWS_QUERY_ORIGINS) {
        ENCODE_TAGVALUELIST(historicalNewsOptions);
    }

    closeAndSend(msg.str());
}

void EClient::reqHeadTimestamp(int tickerId, const Contract &contract, const std::string& whatToShow, int useRTH, int formatDate)
{
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_HEAD_TIMESTAMP) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support head timestamp requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_HEAD_TIMESTAMP);
    ENCODE_FIELD(tickerId);
    ENCODE_FIELD(contract.conId);
    ENCODE_FIELD(contract.symbol);
    ENCODE_FIELD(contract.secType);
    ENCODE_FIELD(contract.lastTradeDateOrContractMonth);
    ENCODE_FIELD(contract.strike);
    ENCODE_FIELD(contract.right);
    ENCODE_FIELD(contract.multiplier);
    ENCODE_FIELD(contract.exchange);
    ENCODE_FIELD(contract.primaryExchange);
    ENCODE_FIELD(contract.currency);
    ENCODE_FIELD(contract.localSymbol);
    ENCODE_FIELD(contract.tradingClass);
    ENCODE_FIELD(contract.includeExpired);
    ENCODE_FIELD(useRTH);
    ENCODE_FIELD(whatToShow);          
    ENCODE_FIELD(formatDate);

    closeAndSend(msg.str());
}

void EClient::cancelHeadTimestamp(int tickerId) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_CANCEL_HEADTIMESTAMP) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support head timestamp requests canceling.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(CANCEL_HEAD_TIMESTAMP);
    ENCODE_FIELD(tickerId);

    closeAndSend(msg.str());
}

void EClient::reqHistogramData(int reqId, const Contract &contract, bool useRTH, const std::string& timePeriod) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_HISTOGRAM) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support histogram requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_HISTOGRAM_DATA);
    ENCODE_FIELD(reqId);
    ENCODE_CONTRACT(contract);
    ENCODE_FIELD(useRTH);
    ENCODE_FIELD(timePeriod);          

    closeAndSend(msg.str());
}

void EClient::cancelHistogramData(int reqId) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_REQ_HEAD_TIMESTAMP) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support histogram requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(CANCEL_HISTOGRAM_DATA);
    ENCODE_FIELD(reqId);      

    closeAndSend(msg.str());
}

void EClient::reqMarketRule(int marketRuleId) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_MARKET_RULES) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support market rule requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_MARKET_RULE);
    ENCODE_FIELD(marketRuleId);

    closeAndSend(msg.str());
}

void EClient::reqPnL(int reqId, const std::string& account, const std::string& modelCode) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_PNL) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support PnL requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_PNL);
    ENCODE_FIELD(reqId);
    ENCODE_FIELD(account);
    ENCODE_FIELD(modelCode);

    closeAndSend(msg.str());
}

void EClient::cancelPnL(int reqId) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_PNL) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support PnL requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(CANCEL_PNL);
    ENCODE_FIELD(reqId);

    closeAndSend(msg.str());
}

void EClient::reqPnLSingle(int reqId, const std::string& account, const std::string& modelCode, int conId) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_PNL) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support PnL requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_PNL_SINGLE);
    ENCODE_FIELD(reqId);
    ENCODE_FIELD(account);
    ENCODE_FIELD(modelCode);
    ENCODE_FIELD(conId);

    closeAndSend(msg.str());
}

void EClient::cancelPnLSingle(int reqId) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_PNL) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support PnL requests.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(CANCEL_PNL_SINGLE);
    ENCODE_FIELD(reqId);

    closeAndSend(msg.str());
}

void EClient::reqHistoricalTicks(int reqId, const Contract &contract, const std::string& startDateTime,
                                 const std::string& endDateTime, int numberOfTicks, const std::string& whatToShow, int useRth, bool ignoreSize, const TagValueListSPtr& miscOptions) {
                                     if( !isConnected()) {
                                         m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
                                         return;
                                     }

                                     if( m_serverVersion < MIN_SERVER_VER_HISTORICAL_TICKS) {
                                         m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                                             "  It does not support historical ticks request request.");
                                         return;
                                     }

                                     std::stringstream msg;
                                     prepareBuffer(msg);

                                     ENCODE_FIELD(REQ_HISTORICAL_TICKS);
                                     ENCODE_FIELD(reqId);
                                     ENCODE_CONTRACT(contract);
                                     ENCODE_FIELD(startDateTime);
                                     ENCODE_FIELD(endDateTime);
                                     ENCODE_FIELD(numberOfTicks);
                                     ENCODE_FIELD(whatToShow);
                                     ENCODE_FIELD(useRth);
                                     ENCODE_FIELD(ignoreSize);
                                     ENCODE_TAGVALUELIST(miscOptions);

                                     closeAndSend(msg.str());    
}

void EClient::reqTickByTickData(int reqId, const Contract &contract, const std::string& tickType, int numberOfTicks, bool ignoreSize) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_TICK_BY_TICK) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support tick-by-tick data request.");
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_TICK_BY_TICK_IGNORE_SIZE) {
        if (numberOfTicks != 0 || ignoreSize) {
            m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support ignoreSize and numberOfTicks parameters in tick-by-tick data requests.");
            return;
        }
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_TICK_BY_TICK_DATA);
    ENCODE_FIELD(reqId);
    ENCODE_FIELD( contract.conId);
    ENCODE_FIELD( contract.symbol);
    ENCODE_FIELD( contract.secType);
    ENCODE_FIELD( contract.lastTradeDateOrContractMonth);
    ENCODE_FIELD( contract.strike);
    ENCODE_FIELD( contract.right);
    ENCODE_FIELD( contract.multiplier);
    ENCODE_FIELD( contract.exchange);
    ENCODE_FIELD( contract.primaryExchange);
    ENCODE_FIELD( contract.currency);
    ENCODE_FIELD( contract.localSymbol);
    ENCODE_FIELD( contract.tradingClass);
    ENCODE_FIELD( tickType);
    if( m_serverVersion >= MIN_SERVER_VER_TICK_BY_TICK_IGNORE_SIZE) {
        ENCODE_FIELD( numberOfTicks);
        ENCODE_FIELD( ignoreSize);
    }

    closeAndSend(msg.str());    
}

void EClient::cancelTickByTickData(int reqId) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_TICK_BY_TICK) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support tick-by-tick data cancel.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(CANCEL_TICK_BY_TICK_DATA);
    ENCODE_FIELD(reqId);

    closeAndSend(msg.str());    
}

void EClient::reqCompletedOrders(bool apiOnly) {
    if( !isConnected()) {
        m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
        return;
    }

    if( m_serverVersion < MIN_SERVER_VER_COMPLETED_ORDERS) {
        m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
            "  It does not support completed orders request.");
        return;
    }

    std::stringstream msg;
    prepareBuffer(msg);

    ENCODE_FIELD(REQ_COMPLETED_ORDERS);
    ENCODE_FIELD(apiOnly);

    closeAndSend(msg.str());    
}

bool EClient::extraAuth() {
    return m_extraAuth;
}

EWrapper * EClient::getWrapper() const
{
    return m_pEWrapper;
}

void EClient::setClientId( int clientId)
{
    m_clientId = clientId;
}

void EClient::setExtraAuth( bool extraAuth)
{
    m_extraAuth = extraAuth;
}

void EClient::setHost( const std::string& host)
{
    m_host = host;
}

void EClient::setPort( int port)
{
    m_port = port;
}


///////////////////////////////////////////////////////////
// callbacks from socket
int EClient::sendConnectRequest()
{
    m_connState = CS_CONNECTING;

    int rval;

    // send client version
    std::stringstream msg;
    if( m_useV100Plus) {
        msg.write( API_SIGN, sizeof(API_SIGN));
        prepareBufferImpl( msg);
        if( MIN_CLIENT_VER < MAX_CLIENT_VER) {
            msg << 'v' << MIN_CLIENT_VER << ".." << MAX_CLIENT_VER;
        }
        else {
            msg << 'v' << MIN_CLIENT_VER;
        }
        if( !m_connectOptions.empty()) {
            msg << ' ' << m_connectOptions;
        }

        rval = closeAndSend( msg.str(), sizeof(API_SIGN)) ? 1 : -1;
    }
    else {
        ENCODE_FIELD( CLIENT_VERSION);

        rval = bufferedSend( msg.str());
    }

    m_connState = rval > 0 ? CS_CONNECTED : CS_DISCONNECTED;

    return rval;
}

