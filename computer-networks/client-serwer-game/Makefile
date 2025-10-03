CXX     = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++17
LDFLAGS =

TARGETS = approx-server approx-client

.PHONY: all clean

all: $(TARGETS)

approx-server: approx-server.o err.o common.o logs.o
	$(CXX) -o $@ $^ $(LDFLAGS)


approx-client: approx-client.o err.o common.o logs.o
	$(CXX) -o $@ $^ $(LDFLAGS)


# Kompilacja plików .cpp z użyciem g++
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGETS) *.o