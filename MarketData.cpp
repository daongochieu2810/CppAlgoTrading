#include "MarketData.h"

//std::mutex _mtx;

void HistoricalData::accessClose(boost::optional<std::vector<double> &> copy)
{
    //TODO: add thread-safe access here
    //std::lock_guard<std::mutex> guard(_mtx);
    if (copy)
    {
        getClose(*copy);
        return;
    }
}

void HistoricalData::accessHigh(boost::optional<std::vector<double> &> copy)
{
    //TODO: add thread-safe access here
    //std::lock_guard<std::mutex> guard(_mtx);
    if (copy)
    {
        getHigh(*copy);
        return;
    }
}

void HistoricalData::accessOpen(boost::optional<std::vector<double> &> copy)
{
    //TODO: add thread-safe access here
    //std::lock_guard<std::mutex> guard(_mtx);
    if (copy)
    {
        getOpen(*copy);
        return;
    }
}

void HistoricalData::accessLow(boost::optional<std::vector<double> &> copy)
{
    //TODO: add thread-safe access here
    //std::lock_guard<std::mutex> guard(_mtx);
    if (copy)
    {
        getLow(*copy);
        return;
    }
}

void HistoricalData::getClose(std::vector<double> &copy) const
{
    copy = close;
}

void HistoricalData::getHigh(std::vector<double> &copy) const
{
    copy = high;
}

void HistoricalData::getOpen(std::vector<double> &copy) const
{
    copy = open;
}

void HistoricalData::getLow(std::vector<double> &copy) const
{
    copy = low;
}

void HistoricalData::addData(std::vector<double> &dataToUpdate, const double &newData)
{
    dataToUpdate.push_back(newData);
}

void HistoricalData::clearData()
{
    this->fiftyEMA.clear();
    this->twoHundredEMA.clear();
    this->fiftySMA.clear();
    this->twoHundredSMA.clear();
    this->rsi.clear();
    this->stochastic.clear();
    this->bollingerBands.clear();
    this->pSar.clear();
}