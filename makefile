TWS=./twsapi/IBJts/builds/*.o
Helpers=ApiService.cpp MarketData.cpp TechnicalAnalysis.cpp Strategy.cpp
CFLAGS=-lboost_system -lcrypto -lssl -lcpprest -lpthread
API_BASE_SRC_DIR=./twsapi/IBJts/source/cppclient/client
API_SAMPLE_DIR=./twsapi/IBJts/samples/Cpp/TestCppClient
INCLUDES=-I${API_BASE_SRC_DIR} -I${API_SAMPLE_DIR}
GPERFTOOLS = -L/usr/local/lib

binance:
	g++ -std=c++11 $(INCLUDES) $(Helpers) Binance.cpp $(TWS) -o Binance.exe $(CFLAGS)
ibkr:
	g++ -std=c++11 $(INCLUDES) $(Helpers) IbkrClient.cpp Ibkr.cpp $(TWS) -o Ibkr.exe $(CFLAGS)
binance_prof:
	g++ -std=c++11 $(INCLUDES) ${GPERFTOOLS} $(Helpers) Binance.cpp $(TWS) -o Binance.exe $(CFLAGS) -Wl,--no-as-needed -lprofiler -ltcmalloc -Wl,--as-needed
ibkr_prof:
	g++ -std=c++11 $(INCLUDES) ${GPERFTOOLS} $(Helpers) IbkrClient.cpp Ibkr.cpp $(TWS) -o Ibkr.exe $(CFLAGS) -Wl,--no-as-needed -lprofiler -ltcmalloc -Wl,--as-needed
clean:
	rm -rf *.exe twsapi/IBJts/builds/*.o