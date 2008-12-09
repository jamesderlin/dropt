/** dropt_string.c
  *
  *     String routines for dropt.
  *
  * Copyright (c) 2006-2008 James D. Lin <jameslin@csua.berkeley.edu>
  *
  * The latest version of this file can be downloaded from:
  * <http://www.taenarum.com/software/dropt/>
  *
  * This software is provided 'as-is', without any express or implied
  * warranty.  In no event will the authors be held liable for any damages
  * arising from the use of this software.
  *
  * Permission is granted to anyone to use this software for any purpose,
  * including commercial applications, and to alter it and redistribute it
  * freely, subject to the following restrictions:
  *
  * 1. The origin of this software must not be misrepresented; you must not
  *    claim that you wrote the original software. If you use this software
  *    in a product, an acknowledgment in the product documentation would be
  *    appreciated but is not required.
  *
  * 2. Altered source versions must be plainly marked as such, and must not be
  *    misrepresented as being the original software.
  *
  * 3. This notice may not be removed or altered from any source distribution.
  */

#ifdef _WIN32
    #include <tchar.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <stdio.h>
#include <assert.h>

#include "dropt_string.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define ARRAY_LENGTH(array) (sizeof (array) / sizeof (array)[0])

#ifdef DROPT_DEBUG_STRING_BUFFERS
    #define DEFAULT_STRINGSTREAM_BUFFER_SIZE 1
#else
    #define DEFAULT_STRINGSTREAM_BUFFER_SIZE 256
#endif

/* Compatibility junk for things that don't yet support ISO C99. */
#if __STDC_VERSION__ < 199901L
    #ifdef _WIN32
        #ifndef va_copy
            #define va_copy(dest, src) (dest = (src))
        #endif
    #else
        #ifndef va_copy
            #error Unsupported platform.  va_copy is not defined.
        #endif
    #endif
#endif



#ifndef DROPT_NO_STRING_BUFFERS
struct dropt_stringstream
{
    dropt_char_t* string; /* The string buffer. */
    size_t maxSize;       /* Size of the string buffer, in dropt_char_t-s, including space for NUL. */
    size_t used;          /* Number of elements used in the string buffer, excluding NUL. */
};
#endif


/** dropt_safe_malloc
  *
  *     A version of malloc that checks for integer overflow.
  *
  * PARAMETERS:
  *     numElements : The number of elements to allocate.
  *     elementSize : The size of each of element, in bytes.
  *
  * RETURNS:
  *     A pointer to the allocated memory, or NULL on failure.
  */
#define dropt_safe_malloc(numElements, elementSize) \
    dropt_safe_realloc(NULL, numElements, elementSize)


/** dropt_safe_realloc
  *
  *     Wrapper around realloc to check for integer overflow.
  *
  * PARAMETERS:
  *     IN/OUT p    : A pointer to the memory block to resize.
  *                   If NULL, a new memory block of the specified size
  *                     will be allocated.
  *     numElements : The number of elements to allocate.
  *     elementSize : The size of each of element, in bytes.
  *
  * RETURNS:
  *     A pointer to the allocated memory, or NULL on failure.
  */
static void*
dropt_safe_realloc(void* p, size_t numElements, size_t elementSize)
{
    size_t numBytes = numElements * elementSize;
    if (numBytes / elementSize != numElements)
    {
        /* Overflow. */
        return NULL;
    }

    return realloc(p, numBytes);
}


/** dropt_strdup
  *
  *     Duplicates a string.
  *
  * PARAMETERS:
  *     IN s : The string to duplicate.
  *
  * RETURNS:
  *     The duplicated string.  The caller is responsible for calling
  *       free() on it when no longer needed.
  *     Returns NULL on error.
  */
dropt_char_t*
dropt_strdup(const dropt_char_t* s)
{
    return dropt_strndup(s, (size_t) -1);
}


/** dropt_strndup
  *
  *     Duplicates the first n characters of a string.
  *
  * PARAMETERS:
  *     IN s : The string to duplicate.
  *     n    : The number of dropt_char_t-s to copy, excluding the
  *              NUL-terminator.
  *            Pass -1 to copy the entire string.
  *
  * RETURNS:
  *     The duplicated string.  The caller is responsible for calling
  *       free() on it when no longer needed.
  *     Returns NULL on error.
  */
