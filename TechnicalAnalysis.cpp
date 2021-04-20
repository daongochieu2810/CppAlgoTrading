#include "TechnicalAnalysis.h"
#include <iostream>

void TechnicalAnalysis::setData(HistoricalData &data)
{
    this->data = data;
}

void TechnicalAnalysis::calcEMA(int period, std::vector<double> &emaData)
{
    double ema = 0.0, sma = 0.0, sum = 0.0, multiplier = 0.0;
    int emaIndex = 0;
    std::vector<double> closePrices;
    this->data.accessClose(closePrices);

    if (closePrices.size() <= period)
    {
        throw "Not enough data for " + std::to_string(period) + " days";
        return;
    }

    std::vector<double> tempEma;
    for (int i = 0; i < period; i++)
    {
        sum += closePrices[i];
    }

    sma = sum / period;
    std::cout << "SMA is " << sma << std::endl;
    tempEma.push_back(sma);
    this->data.addData(emaData, sma);

    multiplier = 2 / (period + 1.0);

    for (int i = period; i < closePrices.size(); i++)
    {
        tempEma.push_back((closePrices[i] - tempEma[emaIndex]) * multiplier + tempEma[emaIndex]);
        emaIndex++;
        this->data.addData(emaData, tempEma[emaIndex]);
    }
    std::cout << std::endl;
}
