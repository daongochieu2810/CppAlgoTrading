#include "MarketData.h"

std::mutex _mtx;

void HistoricalData::accessClose(boost::optional<std::vector<double> &> copy)
{
    std::lock_guard<std::mutex> guard(_mtx);
    if (copy)
    {
        getClose(*copy);
    }
}

void HistoricalData::getClose(std::vector<double> &copy) const
{
    copy = close;
}

void HistoricalData::addData(std::vector<double> &dataToUpdate, const double &newData)
{
    dataToUpdate.push_back(newData);
}