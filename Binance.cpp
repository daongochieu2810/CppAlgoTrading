#include "Binance.h"
#include "ApiService.h"
#include "Utils.h"

ApiService apiService(binanceFutureTestnet);

void BotData::getPriceAction(std::string symbol, std::string interval, long startTime, long endTime,
                             int limit, void callback(std::vector<CandlestickData>))
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
                    std::vector<CandlestickData> candlesticks;
                    for (int i = 0; i < limit; i++)
                    {
                        CandlestickData candlestick;
                        candlestick.openTime = jsonData[i][0].as_integer();
                        candlestick.open = std::stod(jsonData[i][1].as_string());
                        candlestick.high = std::stod(jsonData[i][2].as_string());
                        candlestick.low = std::stod(jsonData[i][3].as_string());
                        candlestick.close = std::stod(jsonData[i][4].as_string());
                        candlestick.volume = std::stod(jsonData[i][5].as_string());
                        candlestick.closeTime = jsonData[i][6].as_integer();
                        candlesticks.push_back(candlestick);
                    }
                    if (callback != NULL)
                    {
                        callback(candlesticks);
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

void BotData::getTime()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long long mslong = (long long)tp.tv_sec * 1000L + tp.tv_usec / 1000;
    std::string time = std::to_string(mslong);
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

int main(int argc, char *argv[])
{
    init();
    //bot.getExchangeInfo();
    //bot.getOrderBook("BTCUSDT");
    bot.getPriceAction(
        "BTCUSDT", "1d", -1, -1, 100, [](std::vector<CandlestickData> x) -> void { std::cout << "GOT U" << std::endl; });
    return 0;
}