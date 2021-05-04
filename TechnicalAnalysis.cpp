#include "TechnicalAnalysis.h"

void TechnicalAnalysis::setData(HistoricalData &data)
{
    //this->data.clearData();
    this->data = data;
}

void TechnicalAnalysis::setTempData(HistoricalData &data)
{
    //this->tempData.clearData();
    this->tempData = data;
}

void TechnicalAnalysis::setUpHeikinAshi()
{
    std::vector<double> close, open, low, high;
    this->data.accessClose(close);
    this->data.accessClose(open);
    this->data.accessLow(low);
    this->data.accessHigh(high);

    double prevCloseHa, prevOpenHa;

    for (int i = 1; i < close.size(); i++)
    {
        double currCloseHa = (close[i] + open[i] + low[i] + high[i]) / 4;
        double currOpenHa = i == 0 ? (open[i] + close[i]) / 2 : (prevOpenHa + prevCloseHa) / 2;

        this->data.addData(this->data.closeHa, currCloseHa);
        this->data.addData(this->data.openHa, currOpenHa);
        this->data.addData(this->data.highHa, std::max(currCloseHa, std::max(currOpenHa, high[i])));
        this->data.addData(this->data.lowHa, std::min(currOpenHa, std::min(currCloseHa, low[i])));

        prevCloseHa = currCloseHa;
        prevOpenHa = currOpenHa;
    }
}

void TechnicalAnalysis::calcEMA(int period, std::vector<double> &emaData)
{
    double ema = 0.0, sma = 0.0, sum = 0.0, multiplier = 0.0;
    int emaIndex = 0;
    std::vector<double> closePrices;
    this->data.accessClose(closePrices);

    if (closePrices.size() <= period)
    {
        std::cout << "Not enough data for " + std::to_string(period) + "-day EMA" << std::endl;
        return;
    }

    std::vector<double> tempEma;
    for (int i = 0; i < period; i++)
    {
        sum += closePrices[i];
    }

    sma = sum / period;
    tempEma.push_back(sma);
    this->data.addData(emaData, sma);

    multiplier = 2 / (period + 1.0);

    for (int i = period; i < closePrices.size(); i++)
    {
        tempEma.push_back((closePrices[i] - tempEma[emaIndex]) * multiplier + tempEma[emaIndex]);
        emaIndex++;
        this->data.addData(emaData, tempEma[emaIndex]);
    }
}

void TechnicalAnalysis::calcSMA(int period, std::vector<double> &smaData)
{
    std::vector<double> closePrices;
    this->data.accessClose(closePrices);
    if (closePrices.size() <= period)
    {
        std::cout << "Not enough data for " + std::to_string(period) + "-day SMA" << std::endl;
        return;
    }

    double initialSMA = std::accumulate(closePrices.begin(), closePrices.begin() + 50, 0.0) / period;
    double currentSMA = initialSMA;
    this->data.addData(smaData, initialSMA);
    for (int i = 0; i < closePrices.size() - period; i++)
    {
        currentSMA = currentSMA - (closePrices[i] - closePrices[i + period]) / period;
        this->data.addData(smaData, currentSMA);
    }
}

void TechnicalAnalysis::calcRSI(int period, std::vector<double> &rsiData)
{
    std::vector<double> closePrices;
    this->data.accessClose(closePrices);
    if (closePrices.size() < period)
    {
        std::cout << "Not enough data for 14-day RSI" << std::endl;
        return;
    }

    std::vector<double> gain, loss, change, avgGain, avgLoss, rs;
    double sumGain, sumLoss;

    auto pushCurrentPeriod = [](const std::vector<double> &currentPeriod, std::vector<double> &change) {
        for (int i = 1; i < currentPeriod.size(); i++)
        {
            change.push_back(currentPeriod[i] - currentPeriod[i - 1]);
        }
    };

    //take the latest data
    std::vector<double> currentPeriod(closePrices.end() - period, closePrices.end());

    for (int i = 0; i < change.size(); i++)
    {
        change[i] > 0 ? gain.push_back(change[i]) : gain.push_back(0.0);
        change[i] < 0 ? loss.push_back(std::abs(change[i])) : loss.push_back(0.0);

        if (i < 14)
        {
            sumGain += gain[i];
            sumLoss += loss[i];
        }
    }

    avgGain.push_back(sumGain / 14);
    avgLoss.push_back(sumLoss / 14);

    for (int i = 14, j = 1; i < gain.size(); i++)
    {
        avgGain.push_back((avgGain[j - 1] * 13 + gain[i]) / 14);
        avgLoss.push_back((avgLoss[j - 1] * 13 + loss[i]) / 14);
        j++;
    }

    for (int i = 0; i < avgGain.size(); i++)
    {
        rs.push_back(avgGain[i] / avgLoss[i]);
        this->data.addData(rsiData, avgLoss[i] == 0 ? 100.0 : 100.0 - (100.0 / (1 + rs[i])));
    }
}

void TechnicalAnalysis::calcStoch(int period, std::vector<double> &stochData)
{
}

void TechnicalAnalysis::calcPSAR(std::vector<double> &psarData)
{
    double af = 0.0;
    const double afStep = 0.02, maxAf = 0.2;
    std::vector<double> psars, high, low;

    this->data.accessHigh(high);
    this->data.accessLow(low);

    bool isUp = (high[1] >= high[0] || low[0] <= low[1]) ? true : false;
    double psar = isUp ? low[0] : high[0];
    double ep = isUp ? high[0] : low[0];

    psars.push_back(psar);

    for (int i = 1; i < high.size(); i++)
    {
        double nextPsar;
        if (isUp)
        {
            //higher high detected, increase af
            if (high[i] > ep)
            {
                ep = high[i];
                af = std::min(maxAf, af + afStep);
            }

            nextPsar = psar + af * (ep - psar);
            //next psar cant be above prior period's low or current low
            nextPsar = std::min(low[i], std::min(low[i - 1], nextPsar));
            //if psar crosses the next period's price range, the trend switches
            if (nextPsar > low[i])
            {
                isUp = false;
                nextPsar = ep;
                ep = low[i];
                af = afStep;
            }
        }
        else
        {
            //lower low detected, increase af
            if (low[i] < ep)
            {
                ep = low[i];
                af = std::min(maxAf, af + afStep);
            }

            nextPsar = psar + af * (ep - psar);
            //next psar cant be below prior period's high or the current high
            nextPsar = std::max(high[i], std::max(high[i - 1], nextPsar));
            //if psar crosses the next period's price range, the trend switches
            if (nextPsar < high[i])
            {
                isUp = true;
                nextPsar = ep;
                ep = high[i];
                af = afStep;
            }
        }

        psars.push_back(nextPsar);
        psar = nextPsar;
    }

    psarData = psars;
}