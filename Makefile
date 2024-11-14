all: server client

server:server.cpp
	g++ -o server server.cpp -lncurses

client:client.cpp
	g++ -o client client.cpp -lncurses

clean:
	rm -rf client server
