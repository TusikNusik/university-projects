CXX     = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++17
LDFLAGS =

TARGETS = peer-time-sync

.PHONY: all clean

all: $(TARGETS)

peer-time-sync: peer-time-sync.o err.o common.o
	$(CXX) -o $@ $^ $(LDFLAGS)

# Kompilacja plików .cpp z użyciem g++
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGETS) *.o