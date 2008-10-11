/** dropt.c
  *
  *     A deliberately rudimentary command-line option parser.
  *
  * Copyright (C) 2006-2008 James D. Lin
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "dropt.h"
#include "dropt_string.h"

#if defined _UNICODE || defined UNICODE
    #define T(s) (L ## s)
#else
    #define T(s) s
#endif

typedef enum { false, true } bool;

struct dropt_context_t
{
    const dropt_option_t* options;
    bool caseSensitive;

    struct
    {
        dropt_error_t err;
        dropt_char_t* optionName;
        dropt_char_t* optionValue;
        dropt_char_t* message;
    } errorDetails;
};

typedef struct
{
    const dropt_option_t* option;
    const dropt_char_t* valString;
    dropt_char_t** argNext;
} parseState_t;


/** dropt_handle_bool
  *
  *     Parses a boolean value from the given string if possible.
  *
  * PARAMETERS:
  *     IN valString    : A string representing a boolean value (0 or 1).
  *                       If NULL, the boolean value is assumed to be
  *                         true.
  *     OUT handlerData : A pointer to a dropt_bool_t.
  *                       On success, set to the interpreted boolean
  *                         value.
  *                       On error, left untouched.
  *
  * RETURNS:
  *     dropt_error_none
  *     dropt_error_mismatch
  */
dropt_error_t
dropt_handle_bool(const dropt_char_t* valString, void* handlerData)
{
    dropt_error_t err = dropt_error_none;
    bool val = false;

    assert(handlerData != NULL);

    if (valString == NULL)
    {
        val = true;
    }
    else if (dropt_strcmp(valString, T("0")) == 0)
    {
        val = false;
    }
    else if (dropt_strcmp(valString, T("1")) == 0)
    {
        val = true;
    }
    else
    {
        err = dropt_error_mismatch;
    }

    if (err == dropt_error_none) { *((dropt_bool_t*) handlerData) = val; }
    return err;
}


/** dropt_handle_int
  *
  *     Parses an integer from the given string.
  *
  * PARAMETERS:
  *     IN valString    : A string representing a base-10 integer.
  *                       If NULL, returns dropt_error_unsufficient_args.
  *     OUT handlerData : A pointer to an int.
  *                       On success, set to the interpreted integer.
  *                       On error, left untouched.
  *
  * RETURNS:
  *     dropt_error_none
  *     dropt_error_insufficient_args
  *     dropt_error_mismatch
  *     dropt_error_overflow
  *     dropt_error_unknown
  */
dropt_error_t
dropt_handle_int(const dropt_char_t* valString, void* handlerData)
{
    dropt_error_t err = dropt_error_none;
    int val = 0;

    assert(handlerData != NULL);

    if (valString == NULL || valString[0] == T('\0'))
    {
        err = dropt_error_insufficient_args;
    }
    else
    {
        dropt_char_t* end;
        long n;
        errno = 0;
        n = dropt_strtol(valString, &end, 10);

        /* Check that we matched at least one digit.
         * (strtol will return 0 if fed a string with no digits.)
         */
        if (*end == T('\0') && end > valString)
        {
            if (errno == ERANGE || n < INT_MIN || n > INT_MAX)
            {
                err = dropt_error_overflow;
                val = (n < 0) ? INT_MIN : INT_MAX;
            }
            else if (errno == 0)
            {
                val = (int) n;
            }
            else
            {
                err = dropt_error_unknown;
            }
        }
        else
        {
            err = dropt_error_mismatch;
        }
    }

    if (err == dropt_error_none) { *((int*) handlerData) = val; }
    return err;
}


/** dropt_handle_uint
  *
  *     Parses an unsigned integer from the given string.
  *
  * PARAMETERS:
  *     IN valString    : A string representing an unsigned base-10
  *                         integer.
  *                       If NULL, returns dropt_error_unsufficient_args.
  *     OUT handlerData : A pointer to an unsigned int.
  *                       On success, set to the interpreted integer.
  *                       On error, left untouched.
  *
  * RETURNS:
  *     dropt_error_none
  *     dropt_error_insufficient_args
  *     dropt_error_mismatch
  *     dropt_error_overflow
  *     dropt_error_unknown
  */
