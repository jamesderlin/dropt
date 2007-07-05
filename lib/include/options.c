#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "options.h"

#ifndef __cplusplus
typedef enum { false, true } bool;
#endif



struct optionsContext_t
{
    const option_t* optionsP;
    optionsError_t err;
    TCHAR* errMessageP;
};


/** optionsParseBool
  *
  *     Parses a boolean value from the given string.
  *
  * PARAMETERS:
  *     IN s     : A non-NULL string representing a boolean value (0 or 1).
  *     OUT valP : A pointer to an unsigned char.
  *                On success, set to the interpreted boolean value.
  *
  * RETURNS:
  *     optionsErrorNone
  *     optionsErrorMismatch
  */
optionsError_t
optionsParseBool(const TCHAR* s, void* valP)
{
    optionsError_t err = optionsErrorNone;
    bool val = false;

    assert(valP != NULL);

    if (s == NULL)
    {
        val = true;
    }
    else if (_tcscmp(s, _T("0")) == 0)
    {
        val = false;
    }
    else if (_tcscmp(s, _T("1")) == 0)
    {
        val = true;
    }
    else
    {
        err = optionsErrorMismatch;
    }

    *((unsigned char*) valP) = val;
    return err;
}


/** optionsParseOptionalBool
  *
  *     Parses a boolean value from the given string if possible.
  *
  * PARAMETERS:
  *     IN s     : A non-NULL string representing a boolean value (0 or 1).
  *     OUT valP : A pointer to an unsigned char.
  *                On success, set to the interpreted boolean value.
  *
  * RETURNS:
  *     optionsErrorNone
  *     optionsErrorNoOptionalArg
  */
optionsError_t
optionsParseOptionalBool(const TCHAR* s, void* valP)
{
    optionsError_t err = optionsParseBool(s, valP);
    if (err != optionsErrorNone) { err = optionsErrorNoOptionalArg; }
    return err;
}


/** optionsParseInt
  *
  *     Parses an integer from the given string.
  *
  * PARAMETERS:
  *     IN s     : A non-NULL string representing a base-10 integer.
  *     OUT valP : A pointer to an int.
  *                On success, set to the interpreted integer.
  *
  * RETURNS:
  *     optionsErrorNone
  *     optionsErrorMismatch
  *     optionsErrorOverflow
  *     optionsErrorUnknown
  */
optionsError_t
optionsParseInt(const TCHAR* s, void* valP)
{
    optionsError_t err = optionsErrorNone;
    int val = 0;
    bool matched = false;

    assert(s != NULL);
    assert(valP != NULL);

    if (s[0] != '\0')
    {
        TCHAR* endP;
        long n;
        errno = 0;
        n = _tcstol(s, &endP, 10);

        /* Check that we matched at least one digit.
         * (strtol will return 0 if fed a string with no digits.)
         */
        if (*endP == '\0' && endP > s)
        {
            matched = true;

            if (errno == ERANGE || n < INT_MIN || n > INT_MAX)
            {
                err = optionsErrorOverflow;
                val = (n < 0) ? INT_MIN : INT_MAX;
            }
            else if (errno == 0)
            {
                val = (int) n;
            }
            else
            {
                err = optionsErrorUnknown;
            }
        }
    }

    if (!matched) { err = optionsErrorMismatch; }
    *((int*) valP) = val;
    return err;
}


/** optionsParseUInt
  *
  *     Parses an unsigned integer from the given string.
  *
  * PARAMETERS:
  *     IN s     : A non-NULL string representing an unsigned base-10
  *                  integer.
  *     OUT valP : A pointer to an unsigned int.
  *                On success, set to the interpreted integer.
  *
  * RETURNS:
  *     optionsErrorNone
  *     optionsErrorMismatch
  *     optionsErrorOverflow
  *     optionsErrorUnknown
  */
optionsError_t
optionsParseUInt(const TCHAR* s, unsigned int* valP)
{
    optionsError_t err = optionsErrorNone;
    int val = 0;
    bool matched = false;

    assert(s != NULL);
    assert(valP != NULL);

    if (s[0] != '\0' && s[0] != _T('-'))
    {
        TCHAR* endP;
        unsigned long n;
        errno = 0;
        n = _tcstoul(s, &endP, 10);

        /* Check that we matched at least one digit.
         * (strtol will return 0 if fed a string with no digits.)
         */
        if (*endP == '\0' && endP > s)
        {
            matched = true;

            if (errno == ERANGE || n > UINT_MAX)
            {
                err = optionsErrorOverflow;
                val = UINT_MAX;
            }
            else if (errno == 0)
            {
                val = (unsigned int) n;
            }
            else
            {
                err = optionsErrorUnknown;
            }
        }
    }

    if (!matched) { err = optionsErrorMismatch; }
    *((unsigned int*) valP) = val;
    return err;
}