dropt_char_t*
dropt_strndup(const dropt_char_t* s, size_t n)
{
    dropt_char_t* copy;
    size_t len;

    assert(s != NULL);

    len = dropt_strlen(s);
    n = (n == (size_t) -1) ? len : MIN(n, len);

    copy = dropt_safe_malloc(n + 1 /* NUL */, sizeof *copy);
    if (copy != NULL)
    {
        memcpy(copy, s, n * sizeof *copy);
        copy[n] = '\0';
    }

    return copy;
}


/** dropt_stricmp
  *
  *     Compares two strings ignoring case differences.  Not recommended
  *       for non-ASCII strings.
  *
  * PARAMETERS:
  *     IN s, t : The strings to compare.
  *
  * RETURNS:
  *     0 if the strings are equivalent,
  *     < 0 if s is lexically less than t,
  *     > 0 if s is lexically greater than t.
  */
int
dropt_stricmp(const dropt_char_t* s, const dropt_char_t* t)
{
    assert(s != NULL);
    assert(t != NULL);
    return dropt_strnicmp(s, t, dropt_strlen(s) + 1 /* NUL */);
}


/** dropt_strnicmp
  *
  *     Compares the first n characters of two strings, ignoring case
  *       differences.  Not recommended for non-ASCII strings.
  *
  * PARAMETERS:
  *     IN s, t : The strings to compare.
  *     n       : The maximum number of dropt_char_t-s to compare.
  *
  * RETURNS:
  *     0 if the strings are equivalent,
  *     < 0 if s is lexically less than t,
  *     > 0 if s is lexically greater than t.
  */
int
dropt_strnicmp(const dropt_char_t* s, const dropt_char_t* t, size_t n)
{
    assert(s != NULL);
    assert(t != NULL);

    if (s == t) { return 0; }

    while (n--)
    {
        if (*s == '\0' && *t == '\0')
        {
            break;
        }
        else if (*s == *t || dropt_tolower(*s) == dropt_tolower(*t))
        {
            s++;
            t++;
        }
        else
        {
            return (dropt_tolower(*s) < dropt_tolower(*t))
                   ? -1
                   : +1;
        }
    }

    return 0;
}


#ifndef DROPT_NO_STRING_BUFFERS
/** dropt_vsnprintf
  *
  *     vsnprintf wrapper to provide ISO C99-compliant behavior.
  *
  * PARAMETERS:
  *     OUT s     : The destination buffer.  May be NULL if n is 0.
  *                 If non-NULL, always NUL-terminated.
  *     n         : The size of the destination buffer, measured in
  *                   dropt_char_t-s.
  *     IN format : printf-style format specifier.  Must not be NULL.
  *     IN args   : Arguments to insert into the formatted string.
  *
  * RETURNS:
  *     The number of characters that would be to the destination buffer if
  *       sufficiently large, excluding the NUL-terminator.
  *     Returns -1 on error.
  */
int
dropt_vsnprintf(dropt_char_t* s, size_t n, const dropt_char_t* format, va_list args)
{
#if __STDC_VERSION__ >= 199901L || __GNUC__
    /* ISO C99-compliant.
     *
     * As far as I can tell, gcc's implementation of vsnprintf has always
     * matched the behavior required by the C99 standard.
     */
    assert(format != NULL);
    return vsnprintf(s, n, format, args);
#elif defined _WIN32
    /* _vsntprintf and _vsnprintf_s on Windows don't have C99 semantics;
     * they return -1 if truncation occurs.
     */
    va_list argsCopy;
    int ret;

    assert(format != NULL);

    va_copy(argsCopy, args);
    ret = _vsctprintf(format, argsCopy);
    va_end(argsCopy);

    if (n != 0)
    {
        assert(s != NULL);

    #if _MSC_VER >= 1400
        (void) _vsntprintf_s(s, n, _TRUNCATE, format, args);
    #else
        /* This version doesn't necessarily NUL-terminate.  Sigh. */
        (void) _vsnprintf(s, n, format, args);
        s[n - 1] = '\0';
    #endif
    }

    return ret;

#else
    #error Unsupported platform.  dropt_vsnprintf unimplemented.
    return -1;
#endif
}