dropt_error_t
dropt_handle_uint(const dropt_char_t* valString, void* handlerData)
{
    dropt_error_t err = dropt_error_none;
    int val = 0;

    assert(handlerData != NULL);

    if (valString == NULL || valString[0] == T('\0'))
    {
        err = dropt_error_insufficient_args;
    }
    else if (valString[0] == T('-'))
    {
        err = dropt_error_mismatch;
    }
    else
    {
        dropt_char_t* end;
        unsigned long n;
        errno = 0;
        n = dropt_strtoul(valString, &end, 10);

        /* Check that we matched at least one digit.
         * (strtol will return 0 if fed a string with no digits.)
         */
        if (*end == T('\0') && end > valString)
        {
            if (errno == ERANGE || n > UINT_MAX)
            {
                err = dropt_error_overflow;
                val = UINT_MAX;
            }
            else if (errno == 0)
            {
                val = (unsigned int) n;
            }
            else
            {
                err = dropt_error_unknown;
            }
        }
        else
        {
            err = dropt_error_mismatch;
        }
    }

    if (err == dropt_error_none) { *((unsigned int*) handlerData) = val; }
    return err;
}


/** dropt_handle_double
  *
  *     Parses a double from the given string.
  *
  * PARAMETERS:
  *     IN valString    : A string representing a base-10 floating-point
  *                         number.
  *                       If NULL, returns dropt_error_unsufficient_args.
  *     OUT handlerData : A pointer to a double.
  *                       On success, set to the interpreted double.
  *                       On error, left untouched.
  *
  * RETURNS:
  *     dropt_error_none
  *     dropt_error_insufficient_args
  *     dropt_error_mismatch
  *     dropt_error_overflow
  *     dropt_error_unknown
  */
dropt_error_t
dropt_handle_double(const dropt_char_t* valString, void* handlerData)
{
    dropt_error_t err = dropt_error_none;
    double val = 0.0;

    assert(handlerData != NULL);

    if (valString == NULL || valString[0] == T('\0'))
    {
        err = dropt_error_insufficient_args;
    }
    else
    {
        dropt_char_t* end;
        errno = 0;
        val = dropt_strtod(valString, &end);

        /* Check that we matched at least one digit.
         * (strtod will return 0 if fed a string with no digits.)
         */
        if (*end == T('\0') && end > valString)
        {
            if (errno == ERANGE)
            {
                err = dropt_error_overflow;
            }
            else if (errno != 0)
            {
                err = dropt_error_unknown;
            }
        }
        else
        {
            err = dropt_error_mismatch;
        }
    }

    if (err == dropt_error_none) { *((double*) handlerData) = val; }
    return err;
}


/** dropt_handle_string
  *
  *     Obtains a string.
  *
  * PARAMETERS:
  *     IN valString    : A string.
  *                       May be NULL.
  *     OUT handlerData : A pointer to pointer-to-char.
  *                       On success, set to the input string.
  *                       On error, left untouched.
  *
  * RETURNS:
  *     dropt_error_none
  *     dropt_error_insufficient_args
  */
dropt_error_t
dropt_handle_string(const dropt_char_t* valString, void* handlerData)
{
    dropt_error_t err = dropt_error_none;

    assert(handlerData != NULL);

    if (valString == NULL)
    {
        err = dropt_error_insufficient_args;
    }

    if (err == dropt_error_none) { *((const dropt_char_t**) handlerData) = valString; }
    return err;
}


/** isValidOption
  *
  * PARAMETERS:
  *     IN option : Specification for an individual option.
  *
  * RETURNS:
  *     true if the specified option is valid, false if it's a sentinel
  *       value.
  */
static bool
isValidOption(const dropt_option_t* option)
{
    return    option != NULL
           && !(   option->longName == NULL
                && option->shortName == T('\0'));
}


/** findOptionLong
  *
  *     Finds the option specification for a "long" option (i.e., an
  *     option of the form "--option").
  *
  * PARAMETERS:
  *     IN options    : The list of option specifications.
  *     IN longName   : The "long" option to search for.
  *     caseSensitive : Pass true to use case-sensitive comparisons, false
  *                       otherwise.
  *
  * RETURNS:
  *     A pointer to the corresponding option specification or NULL if not
  *       found.
  */
