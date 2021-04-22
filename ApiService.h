#ifndef ApiService_h
#define ApiService_h

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <string>
#include <unordered_map>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;
class ApiService
{
    std::string baseUrl;

public:
    ApiService(std::string);
    pplx::task<http_response> request(method, std::string,
                                      std::unordered_map<std::string, std::string>,
                                      bool isSaveAsJson = false, std::string fileName = "");
};

class Order
{
public:
    std::string symbol, side, positionSide, type, timeInForce,
        newClientOrderId, workingType, newOrderRespType, recvWindow;
    double quantity, price, stopPrice, activationPrice, callbackRate;
    long timestamp;
    bool reduceOnly, closePosition, priceProtect;
};

#endif