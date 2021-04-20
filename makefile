TWS=./twsapi/IBJts/builds/*.o
Helpers=ApiService.cpp MarketData.cpp TechnicalAnalysis.cpp
CFLAGS=-lboost_system -lcrypto -lssl -lcpprest -lpthread
API_BASE_SRC_DIR=./twsapi/IBJts/source/cppclient/client
API_SAMPLE_DIR=./twsapi/IBJts/samples/Cpp/TestCppClient
INCLUDES=-I${API_BASE_SRC_DIR} -I${API_SAMPLE_DIR}

binance:
	g++ -std=c++11 $(INCLUDES) $(Helpers) Binance.cpp $(TWS) -o Binance.exe $(CFLAGS)
ibkr:
	g++ -std=c++11 $(INCLUDES) $(Helpers) Ibkr.cpp $(TWS) -o Ibkr.exe $(CFLAGS)