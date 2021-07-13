#include "Strategy.h"

double Strategy::findStop(const double currentPrice, HistoricalData &data, int nowInMillis)
{
    return 0.0;
}

int Strategy::simpleHeikinAshiPsarEMA(const std::vector<double> &openHa,
                                      const std::vector<double> &closeHa,
                                      const std::vector<double> &highHa,
                                      const std::vector<double> &lowHa,
                                      const std::vector<double> &psar,
                                      const std::vector<double> &ema)
{
    const int noTrailing = 5, noSameCandles = 3;

    if (openHa.size() <= noTrailing)
    {
        printf("Not enough data for a trailing value of %d\n", noTrailing);
        return -1;
    }

    const int startIndex = openHa.size() - noTrailing, currentIndex = openHa.size() - 1;
    bool isShort = false;

    int numBelowEMA = 0, numAboveEMA = 0;
    for (int i = currentIndex - noTrailing; i < closeHa.size(); i++)
    {
        if (closeHa[i] < ema[i - 199])
        {
            numBelowEMA++;
        }
        else
        {
            numAboveEMA++;
        }
    }

    //short if price action below 200-day EMA
    if (numBelowEMA > numAboveEMA)
    {
        isShort = true;
    }

    if (isShort)
    {
    }
    else
    {
        int noRed = 0, noGreen = 0;
        //check if we have enough red heikin-ashi candlesticks
        for (int i = startIndex; i < openHa.size() - 1; i++)
        {
            if (openHa[i] > closeHa[i])
            {
                noRed++;
            }
        }

        //long only if the pullback period has more than 2 red candles
        if (noRed >= noSameCandles)
        {
            //the latest 2 candles must be green and the current candle must not have lower shadow
            if (openHa[currentIndex] < closeHa[currentIndex] && openHa[currentIndex] == lowHa[currentIndex] &&
                openHa[currentIndex - 1] < closeHa[currentIndex - 1])
            {
                //psar must be below the current close price
                if (openHa[currentIndex] > psar[currentIndex])
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}