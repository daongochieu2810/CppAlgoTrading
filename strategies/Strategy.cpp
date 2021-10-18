#include "Strategy.h"

const double DEFAULT_STOP = 0.95;

double Strategy::findStop(const double currentPrice, HistoricalData &data)
{
    std::vector<double> lowPrices;
    data.accessLow(lowPrices);
    auto lowIterator = lowPrices.size() > 100 ? lowPrices.begin() + 100 : lowPrices.begin();

    std::vector<double> reSampledLow;
    std::vector<double> tempSample;
    std::vector<double> diff;
    int index = 0;

    for (auto i = lowIterator; i != lowPrices.end(); i++)
    {
        tempSample.push_back(*i);
        index++;
        if (index % 5 == 0)
        {
            reSampledLow.push_back(*std::min_element(tempSample.begin(), tempSample.end()));
            tempSample.clear();
        }
    }

    for (int i = 1; i < reSampledLow.size(); i++)
    {
        diff.push_back(reSampledLow[i] - reSampledLow[i - 1]);
    }

    for (int i = 0; i < diff.size(); i++)
    {
        if (diff[i] < 0)
        {
            return currentPrice * DEFAULT_STOP;
        }
    }

    return *reSampledLow.end() - 0.01;
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

void Strategy::scalpingMA(const std::string &symbol,
                          const double currentPrice,
                          const double ma,
                          const std::vector<Order> &currentOrders,
                          std::vector<Order> &toSell,
                          std::vector<Order> &toBuy,
                          std::vector<Order> &toCancel)
{
    for (int i = 0; i < currentOrders.size(); i++)
    {
        const Order currentOrder = currentOrders[i];
        if (currentOrder.symbol.compare(symbol) == 0 && currentOrder.side.compare("BUY") == 0)
        {
            if (currentOrder.status.compare("NEW") == 0 && currentOrder.price < currentPrice)
            {
                toCancel.push_back(currentOrder);
            }
            else if (currentOrder.status.compare("FILLED") == 0 && currentOrder.price >= currentPrice)
            {
                toSell.push_back(currentOrder);
            }
        }
    }

    if (currentPrice >= ma)
    {
        Order newOrder;
        newOrder.symbol = symbol;
        newOrder.price = currentPrice;
        toBuy.push_back(newOrder);
    }
}