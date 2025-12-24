It has been 5 years since the first commit to this library, and based on the feedback I have
received and my personal experience, I decided to rewrite the library to version 2. The major
differences introduced in this new version are:
* The functions in the library have been given more descriptive names instead of following the
traditional (but often obscure) C language abbreviated names;
* Ownership transfer operations are simplified down to just two: "acquire" and "reference";
* A surprising number of people reported difficulties adding `str.c` source file to their projects,
so now everything is compiled into a static library, although I have to admit I still do not
fully understand what the problem was;
* The original trick with C11's `_Generic` that allowed for polymorphic destination parameter is
now gone, because I agree with people who found it cool but unnecessary;
* Added character replacement functions;
* Added basic _locale-independent_ UTF-8 support;
* Iteration over Unicode codepoints can now be done using the provided `str_decode_utf8` function
instead of a specialised macro;
* The accomplishing tools are gone, as nobody seems to use them.
