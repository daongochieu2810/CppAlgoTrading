#include "TechnicalAnalysis.h"

void TechnicalAnalysis::setData(std::vector<HistoricalData> &data)
{
    this->data = data;
}

double TechnicalAnalysis::calcFiftyEMA()
{
    double SMA = 0.0, sum = 0.0, multiplier = 0.0;
    int period = 50, j = 0;
}

double TechnicalAnalysis::calcFiftySMA()
{
}