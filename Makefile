CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic
LDFLAGS = -lm
EXEC = encode decode
OBJS = trie.o word.o io.o encode.o decode.o

all: encode decode

encode: encode.o trie.o word.o io.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

decode: decode.o trie.o word.o io.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

trie.o: trie.c
	$(CC) $(CFLAGS) -c $<

word.o: word.c
	$(CC) $(CFLAGS) -c $<

io.o: io.c
	$(CC) $(CFLAGS) -c $<

encode.o: encode.c
	$(CC) $(CFLAGS) -c $<

decode.o: decode.c
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(EXEC) $(OBJS)

format:
	clang-format -i -style=file *.[ch]

