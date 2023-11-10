make:
	@mkdir build || true
	@g++ -o build/peer.out peer.cpp helpers/config.cpp helpers/peer_connection.cpp ../network/*.cpp -std=c++20

run:
	@sudo build/peer.out config.txt

clean:
	@rm -r build/
