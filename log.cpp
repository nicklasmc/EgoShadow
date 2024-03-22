#include <stdio.h>
#include <stdarg.h>

static FILE *fpxx;

void logOpen(void)
{
	fpxx = fopen("x.x","w");
}

void logClose(void)
{
	fclose(fpxx);
}

//This function works like printf()
//The name Log is used because "log" is a standard C/C++ math function
//
//Log("my numbers are: %i %i %f\n", num1, num2, fnum1);
//
void Log(const char *fmt, ...)
{
	va_list ap;
	if (fmt == NULL) return;
	va_start(ap, fmt);
	vfprintf(fpxx, fmt, ap);
	fflush(fpxx);
	va_end(ap);
}
