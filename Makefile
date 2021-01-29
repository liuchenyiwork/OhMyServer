all: myhttp

myhttp: httpd.c
	gcc -W -Wall -o ohmyhttp httpd.c -lpthread

clean:
	rm myhttp
