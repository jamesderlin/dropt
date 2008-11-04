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
#include <assert.h>

#include "dropt.h"
#include "dropt_string.h"

#if defined _UNICODE || defined UNICODE
    #define T(s) (L ## s)
#else
    #define T(s) s
#endif

#define IS_CUSTOM_ERROR(e) ((e) >= dropt_error_custom_start && (e) <= dropt_error_custom_last)

typedef enum { false, true } bool;

struct dropt_context_t
{
    const dropt_option_t* options;
    bool caseSensitive;

    dropt_error_handler_t errorHandler;
    void* errorHandlerData;

    struct
    {
        dropt_error_t err;
        dropt_char_t* optionName;
        dropt_char_t* valueString;
        dropt_char_t* message;
    } errorDetails;
};

typedef struct
{
    const dropt_option_t* option;
    const dropt_char_t* valueString;
    dropt_char_t** argNext;
} parseState_t;


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
  *     Finds the option specification for a "long" option (i.e., an option
  *     of the form "--option").
  *
  * PARAMETERS:
  *     IN options    : The list of option specifications.
  *     IN longName   : The "long" option to search for (excluding leading
  *                       dashes).
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

    if (options == NULL)
    {
        assert(!"No options specified.");
        return NULL;
    }

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

    if (options == NULL)
    {
        assert(!"No options specified.");
        return NULL;
    }

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
  *     IN valueString : The value of the option we failed on.
  *                      Pass NULL if unwanted.
  */
static void
dropt_set_error_details(dropt_context_t* context, dropt_error_t err,
                        const dropt_char_t* optionName, const dropt_char_t* valueString)
{
    if (context == NULL)
    {
        assert(!"No dropt context specified.");
        return;
    }

    if (optionName == NULL)
    {
        assert(!"No option specified.");
        return;
    }

    context->errorDetails.err = err;

    free(context->errorDetails.optionName);
    free(context->errorDetails.valueString);

    context->errorDetails.optionName = dropt_strdup(optionName);
    context->errorDetails.valueString = (valueString != NULL)
                                        ? dropt_strdup(valueString)
                                        : NULL;

    if (!IS_CUSTOM_ERROR(err))
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
  *     IN valueString : The value of the option we failed on.
  *                      Pass NULL if unwanted.
  */
static void
setShortOptionErrorDetails(dropt_context_t* context, dropt_error_t err,
                           dropt_char_t shortName, const dropt_char_t* valueString)
{
    dropt_char_t shortNameBuf[3] = T("-?");

    assert(shortName != T('\0'));

    shortNameBuf[1] = shortName;

    dropt_set_error_details(context, err, shortNameBuf, valueString);
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
    if (context == NULL)
    {
        assert(!"No dropt context specified.");
        return dropt_error_bad_configuration;
    }
    return context->errorDetails.err;
}


/** dropt_get_error_details
  *
  *     Retrieves details about the current error.
  *
  * PARAMETERS:
  *     IN context      : The options context.
  *     OUT optionName  : On output, the name of the option we failed on.
  *                         Do not free this string.
  *                       Pass NULL if unwanted.
  *     OUT valueString : On output, the value (possibly NULL) of the
  *                         option we failed on.  Do not free this string.
  *                       Pass NULL if unwanted.
  */
void
dropt_get_error_details(const dropt_context_t* context,
                        dropt_char_t** optionName, dropt_char_t** valueString)
{
    if (optionName != NULL) { *optionName = context->errorDetails.optionName; }
    if (valueString != NULL) { *valueString = context->errorDetails.valueString; }
}


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
    if (context == NULL)
    {
        assert(!"No dropt context specified.");
        return T("");
    }

    if (context->errorDetails.message == NULL)
    {
        if (context->errorHandler != NULL)
        {
            context->errorDetails.message
                = context->errorHandler(context->errorDetails.err,
                                        context->errorDetails.optionName,
                                        context->errorDetails.valueString,
                                        context->errorHandlerData);
        }
        else
        {
#ifndef DROPT_NO_STRING_BUFFERS
            context->errorDetails.message
                = dropt_default_error_handler(context->errorDetails.err,
                                              context->errorDetails.optionName,
                                              context->errorDetails.valueString);
#endif
        }
    }

    return (context->errorDetails.message == NULL)
           ? T("")
           : context->errorDetails.message;
}


