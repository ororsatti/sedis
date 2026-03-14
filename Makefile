run-server: build-server
	./dist/server
	
build-server:
	rm -f server
	mkdir -p dist
	gcc server.c -o dist/server

# build-client:
# 	gcc client.c -o client