static const dropt_option_t*
findOptionLong(const dropt_option_t* options, const dropt_char_t* longName, bool caseSensitive)
{
    const dropt_option_t* option;
    int (*cmp)(const dropt_char_t*, const dropt_char_t*) = caseSensitive ? dropt_strcmp : dropt_stricmp;

    assert(options != NULL);
    assert(longName != NULL);
    for (option = options; isValidOption(option); option++)
    {
        if (   option->longName != NULL
            && cmp(longName, option->longName) == 0)
        {
            return option;
        }
    }
    return NULL;
}


/** findOptionShort
  *
  *     Finds the option specification for a "short" option (i.e., an
  *     option of the form "-o").
  *
  * PARAMETERS:
  *     IN options    : The list of option specifications.
  *     shortName     : The "short" option to search for.
  *     caseSensitive : Pass true to use case-sensitive comparisons, false
  *                       otherwise.
  *
  * RETURNS:
  *     A pointer to the corresponding option specification or NULL if not
  *       found.
  */
static const dropt_option_t*
findOptionShort(const dropt_option_t* options, dropt_char_t shortName, bool caseSensitive)
{
    const dropt_option_t* option;
    assert(options != NULL);
    assert(shortName != T('\0'));
    for (option = options; isValidOption(option); option++)
    {
        if (   shortName == option->shortName
            || (!caseSensitive && dropt_tolower(shortName) == dropt_tolower(option->shortName)))
        {
            return option;
        }
    }
    return NULL;
}


/** dropt_set_error_details
  *
  *     Generates error details in the options context.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     err            : The error code.
  *     IN optionName  : The name of the option we failed on.
  *     IN optionValue : The value of the option we failed on.
  *                      Pass NULL if unwanted.
  */
static void
dropt_set_error_details(dropt_context_t* context, dropt_error_t err,
                        const dropt_char_t* optionName, const dropt_char_t* optionValue)
{
    assert(context != NULL);
    assert(optionName != NULL);

    context->errorDetails.err = err;

    free(context->errorDetails.optionName);
    free(context->errorDetails.optionValue);

    context->errorDetails.optionName = dropt_strdup(optionName);
    context->errorDetails.optionValue = optionValue != NULL
                                        ? dropt_strdup(optionValue)
                                        : NULL;

    if (err != dropt_error_custom)
    {
        /* The message will be generated lazily on retrieval. */
        free(context->errorDetails.message);
        context->errorDetails.message = NULL;
    }
}


/** setShortOptionErrorDetails
  *
  *     Generates error details in the options context.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     err            : The error code.
  *     shortName      : the "short" name of the option we failed on.
  *     IN optionValue : The value of the option we failed on.
  *                      Pass NULL if unwanted.
  */
static void
setShortOptionErrorDetails(dropt_context_t* context, dropt_error_t err,
                           dropt_char_t shortName, const dropt_char_t* optionValue)
{
    dropt_char_t shortNameBuf[3] = T("-?");

    assert(shortName != T('\0'));

    shortNameBuf[1] = shortName;

    dropt_set_error_details(context, err, shortNameBuf, optionValue);
}


/** dropt_get_error
  *
  * PARAMETERS:
  *     IN context : The options context.
  *
  * RETURNS:
  *     The current error code waiting in the options context.
  */
dropt_error_t
dropt_get_error(const dropt_context_t* context)
{
    assert(context != NULL);
    return context->errorDetails.err;
}


/** dropt_get_error_details
  *
  *     Retrieves details about the current error.
  *
  * PARAMETERS:
  *     IN context      : The options context.
  *     OUT optionName  : On output, the name of the option we failed on.
  *                       Pass NULL if unwanted.
  *     OUT optionValue : On output, the value of the option we failed on.
  *                       Pass NULL if unwanted.
  */
void
dropt_get_error_details(const dropt_context_t* context,
                        dropt_char_t** optionName, dropt_char_t** optionValue)
{
    if (optionName != NULL)
    {
        *optionName = context->errorDetails.optionName;
    }

    if (optionValue != NULL)
    {
        *optionValue = context->errorDetails.optionValue;
    }
}


/** dropt_set_error_message
  *
  *     Sets a custom error message in the options context.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     IN message     : The error message.
  */
