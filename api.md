## API Documentation

```C
typedef struct { ... } str;
```
Opaque structure representing a string.<br><br>

```C
str_null
```
Special value for an empty string.

### String Properties
```C
size_t str_len(const str s)
```
Returns the number of bytes in the given string.<br><br>

```C
const char* str_ptr(const str s)
```
Returns a pointer to the first byte of the string. The pointer is guaranteed to be non-null
for all strings, including the empty ones.<br><br>

```C
const char* str_end(const str s)
```
Returns a pointer to the first byte past the end of the string. The pointer is guaranteed
to be non-null for all strings, including the empty ones.<br><br>

```C
bool str_is_empty(const str s)
```
Returns `true` if the string is empty.<br><br>

```C
bool str_is_owner(const str s)
```
Returns `true` if the string owns the memory allocated for it on the heap.<br><br>

```C
bool str_is_ref(const str s)
```
Returns `true` if the string a reference to some other string.<br><br>

```C
uint64_t str_hash(const str s)
```
Calculates hash of the string. The hash is not cryptographically secure, but it is good enough
for applications like hash tables. Currently [rapidhash](https://github.com/Nicoshev/rapidhash)
is used as hashing algorithm.

### String Memory Control
```C
void str_free(const str s)
```
Deallocates memory associated with the string if the string is the owner, otherwise
it's no-op.<br><br>

```C
void str_clear(str* const s)
```
Deallocates memory associated with the string if the string is the owner, then makes
the string empty.<br><br>

```C
void str_assign(str* const dest, const str s)
```
String assignment. Only exists because C language does not allow for operator overloading.
Deallocates the destination string if it is the owner before the assignment.<br><br>

```C
str_auto
```
Type of stack-allocated string variable that ensures string memory released upon exit from
the enclosing function. Cannot be used for string variables residing not on the stack.

### String Constructors
String constructors are the only functions in this library that return a string object. Their
primary use is initialisation of string variables. Constructors themselves never allocate
any memory. The are two types of constructors: those who take ownership ("acquire") the memory
allocated for the string, and those who create a non-owning reference to another string.

```C
str_lit(s)
```
A macro that creates a reference to a C string literal. Has zero runtime cost with modern
compilers.<br><br>

```C
str str_ref(const str s)
```
Creates a reference to the given string.<br><br>

```C
str str_ref_mem(const char* const s, const size_t n)
```
Creates a string object that references the given memory area.<br><br>

```C
str str_ref_ptr(const char* const s)
```
Creates a string object that references the given C string.<br><br>

```C
str str_ref_slice(const str s, const size_t i, const size_t j)
```
Creates a reference to a slice of the given string starting from index `i` and up to
but not including index `j`.<br><br>

```C
str str_acquire(str* const ps)
```
Creates an object that owns the memory previously allocated for its argument string, also
converting the argument to a reference. If the argument is itself a reference, then the returned
object is a reference as well.<br><br>

```C
str str_acquire_mem(const char* const s, const size_t n)
```
Creates an object that owns the given memory region. If the region evaluates to an empty string
then the memory gets `free`'d, and the constructor returns `str_null`.<br><br>

```C
str str_acquire_ptr(const char* const s)
```
Creates an object that owns the given C string. If the C string evaluates to an empty string
then the memory gets `free`'d, and the constructor returns `str_null`.

### String Comparison
```C
int str_cmp(const str s1, const str s2)
```
`strcmp` for `str` type.<br><br>

```C
bool str_eq(const str s1, const str s2)
```
Returns `true` if the two strings are of the same length and byte-by-byte identical.<br><br>

```C
bool str_has_prefix(const str s, const str prefix)
```
Returns `true` if the string starts from the given prefix. Empty prefix is always present.<br><br>

```C
bool str_has_suffix(const str s, const str suffix)
```
Returns `true` if the string ends with the given suffix. Empty suffix is always present.

### Operations on Strings
```C
void str_swap(str* const s1, str* const s2)
```
Swaps the two string objects.<br><br>

```C
bool str_sprintf(str* const dest, const char* const fmt, ...)
```
`sprintf` the variable argument list according to the given format, and assign the result to
the destination string. Returns `true` if no errors were encountered during formatting.<br><br>

```C
void str_clone(str* const dest, const str s)
```
Allocates a copy of the given string.<br><br>

```C
str_concat(dest, ...)
```
Concatenates (variable number of) strings and assigns the result to the destination object.<br><br>

```C
void str_concat_array(str* const dest, const str* array, const size_t count)
```
Concatenates all strings from an array and assigns the result to the destination object.<br><br>

```C
str_join(dest, sep, ...)
```
Joins (variable number of) strings around a separator and assigns the result to the destination
object.<br><br>

```C
void str_join_array(str* const dest, const str sep, const str* array, size_t count)
```
Joins all strings from an array around a separator and assigns the result to the destination
object.

### Search
```C
size_t str_span_chars(const str s, const str charset)
```
Counts the number of initial bytes in the string `s` that belong to the given charset. If the
string contains only the bytes from charset then the length of the string is returned.<br><br>

```C
size_t str_span_nonmatching_chars(const str s, const str charset)
```
Counts the number of initial bytes in the string `s` that do not belong to the given charset. If
the string contains only the bytes not from charset then the length of the string is returned.<br><br>

```C
size_t str_span_until_substring(const str s, const str substr)
```
Counts the number of bytes in the string `s` before the first match of the
given substring. If no match is found then the length of the string is returned.

### Search & Replace
```C
size_t str_replace_substring(str* const s, const str patt, const str repl)
```
Replace `s` with a new string where every occurrence of `patt` is replaced with `repl`.
Returns the number of replacements made.<br><br>

```C
size_t str_replace_chars(str* const s, const str charset, const str repl)
```
Replaces `s` with a new string where every byte from the given charset is replaced with `repl`.
Returns the number of replacements made.<br><br>

```C
size_t str_replace_char_spans(str* const s, const str charset, const str repl)
```
Replaces `s` with a new string where every span of bytes from the given charset is
replaced with `repl`. Returns the number of replacements made.

### Unicode
```C
str_decode_result str_decode_utf8(const char* src, size_t len)
```
Decodes the first UTF-8 codepoint from the memory at `src`, inspecting no more than `len` bytes.
The returned type is defined as
```C
typedef struct {
    uint32_t status     : 2;   // status
    uint32_t num_bytes  : 3;   // bytes to advance (1-4, 0=stop)
    uint32_t utf8_len   : 3;   // UTF-8 sequence length (1-4)
    uint32_t codepoint  : 24;  // Unicode codepoint (0-0x10FFFF), 0 on error
} str_decode_result; // 32 bits total
```
where
* `status` is one of the predefined constants `STR_UTF8_OK`, `STR_UTF8_ERROR`,
or `STR_UTF8_INCOMPLETE`
* `num_bytes` is the number of bytes processed by the call (can be less than `utf8_len` on error),
or 0 if `len` = 0
* `utf8_len` is the number of bytes in the valid UTF-8 encoded codepoint
* `codepoint` is the [codepoint](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-2/#G25564)
value.<br><br>

```C
size_t str_count_codepoints(const str s)
```
Returns the number of UTF-8 codepoints in the string `s`. Invalid UTF-8 sequences are treated
as if substituted with `U+FFFD` symbol.<br><br>

```C
size_t str_to_valid_utf8(str* const dest)
```
Converts the string to a valid UTF-8 string, with all the invalid UTF-8 sequences replaced with
`U+FFFD` symbol.<br><br>

```C
size_t str_encode_codepoint(char* const p, uint32_t cp)
```
Encodes the given Unicode codepoint into UTF-8 byte sequence. For correct operation the buffer
pointed to by `p` must have space for at least 4 bytes.

### I/O functions
```C
int str_concat_array_to_stream(FILE* const stream, const str* src, const size_t count)
```
Writes an array of strings to a file stream. On success returns 0, on error closes the stream
and returns `errno`;<br><br>

```C
str_concat_to_stream(stream, ...)
```
Writes variable list of strings to a file stream. On success returns 0, on error closes the
stream and returns `errno`;<br><br>

```C
int str_concat_array_to_fd(const int fd, const str* src, const size_t count)
```
Writes an array of strings to a file descriptor. On success returns 0, on error closes the
stream and returns `errno`;<br><br>

```C
str_concat_to_fd(fd, ...)
```
Writes variable list of strings to a file descriptor. On success returns 0, on error closes
the stream and returns `errno`;<br><br>

```C
int str_read_all_file(str* const dest, const char* const file_name)
```
Reads the entire file into a string in one shot. The `file_name` must refer to either a regular
file, or a symlink that resolves to a regular file. The file size must not exceed 64Mb.
Returns 0 on success, or `errno` on failure.<br><br>

```C
int str_get_line(str* const dest, FILE* const stream, const int delim)
```
Reads the next line from a file stream into a string. Line ends with the given delimiter.
Returns 0 on success, or -1 on file end. On failure closes the stream and returns `errno`.<br><br>

### String Array Functions
```C
typedef int (*str_cmp_func)(const void*, const void*)
```
String comparison function type.<br><br>

```C
int str_order_asc(const void* const s1, const void* const s2)
int str_order_desc(const void* const s1, const void* const s2)
```
Actual comparison functions for ascending and descending sort.<br><br>

```C
void str_sort_array(const str_cmp_func cmp, const str* const array, const size_t count)
```
Sorts the given array using the provided comparison function. The strings within the array
are only moved around, they are not modified in any way.<br><br>

```C
size_t str_partition_array(bool (*pred)(const str), str* const array, const size_t count)
```
Moves all strings matching the predicate towards the front of the array. Returns the number of
matching strings. The strings within the array are only moved around, they are not not modified in
any way.<br><br>

```C
size_t str_unique_partition_array(str* const array, const size_t count)
```
Moves all unique strings towards the front of the array. Returns the number of unique strings.
Requires sorted array. The strings within the array are only moved around, they are not not
modified in any way.<br><br>
