all: myhttp

myhttp: httpd.cpp
	g++ -W -Wall -o myhttp httpd.cpp -lpthread

clean:
	rm myhttp
