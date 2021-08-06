#ifndef Strategy_h
#define Strategy_h

#include <stdio.h>
#include <vector>

#include "MarketData.h"

class Strategy
{
public:
    int simpleHeikinAshiPsarEMA(const std::vector<double> &,
                                const std::vector<double> &, const std::vector<double> &,
                                const std::vector<double> &, const std::vector<double> &,
                                const std::vector<double> &);
    double findStop(const double, HistoricalData &);
    bool momentumAlgo(const double, HistoricalData &);
};

#endif