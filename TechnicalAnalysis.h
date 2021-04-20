#ifndef TechnicalAnalysis_h
#define TechnicalAnalysis_h

#include <vector>
#include "MarketData.h"

class TechnicalAnalysis
{
public:
    HistoricalData data;
    void setData(HistoricalData &);
    //EMAs
    void calcEMA(const int, std::vector<double> &);

    //SMAs
    void calcSMA(const int, std::vector<double> &);
};

#endif