#include "Strategy.h"

void Strategy::simpleHeikinAshiPsarEMA(const std::vector<double> &openHa,
                                       const std::vector<double> &closeHa,
                                       const std::vector<double> &highHa,
                                       const std::vector<double> &lowHa,
                                       const std::vector<double> &psar,
                                       const std::vector<double> &ema)
{
    const int noTrailing = 10, noSameCandles = 3;

    if (openHa.size() <= noTrailing)
    {
        printf("Not enough data for a trailing value of %d\n", noTrailing);
        return;
    }

    const int startIndex = openHa.size() - noTrailing, currentIndex = openHa.size() - 1;
    int noRed = 0, noGreen = 0;
    bool isShort = false;

    int numBelowEMA = 0, numAboveEMA = 0;
    for (int i = currentIndex - 10; i < closeHa.size(); i++)
    {
        if (closeHa[i] < ema[i])
        {
            numBelowEMA++;
        }
        else
        {
            numAboveEMA++;
        }
    }

    for (int i = startIndex; i < openHa.size() - 1; i++)
    {
        if (openHa[i] > closeHa[i])
        {
            noRed++;
        }
        else
        {
            noGreen++;
        }
    }
}