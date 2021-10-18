#ifndef Strategy_h
#define Strategy_h

#include <stdio.h>
#include <vector>

#include "Order.h"
#include "MarketData.h"

class Strategy
{
public:
    int simpleHeikinAshiPsarEMA(const std::vector<double> &,
                                const std::vector<double> &, const std::vector<double> &,
                                const std::vector<double> &, const std::vector<double> &,
                                const std::vector<double> &);
    double findStop(const double, HistoricalData &);
    void scalpingMA(const std::string &,
                    const double,
                    const double,
                    const std::vector<Order> &,
                    std::vector<Order> &,
                    std::vector<Order> &,
                    std::vector<Order> &);
};

#endif