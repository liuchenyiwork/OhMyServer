#include <stdio.h>
#include <stdlib.h>
#include "error_die.h"
/*
 * 把错误信息写到perror并退出
 * */
void error_die(const char* sc) {
	perror(sc);
	exit(1);
}