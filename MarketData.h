#ifndef MarketData_h
#define MarketData_h

#include <vector>
#include <thread>
#include <mutex>
#include <boost/optional.hpp>

class HistoricalData
{
public:
    std::vector<long> openTime, closeTime;
    std::vector<double> open, high, close, low, volume, quoteAssetVolume;
    std::vector<int> numberOfTrades;
    std::vector<double> fiftyEMA;
    void accessClose(boost::optional<std::vector<double> &>);
    void getClose(std::vector<double> &) const;
    void addData(std::vector<double> &, const double &);
};

#endif