void
dropt_set_error_message(dropt_context_t* context, const dropt_char_t* message)
{
    assert(context != NULL);

    free(context->errorDetails.message);

    context->errorDetails.err = dropt_error_custom;
    context->errorDetails.message = (message != NULL) ? dropt_strdup(message) : NULL;
}


#ifndef DROPT_NO_STRING_BUFFERS
/** dropt_get_error_message
  *
  * PARAMETERS:
  *     IN context : The options context.
  *
  * RETURNS:
  *     The current error message waiting in the options context or the
  *       empty string if there are no errors.
  */
const dropt_char_t*
dropt_get_error_message(dropt_context_t* context)
{
    dropt_char_t* s = NULL;

    assert(context != NULL);

    if (context->errorDetails.message == NULL)
    {
        switch (context->errorDetails.err)
        {
            case dropt_error_none:
                break;

            case dropt_error_invalid:
                s = dropt_aprintf(T("Invalid option: %s"), context->errorDetails.optionName);
                break;
            case dropt_error_insufficient_args:
                s = dropt_aprintf(T("Value required after option %s"),
                                  context->errorDetails.optionName);
                break;
            case dropt_error_mismatch:
                if (context->errorDetails.optionValue == NULL)
                {
                    s = dropt_aprintf(T("Invalid value for option %s"),
                                      context->errorDetails.optionName);
                }
                else
                {
                    s = dropt_aprintf(T("Invalid value for option %s: %s"),
                                      context->errorDetails.optionName,
                                      context->errorDetails.optionValue);
                }
                break;
            case dropt_error_overflow:
                if (context->errorDetails.optionValue == NULL)
                {
                    s = dropt_aprintf(T("Integer overflow for option %s"),
                                      context->errorDetails.optionName);
                }
                else
                {
                    s = dropt_aprintf(T("Integer overflow for option %s: %s"),
                                      context->errorDetails.optionName,
                                      context->errorDetails.optionValue);
                }
                break;
            case dropt_error_custom:
                break;
            case dropt_error_unknown:
            default:
                s = dropt_aprintf(T("Unknown error handling option %s."),
                                  context->errorDetails.optionName);
                break;
        }

        if (context->errorDetails.err != dropt_error_custom) /* Leave custom error messages alone. */
        {
            free(context->errorDetails.message);
            context->errorDetails.message = s;
        }
    }

    return (context->errorDetails.message == NULL)
           ? T("")
           : context->errorDetails.message;
}


/** dropt_get_help
  *
  * PARAMETERS:
  *     IN options : The list of option specifications.
  *     compact    : Pass false to include blank lines between options.
  *
  * RETURNS:
  *     An allocated help string for the available options.  The caller is
  *       responsible for calling free() on it when no longer needed.
  */
dropt_char_t*
dropt_get_help(const dropt_option_t* options, dropt_bool_t compact)
{
    dropt_char_t* helpText = NULL;
    dropt_stringstream* ss = dropt_ssopen();

    assert(options != NULL);

    if (ss != NULL)
    {
        static const int maxWidth = 4;
        const dropt_option_t* option;

        for (option = options; isValidOption(option); option++)
        {
            int n = 0;

            /* Undocumented option.  Ignore it and move on. */
            if (option->description == NULL || option->attr & dropt_attr_hidden)
            {
                continue;
            }

            if (option->longName != NULL && option->shortName != T('\0'))
            {
                /* Both shortName and longName */
                n = dropt_ssprintf(ss, T("  -%c, --%s"), option->shortName, option->longName);
            }
            else if (option->longName != NULL)
            {
                /* longName only */
                n = dropt_ssprintf(ss, T("  --%s"), option->longName);
            }
            else if (option->shortName != T('\0'))
            {
                /* shortName only */
                n = dropt_ssprintf(ss, T("  -%c"), option->shortName);
            }
            else
            {
                assert(!"No option name specified.");
                break;
            }

            if (n < 0) { n = 0; }

            if (option->argDescription != NULL)
            {
                int m = dropt_ssprintf(ss,
                                       (option->attr & dropt_attr_optional_val)
                                       ? T("[=%s]")
                                       : T("=%s"),
                                       option->argDescription);
                if (m > 0) { n += m; }
            }

            if (n > maxWidth)
            {
                dropt_ssprintf(ss, T("\n"));
                n = 0;
            }
            dropt_ssprintf(ss, T("%*s  %s\n"),
                           maxWidth - n, T(""),
                           option->description);
            if (!compact) { dropt_ssprintf(ss, T("\n")); }
        }
        helpText = dropt_ssfinalize(ss);
        dropt_ssclose(ss);
    }

    return helpText;
}


