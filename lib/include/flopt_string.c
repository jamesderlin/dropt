/* Compatibility junk for things that don't yet support ISO C99. */
#if defined _WIN32
    /* For _tcsncpy. */
    #define _CRT_SECURE_NO_DEPRECATE 1

    #include <tchar.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "flopt_string.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define IS_FINALIZED(ssP) ((ssP)->string == NULL)

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



struct flopt_stringstream
{
    TCHAR* string;  /* The string buffer. */
    size_t maxSize; /* Size of the string buffer, including space for NUL. */
    size_t used;    /* Number of bytes used in the string buffer, excluding NUL. */
};


/** flopt_strdup
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
TCHAR*
flopt_strdup(const TCHAR* s)
{
    TCHAR* copyP;
    size_t n;
    assert(s != NULL);
    n = tcslen(s) + 1 /* NUL */;
    copyP = malloc(n * sizeof *copyP);
    if (copyP != NULL) { tcsncpy(copyP, s, n); }
    return copyP;
}


/** flopt_stricmp
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
flopt_stricmp(const TCHAR* s, const TCHAR* t)
{
    if (s == t) { return 0; }

    while (1)
    {
        if (*s == '\0' && *t == '\0')
        {
            return 0;
        }
        else if (*s == *t || tclower(*s) == tclower(*t))
        {
            s++;
            t++;
        }
        else
        {
            return (tclower(*s) < tclower(*t))
                   ? -1
                   : +1;
        }
    }
}


/** flopt_vsnprintf
  *
  *     vsnprintf wrapper to provide ISO C99-compliant behavior.
  *
  * PARAMETERS:
  *     OUT s   : The destination buffer.  May be NULL if n is 0.
  *               If non-NULL, always NUL-terminated.
  *     n       : The size of the destination buffer.
  *     IN fmtP : printf-style format specifier.  Must not be NULL.
  *     IN args : Arguments to insert into the formatted string.
  *
  * RETURNS:
  *     The number of characters that would be to the destination buffer if
  *       sufficiently large, excluding the NUL-terminator.
  *     Returns -1 on error.
  */
static int
flopt_vsnprintf(TCHAR* s, size_t n, const TCHAR* fmtP, va_list args)
{
#if __STDC_VERSION__ >= 199901L || __GNUC__
    /* ISO C99-compliant.
     *
     * As far as I can tell, gcc's implementation of vsnprintf has always
     * matched the behavior required by the C99 standard.
     */
    return vsnprintf(s, n, fmtP, args);

#elif defined _WIN32
    /* _vsntprintf and _vsnprintf_s on Windows don't have C99 semantics;
     * they returns -1 if truncation occurs.
     */
    va_list argsCopy;
    int ret;

    assert(fmtP != NULL);
    va_copy(argsCopy, args);

    ret = _vsctprintf(fmtP, args);

    if (n != 0)
    {
        assert(s != NULL);

    #if _MSC_VER >= 1400
        (void) _vsntprintf_s(s, n, _TRUNCATE, fmtP, argsCopy);
    #else
        /* This version doesn't necessarily NUL-terminate.  Sigh. */
        (void) _vsnprintf(s, n, fmtP, argsCopy);
        s[n - 1] = '\0';
    #endif
    }

    return ret;

#else
    #error Unsupported platform.  flopt_vsnprintf unimplemented.
    return -1;
#endif
}


/** flopt_vaprintf
  *
  *     Allocates a formatted string with vprintf semantics.
  *
  * PARAMETERS:
  *     IN fmtP : printf-style format specifier.  Must not be NULL.
  *     IN args : Arguments to insert into the formatted string.
  *
  * RETURNS:
  *     The formatted string.  The caller is responsible for calling free()
  *       on it when no longer needed.
  *     Returns NULL on error.
  */
