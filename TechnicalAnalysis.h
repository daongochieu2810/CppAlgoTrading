#ifndef TechnicalAnalysis_h
#define TechnicalAnalysis_h

#include <iostream>
#include <numeric>
#include <vector>
#include "MarketData.h"

class TechnicalAnalysis
{
public:
    HistoricalData data, tempData;
    void setData(HistoricalData &);
    void setTempData(HistoricalData &);
    //Heikin-Ashi
    void setUpHeikinAshi(HistoricalData &);
    //MAs
    void calcEMA(const int, std::vector<double> &);
    void calcSMA(const int, std::vector<double> &);
    void calcBB(const int, std::vector<double> &);
    //Momentum
    void calcRSI(const int, std::vector<double> &);
    void calcStoch(const int, std::vector<double> &);
    //Trend
    void calcPSAR(std::vector<double> &);
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