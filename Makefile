CC = gcc
CFLAGS = -Wall -I/Users/shaneconnors/tig/include -I/opt/homebrew/opt/openssl@3/include
LDFLAGS = -Llib -L/usr/lib -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=obj/%.o)
DEPS = $(OBJ:.o=.d)
EXEC = bin/tig

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)


obj/main.o: src/main.c /Users/shaneconnors/tig/include/ioutil.h


obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/*.o obj/*.d $(EXEC)

.PHONY: all clean