/* See dropt_vsnprintf. */
int
dropt_snprintf(dropt_char_t* s, size_t n, const dropt_char_t* format, ...)
{
    int ret;
    va_list args;
    va_start(args, format);
    ret = dropt_vsnprintf(s, n, format, args);
    va_end(args);
    return ret;
}


/** dropt_vasprintf
  *
  *     Allocates a formatted string with vprintf semantics.
  *
  * PARAMETERS:
  *     IN format : printf-style format specifier.  Must not be NULL.
  *     IN args   : Arguments to insert into the formatted string.
  *
  * RETURNS:
  *     The formatted string.  The caller is responsible for calling free()
  *       on it when no longer needed.
  *     Returns NULL on error.
  */
dropt_char_t*
dropt_vasprintf(const dropt_char_t* format, va_list args)
{
    dropt_char_t* s = NULL;
    int len;
    va_list argsCopy;
    assert(format != NULL);

    va_copy(argsCopy, args);
    len = dropt_vsnprintf(NULL, 0, format, argsCopy);
    va_end(argsCopy);

    if (len >= 0)
    {
        size_t n = len + 1 /* NUL */;
        s = dropt_safe_malloc(n, sizeof *s);
        if (s != NULL)
        {
            dropt_vsnprintf(s, n, format, args);
        }
    }

    return s;
}


/* See dropt_vasprintf. */
dropt_char_t*
dropt_asprintf(const dropt_char_t* format, ...)
{
    dropt_char_t* s;

    va_list args;
    va_start(args, format);
    s = dropt_vasprintf(format, args);
    va_end(args);

    return s;
}


/** dropt_ssopen
  *
  *     Constructs a new dropt_stringstream.
  *
  * RETURNS:
  *     An initialized dropt_stringstream.  The caller is responsible for
  *       calling either dropt_ssclose() or dropt_ssfinalize() on it when
  *       no longer needed.
  *     Returns NULL on error.
  */
dropt_stringstream*
dropt_ssopen(void)
{
    dropt_stringstream* ss = malloc(sizeof *ss);
    if (ss != NULL)
    {
        ss->used = 0;
        ss->maxSize = DEFAULT_STRINGSTREAM_BUFFER_SIZE;
        ss->string = dropt_safe_malloc(ss->maxSize, sizeof *ss->string);
        if (ss->string == NULL)
        {
            free(ss);
            ss = NULL;
        }
    }
    return ss;
}


/** dropt_ssclose
  *
  *     Destroys a dropt_stringstream.
  *
  * PARAMETERS:
  *     IN/OUT ss : The dropt_stringstream.
  */
void
dropt_ssclose(dropt_stringstream* ss)
{
    if (ss != NULL)
    {
        free(ss->string);
        free(ss);
    }
}


/** dropt_ssgetfreespace
  *
  * RETURNS:
  *     The amount of free space in the dropt_stringstream's internal
  *       buffer, measured in dropt_char_t-s.  Space used for the
  *       NUL-terminator is considered free. (The amount of free space
  *       therefore is always positive.)
  */
static size_t
dropt_ssgetfreespace(const dropt_stringstream* ss)
{
    assert(ss != NULL);
    assert(ss->maxSize > 0);
    assert(ss->maxSize > ss->used);
    return ss->maxSize - ss->used;
}


/** dropt_ssresize
  *
  *     Resizes a dropt_stringstream's internal buffer.  If the requested
  *     size is less than the amount of buffer already in use, the buffer
  *     will be shrunk to the minimum size necessary.
  *
  * PARAMETERS:
  *     IN/OUT ss : The dropt_stringstream.
  *     n         : The desired buffer size, in dropt_char_t-s.
  *
  * RETURNS:
  *     The new size of the dropt_stringstream's buffer in dropt_char_t-s,
  *       including space for a terminating NUL.
  */
