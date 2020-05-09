#include <stdio.h>

#include "stdio_private.h"

char * gets(char *str, int size)
{
	return(fgets(str, size, stdin));
}