/** optionsParseDouble
  *
  *     Parses a double from the given string.
  *
  * PARAMETERS:
  *     IN s     : A non-NULL string representing a base-10 floating-point
  *                  number.
  *     OUT valP : A pointer to a double.
  *                On success, set to the interpreted double.
  *
  * RETURNS:
  *     optionsErrorNone
  *     optionsErrorMismatch
  *     optionsErrorOverflow
  *     optionsErrorUnknown
  */
optionsError_t
optionsParseDouble(const TCHAR* s, void* valP)
{
    optionsError_t err = optionsErrorNone;
    double val = 0.0;
    bool matched = false;

    assert(s != NULL);
    assert(valP != NULL);

    if (s[0] != '\0')
    {
        TCHAR* endP;
        errno = 0;
        val = _tcstod(s, &endP);

        /* Check that we matched at least one digit.
         * (strtod will return 0 if fed a string with no digits.)
         */
        if (*endP == '\0' && endP > s)
        {
            matched = true;

            if (errno == ERANGE)
            {
                err = optionsErrorOverflow;
            }
            else if (errno != 0)
            {
                err = optionsErrorUnknown;
            }
        }
    }

    if (!matched) { err = optionsErrorMismatch; }
    *((double*) valP) = val;
    return err;
}


/** optionsParseString
  *
  *     Obtains a string.
  *
  * PARAMETERS:
  *     IN s     : A non-NULL string.
  *     OUT valP : A pointer to pointer-to-char.
  *                On success, set to the input string.
  *
  * RETURNS:
  *     optionsErrorNone
  */
optionsError_t
optionsParseString(const TCHAR* s, void* valP)
{
    assert(s != NULL);
    assert(valP != NULL);

    *((const TCHAR**) valP) = s;
    return optionsErrorNone;
}


/** vformat
  *
  *     Allocates a formatted string with vprintf semantics.
  *
  * PARAMETERS:
  *     IN fmtP : printf-style format specifier.  Must not be NULL.
  *     IN args : Arguments to insert into the formatted string.
  *
  * RETURNS:
  *     The formatted string.  The caller is responsible for freeing this
  *       when no longer needed.
  */
static TCHAR*
vformat(const TCHAR* fmtP, va_list args)
{
    TCHAR* s = NULL;
    size_t numChars = 0;

#ifdef _WIN32
    int n = _vsctprintf(fmtP, args);
#else
    int n = vstprintf(NULL, 0, fmtP, args);
#endif

    if (n >= 0)
    {
        numChars = n + 1;
        /* Cast for compatibility with C++ compilers. */
        s = (TCHAR*) malloc(numChars * sizeof *s);

#ifdef _WIN32
        _vsntprintf_s(s, numChars, _TRUNCATE, fmtP, args);
#else
        vstprintf(s, numChars, fmtP, args);
#endif
    }

    return s;
}


/* See vformat. */
static TCHAR*
format(const TCHAR* fmtP, ...)
{
    TCHAR* s;

    va_list args;
    assert(fmtP != NULL);
    va_start(args, fmtP);
    s = vformat(fmtP, args);
    va_end(args);

    return s;
}


/** isValidOption
  *
  * PARAMETERS:
  *     IN optionP : Specification for an individual option.
  *
  * RETURNS:
  *     true if the specified option is valid, false if it's a sentinel
  *       value.
  */
static bool
isValidOption(const option_t* optionP)
{
    return    optionP != NULL
           && !(   optionP->longName == NULL
                && optionP->shortName == '\0');
}


/** findOptionLong
  *
  *     Finds a the option specification for a "long" option (i.e., an
  *     option of the form "--option").
  *
  * PARAMETERS:
  *     IN optionsP  : The list of option specifications.
  *     IN longNameP : The "long" option to search for.
  *
  * RETURNS:
  *     A pointer to the corresponding option specification or NULL if not
  *       found.
  */
