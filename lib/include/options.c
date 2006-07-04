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
    TCHAR* errMessage;
};


static optionsError_t
parseBool(const TCHAR* s, bool* valP)
{
    optionsError_t err = optionsErrorNone;
    bool val = false;

    assert(s != NULL && valP != NULL);

    if (_tcscmp(s, _T("0")) == 0)
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

    *valP = val;
    return err;
}


/** parseInt
  *
  *     Parses an integer from the given string.
  *
  * PARAMETERS:
  *     IN s     : A non-NULL string representing a base-10 integer.
  *     OUT errP :
  *
  * RETURNS:
  *     The interpreted integer.
  *
  * ERRORS:
  *     optionsErrorMismatch
  *     optionsErrorOverflow
  */
static optionsError_t
parseInt(const TCHAR* s, int* valP)
{
    optionsError_t err = optionsErrorNone;
    int val = 0;
    bool matched = false;

    assert(s != NULL && valP != NULL);

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
    *valP = val;
    return err;
}


/** parseUInt
  *
  *     Parses an unsigned integer from the given string.
  *
  * PARAMETERS:
  *     IN s     : A non-NULL string representing an unsigned base-10
  *                  integer.
  *     OUT errP :
  *
  * RETURNS:
  *     The interpreted integer.
  *
  * ERRORS:
  *     optionsErrorMismatch
  *     optionsErrorOverflow
  */
static optionsError_t
parseUInt(const TCHAR* s, unsigned int* valP)
{
    optionsError_t err = { optionsErrorNone };
    int val = 0;
    bool matched = false;

    assert(s != NULL && valP != NULL);

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
    *valP = val;
    return err;
}


