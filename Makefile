
server: ./src/server.c client ./src/helpers.c
	gcc -g -pthread -Wall ./src/server.c ./src/helpers.c -o $@

client: ./src/client.c ./src/helpers.c
	gcc -g -pthread -Wall ./src/client.c ./src/helpers.c -o $@

clean:
	rm -f server client && rm -f ./logs/*.txt && clear

clear:
	rm -f server client && rm -f ./logs/*.txt && clear

reset:
	rm -f server client && rm -f ./logs/*.txt && killall client && clear
	
run: server client
	./server test.txt 2 10