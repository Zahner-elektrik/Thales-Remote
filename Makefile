all:
	g++ -std=c++11 -lpthread main.cpp thalesremoteconnection.cpp thalesremotescriptwrapper.cpp -o RemoteScriptTest
