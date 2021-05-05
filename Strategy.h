#ifndef Strategy_h
#define Strategy_h

#include <stdio.h>
#include <vector>

class Strategy
{
public:
    int simpleHeikinAshiPsarEMA(const std::vector<double> &,
                                const std::vector<double> &, const std::vector<double> &,
                                const std::vector<double> &, const std::vector<double> &,
                                const std::vector<double> &);
};

#endif