TCHAR*
flopt_vaprintf(const TCHAR* fmtP, va_list args)
{
    TCHAR* s = NULL;
    int len;
    va_list argsCopy;
    assert(fmtP != NULL);
    va_copy(argsCopy, args);
    len = flopt_vsnprintf(NULL, 0, fmtP, args);
    if (len >= 0)
    {
        /* Account for NUL. */
        size_t n = len + 1 /* NUL */;
        s = malloc(n * sizeof *s);
        if (s != NULL)
        {
            flopt_vsnprintf(s, n, fmtP, argsCopy);
        }
    }
    va_end(argsCopy);

    return s;
}


/* See flopt_vaprintf. */
TCHAR*
flopt_aprintf(const TCHAR* fmtP, ...)
{
    TCHAR* s;

    va_list args;
    va_start(args, fmtP);
    s = flopt_vaprintf(fmtP, args);
    va_end(args);

    return s;
}


/** flopt_ssopen
  *
  *     Constructs a new flopt_stringstream.
  *
  * RETURNS:
  *     An initialized flopt_stringstream.  The caller is responsible for
  *       calling flopt_ssclose() on it when no longer needed.
  *     Returns NULL on error.
  */
flopt_stringstream*
flopt_ssopen(void)
{
    flopt_stringstream* ssP = malloc(sizeof *ssP);
    if (ssP != NULL)
    {
        ssP->used = 0;
        ssP->maxSize = DEFAULT_STRINGSTREAM_BUFFER_SIZE;
        ssP->string = malloc(ssP->maxSize * sizeof *ssP->string);
        if (ssP->string == NULL)
        {
            free(ssP);
            ssP = NULL;
        }
    }
    return ssP;
}


/** flopt_ssclose
  *
  *     Destroys a flopt_stringstream.
  *
  * PARAMETERS:
  *     IN/OUT ssP : The flopt_stringstream.
  */
void
flopt_ssclose(flopt_stringstream* ssP)
{
    if (ssP != NULL)
    {
        free(ssP->string);
        free(ssP);
    }
}


/** flopt_ssgetfreespace
  *
  * RETURNS:
  *     The amount of free space in the flopt_stringstream's internal
  *       buffer, measured in TCHARs.  Space used for the NUL-terminator is
  *       considered free. (The amount of free space therefore is always
  *       positive.)
  */
static size_t
flopt_ssgetfreespace(const flopt_stringstream* ssP)
{
    assert(ssP != NULL);
    assert(ssP->maxSize > 0);
    assert(ssP->maxSize > ssP->used);
    return ssP->maxSize - ssP->used;
}


/** flopt_ssresize
  *
  *     Resizes a flopt_stringstream's internal buffer.  If the requested
  *     size is less than the amount of buffer already in use, the buffer
  *     will be shrunk to the minimum size necessary.
  *
  * PARAMETERS:
  *     IN/OUT ssP : The flopt_stringstream.
  *     n          : The desired buffer size.
  *
  * RETURNS:
  *     The new size of the flopt_stringstream's buffer in TCHARs,
  *       including space for a terminating NUL.
  */
static size_t
flopt_ssresize(flopt_stringstream* ssP, size_t n)
{
    assert(ssP != NULL);
    if (!IS_FINALIZED(ssP))
    {
        if (n > ssP->maxSize)
        {
            TCHAR* p = realloc(ssP->string, n * sizeof *ssP->string);
            if (p != NULL)
            {
                ssP->string = p;
                ssP->maxSize = n;
            }
        }
        else
        {
            n = MAX(n, ssP->used + 1 /* NUL */);
            realloc(ssP->string, n * sizeof *ssP->string);
            ssP->maxSize = n;
        }
        assert(ssP->maxSize > 0);
    }
    return ssP->maxSize;
}


/** flopt_ssclear
  *
  *     Clears and re-initializes a flopt_stringstream.
  *
  * PARAMETERS:
  *     IN/OUT ssP : The flopt_stringstream
  */
