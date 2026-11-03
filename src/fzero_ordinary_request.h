#pragma once
#include <stddef.h>
/* One bounded snapshot. Delete sharing permits the parent's atomic replacement. */
int FzeroOrdinaryReadFile(const char *path,char *out,size_t size,unsigned long *error);
int FzeroOrdinaryReadValue(const char *text,const char *key,char *out,size_t size);
