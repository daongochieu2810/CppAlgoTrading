#include "Binance.h"
#include "ApiService.h"
#include "Utils.h"

ApiService apiService(binanceFutureTestnet);

void botData::getOrderBook(std::string symbol, int limit)
{
    std::unordered_map<std::string, std::string> params;
    params.insert(std::make_pair("symbol", symbol));
    params.insert(std::make_pair("limit", std::to_string(limit)));

    apiService.request(methods::GET, "/fapi/v1/depth", params, true, "order_book_info.json");
}

// Do not run this frequently in prod, as it retrieves ALL exchange pairs
void botData::getExchangeInfo()
{
    std::unordered_map<std::string, std::string> params;
    apiService.request(methods::GET, "/fapi/v1/exchangeInfo", params, true, "exchange_info.json");
}

void botData::setUpKeys()
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

void botData::getTime()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long long mslong = (long long)tp.tv_sec * 1000L + tp.tv_usec / 1000;
    std::string time = std::to_string(mslong);
}

void botData::checkConnectivity()
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
    bot.getOrderBook("BTCUSDT");
    return 0;
}