#ifndef TechnicalAnalysis_h
#define TechnicalAnalysis_h

#include <vector>
#include "MarketData.h"

class TechnicalAnalysis
{
public:
    HistoricalData data, tempData;
    void setData(HistoricalData &);
    void setTempData(HistoricalData &);
    //EMAs
    void calcEMA(const int, std::vector<double> &);

    //SMAs
    void calcSMA(const int, std::vector<double> &);
};

class Order
{
public:
    std::string symbol, side, positionSide, type, timeInForce,
        newClientOrderId, workingType, newOrderRespType, recvWindow;
    double quantity, price, stopPrice, activationPrice, callbackRate;
    long timestamp;
    bool reduceOnly, closePosition, priceProtect;
};

#endif