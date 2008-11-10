/** test_dropt.c
  *
  *     Unit tests for dropt.
  *
  * Copyright (C) 2007-2008 James D. Lin
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
  * 2. Altered source versions must be plainly marked as such, and must not be
  *    misrepresented as being the original software.
  * 3. This notice may not be removed or altered from any source distribution.
  */

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <float.h>
#include <assert.h>

#include "dropt.h"
#include "dropt_string.h"

/* Compatibility junk. */
#if defined _UNICODE && defined _WIN32
    #define T(s) L ## s

    #define ftprintf fwprintf
    #define fputts fputws
    #define fputtc fputwc

    /* swprintf isn't quite equivalent to snprintf, but it's good enough
     * for our purposes.
     */
    #define sntprintf swprintf
    #define stscanf swscanf

    #define istdigit iswdigit
#else
    #define T(s) s

    #define ftprintf fprintf
    #define fputts fputs
    #define fputtc fputc

    #define sntprintf snprintf
    #define stscanf sscanf

    #define istdigit isdigit
#endif

#ifdef _WIN32
    #define snprintf _snprintf
#endif


#define ARRAY_LENGTH(array) (sizeof (array) / sizeof (array)[0])
#define ABS(x) (((x) < 0) ? -(x) : (x))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define ZERO_MEMORY(p, numBytes) memset(p, 0, numBytes)


enum
{
    my_dropt_error_bad_ip_address = dropt_error_custom_start
};

typedef enum { false, true } bool;

static dropt_bool_t showHelp;
static dropt_bool_t verbose;
static dropt_bool_t normalFlag;
static dropt_bool_t hiddenFlag;
static dropt_char_t* stringVal;
static dropt_char_t* stringVal2;
static int intVal;

static bool unified;
static unsigned int lines;

unsigned int ipAddress;


static void
initOptionDefaults(void)
{
    showHelp = false;
    verbose = false;
    normalFlag = false;
    hiddenFlag = false;
    stringVal = NULL;
    stringVal2 = NULL;
    intVal = 0;

    unified = false;
    lines = 10;

    ipAddress = 0;
}


