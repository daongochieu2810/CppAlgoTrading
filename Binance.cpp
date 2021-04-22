#include "Binance.h"
#include "ApiService.h"
#include "Utils.h"
#include "TechnicalAnalysis.h"

ApiService apiService(binanceFutureTestnet);
TechnicalAnalysis technicalAnalysis;

void BotData::getPriceAction(std::string symbol, std::string interval, long startTime, long endTime,
                             int limit, void callback(HistoricalData &))
{
    std::unordered_map<std::string, std::string> params;
    params.insert(std::make_pair("symbol", symbol));
    params.insert(std::make_pair("interval", interval));

    if (startTime != -1)
    {
        params.insert(std::make_pair("startTime", std::to_string(startTime)));
    }

    if (endTime != -1)
    {
        params.insert(std::make_pair("endTime", std::to_string(endTime)));
    }

    params.insert(std::make_pair("limit", std::to_string(limit)));

    apiService.request(methods::GET, "/fapi/v1/klines", params)
        .then([=](http_response response) {
            response.extract_json()
                .then([=](json::value jsonData) {
                    HistoricalData candlestick;
                    for (int i = 0; i < limit; i++)
                    {
                        candlestick.openTime.push_back(jsonData[i][0].as_number().to_uint64());
                        candlestick.open.push_back(std::stod(jsonData[i][1].as_string()));
                        candlestick.high.push_back(std::stod(jsonData[i][2].as_string()));
                        candlestick.low.push_back(std::stod(jsonData[i][3].as_string()));
                        candlestick.close.push_back(std::stod(jsonData[i][4].as_string()));
                        candlestick.volume.push_back(std::stod(jsonData[i][5].as_string()));
                        candlestick.closeTime.push_back(jsonData[i][6].as_number().to_uint64());
                    }
                    if (callback != NULL)
                    {
                        callback(candlestick);
                    }
                })
                .wait();
        })
        .wait();
}

void BotData::getOrderBook(std::string symbol, int limit)
{
    std::unordered_map<std::string, std::string> params;
    params.insert(std::make_pair("symbol", symbol));
    params.insert(std::make_pair("limit", std::to_string(limit)));

    apiService.request(methods::GET, "/fapi/v1/depth", params, true, "order_book_info.json");
}

// Do not run this frequently in prod, as it retrieves ALL exchange pairs
void BotData::getExchangeInfo()
{
    std::unordered_map<std::string, std::string> params;
    apiService.request(methods::GET, "/fapi/v1/exchangeInfo", params, true, "exchange_info.json");
}

void BotData::setUpKeys()
{
    std::string line;
    bool isSecretKey = true;
    ifstream_t secrets("secrets.txt");

    if (secrets.is_open())
    {
        while (getline(secrets, line))
        {
            if (isSecretKey)
            {
                bot.secretKey = line;
                isSecretKey = false;
            }
            else
            {
                bot.apiKey = line;
            }
        }
        secrets.close();
    }
}

void BotData::checkConnectivity()
{
    std::unordered_map<std::string, std::string> params;
    apiService.request(methods::GET, "/fapi/v1/ping", params)
        .then([=](http_response response) {
            printf("Received response status code:%u\n", response.status_code());
        })
        .wait();
}

void init()
{
    bot.setUpKeys();
}

void printHistoricalData()
{
    for (double openPrice : technicalAnalysis.data.open)
    {
        std::cout << std::setprecision(10) << openPrice << "\n";
    }

    for (double closePrice : technicalAnalysis.data.close)
    {
        std::cout << std::setprecision(10) << closePrice << "\n";
    }
    /*for (double ema : technicalAnalysis.data.fiftyEMA)
        {
            std::cout << std::setprecision(10) << ema << ", ";
        }

        std::cout << std::endl
                  << "------------------------" << std::endl;

        for (double ema : technicalAnalysis.tempData.close)
        {
            std::cout << std::setprecision(10) << ema << ", ";
        }

        std::cout << std::endl
                  << "------------------------" << std::endl;

        for (double ema : technicalAnalysis.data.fiftyEMA)
        {
            std::cout << std::setprecision(10) << ema << ", ";
        }

        std::cout << std::endl;*/
}

void benchmarkPerformance(void fnc())
{
    auto t1 = high_resolution_clock::now();
    fnc();
    auto t2 = high_resolution_clock::now();
    duration<double, std::milli> ms_double = t2 - t1;
    std::cout << ms_double.count() << "ms\n";
}

int main(int argc, char *argv[])
{
    benchmarkPerformance([]() -> void {
        init();
        bot.getPriceAction(
            "BTCUSDT", "15m", -1, -1, 200, [](HistoricalData &x) -> void {
                technicalAnalysis.setData(x);
            });
        while (1)
        {
            //thread t1 is used to prepare data for next time frame
            std::thread t1(&BotData::getPriceAction, bot, "BTCUSDT", "15m", -1, -1, 200,
                           [](HistoricalData &x) -> void {
                               technicalAnalysis.setTempData(x);
                           });

            //other threads produce technical indicators
            std::thread t2(&TechnicalAnalysis::calcEMA, technicalAnalysis, 50, std::ref(technicalAnalysis.data.fiftyEMA));
            std::thread t3(&TechnicalAnalysis::calcEMA, technicalAnalysis, 200, std::ref(technicalAnalysis.data.twoHundredEMA));

            t1.join();
            t2.join();
            t3.join();

            technicalAnalysis.setData(technicalAnalysis.tempData);

            //wait for 15 mins
            sleep(900);
        }
    });

    return 0;
}