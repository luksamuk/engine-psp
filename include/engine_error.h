#ifndef ENGINE_ERROR_H
#define ENGINE_ERROR_H

#include <stdio.h>

void error(char *);

#define ERRORMSG(...) { char msg[128]; sprintf(msg,__VA_ARGS__); error(msg); }

#endif
