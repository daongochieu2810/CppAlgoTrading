CXX=g++
CXXFLAGS=-pthread -Wall -Wno-switch -Wpedantic -std=c++11
ROOT_DIR=../../../source/cppclient
BASE_SRC_DIR=${ROOT_DIR}/client
INCLUDES=-I${BASE_SRC_DIR} -I${ROOT_DIR}
SHARED_LIB_DIRS=-L${BASE_SRC_DIR}
SHARD_LIBS=-lTwsSocketClient
TARGET=TestCppClient

$(TARGET)Static:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(BASE_SRC_DIR)/*.cpp ./*.cpp -o$(TARGET)

$(TARGET):
	$(CXX) $(CXXFLAGS) $(INCLUDES) ./*.cpp -o$(TARGET) $(SHARED_LIB_DIRS) $(SHARD_LIBS)

clean:
	rm -f $(TARGET) *.o

