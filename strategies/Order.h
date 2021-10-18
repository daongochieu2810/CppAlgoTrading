#ifndef Order_h
#define Order_h

#include <string>

class Order
{
public:
    std::string symbol, side, positionSide, type, timeInForce,
        newClientOrderId, workingType, newOrderRespType, recvWindow, status;
    double quantity, price, stopPrice, activationPrice, callbackRate;
    long timestamp;
    bool reduceOnly, closePosition, priceProtect;
};

#endif