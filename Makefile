ifeq ($(OS),Windows_NT)
all:
	g++ -std=c++11 -lpthread main.cpp thalesremoteconnection.cpp thalesremotescriptwrapper.cpp -o RemoteScriptTest.exe -lws2_32
else
all:
	g++ -std=c++11 -lpthread main.cpp thalesremoteconnection.cpp thalesremotescriptwrapper.cpp -o RemoteScriptTest
endif