/** dropt_print_help
  *
  *     Prints help for the available options.
  *
  * PARAMETERS:
  *     IN/OUT f   : The file stream to print to.
  *     IN options : The list of option specifications.
  *     compact    : Pass false to include blank lines between options.
  */
void
dropt_print_help(FILE* f, const dropt_option_t* options, dropt_bool_t compact)
{
    dropt_char_t* helpText = dropt_get_help(options, compact);
    if (helpText != NULL)
    {
        dropt_fputs(helpText, f);
        free(helpText);
    }
}
#endif /* DROPT_NO_STRING_BUFFERS */


/** set
  *
  *     Sets the value for a specified option by invoking the option's
  *     handler callback.
  *
  * PARAMETERS:
  *     IN option    : The option.
  *     IN valString : The option's value.  May be NULL.
  *
  * RETURNS:
  *     An error code.
  */
static dropt_error_t
set(const dropt_option_t* option, const dropt_char_t* valString)
{
    dropt_error_t err = dropt_error_none;

    assert(option != NULL);

    if (option->handler != NULL)
    {
        err = option->handler(valString, option->handlerData);
    }
    else
    {
        assert(!"No option handler specified.");
    }
    return err;
}


/** parseArg
  *
  *     Helper function to dropt_parse to deal with consuming possibly
  *     optional arguments.
  *
  * PARAMETERS:
  *     IN/OUT ps : The current parse state.
  *
  * RETURNS:
  *     An error code.
  */
static dropt_error_t
parseArg(parseState_t* ps)
{
    dropt_error_t err = dropt_error_none;

    bool consumeNextArg = false;

    if (   ps->option->argDescription != NULL
        && ps->valString == NULL
        && *(ps->argNext) != NULL)
    {
        /* The option expects an argument, but none was specified with '='.
         * Try using the next item from the command-line.
         */
        consumeNextArg = true;
        ps->valString = *(ps->argNext);
    }

    /* Even for options that don't ask for arguments, always parse and
     * consume an argument that was specified with '='.
     */
    err = set(ps->option, ps->valString);

    if (   err != dropt_error_none
        && (ps->option->attr & dropt_attr_optional_val)
        && consumeNextArg
        && ps->valString != NULL)
    {
        /* The option's handler didn't like the argument we fed it.  If the
         * argument was optional, try again.
         */
        consumeNextArg = false;
        ps->valString = NULL;
        err = set(ps->option, NULL);
    }

    if (err == dropt_error_none && consumeNextArg) { ps->argNext++; }
    return err;
}


/** dropt_parse
  *
  *     Parses command-line options.
  *
  * PARAMETERS:
  *     IN context  : The options context.
  *     IN/OUT argv : The list of command-line arguments, not including the
  *                     initial program name.  Must be terminated with a
  *                     NULL sentinel value.
  *                   Note that the command-line arguments might be
  *                     mutated in the process.
  *
  * RETURNS:
  *     A pointer to the first unprocessed element in argv.
  *     Never returns NULL.
  */
