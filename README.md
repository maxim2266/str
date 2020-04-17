# str: yet another string library for C language.

[![License: BSD 3 Clause](https://img.shields.io/badge/License-BSD_3--Clause-yellow.svg)](https://opensource.org/licenses/BSD-3-Clause)

## Motivation

Bored with developing the same functionality over and over again, unsatisfied
with existing libraries, so decided to make the right one, once and forever. 🙂

## Features

* Handles both C and binary strings;
* Light-weight references to strings: cheap to create, copy, or pass by value;
* Support for copy and move semantics, although not enforceable by the C language;
* String composition targeting memory, file descriptors, or file streams;

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

Composing strings and writing to a file:

```C
FILE* const stream = fopen(...);

int err = str_join(stream, str_lit(", "),
                   str_lit("Here"),
                   str_lit("there"),
                   str_lit("and everywhere..."));

if(err != 0) { /* handle the error */ }
```

## User Guide

_**Disclaimer:** This is the good old C language, not Rust, so nothing can be enforced
on the language level, and certain programming discipline is required to make sure
there is no corrupt or leaked memory resulting from using this library._

A string is represented by the type `str` that maintains a pointer to some memory containing
the actual string. Objects of type `str` are small enough (a struct of a `const char*` and a `size_t`)
to be cheap to create, copy (pass by value), and move. The `str` structure should be
treated as opaque (i.e., do not attempt to directly access or modify the fields in this structure).
The strings are assumed to be immutable, like those in Java or Go, but only by means of `const char*`
pointers, so it is actually possible to write to such a string, although the required type
cast to `char*` offers at least some (mostly psychological) protection from modifying a string
by mistake.

This library focusses only on handling strings, not gradually composing them like
[StringBuffer](https://docs.oracle.com/javase/7/docs/api/java/lang/StringBuffer.html)
class in Java.

All string objects must be initialised. Uninitialised objects are likely to cause
undefined behaviour. Use the provided constructors, or `str_null` for empty strings.

There are two kinds of `str` objects: those actually owning the memory they point to, and
non-owning references. This property can be queried using `str_is_owner` and `str_is_ref`
functions, otherwise such objects are indistinguishable.

Non-owning string objects are safe to copy and assign to each other, as long as the memory
they refer to is valid. They do not need to be freed. `str_free` is a no-op for reference
objects. A reference object can be cheaply created from a C string, a string literal,
or just a range of bytes.

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
some point, either directly by using `str_free` function, or indirectly by assignment from
`str_assign` or a similar function.
* An owning object can be passed over to another location by using `str_move` function. The
function resets its source object to an empty string.

It is technically possible to create a reference to a string that is not
null-terminated. The library accepts strings without null-terminators, but every new string
allocated by the library is guaranteed to be null-terminated.

### String Construction

A string object can be constructed form any C string, string literal, or a range of bytes.
The provided constructors are computationally cheap to apply. Depending on the constructor,
the new object can either own the actual string it refers to, or be a non-owning reference.
Constructors themselves do not allocate any memory.

### String Object Properties

Querying a property of a string object (like the length of the string via `str_len`) is a
cheap operation.

### Assigning and Moving String Objects

C language does not allow for operator overloading, so this library provides a function
`str_assign` that takes a string object and assigns it to the destination object, freeing
any memory owned by the destination. It is generally recommended to use this function
everywhere outside object initialisation.

An existing object can be moved over to another location via `str_move` function.
The function resets the source object to `str_null` to guarantee the correct move semantics.
The value returned by `str_move` may be either used to initialise a new object, or
assigned to an existing object using `str_assign`.

### String Composition and Generic Destination

String composition [functions](#string-composition) can write their results to different
destinations, depending on the _type_ of their `dest` parameter:

* `str*`: result is written to allocated memory and the reference to it is stored in the string object;
* `int`: result is written to the file descriptor;
* `FILE*` result is written to the file stream.

When writing to a file descriptor or a stream, the composition functions return integer 0
on success, or the value of `errno` as retrieved at the point of failure.

### Detailed Example

Just to make things more clear, here is the same code as in the example above, but with comments:
```C
// declare a variable and initialise it with an empty string
str s = str_null;

// join the given string literals around the separator (second parameter),
// storing the result in object "s" (first parameter)
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

## API brief

`typedef struct { ... } str;`<br>
The string object.

`str_null`<br>
Empty string constant.

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

`str str_lit(s)`<br>
Constructs a non-owning object from a string literal. Implemented as a macro.

`str str_ref(s)`<br>
Constructs a non-owning object from either a null-terminated C string, or another `str` object.
Implemented as a macro.

`str str_ref_chars(const char* const s, const size_t n)`<br>
Constructs a non-owning object referencing the given range of bytes.

`str str_acquire_chars(const char* const s, size_t n)`<br>
Constructs an owning object for the specified range of bytes. The range should be safe
to pass to `free(3)` function.

`str str_acquire(const char* const s)`<br>
Constructs an owning object from the given C string. The string should be safe to pass to
`free(3)` function.

#### String Modification

`void str_assign(str* const ps, const str s)`<br>
Assigns the object `s` to the object pointed to by `ps`. Any memory owned by the target
object is freed before the assignment.

`str str_move(str* const ps)`<br>
Saves the given object to a temporary, resets the source object to `str_null`, and then
returns the saved object.

`void str_clear(str* const ps)`<br>
Sets the target object to `str_null` after freeing any memory owned by the target.

`void str_swap(str* const s1, str* const s2)`<br>
Swaps two string objects.

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

`str_cpy(dest, const str src)`<br>
Copies the source string referenced by `src`
to the [generic](#string-composition-and-generic-destination) destination `dest`.

`str_cat_range(dest, const str* src, size_t count)`<br>
Concatenates `count` strings from the array starting at address `src`, and writes
the result to the [generic](#string-composition-and-generic-destination) destination `dest`.

`str_cat(dest, ...)`<br>
Concatenates a variable list of `str` arguments, and writes the result to the
[generic](#string-composition-and-generic-destination) destination `dest`.

`str_join_range(dest, const str sep, const str* src, size_t count)`<br>
Joins around `sep` the `count` strings from the array starting at address `src`, and writes
the result to the [generic](#string-composition-and-generic-destination) destination `dest`.

`str_join(dest, sep, ...)`<br>
Joins around `sep` the variable list of `str` arguments, and writes the result to the
[generic](#string-composition-and-generic-destination) destination `dest`.

#### Sorting and Searching

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
sorted in ascending order, followed by the partition with remaining objects.
Returns the number of unique objects.

#### Memory Management

`void str_free(const str s)`<br>
Release the memory referenced by the owning object; no-op for references.

By default the library uses `malloc(3)` for memory allocations, and calls `abort(3)`
if the allocation fails. This behaviour can be changed by compiling the library with
the preprocessor symbol `STR_EXT_ALLOC` defined, and providing the following functions:

`void* str_mem_alloc(size_t)`<br>
Allocates memory like `malloc(3)` does, also handling out-of-memory situations. The library
does _not_ check the returned pointer for NULL.

`void str_mem_free(void*)`<br>
Equivalent of `free(3)` function.

### Status
The library requires at least a C11 compiler. So far has been tested on Linux Mint 19.3
with `gcc` version 7.5.0 and `clang` version 6.0.0.
