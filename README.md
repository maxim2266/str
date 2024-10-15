# str: yet another string library for C language.

[![License: BSD 3 Clause](https://img.shields.io/badge/License-BSD_3--Clause-yellow.svg)](https://opensource.org/licenses/BSD-3-Clause)

## Motivation

Bored with developing the same functionality over and over again, unsatisfied
with existing libraries, so decided to make the right one, once and forever. 🙂

## Features

* Handles both C and binary strings;
* Light-weight references to strings: cheap to create, copy, or pass by value;
* Support for copy and move semantics, although not enforceable by the C language;
* String composition functions writing to memory, file descriptors, or file streams;

## Installation
Just clone the project and copy (or symlink) the files `str.h` and `str.c` into your project,
but please respect the [license](LICENSE).

## Code Examples

String composition:

```C
str s = str_null;

str_join(&s, str_lit(", "),
         str_lit("Here"),
         str_lit("there"),
         str_lit("and everywhere"));

str_cat(&s, s, str_lit("..."));

assert(str_eq(s, str_lit("Here, there, and everywhere...")));
str_free(s);
```

Same as above, but writing to a file:

```C
FILE* const stream = fopen(...);

int err = str_join(stream, str_lit(", "),
                   str_lit("Here"),
                   str_lit("there"),
                   str_lit("and everywhere..."));

if(err != 0) { /* handle the error */ }
```

[Discussion](https://news.ycombinator.com/item?id=25212864) on Hacker News.

## User Guide

_**Disclaimer:** This is the good old C language, not C++ or Rust, so nothing can be enforced
on the language level, and certain discipline is required to make sure there is no corrupt
or leaked memory resulting from using this library._

A string is represented by the type `str` that maintains a pointer to some memory containing
the actual string. Objects of type `str` are small enough (a struct of a `const char*` and a `size_t`)
to be cheap to create, copy (pass by value), and move. The `str` structure should be treated
as opaque (i.e., do not attempt to directly access or modify the fields in this structure).
The strings are assumed to be immutable, like those in Java or Go, but only by means of `const char*`
pointers, so it is actually possible to write to such a string, although the required type
cast to `char*` offers at least some (mostly psychological) protection from modifying a string
by mistake.

This library focusses only on handling strings, not gradually composing them like
[StringBuffer](https://docs.oracle.com/javase/7/docs/api/java/lang/StringBuffer.html)
class in Java.

All string objects must be initialised. Uninitialised objects will cause
undefined behaviour. Use the provided constructors, or `str_null` for empty strings.

There are two kinds of `str` objects: those actually owning the memory they point to, and
non-owning references. This property can be queried using `str_is_owner` and `str_is_ref`
functions, otherwise such objects are indistinguishable.

Non-owning string objects are safe to copy and assign to each other, as long as the memory
they refer to is valid. They do not need to be freed. `str_free` is a no-op for reference
objects. A reference object can be cheaply created from a C string, a string literal,
or from a range of bytes.

Owning objects require special treatment, in particular:
* It is a good idea to have only one owning object per each allocated string, but such
a string can have many references to its underlying string, as long as those references do not
outlive the owning object.
Sometimes this rule may be relaxed for code clarity, like in the above example where
the owning object is passed directly to a function, but only if the function does not
store or release the object. When in doubt pass such an object via `str_ref`.
* Direct assignments (like `s2 = s1;`) to owning objects will certainly leak memory, use
`str_assign` function instead. In fact, this function can assign to any string object,
owning or not, so it can be used everywhere, just to avoid any doubt.
* There is no automatic memory management in C, so every owning object must be released at
some point using either `str_free` or `str_clear` function. String objects on the stack
can also be declared as `str_auto` (or `const str_auto`) for automatic cleanup when the variable
goes out of scope.
* An owning object can be moved to another location by using `str_move` function. The
function resets its source object to an empty string.
* An owning object can be passed over to another location by using `str_pass` function. The
function sets its source to a non-owning reference to the original string.

It is technically possible to create a reference to a string that is not
null-terminated. The library accepts strings without null-terminators, but every new string
allocated by the library is guaranteed to be null-terminated.

### String Construction

A string object can be constructed form any C string, string literal, or a range of bytes.
The provided constructors are computationally cheap to apply. Depending on the constructor,
the new object can either own the actual string it refers to, or be a non-owning reference.
Constructors themselves do not allocate any memory. Importantly, constructors are the only
functions in this library that return a string object, while others assign their results
through a pointer to a pre-existing string. This makes constructors suitable for initialisation
of new string objects. In all other situations one should combine construction with assignment,
for example:<br>
`str_assign(&dest, str_acquire_chars(buff, n));`

### String Object Properties

Querying a property of a string object (like the length of the string via `str_len`) is a
cheap operation.

### Assigning, Moving, and Passing String Objects

C language does not allow for operator overloading, so this library provides a function
`str_assign` that takes a string object and assigns it to the destination object, freeing
any memory owned by the destination. It is generally recommended to use this function
everywhere outside object initialisation.

An existing object can be moved over to another location via `str_move` function.
The function resets the source object to `str_null` to guarantee the correct move semantics.
The value returned by `str_move` may be either used to initialise a new object, or
assigned to an existing object using `str_assign`.

An existing object can also be passed over to another location via `str_pass` function. The function
sets the source object to be a non-owning reference to the original string, otherwise the semantics
and usage is the same as `str_move`.

### String Composition and Generic Destination

String composition [functions](#string-composition) can write their results to different
destinations, depending on the _type_ of their `dest` parameter:

* `str*`: result is assigned to the string object;
* `int`: result is written to the file descriptor;
* `FILE*` result is written to the file stream.

The composition functions return 0 on success, or the value of `errno` as retrieved at the point
of failure (including `ENOMEM` on memory allocation error).

### Detailed Example

Just to make things more clear, here is the same code as in the example above, but with comments:
```C
// declare a variable and initialise it with an empty string; could also be declared as "str_auto"
// to avoid explicit call to str_free() below.
str s = str_null;

// join the given string literals around the separator (second parameter),
// storing the result in object "s" (first parameter); in this example we do not check
// the return values of the composition functions, thus ignoring memory allocation failures,
// which is probably not the best idea in general.
str_join(&s, str_lit(", "),
         str_lit("Here"),
         str_lit("there"),
         str_lit("and everywhere"));

// create a new string concatenating "s" and a literal; the function does not modify its
// destination object "s" before the result is computed, also freeing the destination
// before the assignment, so it is safe to use "s" as both a parameter and a destination.
// note: we pass a copy of the owning object "s" as the second parameter, and here it is
// safe to do so because this particular function does not store or release its arguments.
str_cat(&s, s, str_lit("..."));

// check that we have got the expected result
assert(str_eq(s, str_lit("Here, there, and everywhere...")));

// finally, free the memory allocated for the string
str_free(s);
```

There are some useful [code snippets](snippets.md) provided to assist with writing code using
this library.

## API brief

`typedef struct { ... } str;`<br>
The string object.

#### String Properties

`size_t str_len(const str s)`<br>
Returns the number of bytes in the string referenced by the object.

`const char* str_ptr(const str s)`<br>
Returns a pointer to the first byte of the string referenced by the object. The pointer is never NULL.

`const char* str_end(const str s)`<br>
Returns a pointer to the next byte past the end of the string referenced by the object.
The pointer is never NULL, but it is not guaranteed to point to any valid byte or location.
For C strings it points to the terminating null character. For any given string `s` the following
condition is always satisfied: `str_end(s) == str_ptr(s) + str_len(s)`.

`bool str_is_empty(const str s)`<br>
Returns "true" for empty strings.

`bool str_is_owner(const str s)`<br>
Returns "true" if the string object is the owner of the memory it references.

`bool str_is_ref(const str s)`<br>
Returns "true" if the string object does not own the memory it references.

#### String Construction

`str_null`<br>
Empty string constant.

`str str_lit(s)`<br>
Constructs a non-owning object from a string literal. Implemented as a macro.

`str str_ref(s)`<br>
Constructs a non-owning object from either a null-terminated C string, or another `str` object.
Implemented as a macro.

`str str_ref_chars(const char* const s, const size_t n)`<br>
Constructs a non-owning object referencing the given range of bytes.

`str str_acquire_chars(const char* const s, const size_t n)`<br>
Constructs an owning object for the specified range of bytes. The pointer `s` should be safe
to pass to `free(3)` function.

`str str_acquire(const char* const s)`<br>
Constructs an owning object from the given C string. The string should be safe to pass to
`free(3)` function.

`str str_move(str* const ps)`<br>
Saves the given object to a temporary, resets the source object to `str_null`, and then
returns the saved object.

`str str_pass(str* const ps)`<br>
Saves the given object to a temporary, sets the source object to be a non-owning reference to the
original string, and then returns the saved object.

#### String Deallocation

`void str_free(const str s)`<br>
Deallocates any memory held by the owning string object. No-op for references. After a call to
this function the string object is in unknown and unusable state.

String objects on the stack can also be declared as `str_auto` instead of `str` to deallocate
any memory held by the string when the variable goes out of scope.

#### String Modification

`void str_assign(str* const ps, const str s)`<br>
Assigns the object `s` to the object pointed to by `ps`. Any memory owned by the target
object is freed before the assignment.

`void str_clear(str* const ps)`<br>
Sets the target object to `str_null` after freeing any memory owned by the target.

`void str_swap(str* const s1, str* const s2)`<br>
Swaps two string objects.

`int str_from_file(str* const dest, const char* const file_name)`<br>
Reads the entire file (of up to 64MB by default, configurable via `STR_MAX_FILE_SIZE`) into
the destination string. Returns 0 on success, or the value of `errno` on error.

#### String Comparison

`int str_cmp(const str s1, const str s2)`<br>
Lexicographically compares the two string objects, with usual semantics.

`bool str_eq(const str s1, const str s2)`<br>
Returns "true" if the two strings match exactly.

`int str_cmp_ci(const str s1, const str s2)`<br>
Case-insensitive comparison of two strings, implemented using `strncasecmp(3)`.

`bool str_eq_ci(const str s1, const str s2`<br>
Returns "true" is the two strings match case-insensitively.

`bool str_has_prefix(const str s, const str prefix)`<br>
Tests if the given string `s` starts with the specified prefix.

`bool str_has_suffix(const str s, const str suffix)`<br>
Tests if the given string `s` ends with the specified suffix.

#### String Composition

`int str_cpy(dest, const str src)`<br>
Copies the source string referenced by `src` to the
[generic](#string-composition-and-generic-destination) destination `dest`. Returns 0 on success,
or the value of `errno` on failure.

`int str_cat_range(dest, const str* src, size_t count)`<br>
Concatenates `count` strings from the array starting at address `src`, and writes
the result to the [generic](#string-composition-and-generic-destination) destination `dest`.
Returns 0 on success, or the value of `errno` on failure.

`int str_cat(dest, ...)`<br>
Concatenates a variable list of `str` arguments, and writes the result to the
[generic](#string-composition-and-generic-destination) destination `dest`.
Returns 0 on success, or the value of `errno` on failure.

`int str_join_range(dest, const str sep, const str* src, size_t count)`<br>
Joins around `sep` the `count` strings from the array starting at address `src`, and writes
the result to the [generic](#string-composition-and-generic-destination) destination `dest`.
Returns 0 on success, or the value of `errno` on failure.

`int str_join(dest, const str sep, ...)`<br>
Joins a variable list of `str` arguments around `sep` delimiter, and writes the result to the
[generic](#string-composition-and-generic-destination) destination `dest`.
Returns 0 on success, or the value of `errno` on failure.

#### Searching and Sorting

`bool str_partition(const str src, const str patt, str* const prefix, str* const suffix)`<br>
Splits the string `src` on the first match of `patt`, assigning a reference to the part
of the string before the match to the `prefix` object, and the part after the match to the
`suffix` object. Returns `true` if a match has been found, or `false` otherwise, also
setting `prefix` to reference the entire `src` string, and clearing the `suffix` object.
Empty pattern `patt` never matches.

`void str_sort_range(const str_cmp_func cmp, str* const array, const size_t count)`<br>
Sorts the given array of `str` objects using the given comparison function. A number
of typically used comparison functions is also provided:
* `str_order_asc` (ascending sort)
* `str_order_desc` (descending sort)
* `str_order_asc_ci` (ascending case-insensitive sort)
* `str_order_desc_ci` (descending case-insensitive sort)

`const str* str_search_range(const str key, const str* const array, const size_t count)`<br>
Binary search for the given key. The input array must be sorted using `str_order_asc`.
Returns a pointer to the string matching the key, or NULL.

`size_t str_partition_range(bool (*pred)(const str), str* const array, const size_t count)`<br>
Reorders the string objects in the given range in such a way that all elements for which
the predicate `pred` returns "true" precede the elements for which predicate `pred`
returns "false". Returns the number of preceding objects.

`size_t str_unique_range(str* const array, const size_t count)`<br>
Reorders the string objects in the given range in such a way that there are two partitions:
one where each object is unique within the input range, and another partition with all the
remaining objects. The unique partition is stored at the beginning of the array, and is
sorted in ascending order, followed by the partition with all remaining objects.
Returns the number of unique objects.

#### UNICODE support

`for_each_codepoint(var_name, src_string)`<br>
A macro that expands to a loop iterating over the given string `src_string` (of type `str`) by UTF-32
code points. On each iteration the variable `var_name` (of type `char32_t`) is assigned
the value of the next valid UTF-32 code point from the source string. Upon exit from the loop the
variable has one on the following values:
* `CPI_END_OF_STRING`: the iteration has reached the end of source string;
* `CPI_ERR_INCOMPLETE_SEQ`: an incomplete byte sequence has been detected;
* `CPI_ERR_INVALID_ENCODING`: an invalid byte sequence has been detected.

The source string is expected to be encoded in the _current program locale_, as set by the most
recent call to `setlocale(3)`.

Usage pattern:
```c
#include <uchar.h>
...
str s = ...
...
char32_t c;	// variable to receive UTF-32 values on each iteration

for_each_codepoint(c, s)
{
	/* process c */
}

if(c != CPI_END_OF_STRING)
{
	/* handle error */
}
```

#### Tokeniser

Tokeniser interface provides functionality similar to `strtok(3)` function. The tokeniser
is fully re-entrant with no hidden state, and its input string is not modified while being
parsed.

##### Typical usage:
```C
// declare and initialise tokeniser state
str_tok_state state;

str_tok_init(&state, source_string, delimiter_set);

// object to receive tokens
str token = str_null;

// token iterator
while(str_tok(&token, &state))
{
    /* process "token" */
}
```

##### Tokeniser API

`void str_tok_init(str_tok_state* const state, const str src, const str delim_set)`<br>
Initialises tokeniser state with the given source string and delimiter set. The delimiter set
is treated as bytes, _not_ as UNICODE code points encoded in UTF-8.

`bool str_tok(str* const dest, str_tok_state* const state)`<br>
Retrieves the next token and stores it in the `dest` object. Returns `true` if the token has
been read, or `false` if the end of input has been reached. Retrieved token is always
a reference to a slice of the source string.

`void str_tok_delim(str_tok_state* const state, const str delim_set)`<br>
Changes the delimiter set associated with the given tokeniser state. The delimiter set is
treated as bytes, _not_ as UNICODE code points encoded in UTF-8.

## Tools

All the tools are located in `tools/` directory. Currently, there are the following tools:

* `file-to-str`: The script takes a file (text or binary) and a C variable name, and
writes to `stdout` C source code where the variable (of type `str`) is defined
and initialised with the content of the file.

* `gen-char-class`: Generates character classification functions that do the same as their
`isw*()` counterparts under the current locale as specified by `LC_ALL` environment variable.
Run `tools/gen-char-class --help` for further details, or `tools/gen-char-class --space`
to see an example of its output.

## Project Status
The library requires at least a C11 compiler. So far has been tested on Linux Mint versions from 19.3 to 22.0,
with `gcc` versions up to 13.2.0, and `clang` versions up to 18.1.3; it is also reported to work
on ALT Linux 9.1 for Elbrus, with `lcc` version 1.25.09.