static const option_t*
findOptionLong(const option_t* optionsP, const TCHAR* longNameP)
{
    const option_t* optionP;
    assert(optionsP != NULL);
    assert(longNameP != NULL);
    for (optionP = optionsP; isValidOption(optionP); optionP++)
    {
        if (   optionP->longName != NULL
            && _tcscmp(longNameP, optionP->longName) == 0)
        {
            return optionP;
        }
    }
    return NULL;
}


/** findOptionShort
  *
  *     Finds a the option specification for a "short" option (i.e., an
  *     option of the form "-o").
  *
  * PARAMETERS:
  *     IN optionsP : The list of option specifications.
  *     shortName   : The "short" option to search for.
  *
  * RETURNS:
  *     A pointer to the corresponding option specification or NULL if not
  *       found.
  */
static const option_t*
findOptionShort(const option_t* optionsP, TCHAR shortName)
{
    const option_t* optionP;
    assert(optionsP != NULL);
    assert(shortName != '\0');
    for (optionP = optionsP; isValidOption(optionP); optionP++)
    {
        if (shortName == optionP->shortName)
        {
            return optionP;
        }
    }
    return NULL;
}


/** setErrorDetails
  *
  *     Generates error details in the options context.
  *
  * PARAMETERS:
  *     IN/OUT contextP : The options context.
  *     err             : The error code.
  *     IN longNameP    : The "long" name of the option we failed on.
  *                       Optional.  Pass NULL if unwanted.
  *                       Cannot be used with shortName.
  *     shortName       : the "short" name of the option we failed on.
  *                       Optional.  Pass '\0' if unwanted.
  *                       Cannot be used with longNameP.
  *     IN valP         : Optional.  Pass NULL if unwanted.
  */
static void
setErrorDetails(optionsContext_t* contextP,
                optionsError_t err,
                const TCHAR* longNameP, TCHAR shortName,
                const TCHAR* valP)
{
    const TCHAR* nameP;
    TCHAR shortNameBuf[3] = _T("-?");
    TCHAR* s = NULL;

    assert(contextP != NULL);
    assert((longNameP == NULL) != (shortName == '\0'));

    shortNameBuf[1] = shortName;

    nameP = (longNameP != NULL)
            ? longNameP
            : shortNameBuf;

    switch (err)
    {
        case optionsErrorNone:
            break;
        case optionsErrorNoOptionalArg:
        case optionsErrorCancel:
            /* These aren't really errors. */
            err = optionsErrorNone;
            break;

        case optionsErrorInvalid:
            s = format(_T("Invalid option: %s"), nameP);
            break;
        case optionsErrorInsufficientArgs:
            s = format(_T("Insufficient arguments to option %s"), nameP);
            break;
        case optionsErrorMismatch:
            if (valP == NULL)
            {
                s = format(_T("Invalid argument to option %s"), nameP);
            }
            else
            {
                s = format(_T("Invalid argument to option %s: %s"),
                           nameP, valP);
            }
            break;
        case optionsErrorOverflow:
            if (valP == NULL)
            {
                s = format(_T("Integer overflow for option %s"), nameP);
            }
            else
            {
                s = format(_T("Integer overflow for option %s: %s"),
                           nameP, valP);
            }
            break;
        case optionsErrorBadPlacement:
            s = format(_T("Value required after option %s"), nameP);
            break;
        case optionsErrorCustom:
            break;
        case optionsErrorUnknown:
        default:
            s = format(_T("Unknown error handling option %s."), nameP);
            break;
    }

    if (err != optionsErrorCustom) /* Leave custom error messages alone. */
    {
        optionsSetErrorMessage(contextP, s);
        free(s);
    }

    contextP->err = err;
}


/** optionsSetErrorMessage
  *
  *     Sets a custom error message in the options context.
  *
  * PARAMETERS:
  *     IN/OUT contextP : The options context.
  *     IN messageP     : The error message.
  */
void
optionsSetErrorMessage(optionsContext_t* contextP, const TCHAR* messageP)
{
    TCHAR* oldMessageP = contextP->errMessageP;
    TCHAR* s = NULL;

    assert(contextP != NULL);

    if (messageP != NULL)
    {
        size_t n = _tcslen(messageP) + 1;
        /* Cast for compatibility with C++ compilers. */
        s = (TCHAR*) malloc(n * sizeof *messageP);
        if (s != NULL)
        {
            _tcsncpy(s, messageP, n);
        }
    }

    contextP->err = optionsErrorCustom;
    contextP->errMessageP = s;
    free(oldMessageP);
}