dropt_char_t**
dropt_parse(dropt_context_t* context,
             dropt_char_t** argv)
{
    dropt_error_t err = dropt_error_none;

    dropt_char_t* arg;
    parseState_t ps;

    assert(context != NULL);
    assert(argv != NULL);

    ps.option = NULL;
    ps.valString = NULL;
    ps.argNext = argv;

    while (   (arg = *ps.argNext) != NULL
           && arg[0] == T('-'))
    {
        assert(err == dropt_error_none);

        if (arg[1] == T('\0'))
        {
            /* - */
            goto exit;
        }

        ps.argNext++;

        if (arg[1] == T('-'))
        {
            dropt_char_t* longName = arg + 2;
            if (longName[0] == T('\0'))
            {
                /* -- */
                goto exit;
            }
            else
            {
                /* --longName */
                {
                    dropt_char_t* p = dropt_strchr(longName, T('='));
                    if (p != NULL)
                    {
                        *p = T('\0');
                        ps.valString = p + 1;
                    }
                }

                ps.option = findOptionLong(context->options, longName,
                                           context->caseSensitive);
                if (ps.option == NULL)
                {
                    err = dropt_error_invalid;
                    dropt_set_error_details(context, err, arg, NULL);
                    goto exit;
                }
                else
                {
                    err = parseArg(&ps);
                    if (err != dropt_error_none)
                    {
                        dropt_set_error_details(context, err, arg, ps.valString);
                        goto exit;
                    }
                }

                if (ps.option->attr & dropt_attr_halt) { goto exit; }
            }
        }
        else
        {
            size_t len;
            size_t j;

            {
                const dropt_char_t* p = dropt_strchr(arg, T('='));
                if (p == NULL)
                {
                    len = dropt_strlen(arg);
                    ps.valString = NULL;
                }
                else
                {
                    len = p - arg;
                    ps.valString = p + 1;
                }
            }

            for (j = 1; j < len; j++)
            {
                ps.option = findOptionShort(context->options, arg[j],
                                            context->caseSensitive);
                if (ps.option == NULL)
                {
                    err = dropt_error_invalid;
                    setShortOptionErrorDetails(context, err, arg[j], NULL);
                    goto exit;
                }
                else
                {
                    if (j + 1 == len)
                    {
                        /* The last short option in a condensed list gets
                         * to use an argument.
                         */
                        err = parseArg(&ps);
                        if (err != dropt_error_none)
                        {
                            setShortOptionErrorDetails(context, err, arg[j], ps.valString);
                            goto exit;
                        }
                    }
                    else if (   ps.option->argDescription == NULL
                             || (ps.option->attr & dropt_attr_optional_val))
                    {
                        err = set(ps.option, NULL);
                        if (err != dropt_error_none)
                        {
                            setShortOptionErrorDetails(context, err, arg[j], NULL);
                            goto exit;
                        }
                    }
                    else
                    {
                        /* Short options with required arguments can't be
                         * used in condensed lists except in the last
                         * position.
                         *
                         * e.g. -abcd arg
                         *          ^
                         */
                        err = dropt_error_insufficient_args;
                        setShortOptionErrorDetails(context, err, arg[j], NULL);
                    }
                }

                if (ps.option->attr & dropt_attr_halt) { goto exit; }
            }
        }

        ps.option = NULL;
        ps.valString = NULL;
    }

exit:
    return ps.argNext;
}


/** dropt_new_context
  *
  *     Creates a new options context.
  *
  * RETURNS:
  *     An allocated options context.  The caller is responsible for
  *       freeing it with dropt_free_context when no longer needed.
  *     Returns NULL on error.
  */
dropt_context_t*
dropt_new_context(void)
{
    dropt_context_t* context = malloc(sizeof *context);
    if (context != NULL)
    {
        context->options = NULL;
        context->caseSensitive = true;
        context->errorDetails.err = dropt_error_none;
        context->errorDetails.optionName = NULL;
        context->errorDetails.optionValue = NULL;
        context->errorDetails.message = NULL;
    }
    return context;
}


/** dropt_free_context
  *
  *     Frees an options context.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context to free.
  */
void
dropt_free_context(dropt_context_t* context)
{
    if (context != NULL)
    {
        free(context->errorDetails.optionName);
        free(context->errorDetails.optionValue);
        free(context->errorDetails.message);
        free(context);
    }
}


/** dropt_set_options
  *
  *     Specifies a list of options to use with an options context.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     IN options     : The list of option specifications.
  */
void
dropt_set_options(dropt_context_t* context, const dropt_option_t* options)
{
    assert(context != NULL);
    context->options = options;
}


/** dropt_set_case_sensitive
  *
  *     Specifies whether options should be case-sensitive. (Options are
  *     case-sensitive by default.)
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     caseSensitive  : Pass 1 if options should be case-sensitive,
  *                        0 otherwise.
  */
void
dropt_set_case_sensitive(dropt_context_t* context, dropt_bool_t caseSensitive)
{
    assert(context != NULL);
    context->caseSensitive = (caseSensitive != 0);
}