void
flopt_ssclear(flopt_stringstream* ssP)
{
    assert(ssP != NULL);
    if (!IS_FINALIZED(ssP))
    {
        ssP->string[0] = '\0';
        ssP->used = 0;
        ssP->maxSize = DEFAULT_STRINGSTREAM_BUFFER_SIZE;
        realloc(ssP->string, ssP->maxSize * sizeof *ssP->string);
    }
}


/** flopt_ssfinalize
  *
  *     Finalizes a flopt_stringstream.  Except for flopt_ssclose(), no
  *     further operations may be performed on the flopt_stringstream.
  *
  * PARAMETERS:
  *     IN/OUT ssP : The flopt_stringstream.
  *
  * RETURNS:
  *     The flopt_stringstream's string.  Note that the caller assumes
  *       ownership of the returned string.  The string remains valid after
  *       calling flopt_ssclose() on the stringstream, and the the caller
  *       is responsible for calling free() on the returned string when no
  *       longer needed.
  */
TCHAR*
flopt_ssfinalize(flopt_stringstream* ssP)
{
    TCHAR* s;
    assert(ssP != NULL);
    flopt_ssresize(ssP, 0);
    s = ssP->string;
    ssP->string = NULL;
    ssP->maxSize = 0;
    ssP->used = 0;
    return s;
}


/** flopt_ssgetstring
  *
  * PARAMETERS:
  *     IN ssP : The flopt_stringstream.
  *
  * RETURNS:
  *     The flopt_stringstream's string.  The returned string is valid only
  *       for the lifetime of the flopt_stringstream.
  */
const TCHAR*
flopt_ssgetstring(const flopt_stringstream* ssP)
{
    assert(ssP != NULL);
    return ssP->string;
}


/** flopt_vssprintf
  *
  *     Prints a formatted string with vprintf semantics to a
  *     flopt_stringstream.
  *
  * PARAMETERS:
  *     IN/OUT ssP : The flopt_stringstream.
  *     IN fmtP    : printf-style format specifier.  Must not be NULL.
  *     IN args    : Arguments to insert into the formatted string.
  *
  * RETURNS:
  *     The number of characters written to the flopt_stringstream.
  *     Returns a negative value on error.
  */
int
flopt_vssprintf(flopt_stringstream* ssP, const TCHAR* fmtP, va_list args)
{
    int n;
    va_list argsCopy;
    assert(ssP != NULL);
    assert(fmtP != NULL);
    va_copy(argsCopy, args);
    n = flopt_vsnprintf(NULL, 0, fmtP, args);
    if (n > 0 && !IS_FINALIZED(ssP))
    {
        size_t available = flopt_ssgetfreespace(ssP);
        if ((unsigned int) n + 1 > available)
        {
#ifdef NDEBUG
            size_t newSize = MAX(ssP->maxSize * 2, ssP->maxSize + n);
#else
            size_t newSize = ssP->maxSize + n;
#endif
            flopt_ssresize(ssP, newSize);
            available = flopt_ssgetfreespace(ssP);
        }
        assert(available > 0); /* Space always is reserved for NUL. */

        /* snprintf's family of functions return the number of characters
         * that would be output with a sufficiently large buffer, excluding
         * NUL.
         */
        n = flopt_vsnprintf(ssP->string + ssP->used, available, fmtP, argsCopy);

#if 0
        /* Determine how many characters actually were written. */
        if ((unsigned int) n >= available) { n = available - 1; }
#else
        /* We couldn't allocate enough space. */
        if ((unsigned int) n >= available)
        {
            ssP->string[ssP->used] = '\0';
            n = -1;
        }
#endif
        if (n > 0) { ssP->used += n; }
    }
    return n;
}


/* See flopt_vssprintf. */
int
flopt_ssprintf(flopt_stringstream* ssP, const TCHAR* fmtP, ...)
{
    int n;

    va_list args;
    va_start(args, fmtP);
    n = flopt_vssprintf(ssP, fmtP, args);
    va_end(args);

    return n;
}
