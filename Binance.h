#ifndef binance_h
#define binance_h

#define binanceHost "https://api.binance.com/"

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/ws_client.h>
#include <openssl/hmac.h>
#include <iostream>
#include <string>
#include <chrono>
#include <stdio.h>
#include <iomanip>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <stdlib.h>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;
using namespace std::chrono;

class botData
{
    double price, pastPrice, sellPercent, RSI;
    std::vector<long> openTime, closeTime, numTrades;
    std::vector<long double> open, high, low, close, volume, quoteAssetVolume, takerBuyAssetVol, takerBuyQuoteAssetVol, Ignore;
    bool algoCheck, algoBuy;
    std::string pair;
    std::string epochTime;
    std::string signature;
    std::string secretKey;
    std::string apiKey;

public:
    void setUpKeys();
    void setPair();
    void setPrice(double foundPrice);
    void setSellPercent();
    void checkConnectivity();
    void getExchangeInfo();
    void getOrderBook(std::string, int limit = 100);
    void getPrice(const std::string);
    void formatPrice(json::value const &);
    void getTime();
    void HMACsha256(std::string const &, std::string const &);
    void checkBuy();
    void checkSell();
    void getHistoricalPrices();
    void formatHistoricalPrices(json::value const &);
    void calcRSI();

} bot;

#endif