static size_t
dropt_ssresize(dropt_stringstream* ss, size_t n)
{
    assert(ss != NULL);

    /* Don't allow shrinking if it will truncate the string. */
    if (n < ss->maxSize) { n = MAX(n, ss->used + 1 /* NUL */); }

    if (n != ss->maxSize)
    {
        dropt_char_t* p = dropt_safe_realloc(ss->string, n, sizeof *ss->string);
        if (p != NULL)
        {
            ss->string = p;
            ss->maxSize = n;
            assert(ss->maxSize > 0);
         }
    }
    return ss->maxSize;
}


/** dropt_ssclear
  *
  *     Clears and re-initializes a dropt_stringstream.
  *
  * PARAMETERS:
  *     IN/OUT ss : The dropt_stringstream
  */
void
dropt_ssclear(dropt_stringstream* ss)
{
    assert(ss != NULL);

    ss->string[0] = '\0';
    ss->used = 0;

    dropt_ssresize(ss, DEFAULT_STRINGSTREAM_BUFFER_SIZE);
}


/** dropt_ssfinalize
  *
  *     Finalizes a dropt_stringstream; returns the contained string and
  *     destroys the dropt_stringstream.
  *
  * PARAMETERS:
  *     IN/OUT ss : The dropt_stringstream.
  *
  * RETURNS:
  *     The dropt_stringstream's string.  Note that the caller assumes
  *       ownership of the returned string and is responsible for calling
  *       free() on it when no longer needed.
  */
dropt_char_t*
dropt_ssfinalize(dropt_stringstream* ss)
{
    dropt_char_t* s;
    assert(ss != NULL);

    /* Shrink to fit. */
    dropt_ssresize(ss, 0);

    s = ss->string;
    ss->string = NULL;

    dropt_ssclose(ss);

    return s;
}


/** dropt_ssgetstring
  *
  * PARAMETERS:
  *     IN ss : The dropt_stringstream.
  *
  * RETURNS:
  *     The dropt_stringstream's string.  The returned string will no
  *       longer be valid if further operations are performed on the
  *       dropt_stringstream or if the dropt_stringstream is closed.
  */
const dropt_char_t*
dropt_ssgetstring(const dropt_stringstream* ss)
{
    assert(ss != NULL);
    return ss->string;
}


/** dropt_vssprintf
  *
  *     Prints a formatted string with vprintf semantics to a
  *     dropt_stringstream.
  *
  * PARAMETERS:
  *     IN/OUT ss : The dropt_stringstream.
  *     IN format : printf-style format specifier.  Must not be NULL.
  *     IN args   : Arguments to insert into the formatted string.
  *
  * RETURNS:
  *     The number of characters written to the dropt_stringstream.
  *     Returns a negative value on error.
  */
int
dropt_vssprintf(dropt_stringstream* ss, const dropt_char_t* format, va_list args)
{
    int n;
    va_list argsCopy;
    assert(ss != NULL);
    assert(format != NULL);

    va_copy(argsCopy, args);
    n = dropt_vsnprintf(NULL, 0, format, argsCopy);
    va_end(argsCopy);

    if (n > 0)
    {
        size_t available = dropt_ssgetfreespace(ss);
        if ((unsigned int) n + 1 > available)
        {
#ifdef DROPT_DEBUG_STRING_BUFFERS
            size_t newSize = ss->maxSize + n;
#else
            size_t newSize = MAX(ss->maxSize * 2, ss->maxSize + n);
#endif
            dropt_ssresize(ss, newSize);
            available = dropt_ssgetfreespace(ss);
        }
        assert(available > 0); /* Space always is reserved for NUL. */

        /* snprintf's family of functions return the number of characters
         * that would be output with a sufficiently large buffer, excluding
         * NUL.
         */
        n = dropt_vsnprintf(ss->string + ss->used, available, format, args);

        /* We couldn't allocate enough space. */
        if ((unsigned int) n >= available)
        {
            ss->string[ss->used] = '\0';
            n = -1;
        }

        if (n > 0) { ss->used += n; }
    }
    return n;
}


/* See dropt_vssprintf. */
int
dropt_ssprintf(dropt_stringstream* ss, const dropt_char_t* format, ...)
{
    int n;

    va_list args;
    va_start(args, format);
    n = dropt_vssprintf(ss, format, args);
    va_end(args);

    return n;
}
#endif /* DROPT_NO_STRING_BUFFERS */
