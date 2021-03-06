#ifndef binance_h
#define binance_h

#define binanceHost "https://api.binance.com/api/v3/"
#define binanceSpotTestnet "https://testnet.binance.vision/api/v3/"
#define binanceFutureTestnet "https://testnet.binancefuture.com/fapi/v1/"

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/ws_client.h>
#include <openssl/hmac.h>
#include <iostream>
#include <cstdarg>
#include <string>
#include <stdio.h>
#include <iomanip>
#include <functional>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>

#include "TechnicalAnalysis.h"
#include "strategies/Strategy.h"

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;

class BotData
{
    double price, pastPrice, sellPercent, RSI;
    std::vector<Order> globalOrders;
    std::vector<long> openTime, closeTime, numTrades;
    std::vector<long double> open, high, low, close, volume, quoteAssetVolume, takerBuyAssetVol, takerBuyQuoteAssetVol, Ignore;
    bool algoCheck, algoBuy;
    std::string pair;
    std::string secretKey;
    std::string apiKey;

public:
    void setUpKeys();
    void setPair();
    void setPrice(double foundPrice);
    void setSellPercent();
    void getAllOrders(std::string);

    void getPriceAction(std::string const &, std::string const &, long startTime = -1, long endTime = -1,
                        int limit = 500, std::function<void(HistoricalData &)> = NULL);
    void checkConnectivity();
    void getAllTradingPairs(void (*f)(std::list<std::string> &) = NULL);
    void getOrderBook(std::string, int limit = 100);
    void getCurrentAveragePrice(std::string const &, double &);
    void formatPrice(json::value const &);
    void HMACsha256(std::string const &, std::string const &, std::string &);
    void newOrder(Order const &);
    void newOco(const std::vector<Order> &);
    void checkBuy();
    void checkSell();
    void accessOrders(boost::optional<std::vector<Order> &>);
    void modifyOrders(boost::optional<std::vector<Order> &>, const std::vector<Order> &);
    void getHistoricalPrices();
    void formatHistoricalPrices(json::value const &);
    void calcRSI();

} bot;

#endif