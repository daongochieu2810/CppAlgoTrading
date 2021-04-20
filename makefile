binance: Binance.h Binance.cpp ApiService.h ApiService.cpp TechnicalAnalysis.cpp TechnicalAnalysis.h Utils.h
	g++ -std=c++11 ApiService.cpp TechnicalAnalysis.cpp Binance.cpp -o Binance.exe -lboost_system -lcrypto -lssl -lcpprest -lpthread
ibkr: Ibkr.h Ibkr.cpp ApiService.h ApiService.cpp TechnicalAnalysis.cpp TechnicalAnalysis.h Utils.h
	g++ -std=c++11 ApiService.cpp TechnicalAnalysis.cpp Ibkr.cpp -o Ibkr.exe -lboost_system -lcrypto -lssl -lcpprest -lpthread