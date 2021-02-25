all: myhttp

myhttp: httpd.cpp
	g++ -W -Wall -o myhttp httpd.cpp src/startup.cpp src/get_line.cpp src/ans.cpp src/execute_cgi.cpp src/serve_file.cpp src/accept_request.cpp src/error_die.cpp src/epoll_helper.cpp -lpthread



clean:
	rm myhttp
