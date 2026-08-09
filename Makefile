.DEFAULT_GOAL := cli
CLI_NAME=varsub
CFLAGS= -Wall -Wextra -Iinclude

PREFIX=$(HOME)/.local
BIN=$(PREFIX)/bin

.PHONY: cli
cli:
	gcc $(CFLAGS) cli/main.c src/*.c -o dist/$(CLI_NAME)

install:
	cp dist/varsub $(BIN)
