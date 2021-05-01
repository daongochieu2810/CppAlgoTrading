#ifndef MarketData_h
#define MarketData_h

#include <vector>
#include <thread>
#include <mutex>
#include <boost/optional.hpp>

//Ha = Heikin-Ashi
class HistoricalData
{
public:
    std::vector<long> openTime, closeTime;
    std::vector<double> open, high, close, low,
        openHa, highHa, closeHa, lowHa,
        volume, quoteAssetVolume;
    std::vector<int> numberOfTrades;
    std::vector<double> fiftyEMA, twoHundredEMA, fiftySMA, twoHundredSMA,
        rsi, pSar, bollingerBands, stochastic;
    void accessClose(boost::optional<std::vector<double> &>);
    void getClose(std::vector<double> &) const;
    void accessHigh(boost::optional<std::vector<double> &>);
    void getHigh(std::vector<double> &) const;
    void accessOpen(boost::optional<std::vector<double> &>);
    void getOpen(std::vector<double> &) const;
    void accessLow(boost::optional<std::vector<double> &>);
    void getLow(std::vector<double> &) const;
    void addData(std::vector<double> &, const double &);
    void clearData();
};

#endif