static dropt_error_t
handleUnified(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
{
    dropt_error_t err = dropt_error_none;
    if (valueString != NULL) { err = dropt_handle_uint(context, valueString, &lines); }
    if (err == dropt_error_none) { unified = true; }
    return err;
}


static dropt_error_t
handleIPAddress(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
{
    dropt_error_t err = dropt_error_none;
    unsigned int octet[4];
    size_t i;
    int n;

    assert(handlerData != NULL);

    if (valueString == NULL || valueString[0] == T('\0'))
    {
        err = dropt_error_insufficient_args;
        goto exit;
    }

    {
        const dropt_char_t* p = valueString;
        while (*p != T('\0'))
        {
            if (!istdigit(*p) && *p != T('.'))
            {
                err = my_dropt_error_bad_ip_address;
                goto exit;
            }
            p++;
        }
    }

    n = stscanf(valueString, T("%u.%u.%u.%u"), &octet[0], &octet[1], &octet[2], &octet[3]);
    if (n != ARRAY_LENGTH(octet))
    {
        err = my_dropt_error_bad_ip_address;
        goto exit;
    }

    for (i = 0; i < ARRAY_LENGTH(octet); i++)
    {
        if (octet[i] > 0xFF)
        {
            err = my_dropt_error_bad_ip_address;
            goto exit;
        }
    }

    *((unsigned int*) handlerData) = (octet[0] << 24) | (octet[1] << 16) | (octet[2] << 8) | octet[3];

exit:
    return err;
}


static dropt_char_t*
myDroptErrorHandler(dropt_error_t error, const dropt_char_t* optionName,
                    const dropt_char_t* valueString, void* handlerData)
{
#ifdef DROPT_NO_STRING_BUFFERS
    if (error == my_dropt_error_bad_ip_address)
    {
        return dropt_strdup(T("Invalid IP address"));
    }
    else
    {
        dropt_char_t buf[256];
        sntprintf(buf, ARRAY_LENGTH(buf), T("Failed on: %s=%s"),
                  optionName, valueString ? valueString : T("(null)"));
        buf[ARRAY_LENGTH(buf) - 1] = T('\0');
        return dropt_strdup(buf);
    }
#else
    if (error == my_dropt_error_bad_ip_address)
    {
        return dropt_asprintf(T("Invalid IP address for option %s: %s"), optionName, valueString);
    }
    else
    {
        return dropt_default_error_handler(error, optionName, valueString);
    }
#endif
}


dropt_option_t options[] = {
    { T('h'), T("help"), T("Shows help."), NULL, dropt_handle_bool, &showHelp, dropt_attr_halt },
    { T('?'), NULL, NULL, NULL, dropt_handle_bool, &showHelp, dropt_attr_halt },
    { T('v'), T("verbose"), T("Verbose mode."), NULL, dropt_handle_bool, &verbose },
    { T('n'), T("normalFlag"), T("Blah blah blah."), NULL, dropt_handle_bool, &normalFlag },
    { T('H'), T("hiddenFlag"), T("This is hidden."), NULL, dropt_handle_bool, &hiddenFlag, dropt_attr_hidden },
    { T('s'), T("string"), T("Test string value."), T("foo"), dropt_handle_string, &stringVal },
    { T('S'), T("string2"), T("Test string value."), T("foo"), dropt_handle_string, &stringVal2 },
    { T('i'), T("int"), T("Test integer value."), T("int"), dropt_handle_int, &intVal },
    { T('u'), T("unified"), T("Unified"), T("lines"), handleUnified, NULL, dropt_attr_optional_val },
    { T('\0'), T("ip"), T("Test IP address."), T("address"), handleIPAddress, &ipAddress},
    { 0 }
};



#define MAKE_EQUALITY_FUNC(name, type) static bool name(type a, type b) { return a == b; }
MAKE_EQUALITY_FUNC(boolEqual, dropt_bool_t)
MAKE_EQUALITY_FUNC(intEqual, int)
MAKE_EQUALITY_FUNC(uintEqual, unsigned int)


static bool
doubleEqual(double a, double b)
{
    double a0 = ABS(a);
    double b0 = ABS(b);
    double d = a - b;
    return ABS(d) <= DBL_EPSILON * MAX(a0, b0);
}


static bool
stringEqual(const dropt_char_t* a, const dropt_char_t* b)
{
    if (a == NULL || b == NULL)
    {
        return a == b;
    }
    else
    {
        return dropt_strcmp(a, b) == 0;
    }
}


static bool
testStrings(void)
{
#ifdef DROPT_NO_STRING_BUFFERS
    return true;
#else
    bool success = true;

    {
        const dropt_char_t* s = T("foo bar");
        const dropt_char_t* t = T("FOO QUX");

        dropt_char_t* copy;

        copy = dropt_strndup(s, 3);
        if (copy == NULL)
        {
            fputts(T("Insufficient memory.\n"), stderr);
            success = false;
            goto exit;
        }

        success = (dropt_strcmp(copy, T("foo")) == 0);
        free(copy);

        if (!success)
        {
            fputts(T("FAILED: dropt_strndup\n"), stderr);
            goto exit;
        }

        copy = dropt_strdup(s);
        if (copy == NULL)
        {
            fputts(T("Insufficient memory.\n"), stderr);
            success = false;
            goto exit;
        }

        success = (dropt_strcmp(copy, s) == 0);
        free(copy);

        if (!success)
        {
            fputts(T("FAILED: dropt_strdup\n"), stderr);
            goto exit;
        }

        success &= (dropt_strnicmp(s, t, 4) == 0);
        success &= (dropt_strnicmp(s, t, 5) < 0);
        success &= (dropt_strnicmp(t, s, 5) > 0);
        if (!success)
        {
            fputts(T("FAILED: dropt_strnicmp\n"), stderr);
            goto exit;
        }

        success &= (dropt_stricmp(s, t) < 0);
        success &= (dropt_stricmp(t, s) > 0);
        success &= (dropt_stricmp(T("foo"), T("FOO")) == 0);
        if (!success)
        {
            fputts(T("FAILED: dropt_stricmp\n"), stderr);
            goto exit;
        }
    }

    {
        dropt_char_t buf[4];

        ZERO_MEMORY(buf, sizeof buf);
        success &= dropt_snprintf(buf, ARRAY_LENGTH(buf), T("%s"), T("foo")) == 3;
        success &= stringEqual(buf, T("foo"));

        ZERO_MEMORY(buf, sizeof buf);
        success &= dropt_snprintf(buf, ARRAY_LENGTH(buf), T("%s"), T("bar baz")) == 7;
        success &= stringEqual(buf, T("bar"));

        if (!success)
        {
            fputts(T("FAILED: dropt_snprintf\n"), stderr);
            goto exit;
        }
    }

    {
        dropt_char_t* s;
        dropt_stringstream* ss = dropt_ssopen();
        if (ss == NULL)
        {
            fputts(T("Insufficient memory.\n"), stderr);
            success = false;
            goto exit;
        }

        dropt_ssprintf(ss, T("hello %s %X %d%c"), T("world"), 0xCAFEBABE, 31337, T('!'));
        dropt_ssprintf(ss, T("%c"), T('\n'));

        /* About 300 characters to make sure we overflow the default buffer
         * of 256 characters.
         */
        dropt_ssprintf(ss, T("Lorem ipsum dolor sit amet, consectetuer adipiscing elit. "));
        dropt_ssprintf(ss, T("Aenean quis mauris. In augue. "));
        dropt_ssprintf(ss, T("Suspendisse orci felis, tristique eget, lacinia rhoncus, interdum at, lorem."));
        dropt_ssprintf(ss, T("Aliquam gravida dui nec erat. Integer pede. Aliquam erat volutpat."));
        dropt_ssprintf(ss, T("In eu nisl. Curabitur non tellus id arcu feugiat porta orci aliquam."));
        s = dropt_ssfinalize(ss);
        dropt_ssclose(ss);

        success = stringEqual(s, T("hello world CAFEBABE 31337!\n")
                                 T("Lorem ipsum dolor sit amet, consectetuer adipiscing elit. ")
                                 T("Aenean quis mauris. In augue. ")
                                 T("Suspendisse orci felis, tristique eget, lacinia rhoncus, interdum at, lorem.")
                                 T("Aliquam gravida dui nec erat. Integer pede. Aliquam erat volutpat.")
                                 T("In eu nisl. Curabitur non tellus id arcu feugiat porta orci aliquam."));
        free(s);
    }

    if (!success)
    {
        fputts(T("FAILED: dropt_ssprintf\n"), stderr);
        goto exit;
    }

exit:
    return success;
#endif
}


#define MAKE_TEST_FOR_HANDLER(handler, type, valueEqualityFunc, formatSpecifier) \
static bool \
test_ ## handler(dropt_context_t* context, const dropt_char_t* valueString, \
                 dropt_error_t expectedError, type expectedValue, type initValue) \
{ \
    bool success = false; \
    type value = initValue; \
    dropt_error_t error = handler(context, valueString, &value); \
    if (error == expectedError && valueEqualityFunc(value, expectedValue)) \
    { \
        success = true; \
    } \
    else \
    { \
        ftprintf(stderr, \
                 T("FAILED: %s(\"%s\") ") \
                 T("returned %d, expected %d.  ") \
                 T("Output ") formatSpecifier T(", expected ") formatSpecifier T(".\n"), \
                 T(#handler), valueString ? valueString : T("(null)"), \
                 error, expectedError, \
                 value, expectedValue); \
    } \
    return success; \
}


MAKE_TEST_FOR_HANDLER(dropt_handle_bool, dropt_bool_t, boolEqual, T("%d"))
MAKE_TEST_FOR_HANDLER(dropt_handle_verbose_bool, dropt_bool_t, boolEqual, T("%d"))
MAKE_TEST_FOR_HANDLER(dropt_handle_int, int, intEqual, T("%d"))
MAKE_TEST_FOR_HANDLER(dropt_handle_uint, unsigned int, uintEqual, T("%u"))
MAKE_TEST_FOR_HANDLER(dropt_handle_double, double, doubleEqual, T("%g"))
MAKE_TEST_FOR_HANDLER(dropt_handle_string, dropt_char_t*, stringEqual, T("%s"))


static bool
testDroptHandlers(dropt_context_t* context)
{
    bool success = true;

    const int i = 42;
    const unsigned int u = 0xCAFEBABE;
    const double d = 2.71828;

    success &= test_dropt_handle_bool(context, NULL, dropt_error_none, 1, 0);
    success &= test_dropt_handle_bool(context, T(""), dropt_error_insufficient_args, 0, 0);
    success &= test_dropt_handle_bool(context, T(" "), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_bool(context, T("1"), dropt_error_none, 1, 0);
    success &= test_dropt_handle_bool(context, T("0"), dropt_error_none, 0, 0);
    success &= test_dropt_handle_bool(context, T("2"), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_bool(context, T("-1"), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_bool(context, T("01"), dropt_error_none, 1, 0);
    success &= test_dropt_handle_bool(context, T("11"), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_bool(context, T("a"), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_bool(context, T("a"), dropt_error_mismatch, 1, 1);
    success &= test_dropt_handle_bool(context, T("true"), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_bool(context, T("false"), dropt_error_mismatch, 0, 0);

    success &= test_dropt_handle_verbose_bool(context, NULL, dropt_error_none, 1, 0);
    success &= test_dropt_handle_verbose_bool(context, T(""), dropt_error_insufficient_args, 0, 0);
    success &= test_dropt_handle_verbose_bool(context, T(" "), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_verbose_bool(context, T("1"), dropt_error_none, 1, 0);
    success &= test_dropt_handle_verbose_bool(context, T("0"), dropt_error_none, 0, 0);
    success &= test_dropt_handle_verbose_bool(context, T("2"), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_verbose_bool(context, T("-1"), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_verbose_bool(context, T("01"), dropt_error_none, 1, 0);
    success &= test_dropt_handle_verbose_bool(context, T("11"), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_verbose_bool(context, T("a"), dropt_error_mismatch, 0, 0);
    success &= test_dropt_handle_verbose_bool(context, T("a"), dropt_error_mismatch, 1, 1);
    success &= test_dropt_handle_verbose_bool(context, T("true"), dropt_error_none, 1, 0);
    success &= test_dropt_handle_verbose_bool(context, T("false"), dropt_error_none, 0, 0);

    success &= test_dropt_handle_int(context, NULL, dropt_error_insufficient_args, i, i);
    success &= test_dropt_handle_int(context, T(""), dropt_error_insufficient_args, i, i);
    success &= test_dropt_handle_int(context, T(" "), dropt_error_mismatch, i, i);
    success &= test_dropt_handle_int(context, T("0"), dropt_error_none, 0, 0);
    success &= test_dropt_handle_int(context, T("-0"), dropt_error_none, 0, 0);
    success &= test_dropt_handle_int(context, T("123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_int(context, T("0123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_int(context, T("+123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_int(context, T("-123"), dropt_error_none, -123, 0);
    success &= test_dropt_handle_int(context, T("12.3"), dropt_error_mismatch, i, i);
    success &= test_dropt_handle_int(context, T("a"), dropt_error_mismatch, i, i);
    success &= test_dropt_handle_int(context, T("123a"), dropt_error_mismatch, i, i);
    success &= test_dropt_handle_int(context, T("3000000000"), dropt_error_overflow, i, i);
    success &= test_dropt_handle_int(context, T("-3000000000"), dropt_error_overflow, i, i);

    success &= test_dropt_handle_uint(context, NULL, dropt_error_insufficient_args, u, u);
    success &= test_dropt_handle_uint(context, T(""), dropt_error_insufficient_args, u, u);
    success &= test_dropt_handle_uint(context, T(" "), dropt_error_mismatch, u, u);
    success &= test_dropt_handle_uint(context, T("0"), dropt_error_none, 0, 0);
    success &= test_dropt_handle_uint(context, T("-0"), dropt_error_mismatch, u, u);
    success &= test_dropt_handle_uint(context, T("123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_uint(context, T("0123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_uint(context, T("123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_uint(context, T("+123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_uint(context, T("-123"), dropt_error_mismatch, u, u);
    success &= test_dropt_handle_uint(context, T("12.3"), dropt_error_mismatch, u, u);
    success &= test_dropt_handle_uint(context, T("a"), dropt_error_mismatch, u, u);
    success &= test_dropt_handle_uint(context, T("123a"), dropt_error_mismatch, u, u);
    success &= test_dropt_handle_uint(context, T("3000000000"), dropt_error_none, 3000000000u, 0);
    success &= test_dropt_handle_uint(context, T("-3000000000"), dropt_error_mismatch, u, u);
    success &= test_dropt_handle_uint(context, T("5000000000"), dropt_error_overflow, u, u);

    success &= test_dropt_handle_double(context, NULL, dropt_error_insufficient_args, d, d);
    success &= test_dropt_handle_double(context, T(""), dropt_error_insufficient_args, d, d);
    success &= test_dropt_handle_double(context, T(" "), dropt_error_mismatch, d, d);
    success &= test_dropt_handle_double(context, T("123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_double(context, T("0123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_double(context, T("+123"), dropt_error_none, 123, 0);
    success &= test_dropt_handle_double(context, T("-123"), dropt_error_none, -123, 0);
    success &= test_dropt_handle_double(context, T("12.3"), dropt_error_none, 12.3, 0);
    success &= test_dropt_handle_double(context, T(".123"), dropt_error_none, 0.123, 0);
    success &= test_dropt_handle_double(context, T("123e-1"), dropt_error_none, 12.3, 0);
    success &= test_dropt_handle_double(context, T("12.3e-1"), dropt_error_none, 1.23, 0);
    success &= test_dropt_handle_double(context, T("a"), dropt_error_mismatch, d, d);
    success &= test_dropt_handle_double(context, T("123a"), dropt_error_mismatch, d, d);
    success &= test_dropt_handle_double(context, T("1e1024"), dropt_error_overflow, d, d);
    success &= test_dropt_handle_double(context, T("1e-1024"), dropt_error_underflow, d, d);

    success &= test_dropt_handle_string(context, NULL, dropt_error_insufficient_args, T("qux"), T("qux"));
    success &= test_dropt_handle_string(context, T(""), dropt_error_none, T(""), NULL);
    success &= test_dropt_handle_string(context, T(" "), dropt_error_none, T(" "), NULL);
    success &= test_dropt_handle_string(context, T("foo"), dropt_error_none, T("foo"), NULL);
    success &= test_dropt_handle_string(context, T("foo bar"), dropt_error_none, T("foo bar"), NULL);

    return success;
}


#define VERIFY(expr) verify(expr, #expr, __LINE__)
static bool
verify(bool b, const char* s, unsigned int line)
{
    if (!b) { fprintf(stderr, "FAILED: %s (line: %u)\n", s, line); }
    return b;
}


static dropt_error_t
getAndPrintDroptError(dropt_context_t* context)
{
    dropt_error_t error = dropt_get_error(context);
    if (error != dropt_error_none)
    {
        ftprintf(stderr, T("[%d] %s\n"), error, dropt_get_error_message(context));
        dropt_clear_error(context);
    }
    return error;
}


static bool
testDroptParse(dropt_context_t* context)
{
    bool success = true;
    dropt_char_t** rest = NULL;

    /* Basic test for boolean options. */
    {
        dropt_char_t* args[] = { T("-n"), T("--hiddenFlag"), NULL };
        normalFlag = false;
        hiddenFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_none);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(hiddenFlag == true);
        success &= VERIFY(*rest == NULL);
    }

    /* Test that boolean options can be turned on with "=1" also. */
    {
        dropt_char_t* args[] = { T("-n=1"), T("--hiddenFlag=1"), NULL };
        normalFlag = false;
        hiddenFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(hiddenFlag == true);
        success &= VERIFY(*rest == NULL);
    }

    /* Test that boolean options can be turned off with "=0". */
    {
        dropt_char_t* args[] = { T("-n=0"), T("--hiddenFlag=0"), NULL };
        normalFlag = true;
        hiddenFlag = true;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == false);
        success &= VERIFY(hiddenFlag == false);
        success &= VERIFY(*rest == NULL);
    }

    /* Test that the last option wins if the same option is used multiple times. */
    {
        dropt_char_t* args[] = { T("-n=1"), T("-H"), T("-n=0"), T("--hiddenFlag=0"), NULL };
        normalFlag = false;
        hiddenFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == false);
        success &= VERIFY(hiddenFlag == false);
        success &= VERIFY(*rest == NULL);
    }

    /* Test that normal boolean options don't consume the next argument. */
    {
        dropt_char_t* args[] = { T("-n"), T("1"), NULL };
        normalFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(rest == &args[1]);
    }

    {
        dropt_char_t* args[] = { T("--normalFlag"), T("1"), NULL };
        normalFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(rest == &args[1]);
    }

    /* Test grouping short boolean options. */
    {
        dropt_char_t* args[] = { T("-Hn"), NULL };
        hiddenFlag = false;
        normalFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(hiddenFlag == true);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(*rest == NULL);
    }

    /* Test grouping short boolean options with a value. */
    {
        dropt_char_t* args[] = { T("-Hn=0"), NULL };
        hiddenFlag = false;
        normalFlag = true;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(hiddenFlag == true);
        success &= VERIFY(normalFlag == false);
        success &= VERIFY(*rest == NULL);
    }

    /* Test optional arguments with no acceptable argument provided. */
    {
        dropt_char_t* args[] = { T("-u"), T("-n"), NULL };
        unified = false;
        lines = 10;
        normalFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(unified == true);
        success &= VERIFY(lines == 10);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(*rest == NULL);
    }

    {
        dropt_char_t* args[] = { T("--unified"), T("-n"), NULL };
        unified = false;
        lines = 10;
        normalFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(unified == true);
        success &= VERIFY(lines == 10);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(*rest == NULL);
    }

    /* Test that optional arguments are consumed when possible. */
    {
        dropt_char_t* args[] = { T("-u"), T("42"), T("-n"), NULL };
        unified = false;
        lines = 10;
        normalFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(unified == true);
        success &= VERIFY(lines == 42);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(*rest == NULL);
    }

    {
        dropt_char_t* args[] = { T("--unified"), T("42"), T("-n"), NULL };
        unified = false;
        lines = 10;
        normalFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(unified == true);
        success &= VERIFY(lines == 42);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(*rest == NULL);
    }

    /* Test grouping short boolean options where one has an optional argument. */
    {
        dropt_char_t* args[] = { T("-un"), NULL };
        unified = false;
        lines = 10;
        normalFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(unified == true);
        success &= VERIFY(lines == 10);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(*rest == NULL);
    }

    {
        dropt_char_t* args[] = { T("-nu"), T("42"), NULL };
        normalFlag = false;
        unified = false;
        lines = 10;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(unified == true);
        success &= VERIFY(lines == 42);
        success &= VERIFY(*rest == NULL);
    }

    /* Test options that require arguments. */
    {
        dropt_char_t* args[] = { T("-s"), NULL };
        stringVal = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_insufficient_args);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("--string"), NULL };
        stringVal = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_insufficient_args);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    /* Test grouping short options where one has a required argument. */
    {
        dropt_char_t* args[] = { T("-sn"), NULL };
        normalFlag = false;
        stringVal = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_insufficient_args);
        success &= VERIFY(normalFlag == false);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("-ns"), NULL };
        normalFlag = false;
        stringVal = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_insufficient_args);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("-ns=foo"), NULL };
        normalFlag = false;
        stringVal = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(stringEqual(stringVal, T("foo")));
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("-ns"), T("foo"), NULL };
        normalFlag = false;
        stringVal = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(stringEqual(stringVal, T("foo")));
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    /* Test passing empty strings as arguments. */
    {
        dropt_char_t* args[] = { T("-s="), T("--string2="), NULL };
        stringVal = NULL;
        stringVal2 = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(stringEqual(stringVal, T("")));
        success &= VERIFY(stringEqual(stringVal2, T("")));
        success &= VERIFY(*rest == NULL);
    }

    {
        dropt_char_t* args[] = { T("-s"), T(""), T("--string2"), T(""), NULL };
        stringVal = NULL;
        stringVal2 = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(stringEqual(stringVal, T("")));
        success &= VERIFY(stringEqual(stringVal2, T("")));
        success &= VERIFY(*rest == NULL);
    }

    /* Test passing normal arguments. */
    {
        dropt_char_t* args[] = { T("-s=foo bar"), T("--string2=baz qux"), NULL };
        stringVal = NULL;
        stringVal2 = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(stringEqual(stringVal, T("foo bar")));
        success &= VERIFY(stringEqual(stringVal2, T("baz qux")));
        success &= VERIFY(*rest == NULL);
    }

    {
        dropt_char_t* args[] = { T("-s"), T("foo bar"), T("--string2"), T("baz qux"), NULL };
        stringVal = NULL;
        stringVal2 = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(stringEqual(stringVal, T("foo bar")));
        success &= VERIFY(stringEqual(stringVal2, T("baz qux")));
        success &= VERIFY(*rest == NULL);
    }

    /* Test arguments with embedded '=' characters. */
    {
        dropt_char_t* args[] = { T("-s=foo=bar"), T("--string2=baz=qux"), NULL };
        stringVal = NULL;
        stringVal2 = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(stringEqual(stringVal, T("foo=bar")));
        success &= VERIFY(stringEqual(stringVal2, T("baz=qux")));
        success &= VERIFY(*rest == NULL);
    }

    {
        dropt_char_t* args[] = { T("-s==foo"), T("--string2==bar"), NULL };
        stringVal = NULL;
        stringVal2 = NULL;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(stringEqual(stringVal, T("=foo")));
        success &= VERIFY(stringEqual(stringVal2, T("=bar")));
        success &= VERIFY(*rest == NULL);
    }

    /* Test that options that require arguments greedily consume the next
     * token, even if it looks like an option.
     */
    {
        dropt_char_t* args[] = { T("-s"), T("-n"), T("--string2"), T("-H"), NULL };
        stringVal = NULL;
        normalFlag = false;
        stringVal2 = NULL;
        hiddenFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(stringEqual(stringVal, T("-n")));
        success &= VERIFY(normalFlag == false);
        success &= VERIFY(stringEqual(stringVal2, T("-H")));
        success &= VERIFY(hiddenFlag == false);
        success &= VERIFY(*rest == NULL);
    }

    /* Test dropt_attr_halt. */
    {
        dropt_char_t* args[] = { T("-h"), T("-n"), T("-h=invalid"), NULL };
        showHelp = false;
        normalFlag = false;
        hiddenFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(showHelp == true);
        success &= VERIFY(normalFlag == false);
        success &= VERIFY(hiddenFlag == false);
        success &= VERIFY(rest == &args[1]);
    }

    /* Test --. */
    {
        dropt_char_t* args[] = { T("-n"), T("--"), T("-h"), NULL };
        normalFlag = false;
        hiddenFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(hiddenFlag == false);
        success &= VERIFY(rest == &args[2]);
    }

    /* Test -. */
    {
        dropt_char_t* args[] = { T("-n"), T("-"), T("-h"), NULL };
        normalFlag = false;
        hiddenFlag = false;
        rest = dropt_parse(context, args);
        success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
        success &= VERIFY(normalFlag == true);
        success &= VERIFY(hiddenFlag == false);
        success &= VERIFY(rest == &args[1]);
    }

    /* Test invalid options. */
    {
        dropt_char_t* args[] = { T("-X"), NULL };
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("-nX"), NULL };
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("-Xn"), NULL };
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("--bogus"), NULL };
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("--n"), NULL };
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("--normalFlagX"), NULL };
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    /* Test some pathological cases. */
    {
        dropt_char_t* args[] = { T("-="), NULL };
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    {
        dropt_char_t* args[] = { T("--="), NULL };
        rest = dropt_parse(context, args);
        success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
        success &= VERIFY(*rest == NULL);
        dropt_clear_error(context);
    }

    /* Test strncmp callback. */
    {
        {
            dropt_char_t* args[] = { T("-N"), NULL };
            rest = dropt_parse(context, args);
            success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
            success &= VERIFY(*rest == NULL);
            dropt_clear_error(context);
        }

        {
            dropt_char_t* args[] = { T("--NORMALFLAG"), NULL };
            rest = dropt_parse(context, args);
            success &= VERIFY(dropt_get_error(context) == dropt_error_invalid_option);
            success &= VERIFY(*rest == NULL);
            dropt_clear_error(context);
        }

        dropt_set_strncmp(context, dropt_strnicmp);

        {
            dropt_char_t* args[] = { T("-N"), NULL };
            normalFlag = false;
            rest = dropt_parse(context, args);
            success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
            success &= VERIFY(normalFlag == true);
            success &= VERIFY(*rest == NULL);
        }

        {
            dropt_char_t* args[] = { T("--NORMALFLAG"), NULL };
            normalFlag = false;
            rest = dropt_parse(context, args);
            success &= VERIFY(getAndPrintDroptError(context) == dropt_error_none);
            success &= VERIFY(normalFlag == true);
            success &= VERIFY(*rest == NULL);
        }

        dropt_set_strncmp(context, NULL);
    }

    /* TO DO: Test repeated invocations of dropt_parse. */

    return success;
}


#if defined _UNICODE && defined _WIN32
int
wmain(int argc, wchar_t** argv)
#else
int
main(int argc, char** argv)
#endif
{
    bool success = false;

    dropt_char_t** rest = NULL;
    dropt_context_t* droptContext = NULL;

    success = testStrings();
    if (!success) { goto exit; }

    droptContext = dropt_new_context();
    if (droptContext == NULL)
    {
        fputts(T("Insufficient memory.\n"), stderr);
        goto exit;
    }

    success = testDroptHandlers(droptContext);
    if (!success) { goto exit; }

    dropt_set_error_handler(droptContext, myDroptErrorHandler, NULL);
    dropt_set_options(droptContext, options);

    initOptionDefaults();
    success = testDroptParse(droptContext);
    if (!success) { goto exit; }

    initOptionDefaults();
    rest = dropt_parse(droptContext, &argv[1]);
    if (getAndPrintDroptError(droptContext) != dropt_error_none) { fputtc(T('\n'), stdout); }

    if (showHelp)
    {
        fputts(T("Usage: test_dropt [options] [operands]\n\n"), stdout);
#ifndef DROPT_NO_STRING_BUFFERS
        fputts(T("Options:\n"), stdout);
        dropt_print_help(stdout, options, 0);
#endif
        goto exit;
    }

    if (verbose)
    {
        dropt_char_t** arg;

        ftprintf(stdout, T("Compilation flags: %s%s\n")
                         T("verbose: %u\n")
                         T("normalFlag: %u\n")
                         T("hiddenFlag: %u\n")
                         T("string: %s\n")
                         T("intVal: %d\n")
                         T("unified: %u\n")
                         T("lines: %u\n")
                         T("ipAddress: %u.%u.%u.%u (%u)\n")
                         T("\n"),
#ifdef DROPT_NO_STRING_BUFFERS
                 T("DROPT_NO_STRING_BUFFERS "),
#else
                 T(""),
#endif
#if defined _UNICODE && defined _WIN32
                 T("_UNICODE "),
#else
                 T(""),
#endif
                 verbose, normalFlag, hiddenFlag,
                 (stringVal == NULL) ? T("(null)") : stringVal,
                 intVal, unified, lines,
                 (ipAddress >> 24) & 0xFF,
                 (ipAddress >> 16) & 0xFF,
                 (ipAddress >> 8) & 0xFF,
                 ipAddress & 0xFF,
                 ipAddress);
        ftprintf(stdout, T("Rest:"));
        for (arg = rest; *arg != NULL; arg++)
        {
            ftprintf(stdout, T(" %s"), *arg);
        }
        fputtc(T('\n'), stdout);
    }

exit:
    dropt_free_context(droptContext);

    if (!success) { fputts(T("One or more tests failed.\n"), stderr); }
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
