# Sedis - (S)imple R(edis)

A programming practice to learn TCP and better learn C.

## Usage:
1. start the server `make run-server`
2. build client `make build-client`
3. run a command in client `./dist/client PING`

## Architecture:
1. Simple TCP server.
2. RESP parser.
3. Map as simple cache.


## Supports: 
  - `PING`
  - `SET <key> <value>`
  - `GET <key>`
  - `del <key1> <key2> ...`

## Planning to supprt:
  - `SCAN <cursor> <size>`
  - `KEYS <pattern>`


