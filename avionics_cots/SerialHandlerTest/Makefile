CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2
LDFLAGS = -pthread

SRCS = main.cpp SerialHandler.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = cosco_serial_tester

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
    $(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
    $(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
    rm -f $(OBJS) $(TARGET)