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


/** optionsParseInt
  *
  *     Parses an integer from the given string.
  *
  * PARAMETERS:
  *     IN s     : A non-NULL string representing a base-10 integer.
  *     OUT valP : On success, set to the interpreted integer.
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
  *     OUT valP : On success, set to the interpreted integer.
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


optionsError_t
optionsParseString(const TCHAR* s, void* valP)
{
    assert(s != NULL);
    assert(valP != NULL);

    *((const TCHAR**) valP) = s;
    return optionsErrorNone;
}


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


static TCHAR*
format(const TCHAR* fmtP, ...)
{
    TCHAR* s;

    va_list args;
    va_start(args, fmtP);
    s = vformat(fmtP, args);
    va_end(args);

    return s;
}


static bool
isValidOption(const option_t* optionP)
{
    return    optionP != NULL
           && !(   optionP->longName == NULL
                && optionP->shortName == '\0');
}


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


static void
optionsSetErrorDetails(optionsContext_t* contextP,
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


const TCHAR*
optionsGetErrorMessage(const optionsContext_t* contextP)
{
    assert(contextP != NULL);
    return (contextP->errMessageP == NULL) ? _T("") : contextP->errMessageP;;
}


optionsError_t
optionsGetError(const optionsContext_t* contextP)
{
    assert(contextP != NULL);
    return contextP->err;
}


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


static optionsError_t
set(const option_t* optionP, const TCHAR* valStrP)
{
    optionsError_t err = optionsErrorNone;

    assert(optionP != NULL);

    if (optionP->argDescription != NULL && valStrP == NULL)
    {
        err = optionsErrorInsufficientArgs;
    }
    else
    {
        assert(optionP->handler != NULL);
        err = optionP->handler(valStrP, optionP->handlerDataP);
    }
    return err;
}


TCHAR**
optionsParse(optionsContext_t* contextP,
             TCHAR** argv)
{
    optionsError_t err = optionsErrorNone;

    TCHAR** argNextPP;
    TCHAR* argP;

    assert(contextP != NULL);
    assert(argv != NULL);

    /* Assume argv[0] is the program name. */
    argNextPP = argv + 1;

    while (   (argP = *argNextPP) != NULL
           && argP[0] == _T('-'))
    {
        assert(err == optionsErrorNone);

        if (argP[1] == '\0')
        {
            /* - */
            break;
        }

        argNextPP++;

        if (argP[1] == _T('-'))
        {
            TCHAR* argNameP = argP + 2;
            if (argNameP[0] == '\0')
            {
                /* -- */
                break;
            }
            else
            {
                /* --longName */
                const option_t* optionP = NULL;
                const TCHAR* valStrP = NULL;

                {
                    TCHAR* p = _tcschr(argNameP, _T('='));
                    if (p != NULL)
                    {
                        *p = '\0';
                        valStrP = p + 1;
                    }
                }

                optionP = findOptionLong(contextP->optionsP, argNameP);
                if (optionP == NULL)
                {
                    err = optionsErrorInvalid;
                    optionsSetErrorDetails(contextP, err, argP, '\0', NULL);
                    goto abort;
                }
                else
                {
                    bool consumeNextArg = false;
                    if (optionP->argDescription != NULL && valStrP == NULL)
                    {
                        consumeNextArg = true;
                        valStrP = *argNextPP;
                    }

                    err = set(optionP, valStrP);

                    if (err != optionsErrorNone)
                    {
                        if (err != optionsErrorNoOptionalArg)
                        {
                            optionsSetErrorDetails(contextP, err, argP, '\0', valStrP);
                            goto abort;
                        }
                        err = optionsErrorNone;
                    }
                    else if (consumeNextArg)
                    {
                        argNextPP++;
                    }
                }

                if (optionP->attr & optionsAttrHalt) { break; }
            }
        }
        else
        {
            const option_t* optionP;
            const TCHAR* valStrP;
            size_t len;
            size_t j;

            {
                const TCHAR* p = _tcschr(argP, _T('='));
                if (p == NULL)
                {
                    len = _tcslen(argP);
                    valStrP = NULL;
                }
                else
                {
                    len = p - argP;
                    valStrP = p + 1;
                }
            }

            for (j = 1; j < len; j++)
            {
                optionP = findOptionShort(contextP->optionsP, argP[j]);
                if (optionP == NULL)
                {
                    err = optionsErrorInvalid;
                    optionsSetErrorDetails(contextP, err, NULL, argP[j], NULL);
                    goto abort;
                }
                else
                {
                    if (j + 1 == len)
                    {
                        bool consumeNextArg = false;
                        if (optionP->argDescription != NULL && valStrP == NULL)
                        {
                            consumeNextArg = true;
                            valStrP = *argNextPP;
                        }

                        /* Even for options that don't ask for arguments, always
                         * pass an argument that was specified with '='.
                         */
                        err = set(optionP, valStrP);

                        if (err != optionsErrorNone)
                        {
                            if (err != optionsErrorNoOptionalArg)
                            {
                                optionsSetErrorDetails(contextP, err, NULL, argP[j], valStrP);
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
                            optionsSetErrorDetails(contextP, err, NULL, argP[j], NULL);
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
                        optionsSetErrorDetails(contextP, err, NULL, argP[j], NULL);
                        goto abort;
                    }
                }

                if (optionP->attr & optionsAttrHalt) { break; }
            }
        }
    }

abort:
    contextP->err = err;
    return argNextPP;
}


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


void
optionsFreeContext(optionsContext_t* contextP)
{
    if (contextP != NULL)
    {
        free(contextP->errMessageP);
        free(contextP);
    }
}


void
optionsSet(optionsContext_t* contextP, const option_t* optionsP)
{
    assert(contextP != NULL);
    contextP->optionsP = optionsP;
}