/** dropt_clear_error
  *
  *     Clears the error waiting in the dropt context.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context to free.
  *                      May be NULL.
  */
void
dropt_clear_error(dropt_context_t* context)
{
    if (context != NULL)
    {
        context->errorDetails.err = dropt_error_none;

        free(context->errorDetails.optionName);
        context->errorDetails.optionName = NULL;

        free(context->errorDetails.valueString);
        context->errorDetails.valueString = NULL;

        free(context->errorDetails.message);
        context->errorDetails.message = NULL;
    }
}


#ifndef DROPT_NO_STRING_BUFFERS
/** dropt_default_error_handler
  *
  *     Default error handler.
  *
  * PARAMETERS:
  *     error          : The error code.
  *     IN optionName  : The name of the option we failed on.
  *     IN valueString : The value of the option we failed on.
  *                      Pass NULL if unwanted.
  *
  * RETURNS:
  *     An allocated string for the given error.  The caller is responsible
  *       for calling free() on it when no longer needed.
  *     May return NULL.
  */
dropt_char_t*
dropt_default_error_handler(dropt_error_t error,
                            const dropt_char_t* optionName,
                            const dropt_char_t* valueString)
{
    dropt_char_t* s = NULL;
    bool hasValue = valueString != NULL;

    switch (error)
    {
        case dropt_error_none:
            break;

        case dropt_error_bad_configuration:
            s = dropt_strdup(T("Invalid option configuration."));
            break;

        case dropt_error_invalid:
            s = dropt_asprintf(T("Invalid option: %s"), optionName);
            break;
        case dropt_error_insufficient_args:
            s = dropt_asprintf(T("Value required after option %s"),
                              optionName);
            break;
        case dropt_error_mismatch:
            s = dropt_asprintf(T("Invalid value for option %s%s%s"),
                               optionName,
                               hasValue ? T(": ") : T(""),
                               hasValue ? valueString : T(""));
            break;
        case dropt_error_overflow:
            s = dropt_asprintf(T("Value too large for option %s%s%s"),
                               optionName,
                               hasValue ? T(": ") : T(""),
                               hasValue ? valueString : T(""));
            break;
        case dropt_error_underflow:
            s = dropt_asprintf(T("Value too small for option %s%s%s"),
                               optionName,
                               hasValue ? T(": ") : T(""),
                               hasValue ? valueString : T(""));
            break;
        case dropt_error_insufficient_memory:
            s = dropt_strdup(T("Insufficient memory."));
            break;
        case dropt_error_unknown:
        default:
            if (IS_CUSTOM_ERROR(error))
            {
                /* Do nothing.  The client is responsible for producing
                 * an appropriate error message.
                 */
            }
            else
            {
                s = dropt_asprintf(T("Unknown error handling option %s."), optionName);
            }
            break;
    }

    return s;
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
  *     Returns NULL on error.
  */
dropt_char_t*
dropt_get_help(const dropt_option_t* options, dropt_bool_t compact)
{
    dropt_char_t* helpText = NULL;
    dropt_stringstream* ss = dropt_ssopen();

    if (options == NULL)
    {
        assert(!"No option list specified.");
    }
    else if (ss != NULL)
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
  *     IN/OUT context : The options context.
  *     IN option      : The option.
  *     IN valueString : The option's value.  May be NULL.
  *
  * RETURNS:
  *     An error code.
  */
static dropt_error_t
set(dropt_context_t* context, const dropt_option_t* option, const dropt_char_t* valueString)
{
    dropt_error_t err = dropt_error_none;

    assert(option != NULL);

    if (option->handler != NULL)
    {
        err = option->handler(context, valueString, option->handlerData);
    }
    else
    {
        assert(!"No option handler specified.");
        err = dropt_error_bad_configuration;
    }
    return err;
}


/** parseArg
  *
  *     Helper function to dropt_parse to deal with consuming possibly
  *     optional arguments.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     IN/OUT ps      : The current parse state.
  *
  * RETURNS:
  *     An error code.
  */
static dropt_error_t
parseArg(dropt_context_t* context, parseState_t* ps)
{
    dropt_error_t err = dropt_error_none;

    bool consumeNextArg = false;

    if (   ps->option->argDescription != NULL
        && ps->valueString == NULL
        && *(ps->argNext) != NULL)
    {
        /* The option expects an argument, but none was specified with '='.
         * Try using the next item from the command-line.
         */
        consumeNextArg = true;
        ps->valueString = *(ps->argNext);
    }

    /* Even for options that don't ask for arguments, always parse and
     * consume an argument that was specified with '='.
     */
    err = set(context, ps->option, ps->valueString);

    if (   err != dropt_error_none
        && (ps->option->attr & dropt_attr_optional_val)
        && consumeNextArg
        && ps->valueString != NULL)
    {
        /* The option's handler didn't like the argument we fed it.  If the
         * argument was optional, try again without it.
         */
        consumeNextArg = false;
        ps->valueString = NULL;
        err = set(context, ps->option, NULL);
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
  *                   Must not be NULL.
  *     IN/OUT argv : The list of command-line arguments, not including the
  *                     initial program name.  Must be terminated with a
  *                     NULL sentinel value.
  *                   Note that the command-line arguments might be
  *                     (non-destructively) mutated in the process.
  *
  * RETURNS:
  *     A pointer to the first unprocessed element in argv.
  */
dropt_char_t**
dropt_parse(dropt_context_t* context,
            dropt_char_t** argv)
{
    dropt_error_t err = dropt_error_none;

    dropt_char_t* arg;
    parseState_t ps;

    ps.option = NULL;
    ps.valueString = NULL;
    ps.argNext = argv;

    if (argv == NULL)
    {
        /* Nothing to do. */
        goto exit;
    }

    if (context == NULL)
    {
        assert(!"No dropt context specified.");
        goto exit;
    }

#ifdef DROPT_NO_STRING_BUFFERS
    if (context->errorHandler == NULL)
    {
        assert(!"No error handler specified.");
        dropt_set_error_details(context, dropt_error_bad_configuration, "", NULL);
        goto exit;
    }
#endif

    while (   (arg = *ps.argNext) != NULL
           && arg[0] == T('-'))
    {
        assert(err == dropt_error_none);

        if (arg[1] == T('\0'))
        {
            /* - */

            /* This intentionally leaves "-" unprocessed for the caller to
             * deal with.  This allows construction of programs that treat
             * "-" to mean "stdin". (See the documentation.)
             */
            goto exit;
        }

        ps.argNext++;

        if (arg[1] == T('-'))
        {
            dropt_char_t* longName = arg + 2;
            if (longName[0] == T('\0'))
            {
                /* -- */

                /* This is used to mark the end of the option processing
                 * to prevent some arguments with leading '-' characters
                 * from being treated as options. (See the documentation.)
                 *
                 * Don't pass this back to the caller.
                 */
                goto exit;
            }
            else if (longName[0] == T('='))
            {
                /* Deal with the pathological case of a user supplying
                 * "--=".  It's preferable to do this before we mutate the
                 * string.
                 */
                err = dropt_error_invalid;
                dropt_set_error_details(context, err, arg, NULL);
                goto exit;
            }
            else
            {
                /* --longName */
                dropt_char_t* p = dropt_strchr(longName, T('='));
                if (p != NULL)
                {
                    *p = T('\0');
                    ps.valueString = p + 1;
                }

                ps.option = findOptionLong(context->options, longName,
                                           context->caseSensitive);
                if (ps.option == NULL)
                {
                    err = dropt_error_invalid;
                    dropt_set_error_details(context, err, arg, NULL);
                }
                else
                {
                    err = parseArg(context, &ps);
                    if (err != dropt_error_none)
                    {
                        dropt_set_error_details(context, err, arg, ps.valueString);
                    }
                }

                /* Undo the mutation we made. */
                if (p != NULL) { *p = T('='); }

                if (   err != dropt_error_none
                    || ps.option->attr & dropt_attr_halt)
                {
                    goto exit;
                }
            }
        }
        else
        {
            /* Short name. */
            size_t len;
            size_t j;

            if (arg[1] == T('='))
            {
                /* Deal with the pathological case of a user supplying
                 * "-=".
                 */
                err = dropt_error_invalid;
                dropt_set_error_details(context, err, arg, NULL);
                goto exit;
            }
            else
            {
                const dropt_char_t* p = dropt_strchr(arg, T('='));
                if (p == NULL)
                {
                    len = dropt_strlen(arg);
                    ps.valueString = NULL;
                }
                else
                {
                    len = p - arg;
                    ps.valueString = p + 1;
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
                        err = parseArg(context, &ps);
                        if (err != dropt_error_none)
                        {
                            setShortOptionErrorDetails(context, err, arg[j], ps.valueString);
                            goto exit;
                        }
                    }
                    else if (   ps.option->argDescription == NULL
                             || (ps.option->attr & dropt_attr_optional_val))
                    {
                        err = set(context, ps.option, NULL);
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
                        goto exit;
                    }
                }

                if (ps.option->attr & dropt_attr_halt) { goto exit; }
            }
        }

        ps.option = NULL;
        ps.valueString = NULL;
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
        context->errorHandler = NULL;
        context->errorHandlerData = NULL;
        context->errorDetails.err = dropt_error_none;
        context->errorDetails.optionName = NULL;
        context->errorDetails.valueString = NULL;
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
  *                      May be NULL.
  */
void
dropt_free_context(dropt_context_t* context)
{
    dropt_clear_error(context);
    free(context);
}


/** dropt_set_options
  *
  *     Specifies a list of options to use with an options context.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     IN options     : The list of option specifications.
  *
  * RETURNS:
  *     dropt_error_none
  *     dropt_error_bad_configuration
  */
dropt_error_t
dropt_set_options(dropt_context_t* context, const dropt_option_t* options)
{
    dropt_error_t err = dropt_error_none;

    if (context == NULL)
    {
        assert(!"No dropt context specified.");
        err = dropt_error_bad_configuration;
        goto exit;
    }

    context->options = options;

    /* Sanity-check the options. */
    if (options == NULL)
    {
        assert(!"No option list specified.");
        err = dropt_error_bad_configuration;
    }
    else
    {
        const dropt_option_t* option;
        for (option = options; isValidOption(option); option++)
        {
            if (   option->shortName == T('=')
                || (   option->longName != NULL
                    && dropt_strcmp(option->longName, T("=")) == 0))
            {
                assert(!"Invalid option list. "
                        "'=' may not be used for an option shortname or longname.");
                err = dropt_error_bad_configuration;
                goto exit;
            }
        }
    }

exit:
    return err;
}


/** dropt_set_case_sensitive
  *
  *     Specifies whether options should be case-sensitive. (Options are
  *     case-sensitive by default.)
  *
  *     Not recommended for non-ASCII strings.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     caseSensitive  : Pass 1 if options should be case-sensitive,
  *                        0 otherwise.
  */
void
dropt_set_case_sensitive(dropt_context_t* context, dropt_bool_t caseSensitive)
{
    if (context == NULL)
    {
        assert(!"No dropt context specified.");
        return;
    }

    context->caseSensitive = (caseSensitive != 0);
}


/** dropt_set_error_handler
  *
  *     Sets the callback function used to generate error strings from
  *     error codes.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     handler        : The error handler callback.
  *     handlerData    : Caller-defined callback data.
  */
void
dropt_set_error_handler(dropt_context_t* context, dropt_error_handler_t handler, void* handlerData)
{
    if (context == NULL)
    {
        assert(!"No dropt context specified.");
        return;
    }

    context->errorHandler = handler;
    context->errorHandlerData = handlerData;
}
