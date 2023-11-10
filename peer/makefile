make:
	@mkdir build || true
	@g++ -o build/peer.out peer.cpp helpers/*.cpp network/*.cpp -std=c++20

run:
	@build/peer.out config.txt

clean:
	@rm -r build/
