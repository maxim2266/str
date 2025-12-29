# str: yet another string library for C language.

[![License: BSD 3 Clause](https://img.shields.io/badge/License-BSD_3--Clause-yellow.svg)](https://opensource.org/licenses/BSD-3-Clause)

_Note_: This is version 2 of the library, v1 is now in maintenance mode, and is available in this
repository under `v1` branch. The differences between v1 and v2 are summarised [here](changes-from-v1.md).

## Features

* Handles both C strings and binary data
* Explicit string ownership (though C cannot enforce that)
* String search and replace
* Support for typical i/o operations with strings
* Operations on sorted arrays of strings
* Basic locale-independent UTF-8 support
* Portable: compiles with `gcc` or `clang`, links with `libc` or `musl`

## Quick Start

```bash
git clone --recurse-submodules https://github.com/maxim2266/str.git
cd str
make        # or 'make test' to also run unit tests
```

Link the resulting static library `libstr.a` with your binary. All library definitions are in
the `str.h` header file.

## Code Example

```C
// declare a string object
str s = str_null;

// join string literals with a separator
str_join(&s, str_lit(", "),
         str_lit("Here"),
         str_lit("there"),
         str_lit("and everywhere"));

// append more content
str_concat(&s, s, str_lit("..."));

assert(str_eq(s, str_lit("Here, there, and everywhere...")));

// ensure the string contains valid UTF-8
str_to_valid_utf8(&s);

// write to a file stream
str_concat_to_stream(stdout, s, str_lit("\n"));

// free owned memory
str_free(s);
```

Complete API documentation is available [here](api.md).

## Overview

**Disclaimer:** _This is C, not Go or Rust. The language cannot enforce memory safety or ownership
rules—discipline is required to avoid leaks and access violations._

Each string is an opaque object of type `str`. Conceptually, it contains only a pointer and
a byte count (of type `size_t`), making it cheap to create, copy, and pass by value. A `str`
object can either own the memory it points to, or act as a reference to another string. Owning
objects must be explicitly freed with `str_free()`. Stack-allocated objects can be declared as
`str_auto` to deallocate owned memory automatically when leaving the function's scope.

Direct assignment (`str a = b;`) is correct **only** if the left-hand side is a non-owning
reference. For a generic and always safe assignment, use `str_assign()`. Every object must
be initialized before use. An object with all bits zero is a valid empty string; the library
provides `str_null` for this purpose, along with several string constructors.

Strings in this library are treated as immutable. They are not guaranteed to be null-terminated,
though all strings allocated by the library or created from C string literals do contain a
null terminator, which is not counted towards the string length.

Library functions take string parameters either by value (as `const str`) or via pointer
(`str*`). Parameters passed by value are never modified; it is safe to pass owning objects this
way. Functions that take a pointer to destination string guarantee the modification occurs at the
very end, avoiding self-assignment issues.

Functions that return a string object are called constructors. Constructors never allocate
memory themselves. There are two kinds:

1. **Acquiring constructors** that take ownership of pre-allocated memory (e.g., `str_acquire()`)
2. **Reference constructors** that create non-owning references (e.g., `str_ref()`)

Other functions that modify strings (and may allocate memory) do not return a new string;
they modify an existing string object via pointer.

Iterate over a string `s` as follows:

```C
for(const char* p = str_ptr(s); p < str_end(s); ++p) {
    // use p
}
```

Both `str_ptr()` and `str_end()` guarantee non-null pointers, even for empty strings, where
`str_ptr(s) == str_end(s)`.

### String Ownership Management
It is the user's responsibility to maintain the following:
* **Initialization Contract**: All `str` instances must be initialized before use via constructors,
`str_null`, or complete zeroing (for struct/array members)
* **Single-Owner Invariant**: Every heap-allocated string buffer has exactly one owning `str`
object at any point in program execution
* **Transfer Discipline**: Ownership transfer occurs only through designated API functions;
misuse risks double-frees or leaks
* **Lifetime Bounds**: Non-owning references are invalidated when their referent is freed

And finally, the good old C strings are still in the game, and they may (or should) be used where
appropriate.

## License
BSD-3-Clause

[rapidhash](https://github.com/Nicoshev/rapidhash): MIT license<br>
[mite](https://github.com/maxim2266/mite): BSD-3-Clause license
