MODE = debug

include config.mk

.PHONY: all
all: test

test: test.o threadpool.o list.o
	gcc $(LDFLAGS) -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -c $<

.PHONY: json
json: clean
	bear -- make

.PHONY: clean
clean:
	-rm -f *.o test
