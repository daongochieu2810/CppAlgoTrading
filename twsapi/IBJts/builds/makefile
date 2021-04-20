CXX=g++
CXXFLAGS=-c -pthread -Wall -Wno-switch -Wpedantic -std=c++11
ROOT_DIR=../source/cppclient
BASE_SRC_DIR=${ROOT_DIR}/client
SAMPLE_SRC_DIR=../samples/Cpp/TestCppClient
INCLUDES=-I${BASE_SRC_DIR} -I${ROOT_DIR}
SHARED_LIB_DIRS=-L${BASE_SRC_DIR}
SHARED_LIBS=-lTwsSocketClient

Tws:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(BASE_SRC_DIR)/*.cpp
Sample:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(filter-out $(SAMPLE_SRC_DIR)/Main.cpp, $(wildcard $(SAMPLE_SRC_DIR)/*.cpp))

clean:
	rm -f ./*.o