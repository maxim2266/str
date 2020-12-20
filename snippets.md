### Code Examples

Here I provide various (hopefully, useful) functions and code examples that are not included into the
main library. Some examples use non-POSIX and/or compiler-specific features that may or may
not be suitable for a particular project. Also, these snippets were tested while being developed,
but they may break in the future as the library evolves.

##### `void str_sprintf(str* const dest, const char* fmt, ...)`

Probably the simplest implementation utilising non-POSIX `asprintf(3)` function:
```C
#define _GNU_SOURCE

#include "str.h"

#define str_sprintf(dest, fmt, ...)	\
({	\
    char* ___p;	\
    const int ___n = asprintf(&___p, (fmt),  ##__VA_ARGS__);	\
    str_assign((dest), str_acquire_chars(___p, ___n));	\
})
```
This code does not check for errors. A more standard-conforming implementation would probably go
through `open_memstream(3)` function.

##### `int str_from_int(str* const dest, const int val)`
```C
int str_from_int(str* const dest, const int val)
{
    char buff[256];	// of some "big enough" size

    return str_cpy(dest, str_ref_chars(buff, snprintf(buff, sizeof(buff), "%d", val)));
}
```

This code can also be used as a template for other functions converting from `double`, `struct tm`, etc.

##### `int str_append(str* const dest, ...)`
```C
#define str_append(dest, ...)	\
    ({ str* const ___p = (dest); str_cat(___p, *___p, ##__VA_ARGS__); })
```
Test case and usage example:
```C
    str s = str_lit("zzz");

    assert(str_append(&s, str_lit(" "), str_lit("aaa")) == 0);
    assert(str_eq(s, str_lit("zzz aaa")));

    str_free(s);
```

##### Using `str` objects with `printf` family of functions

Since a string object is not guaranteed to refer to a null-terminated string it should be formatted
with explicitly specified length, for example:
```C
	str s = ...

	printf("%.*s\n", (int)str_len(s), str_ptr(s));
```
_Note:_ The maximum length of the string is limited to `INT_MAX` bytes, and formatting will stop
at the first null byte within the string.
