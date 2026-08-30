.DEFAULT_GOAL := cli
CLI_NAME=varsub
CFLAGS= -Wall -Wextra -Iinclude

DEBUG= -std=c17 -Wall -Wextra -Wpedantic -g3 -O0 -fsanitize=address,undefined -Iinclude

PREFIX=$(HOME)/.local
BIN=$(PREFIX)/bin

.PHONY: cli
cli:
	gcc $(CFLAGS) cli/main.c src/*.c -o dist/$(CLI_NAME)

cli-debug:
	gcc $(DEBUG) cli/main.c src/*.c -o dist/$(CLI_NAME)

install:
	cp dist/varsub $(BIN)
