dropt (deliberately rudimentary options)
========================================

dropt is yet another C library for parsing command-line options.

---

Goals
-----

Aren't there plenty of existing option-parsing libraries already?  Yes, there
are, but none of the ones that I had seen had the ease of use I wanted. The
primary design goal for dropt is to have **minimal barriers to entry**.
Specifically, this includes:

* **High portability.** dropt is written in standard C99 with compatibility
  code for most non-pathological C89 compilers. (`wchar_t` support for the
  help facility is supported only on Windows platforms, however.) dropt is
  written in C to make it easily consumable.  C++ wrappers also are provided
  as a convenience for C++ clients.
* (Hopefully) straightforward usage.
* No global variables.
* **Minimal dependencies.** dropt has no dependencies other than the standard
  C library.  dropt also lets clients opt-in to use only the specific features
  they want.
* **An unrestrictive license.** dropt uses the [zlib/libpng license] and may
  be statically linked with existing code freely.  This avoids some of the
  hassles that sometimes come with dynamically-linked libraries (such as
  ensuring that the compiler options used to build the library match those
  used for the main binary).
* **(Approximate) POSIX compatibility.** Existing applications that use
  POSIX-style options hopefully should be able to use dropt without breaking
  their command-line interface. (The notable difference is that dropt by
  default does not allow options to be concatenated with their arguments
  (e.g. `-oARGUMENT`).  POSIX also discourages this but makes exceptions for
  backwards compatibility reasons.  dropt requires that applications opt-in to
  this behavior.)
* **Flexibility.** dropt is designed to be callback-based.  This allows it to
  handle arbitrary arguments to command-line options.  All types are
  considered equal.  Even basic types such as integers or strings are handled
  by callbacks; dropt provides built-in handlers for them.  Exposing the
  handlers for basic types also allows custom handlers to leverage them via
  composition.

Secondary goals include being secure, performant, maintainable, and
extensible; minimizing memory usage; and gracefully failing if memory is
exhausted.  Code simplicity is prioritized over non-critical features.


Features
--------

dropt provides basic option-parsing features:

* GNU-style long options (e.g. `--option`).

* Grouping for short options (e.g. `-abc` is equivalent to `-a -b -c`).

* Automatically stops parsing when encountering a `--` token.  This allows
  programs to take arguments that start with `-` and not have them be treated
  as options.  Examples:

  * `rm -- -rf`
  * `some_numeric_program -- -123`

* Unambiguous syntax for arguments to options.  Arguments may always be
  specified with `=` (e.g. `--option=1`, `-x=1`).

* Overridable option values.  For example, `--option=1 --option=2` will use
  the value 2.  Boolean flags can be disabled by using `--flag=0` (`--flag` by
  itself is syntactic sugar for `--flag=1`).  This can be used to override
  options specified by shell aliases.  Note that clients that specify custom
  handlers can implement different behaviors if desired.

The implementation is intended to be minimalistic.  What dropt does *not* do:

* Localization.  dropt does not provide localization facilities, but it does
  try not to stand in the way of programs that want to localize their help
  text however they choose.

* Tokenizing a single command-line string into an `argv` array.  This is the
  shell's responsibility. (On Windows, where `WinMain()` does not receive a
  tokenized `argv` array, developers should use `CommandLineToArgvW` or should
  use the `__argv` global from Microsoft's C runtime library.)

* Command-line argument permutation.  dropt always expects options to come
  before non-options.

* Handling of different option styles (e.g. `/option`, `-option`). dropt is
  meant to encourage consistent command-line option interfaces.


Usage
-----

There's no formal documentation yet.  In the meantime, [`dropt_example.c`] or
[`droptxx_example.cpp`] should be suitable tutorials.


Download
--------

