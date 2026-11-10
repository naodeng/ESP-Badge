#pragma once
#include <stddef.h>
/* Blocking HTTP GET into out (NUL terminated). >0 bytes, <0 error. */
int codex_http_get(const char *url, char *out, size_t cap);
