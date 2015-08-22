all: sha1appender

sha1appender: sha1appender.c
	gcc -ggdb -o $@ $^ -lssl -lcrypto


.PHONY: clean mrproper
clean:

mrproper: clean
	rm -f sha1appender
