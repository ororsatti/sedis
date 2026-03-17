SERVER_EXEC=dist/server-build
CLIENT_EXEC=dist/client-build

run-client: build-client
	./$(CLIENT_EXEC)

run-server: build-server
	./$(SERVER_EXEC)
	
build-server:
	rm -f $(SERVER_EXEC)
	mkdir -p dist
	gcc server/*.c -o $(SERVER_EXEC) -pedantic -Werror -g

build-client:
	rm -f $(CLIENT_EXEC)
	mkdir -p dist
	gcc client/*.c -o $(CLIENT_EXEC) -pedantic -Werror -g


