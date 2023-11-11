make:
	@g++ -o ../docker/peer.out peer.cpp helpers/*.cpp network/*.cpp -std=c++20

run:
	@../docker/peer.out config.txt

clean:
	@rm ../docker/peer.out
