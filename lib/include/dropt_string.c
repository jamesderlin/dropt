#if defined _WIN32
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

#define IS_FINALIZED(ss) ((ss)->string == NULL)

#ifdef NDEBUG
    #define DEFAULT_STRINGSTREAM_BUFFER_SIZE 256
#else
    #define DEFAULT_STRINGSTREAM_BUFFER_SIZE 1
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
    size_t maxSize;       /* Size of the string buffer, including space for NUL. */
    size_t used;          /* Number of bytes used in the string buffer, excluding NUL. */
};
#endif


/** dropt_strdup
  *
  *     Duplicates a string.
  *
  * PARAMETERS:
  *     IN s : The strings to duplicate.
  *
  * RETURNS:
  *     The duplicated string.  The caller is responsible for calling
  *       free() on it when no longer needed.
  *     Returns NULL on error.
  */
dropt_char_t*
dropt_strdup(const dropt_char_t* s)
{
    dropt_char_t* copy;
    size_t n;
    assert(s != NULL);
    n = (dropt_strlen(s) + 1 /* NUL */) * sizeof *copy;
    copy = malloc(n);
    if (copy != NULL) { memcpy(copy, s, n); }
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
    if (s == t) { return 0; }

    while (1)
    {
        if (*s == '\0' && *t == '\0')
        {
            return 0;
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
}


#ifndef DROPT_NO_STRING_BUFFERS
/** dropt_vsnprintf
  *
  *     vsnprintf wrapper to provide ISO C99-compliant behavior.
  *
  * PARAMETERS:
  *     OUT s     : The destination buffer.  May be NULL if n is 0.
  *                 If non-NULL, always NUL-terminated.
  *     n         : The size of the destination buffer.
  *     IN format : printf-style format specifier.  Must not be NULL.
  *     IN args   : Arguments to insert into the formatted string.
  *
  * RETURNS:
  *     The number of characters that would be to the destination buffer if
  *       sufficiently large, excluding the NUL-terminator.
  *     Returns -1 on error.
  */
static int
dropt_vsnprintf(dropt_char_t* s, size_t n, const dropt_char_t* format, va_list args)
{
#if __STDC_VERSION__ >= 199901L || __GNUC__
    /* ISO C99-compliant.
     *
     * As far as I can tell, gcc's implementation of vsnprintf has always
     * matched the behavior required by the C99 standard.
     */
    return vsnprintf(s, n, format, args);

#elif defined _WIN32
    /* _vsntprintf and _vsnprintf_s on Windows don't have C99 semantics;
     * they return -1 if truncation occurs.
     */
    va_list argsCopy;
    int ret;

    assert(format != NULL);
    va_copy(argsCopy, args);

    ret = _vsctprintf(format, args);

    if (n != 0)
    {
        assert(s != NULL);

    #if _MSC_VER >= 1400
        (void) _vsntprintf_s(s, n, _TRUNCATE, format, argsCopy);
    #else
        /* This version doesn't necessarily NUL-terminate.  Sigh. */
        (void) _vsnprintf(s, n, format, argsCopy);
        s[n - 1] = '\0';
    #endif
    }

    return ret;

#else
    #error Unsupported platform.  dropt_vsnprintf unimplemented.
    return -1;
#endif
}


/** dropt_vaprintf
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
dropt_vaprintf(const dropt_char_t* format, va_list args)
{
    dropt_char_t* s = NULL;
    int len;
    va_list argsCopy;
    assert(format != NULL);
    va_copy(argsCopy, args);
    len = dropt_vsnprintf(NULL, 0, format, args);
    if (len >= 0)
    {
        size_t n = len + 1 /* NUL */;
        s = malloc(n * sizeof *s);
        if (s != NULL)
        {
            dropt_vsnprintf(s, n, format, argsCopy);
        }
    }
    va_end(argsCopy);

    return s;
}


/* See dropt_vaprintf. */
dropt_char_t*
dropt_aprintf(const dropt_char_t* format, ...)
{
    dropt_char_t* s;

    va_list args;
    va_start(args, format);
    s = dropt_vaprintf(format, args);
    va_end(args);

    return s;
}


/** dropt_ssopen
  *
  *     Constructs a new dropt_stringstream.
  *
  * RETURNS:
  *     An initialized dropt_stringstream.  The caller is responsible for
  *       calling dropt_ssclose() on it when no longer needed.
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
        ss->string = malloc(ss->maxSize * sizeof *ss->string);
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
  *     n         : The desired buffer size.
  *
  * RETURNS:
  *     The new size of the dropt_stringstream's buffer in dropt_char_t-s,
  *       including space for a terminating NUL.
  */
static size_t
dropt_ssresize(dropt_stringstream* ss, size_t n)
{
    assert(ss != NULL);
    if (!IS_FINALIZED(ss))
    {
        if (n > ss->maxSize)
        {
            dropt_char_t* p = realloc(ss->string, n * sizeof *ss->string);
            if (p != NULL)
            {
                ss->string = p;
                ss->maxSize = n;
            }
        }
        else
        {
            n = MAX(n, ss->used + 1 /* NUL */);
            realloc(ss->string, n * sizeof *ss->string);
            ss->maxSize = n;
        }
        assert(ss->maxSize > 0);
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
    if (!IS_FINALIZED(ss))
    {
        ss->string[0] = '\0';
        ss->used = 0;
        ss->maxSize = DEFAULT_STRINGSTREAM_BUFFER_SIZE;
        realloc(ss->string, ss->maxSize * sizeof *ss->string);
    }
}


/** dropt_ssfinalize
  *
  *     Finalizes a dropt_stringstream.  Except for dropt_ssclose(), no
  *     further operations may be performed on the dropt_stringstream.
  *
  * PARAMETERS:
  *     IN/OUT ss : The dropt_stringstream.
  *
  * RETURNS:
  *     The dropt_stringstream's string.  Note that the caller assumes
  *       ownership of the returned string.  The string remains valid after
  *       calling dropt_ssclose() on the stringstream, and the the caller
  *       is responsible for calling free() on the returned string when no
  *       longer needed.
  */
dropt_char_t*
dropt_ssfinalize(dropt_stringstream* ss)
{
    dropt_char_t* s;
    assert(ss != NULL);
    dropt_ssresize(ss, 0);
    s = ss->string;
    ss->string = NULL;
    ss->maxSize = 0;
    ss->used = 0;
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
    n = dropt_vsnprintf(NULL, 0, format, args);
    if (n > 0 && !IS_FINALIZED(ss))
    {
        size_t available = dropt_ssgetfreespace(ss);
        if ((unsigned int) n + 1 > available)
        {
#ifdef NDEBUG
            size_t newSize = MAX(ss->maxSize * 2, ss->maxSize + n);
#else
            size_t newSize = ss->maxSize + n;
#endif
            dropt_ssresize(ss, newSize);
            available = dropt_ssgetfreespace(ss);
        }
        assert(available > 0); /* Space always is reserved for NUL. */

        /* snprintf's family of functions return the number of characters
         * that would be output with a sufficiently large buffer, excluding
         * NUL.
         */
        n = dropt_vsnprintf(ss->string + ss->used, available, format, argsCopy);

#if 0
        /* Determine how many characters actually were written. */
        if ((unsigned int) n >= available) { n = available - 1; }
#else
        /* We couldn't allocate enough space. */
        if ((unsigned int) n >= available)
        {
            ss->string[ss->used] = '\0';
            n = -1;
        }
#endif
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
