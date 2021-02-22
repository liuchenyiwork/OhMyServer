all: myhttp

myhttp: httpd.cpp
	g++ -W -Wall -o myhttp httpd.cpp get_line.cpp ans.cpp execute_cgi.cpp serve_file.cpp accept_request.cpp error_die.cpp startup.cpp epoll_helper.cpp -lpthread

clean:
	rm myhttp
