CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = webserver
OBJS = code/log/*.cpp code/pool/*.cpp code/timer/*.cpp \
       code/http/*.cpp code/server/*.cpp \
       code/buffer/*.cpp code/main.cpp


all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -pthread -lmysqlclient

clean:
	rm $(OBJS) $(TARGET)




