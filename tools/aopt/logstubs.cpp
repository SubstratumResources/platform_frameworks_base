#include <stdio.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <string.h>
#include <stdlib.h>

#ifndef _BYPASS_DSO_ERROR
#define _BYPASS_DSO_ERROR

void* __dso_handle;
//extern "C" void  *__dso_handle =0;

#endif // _BYPASS_DSO_ERROR
