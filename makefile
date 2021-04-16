binance: Binance.h Binance.cpp ApiService.h ApiService.cpp Utils.h
	g++ -std=c++11 ApiService.cpp Binance.cpp -o Binance.exe -lboost_system -lcrypto -lssl -lcpprest -lpthread