* [`dropt-2.0.1.zip`](https://github.com/jamesderlin/dropt/archive/v2.0.1.zip)
* [`dropt-2.0.1.tar.gz`](https://github.com/jamesderlin/dropt/archive/v2.0.1.tar.gz)


Version History
---------------
* 2.0.1 (2019-07-10)
  * Fixed contact information and other minor comment and documentation tweaks.
  * Minor code refactoring.
* 2.0.0 (2018-01-24)
  * Modified the signature for option handlers to accept a pointer to the
    matched `dropt_option` entry.  Custom option handlers will need to be
    adjusted.
  * Added a new field to `dropt_option` to store additional callback data.
    This warranted renaming the existing `handler_data` member.  Code that
    initialized `dropt_option` members by name will need to be adjusted.
  * Added a new `dropt_handle_const` handler that uses the new callback data
    to store predefined values.
  * Reformatted code and comments.
* 1.1.1 (2013-03-17)
  * Fixed a build issue with gcc with optimizations enabled.
  * Changed `dropt_error` to be an `unsigned int` type instead of an `enum`
    type to avoid potential comparison warnings with custom error values.
  * Fixed `dropt_misuse` to terminate in debug builds. (I accidentally
    disabled termination in dropt 1.1.0.)
  * Added a Makefile for clang.
  * Added an `INSTALL` file with build instructions.
  * Added a `droptxx_example.cpp` file as a C++ sample program.
  * Changed the directory layout a bit.
* 1.1.0 (2012-05-06)
  * For scalability, option lookup now uses binary search instead of a linear
    search.
  * Added some explicit safeguards against integer overflow attacks.
    (Previously overflow was safely handled implicitly, but now favor
    defensive paranoia.)
  * Fixed a null pointer dereference if no handler data was specified for
    `dropt_handle_verbose_bool`.
  * In `test_dropt.c`, fixed a sign-extension bug and a `printf` format
    specifier mismatch.
  * Made some other minor style adjustments.
* 1.0.4 (2010-09-12)
  * The `DROPT_HANDLER_DECL` macro has been replaced with a
    `dropt_option_handler_decl` `typedef`.  I apologize for breaking
    compatibility in a minor version update, but in this case the breakage
    should be minor, and it should be trivial to fix sites that used the old
    macro:
    * Replace `DROPT_HANDLER_DECL(func);` with `dropt_option_handler_decl
      func;`
    * Alternatively, re-create the macro: `#define DROPT_HANDLER_DECL(func)
      dropt_option_handler_decl func`
* 1.0.3 (2010-08-08)
  * `dropt_handle_bool` and `dropt_handle_verbose_bool` now return
    `dropt_error_mismatch` instead of `dropt_error_insufficient_arguments` if
    given empty string arguments.
  * The error messages from `dropt_default_error_handler` now consistently do
    not end with periods.
* 1.0.2 (2010-07-28)
  * `dropt_handle_bool` and `dropt_handle_verbose_bool` now return
    `dropt_error_mismatch` instead of `dropt_error_overflow` if given very
    large numbers.
  * Fixed the compilation of `test_dropt.c` when building with `_UNICODE=1`.
* 1.0.1 (2010-07-03)
  * Calling `dropt_ssgetstring` on a newly-opened `dropt_stringstream`
    returned garbage.  Fixed.  This would have affected only clients that used
    `dropt_stringstream` directly.
  * The `MIN` and `MAX` macros in `dropt_string.c` are now conditionally
    defined.
  * `dropt_safe_malloc` and `dropt_safe_realloc` are now publicly declared in
    `dropt_string.h`.
  * Renamed `DROPT_MISUSE_PANIC` to `DROPT_MISUSE` since it's non-fatal in
    non-debug builds.
  * `DEFAULT_STRINGSTREAM_BUFFER_SIZE` is now an `enum` constant instead of a
    preprocessor macro.
  * Modified `test_dropt.c`.  Now tries harder to report line numbers on
    failure, and more tests continue executing upon failure.  Added a few more
    tests for `dropt_stringstream` functions.  Rewrote one of the sample
    option handler callbacks to be more general.
* 1.0 (2010-06-19)
  * Initial release.

Questions?  Comments?  Bugs?  I welcome feedback. [Contact me].

Copyright © 2010–2019 James D. Lin
<http://www.taenarum.com/software/>


[zlib/libpng license]: http://opensource.org/licenses/Zlib
[`dropt_example.c`]: https://github.com/jamesderlin/dropt/blob/master/dropt_example.c
[`droptxx_example.cpp`]: https://github.com/jamesderlin/dropt/blob/master/droptxx_example.cpp
[Contact me]: http://www.taenarum.com/contact.html
