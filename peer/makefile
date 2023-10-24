make:
	@mkdir build || true
	@g++ -o build/peer.out peer.cpp helpers/*.cpp

run:
	@sudo build/peer.out config.txt

clean:
	@rm -r build/
