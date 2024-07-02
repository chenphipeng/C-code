all:client

CFLAGS+=-I ../include -Wall

client:client.o
	gcc $^ $(CFLAGS) -o $@

clean:
	rm -rf *.o client