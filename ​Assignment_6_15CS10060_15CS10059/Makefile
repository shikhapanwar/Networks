all: server 

server: source_code.cpp
	g++ -w source_code.cpp -o server  -lm -lcrypto -lssl -l pthread


clean:
	rm server 