/** optionsGetErrorMessage
  *
  * PARAMETERS:
  *     IN/OUT contextP : The options context.
  *
  * RETURNS:
  *     The current error message waiting in the options context or the
  *       empty string if there are no errors.
  */
const TCHAR*
optionsGetErrorMessage(const optionsContext_t* contextP)
{
    assert(contextP != NULL);
    return (contextP->errMessageP == NULL) ? _T("") : contextP->errMessageP;;
}


/** optionsGetError
  *
  * PARAMETERS:
  *     IN/OUT contextP : The options context.
  *
  * RETURNS:
  *     The current error code waiting in the options context.
  */
optionsError_t
optionsGetError(const optionsContext_t* contextP)
{
    assert(contextP != NULL);
    return contextP->err;
}


/** optionsPrintHelp
  *
  *     Prints help for available options.
  *
  * PARAMETERS:
  *     IN/OUT fP   : The file stream to print to.
  *     IN optionsP : The list of option specifications.
  *     compact     : Pass false to include blank lines between options.
  */
void
optionsPrintHelp(FILE* fp, const option_t* optionsP, unsigned char compact)
{
    static const size_t maxWidth = 4;

    const option_t* optionP;
    size_t n;

    assert(optionsP != NULL);

    for (optionP = optionsP; isValidOption(optionP); optionP++)
    {
        /* Undocumented option.  Ignore it and move on. */
        if (optionP->description == NULL || optionP->attr & optionsAttrHidden)
        {
            continue;
        }

        if (optionP->longName != NULL && optionP->shortName != '\0')
        {
            /* Both shortName and longName */
            n = _ftprintf(fp, _T("  -%c, --%s"), optionP->shortName, optionP->longName);
        }
        else if (optionP->longName != NULL)
        {
            /* longName only */
            n = _ftprintf(fp, _T("  --%s"), optionP->longName);
        }
        else if (optionP->shortName != '\0')
        {
            /* shortName only */
            n = _ftprintf(fp, _T("  -%c"), optionP->shortName);
        }
        else
        {
            assert(0);
            break;
        }

        if (optionP->argDescription != NULL)
        {
            n += _ftprintf(fp, _T("=%s"), optionP->argDescription);
        }

        if (n > maxWidth)
        {
            _ftprintf(fp, _T("\n"));
            n = 0;
        }
        _ftprintf(fp,
                  _T("%*s  %s\n"),
                  maxWidth - n, _T(""),
                  optionP->description);
        if (!compact) { _ftprintf(fp, _T("\n")); }
    }
}


/** set
  *
  *     Sets the value for a specified option by invoking the option's
  *     handler callback.
  *
  * PARAMETERS:
  *     IN optionP : The option.
  *     IN valP    : The option's value.
  *
  * RETURNS:
  *     An error code.
  */
static optionsError_t
set(const option_t* optionP, const TCHAR* valP)
{
    optionsError_t err = optionsErrorNone;

    assert(optionP != NULL);

    if (optionP->argDescription != NULL && valP == NULL)
    {
        err = optionsErrorInsufficientArgs;
    }
    else
    {
        assert(optionP->handler != NULL);
        err = optionP->handler(valP, optionP->handlerDataP);
    }
    return err;
}


/** optionsParse
  *
  *     Parses command-line options.
  *
  * PARAMETERS:
  *     IN contextP : The options context.
  *     IN/OUT argv : The list of command-line arguments, not including the
  *                     initial program name.  Must be terminated with a
  *                     NULL sentinel value.
  *                   Note that the command-line arguments might be
  *                     mutated in the process.
  *
  * RETURNS:
  *     A pointer to the first unprocessed element in argv.
  */