static optionsError_t
parseDouble(const TCHAR* s, double* valP)
{
    optionsError_t err = optionsErrorNone;
    double val = 0.0;
    bool matched = false;

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
    *valP = val;
    return err;
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
findOptionLong(const option_t* optionsP, const TCHAR* longName)
{
    const option_t* optionP;
    assert(optionsP != NULL);
    assert(longName != NULL);
    for (optionP = optionsP; isValidOption(optionP); optionP++)
    {
        if (   optionP->longName != NULL
            && _tcscmp(longName, optionP->longName) == 0)
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
                       const TCHAR* longName, TCHAR shortName,
                       const TCHAR* valP)
{
    const TCHAR* name;
    TCHAR shortNameBuf[3] = _T("-?");
    TCHAR* s = NULL;

    assert(contextP != NULL);
    assert((longName == NULL) != (shortName == '\0'));

    shortNameBuf[1] = shortName;

    name = (longName != NULL)
           ? longName
           : shortNameBuf;

    switch (err)
    {
        case optionsErrorNone:
            break;
        case optionsErrorInvalid:
            s = format(_T("Invalid option: %s"), name);
            break;
        case optionsErrorInsufficientArgs:
            s = format(_T("Insufficient arguments to option %s"), name);
            break;
        case optionsErrorMismatch:
            assert(valP != NULL);
            s = format(_T("Invalid argument to option %s: %s"),
                       name, valP);
            break;
        case optionsErrorOverflow:
            s = format(_T("Integer overflow: %s"), valP);
            break;
        case optionsErrorBadPlacement:
            s = format(_T("Value required after option %s"), name);
            break;
        case optionsErrorUnknown:
        default:
            s = format(_T("Unknown error handling option %s."), name);
            break;
    }

    {
        TCHAR* oldMessage = contextP->errMessage;
        contextP->err = err;
        contextP->errMessage = s;
        free(oldMessage);
    }
}


const TCHAR*
optionsGetErrorMessage(const optionsContext_t* contextP)
{
    return (contextP->errMessage == NULL) ? _T("") : contextP->errMessage;;
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
        if (optionP->description == NULL) { continue; }

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


/* refOp is an operator to obtain the address of the given value.  This is
 * necessary because the client-specified handler must be able to take the
 * value as a const void* argument.  For most data types, simply pass the
 * usual address-of operator (unary &).
 *
 * For optionsTypeUnchecked (i.e., strings), we already start with a
 * pointer, so we can pass that directly to a client-specified handler
 * as-is. (Although it makes this code less uniform, it allows client code
 * to be simpler.) To create such a no-op operator, chain the dereference
 * and address-of operators (*&).
 */
#define MAKE_SETTER(func, val_t, addrOp)                \
static optionsError_t                                   \
func(const option_t* optionP, val_t val)                \
{                                                       \
    optionsError_t err = optionsErrorNone;              \
    if (optionP->handler != NULL)                       \
    {                                                   \
        err = optionP->handler(optionP, addrOp val);    \
    }                                                   \
    else                                                \
    {                                                   \
        assert(optionP->varP != NULL);                  \
        *((val_t*) optionP->varP) = val;                \
    }                                                   \
    return err;                                         \
}

MAKE_SETTER(setUnchecked, const TCHAR*, *&); /* See comments above. */
MAKE_SETTER(setBool, unsigned char, &);
MAKE_SETTER(setInt, int, &);
MAKE_SETTER(setUInt, unsigned int, &);
MAKE_SETTER(setDouble, double, &);


static optionsError_t
set(const option_t* optionP, const TCHAR* valStr)
{
    optionsError_t err = optionsErrorNone;

    assert(optionP != NULL);

    if (valStr == NULL)
    {
        err = optionsErrorInsufficientArgs;
    }
    else switch (optionP->type)
    {
        case optionsTypeUnchecked:
            err = setUnchecked(optionP, valStr);
            break;

        case optionsTypeInt:
        {
            int val;
            err = parseInt(valStr, &val);
            if (err == optionsErrorNone) { err = setInt(optionP, val); }
            break;
        }
        case optionsTypeUInt:
        {
            unsigned int val;
            err = parseUInt(valStr, &val);
            if (err == optionsErrorNone) { err = setUInt(optionP, val); }
            break;
        }
        case optionsTypeDouble:
        {
            double val;
            err = parseDouble(valStr, &val);
            if (err == optionsErrorNone) { err = setDouble(optionP, val); }
            break;
        }
        case optionsTypeBool:
            /* Boolean types are special since their arguments are
             * optional.  They must be handled elsewhere.
             */
        default:
            assert(0);
            break;
    }

    return err;
}


static optionsError_t
getAndSetOptional(const option_t* optionP, const TCHAR* valStr)
{
    optionsError_t err = optionsErrorNone;
    assert(optionP != NULL);
    if (optionP->type == optionsTypeBool)
    {
        bool val = true;
        if (valStr != NULL) { err = parseBool(valStr, &val); }
        if (err == optionsErrorNone) { err = setBool(optionP, val); }
    }
    else
    {
        err = set(optionP, valStr);
    }

    return err;
}


TCHAR**
optionsParse(optionsContext_t* contextP,
             TCHAR** argv)
{
    optionsError_t err = optionsErrorNone;

    TCHAR** argNext;
    TCHAR* arg;

    assert(contextP != NULL && argv != NULL);

    /* Assume argv[0] is the program name. */
    argNext = argv + 1;

    while (   (arg = *argNext) != NULL
           && arg[0] == _T('-'))
    {
        assert(err == optionsErrorNone);

        if (arg[1] == '\0')
        {
            /* - */
            break;
        }

        argNext++;

        if (arg[1] == _T('-'))
        {
            TCHAR* argName = arg + 2;
            if (argName[0] == '\0')
            {
                /* -- */
                break;
            }
            else
            {
                /* --longName */
                const option_t* optionP = NULL;
                const TCHAR* valStr = NULL;

                {
                    TCHAR* p = _tcschr(argName, _T('='));
                    if (p != NULL)
                    {
                        *p = '\0';
                        valStr = p + 1;
                    }
                }

                optionP = findOptionLong(contextP->optionsP, argName);
                if (optionP == NULL)
                {
                    err = optionsErrorInvalid;
                    optionsSetErrorDetails(contextP, err, arg, '\0', NULL);
                    goto abort;
                }
                else
                {
                    bool optionalArg = valStr == NULL && optionP->type != optionsTypeBool;
                    if (optionalArg) { valStr = *argNext; }
                    err = getAndSetOptional(optionP, valStr);
                    if (err == optionsErrorNone && optionalArg) { argNext++; }
                }

                if (err != optionsErrorNone)
                {
                    optionsSetErrorDetails(contextP, err, arg, '\0', valStr);
                    goto abort;
                }

                if (optionP->attr & optionsAttrHalt) { break; }
            }
        }
        else
        {
            const option_t* optionP;
            const TCHAR* valStr;
            size_t len;
            size_t j;

            {
                const TCHAR* p = _tcschr(arg, _T('='));
                if (p == NULL)
                {
                    len = _tcslen(arg);
                    valStr = NULL;
                }
                else
                {
                    len = p - arg;
                    valStr = p + 1;
                }
            }

            for (j = 1; j < len; j++)
            {
                optionP = findOptionShort(contextP->optionsP, arg[j]);
                if (optionP == NULL)
                {
                    err = optionsErrorInvalid;
                    optionsSetErrorDetails(contextP, err, NULL, arg[j], NULL);
                    goto abort;
                }
                else if (j + 1 == len)
                {
                    bool optionalArg = valStr == NULL && optionP->type != optionsTypeBool;
                    if (optionalArg) { valStr = *argNext; }
                    err = getAndSetOptional(optionP, valStr);
                    if (err == optionsErrorNone && optionalArg) { argNext++; }
                }
                else if (optionP->type == optionsTypeBool)
                {
                    err = setBool(optionP, 1);
                }
                else
                {
                    /* Option not allowed here. */
                    err = optionsErrorBadPlacement;
                }

                if (err != optionsErrorNone)
                {
                    optionsSetErrorDetails(contextP, err, NULL, arg[j], valStr);
                    goto abort;
                }

                if (optionP->attr & optionsAttrHalt) { break; }
            }
        }
    }

abort:
    contextP->err = err;
    return argNext;
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
        contextP->errMessage = NULL;
    }
    return contextP;
}


void
optionsFreeContext(optionsContext_t* contextP)
{
    free(contextP->errMessage);
    free(contextP);
}


void
optionsSet(optionsContext_t* contextP, const option_t* optionsP)
{
    assert(contextP != NULL);
    contextP->optionsP = optionsP;
}


optionsError_t
optionsGetError(const optionsContext_t* contextP)
{
    assert(contextP != NULL);
    return contextP->err;
}
