CXX = g++
CXXFLAGS = -std=c++20 -g

all: peer.out

peer.out: peer.cpp helpers/*.cpp network/*.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f peer.out

make:
	@g++ -o ../docker/peer.out peer.cpp helpers/*.cpp network/*.cpp -std=c++20

run:
	@../docker/peer.out config.txt