TCHAR**
optionsParse(optionsContext_t* contextP,
             TCHAR** argv)
{
    optionsError_t err = optionsErrorNone;

    TCHAR** argNextPP = argv;
    TCHAR* argP;

    assert(contextP != NULL);
    assert(argv != NULL);

    while (   (argP = *argNextPP) != NULL
           && argP[0] == _T('-'))
    {
        assert(err == optionsErrorNone);

        if (argP[1] == '\0')
        {
            /* - */
            goto abort;
        }

        argNextPP++;

        if (argP[1] == _T('-'))
        {
            TCHAR* argNameP = argP + 2;
            if (argNameP[0] == '\0')
            {
                /* -- */
                goto abort;
            }
            else
            {
                /* --longName */
                const option_t* optionP = NULL;
                const TCHAR* valP = NULL;

                {
                    TCHAR* p = _tcschr(argNameP, _T('='));
                    if (p != NULL)
                    {
                        *p = '\0';
                        valP = p + 1;
                    }
                }

                optionP = findOptionLong(contextP->optionsP, argNameP);
                if (optionP == NULL)
                {
                    err = optionsErrorInvalid;
                    setErrorDetails(contextP, err, argP, '\0', NULL);
                    goto abort;
                }
                else
                {
                    bool consumeNextArg = false;
                    if (optionP->argDescription != NULL && valP == NULL)
                    {
                        consumeNextArg = true;
                        valP = *argNextPP;
                    }

                    err = set(optionP, valP);

                    if (err != optionsErrorNone)
                    {
                        if (err != optionsErrorNoOptionalArg)
                        {
                            setErrorDetails(contextP, err, argP, '\0', valP);
                            goto abort;
                        }
                        err = optionsErrorNone;
                    }
                    else if (consumeNextArg)
                    {
                        argNextPP++;
                    }
                }

                if (optionP->attr & optionsAttrHalt) { goto abort; }
            }
        }
        else
        {
            const option_t* optionP;
            const TCHAR* valP;
            size_t len;
            size_t j;

            {
                const TCHAR* p = _tcschr(argP, _T('='));
                if (p == NULL)
                {
                    len = _tcslen(argP);
                    valP = NULL;
                }
                else
                {
                    len = p - argP;
                    valP = p + 1;
                }
            }

            for (j = 1; j < len; j++)
            {
                optionP = findOptionShort(contextP->optionsP, argP[j]);
                if (optionP == NULL)
                {
                    err = optionsErrorInvalid;
                    setErrorDetails(contextP, err, NULL, argP[j], NULL);
                    goto abort;
                }
                else
                {
                    if (j + 1 == len)
                    {
                        bool consumeNextArg = false;
                        if (optionP->argDescription != NULL && valP == NULL)
                        {
                            consumeNextArg = true;
                            valP = *argNextPP;
                        }

                        /* Even for options that don't ask for arguments, always
                         * pass an argument that was specified with '='.
                         */
                        err = set(optionP, valP);

                        if (err != optionsErrorNone)
                        {
                            if (err != optionsErrorNoOptionalArg)
                            {
                                setErrorDetails(contextP, err, NULL, argP[j], valP);
                                goto abort;
                            }
                            err = optionsErrorNone;
                        }
                        else if (consumeNextArg)
                        {
                            argNextPP++;
                        }
                    }
                    else if (optionP->argDescription == NULL)
                    {
                        err = set(optionP, NULL);
                        if (err != optionsErrorNone)
                        {
                            setErrorDetails(contextP, err, NULL, argP[j], NULL);
                            goto abort;
                        }
                    }
                    else
                    {
                        /* Short options with arguments can't be used in
                         * condensed lists except in the last position.
                         * e.g. -abcd arg
                         *          ^
                         */
                        err = optionsErrorBadPlacement;
                        setErrorDetails(contextP, err, NULL, argP[j], NULL);
                        goto abort;
                    }
                }

                if (optionP->attr & optionsAttrHalt) { goto abort; }
            }
        }
    }

abort:
    contextP->err = err;
    return argNextPP;
}


/** optionsNewContext
  *
  *     Creates a new options context.
  *
  * RETURNS:
  *     An allocated options context.  The caller is responsible for
  *       freeing it with optionsFreeContext when no longer needed.
  */
optionsContext_t*
optionsNewContext(void)
{
    /* Cast for compatibility with C++ compilers. */
    optionsContext_t* contextP = (optionsContext_t*) malloc(sizeof *contextP);
    if (contextP != NULL)
    {
        contextP->optionsP = NULL;
        contextP->err = optionsErrorNone;
        contextP->errMessageP = NULL;
    }
    return contextP;
}


/** optionsFreeContext
  *
  *     Frees an options context.
  *
  * PARAMETERS:
  *     IN/OUT contextP : The options context to free.
  */
void
optionsFreeContext(optionsContext_t* contextP)
{
    if (contextP != NULL)
    {
        free(contextP->errMessageP);
        free(contextP);
    }
}


/** optionsSet
  *
  *     Specifies a list of options to use with an options context.
  *
  * PARAMETERS:
  *     IN/OUT contextP : The options context.
  *     IN options      : The list of option specifications.
  */
void
optionsSet(optionsContext_t* contextP, const option_t* optionsP)
{
    assert(contextP != NULL);
    contextP->optionsP = optionsP;
}
