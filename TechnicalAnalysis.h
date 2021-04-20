#ifndef TechnicalAnalysis_h
#define TechnicalAnalysis_h

#include <vector>
class HistoricalData
{
public:
    long openTime, closeTime;
    double open, high, close, low, volume, quoteAssetVolume;
    int numberOfTrades;
};

class TechnicalAnalysis
{
public:
    std::vector<HistoricalData> data;
    void setData(std::vector<HistoricalData> &);
    //EMAs
    double calcFiftyEMA();
    double calcHundredEMA();
    double calcTwoHundredEMA();
    //SMAs
    double calcFiftySMA();
};

#endif