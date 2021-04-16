binance: binance.h binance.cpp ApiService.h ApiService.cpp
	g++ -std=c++11 ApiService.cpp binance.cpp -o binance.exe -lboost_system -lcrypto -lssl -lcpprest -lpthread