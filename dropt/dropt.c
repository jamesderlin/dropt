/** dropt.c
  *
  * A deliberately rudimentary command-line option parser.
  *
  * Copyright (c) 2006-2010 James D. Lin <jameslin@cal.berkeley.edu>
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <assert.h>

#include "dropt.h"
#include "dropt_string.h"

#define ARRAY_LENGTH(array) (sizeof (array) / sizeof (array)[0])

#define OPTION_TAKES_ARG(option) ((option)->arg_description != NULL)

typedef enum { false, true } bool;

enum
{
    default_help_indent = 2,
    default_description_start_column = 6
};

struct dropt_context
{
    const dropt_option* options;

    bool allowConcatenatedArgs;

    dropt_error_handler_func errorHandler;
    void* errorHandlerData;

    struct
    {
        dropt_error err;
        dropt_char* optionName;
        dropt_char* optionArgument;
        dropt_char* message;
    } errorDetails;

    dropt_strncmp_func strncmp;
};

typedef struct
{
    const dropt_option* option;
    const dropt_char* optionArgument;
    dropt_char** argNext;
    int argsLeft;
} parseState;


/** is_valid_option
  *
  * PARAMETERS:
  *     IN option : Specification for an individual option.
  *
  * RETURNS:
  *     true if the specified option is valid, false if it's a sentinel
  *       value.
  */
static bool
is_valid_option(const dropt_option* option)
{
    return    option != NULL
           && !(   option->long_name == NULL
                && option->short_name == DROPT_TEXT_LITERAL('\0')
                && option->description == NULL
                && option->arg_description == NULL
                && option->handler == NULL
                && option->handler_data == NULL
                && option->attr == 0);
}


/** find_long_option
  *
  *     Finds the option specification for a "long" option (i.e., an option
  *     of the form "--option").
  *
  * PARAMETERS:
  *     IN context     : The options context.
  *     IN longName    : The "long" option to search for (excluding leading
  *                        dashes).
  *                      This might not be NUL-terminated.
  *     IN longNameLen : The length of the longName string, excluding a
  *                        NUL-terminator.
  *
  * RETURNS:
  *     A pointer to the corresponding option specification or NULL if not
  *       found.
  */
static const dropt_option*
find_long_option(const dropt_context* context,
                 const dropt_char* longName, size_t longNameLen)
{
    dropt_strncmp_func cmp;
    const dropt_option* option;

    assert(context != NULL);
    assert(longName != NULL);

    cmp = (context->strncmp != NULL)
          ? context->strncmp
          : dropt_strncmp;

    for (option = context->options; is_valid_option(option); option++)
    {
        if (   option->long_name != NULL
            && longNameLen == dropt_strlen(option->long_name)
            && cmp(longName, option->long_name, longNameLen) == 0)
        {
            return option;
        }
    }
    return NULL;
}


/** find_short_option
  *
  *     Finds the option specification for a "short" option (i.e., an
  *     option of the form "-o").
  *
  * PARAMETERS:
  *     IN context   : The options context.
  *     IN shortName : The "short" option to search for.
  *
  * RETURNS:
  *     A pointer to the corresponding option specification or NULL if not
  *       found.
  */
static const dropt_option*
find_short_option(const dropt_context* context, dropt_char shortName)
{
    const dropt_option* option;

    assert(context != NULL);
    assert(shortName != DROPT_TEXT_LITERAL('\0'));

    for (option = context->options; is_valid_option(option); option++)
    {
        if (   shortName == option->short_name
            || (   context->strncmp != NULL
                && context->strncmp(&shortName, &option->short_name, 1) == 0))
        {
            return option;
        }
    }
    return NULL;
}


/** set_error_details
  *
  *     Generates error details in the options context.
  *
  * PARAMETERS:
  *     IN/OUT context    : The options context.
  *                         Must not be NULL.
  *     IN err            : The error code.
  *     IN optionName     : The name of the option we failed on.
  *                         This might not be NUL-terminated.
  *                         Must not be NULL.
  *     IN optionNameLen  : The length of the optionName string, excluding
  *                           a NUL-terminator.
  *     IN optionArgument : The value of the option we failed on.
  *                         Pass NULL if unwanted.
  */
static void
set_error_details(dropt_context* context, dropt_error err,
                  const dropt_char* optionName, size_t optionNameLen,
                  const dropt_char* optionArgument)
{
    assert(context != NULL);
    assert(optionName != NULL);

    context->errorDetails.err = err;

    free(context->errorDetails.optionName);
    free(context->errorDetails.optionArgument);

    context->errorDetails.optionName = dropt_strndup(optionName, optionNameLen);
    context->errorDetails.optionArgument = (optionArgument != NULL)
                                           ? dropt_strdup(optionArgument)
                                           : NULL;

    /* The message will be generated lazily on retrieval. */
    free(context->errorDetails.message);
    context->errorDetails.message = NULL;
}


/** set_short_option_error_details
  *
  *     Generates error details in the options context.
  *
  * PARAMETERS:
  *     IN/OUT context    : The options context.
  *     IN err            : The error code.
  *     IN shortName      : the "short" name of the option we failed on.
  *     IN optionArgument : The value of the option we failed on.
  *                         Pass NULL if unwanted.
  */
static void
set_short_option_error_details(dropt_context* context, dropt_error err,
                               dropt_char shortName, const dropt_char* optionArgument)
{
    dropt_char shortNameBuf[] = DROPT_TEXT_LITERAL("-?");

    assert(context != NULL);
    assert(shortName != DROPT_TEXT_LITERAL('\0'));

    shortNameBuf[1] = shortName;

    set_error_details(context, err,
                      shortNameBuf, ARRAY_LENGTH(shortNameBuf) - 1,
                      optionArgument);
}


/** dropt_get_error
  *
  * PARAMETERS:
  *     IN context : The options context.
  *                  Must not be NULL.
  *
  * RETURNS:
  *     The current error code waiting in the options context.
  */
dropt_error
dropt_get_error(const dropt_context* context)
{
    if (context == NULL)
    {
        DROPT_MISUSE_PANIC("No dropt context specified.");
        return dropt_error_bad_configuration;
    }
    return context->errorDetails.err;
}


/** dropt_get_error_details
  *
  *     Retrieves details about the current error.
  *
  * PARAMETERS:
  *     IN context         : The options context.
  *     OUT optionName     : On output, the name of the option we failed
  *                            on.  Do not free this string.
  *                          Pass NULL if unwanted.
  *     OUT optionArgument : On output, the value (possibly NULL) of the
  *                            option we failed on.  Do not free this
  *                            string.
  *                          Pass NULL if unwanted.
  */
void
dropt_get_error_details(const dropt_context* context,
                        dropt_char** optionName, dropt_char** optionArgument)
{
    if (optionName != NULL) { *optionName = context->errorDetails.optionName; }
    if (optionArgument != NULL) { *optionArgument = context->errorDetails.optionArgument; }
}


/** dropt_get_error_message
  *
  * PARAMETERS:
  *     IN context : The options context.
  *                  Must not be NULL.
  *
  * RETURNS:
  *     The current error message waiting in the options context or the
  *       empty string if there are no errors.  Note that calling any dropt
  *       function other than dropt_get_error, dropt_get_error_details, and
  *       dropt_get_error_message may invalidate a previously-returned
  *       string.
  */
const dropt_char*
dropt_get_error_message(dropt_context* context)
{
    if (context == NULL)
    {
        DROPT_MISUSE_PANIC("No dropt context specified.");
        return DROPT_TEXT_LITERAL("");
    }

    if (context->errorDetails.err == dropt_error_none)
    {
        return DROPT_TEXT_LITERAL("");
    }

    if (context->errorDetails.message == NULL)
    {
        if (context->errorHandler != NULL)
        {
            context->errorDetails.message
                = context->errorHandler(context->errorDetails.err,
                                        context->errorDetails.optionName,
                                        context->errorDetails.optionArgument,
                                        context->errorHandlerData);
        }
        else
        {
#ifndef DROPT_NO_STRING_BUFFERS
            context->errorDetails.message
                = dropt_default_error_handler(context->errorDetails.err,
                                              context->errorDetails.optionName,
                                              context->errorDetails.optionArgument);
#endif
        }
    }

    return (context->errorDetails.message == NULL)
           ? DROPT_TEXT_LITERAL("Unknown error")
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
dropt_clear_error(dropt_context* context)
{
    if (context != NULL)
    {
        context->errorDetails.err = dropt_error_none;

        free(context->errorDetails.optionName);
        context->errorDetails.optionName = NULL;

        free(context->errorDetails.optionArgument);
        context->errorDetails.optionArgument = NULL;

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
  *     IN error          : The error code.
  *     IN optionName     : The name of the option we failed on.
  *     IN optionArgument : The value of the option we failed on.
  *                         Pass NULL if unwanted.
  *
  * RETURNS:
  *     An allocated string for the given error.  The caller is responsible
  *       for calling free() on it when no longer needed.
  *     May return NULL.
  */
dropt_char*
dropt_default_error_handler(dropt_error error,
                            const dropt_char* optionName,
                            const dropt_char* optionArgument)
{
    dropt_char* s = NULL;
    bool hasArgument = optionArgument != NULL;

    switch (error)
    {
        case dropt_error_none:
            break;

        case dropt_error_bad_configuration:
            s = dropt_strdup(DROPT_TEXT_LITERAL("Invalid option configuration."));
            break;

        case dropt_error_invalid_option:
            s = dropt_asprintf(DROPT_TEXT_LITERAL("Invalid option: %s"),
                               optionName);
            break;
        case dropt_error_insufficient_arguments:
            s = dropt_asprintf(DROPT_TEXT_LITERAL("Value required after option %s"),
                               optionName);
            break;
        case dropt_error_mismatch:
            s = dropt_asprintf(DROPT_TEXT_LITERAL("Invalid value for option %s%s%s"),
                               optionName,
                               hasArgument ? DROPT_TEXT_LITERAL(": ")
                                           : DROPT_TEXT_LITERAL(""),
                               hasArgument ? optionArgument
                                           : DROPT_TEXT_LITERAL(""));
            break;
        case dropt_error_overflow:
            s = dropt_asprintf(DROPT_TEXT_LITERAL("Value too large for option %s%s%s"),
                               optionName,
                               hasArgument ? DROPT_TEXT_LITERAL(": ")
                                           : DROPT_TEXT_LITERAL(""),
                               hasArgument ? optionArgument
                                           : DROPT_TEXT_LITERAL(""));
            break;
        case dropt_error_underflow:
            s = dropt_asprintf(DROPT_TEXT_LITERAL("Value too small for option %s%s%s"),
                               optionName,
                               hasArgument ? DROPT_TEXT_LITERAL(": ")
                                           : DROPT_TEXT_LITERAL(""),
                               hasArgument ? optionArgument
                                           : DROPT_TEXT_LITERAL(""));
            break;
        case dropt_error_insufficient_memory:
            s = dropt_strdup(DROPT_TEXT_LITERAL("Insufficient memory."));
            break;
        case dropt_error_unknown:
        default:
            s = dropt_asprintf(DROPT_TEXT_LITERAL("Unknown error handling option %s."), optionName);
            break;
    }

    return s;
}


/** dropt_get_help
  *
  * PARAMETERS:
  *     IN context    : The options context.
  *                     Must not be NULL.
  *     IN helpParams : The help parameters.
  *                     Pass NULL to use the default help parameters.
  *
  * RETURNS:
  *     An allocated help string for the available options.  The caller is
  *       responsible for calling free() on it when no longer needed.
  *     Returns NULL on error.
  */
dropt_char*
dropt_get_help(const dropt_context* context, const dropt_help_params* helpParams)
{
    dropt_char* helpText = NULL;
    dropt_stringstream* ss = dropt_ssopen();

    if (context == NULL)
    {
        DROPT_MISUSE_PANIC("No dropt context specified.");
    }
    else if (ss != NULL)
    {
        const dropt_option* option;
        dropt_help_params hp;

        if (helpParams == NULL)
        {
            dropt_init_help_params(&hp);
        }
        else
        {
            hp = *helpParams;
        }

        for (option = context->options; is_valid_option(option); option++)
        {
            bool hasLongName =    option->long_name != NULL
                               && option->long_name[0] != DROPT_TEXT_LITERAL('\0');
            bool hasShortName = option->short_name != DROPT_TEXT_LITERAL('\0');

            /* The number of characters printed on the current line so far. */
            int n;

            if (option->description == NULL || (option->attr & dropt_attr_hidden))
            {
                /* Undocumented option.  Ignore it and move on. */
                continue;
            }
            else if (hasLongName && hasShortName)
            {
                n = dropt_ssprintf(ss, DROPT_TEXT_LITERAL("%*s-%c, --%s"),
                                   hp.indent, DROPT_TEXT_LITERAL(""),
                                   option->short_name, option->long_name);
            }
            else if (hasLongName)
            {
                n = dropt_ssprintf(ss, DROPT_TEXT_LITERAL("%*s--%s"),
                                   hp.indent, DROPT_TEXT_LITERAL(""),
                                   option->long_name);
            }
            else if (hasShortName)
            {
                n = dropt_ssprintf(ss, DROPT_TEXT_LITERAL("%*s-%c"),
                                   hp.indent, DROPT_TEXT_LITERAL(""),
                                   option->short_name);
            }
            else if (option->description != NULL)
            {
                /* Comment text.  Don't bother with indentation. */
                dropt_ssprintf(ss, DROPT_TEXT_LITERAL("%s\n"), option->description);
                goto next;
            }
            else
            {
                DROPT_MISUSE_PANIC("No option name specified.");
                break;
            }

            if (n < 0) { n = 0; }

            if (option->arg_description != NULL)
            {
                int m = dropt_ssprintf(ss,
                                       (option->attr & dropt_attr_optional_val)
                                       ? DROPT_TEXT_LITERAL("[=%s]")
                                       : DROPT_TEXT_LITERAL("=%s"),
                                       option->arg_description);
                if (m > 0) { n += m; }
            }

            /* Check for equality to make sure that there's at least one
             * space between the option name and its description.
             */
            if ((unsigned int) n >= hp.description_start_column)
            {
                dropt_ssprintf(ss, DROPT_TEXT_LITERAL("\n"));
                n = 0;
            }

            {
                const dropt_char* line = option->description;
                while (line != NULL)
                {
                    int lineLen;
                    const dropt_char* nextLine;
                    const dropt_char* newline = dropt_strchr(line, DROPT_TEXT_LITERAL('\n'));

                    if (newline == NULL)
                    {
                        lineLen = dropt_strlen(line);
                        nextLine = NULL;
                    }
                    else
                    {
                        lineLen = newline - line;
                        nextLine = newline + 1;
                    }

                    dropt_ssprintf(ss, DROPT_TEXT_LITERAL("%*s%.*s\n"),
                                   hp.description_start_column - n, DROPT_TEXT_LITERAL(""),
                                   lineLen, line);
                    n = 0;

                    line = nextLine;
                }
            }

        next:
            if (hp.blank_lines_between_options)
            {
                dropt_ssprintf(ss, DROPT_TEXT_LITERAL("\n"));
            }
        }
        helpText = dropt_ssfinalize(ss);
    }

    return helpText;
}


/** dropt_print_help
  *
  *     Prints help for the available options.
  *
  * PARAMETERS:
  *     IN/OUT f      : The file stream to print to.
  *     IN context    : The options context.
  *                     Must not be NULL.
  *     IN helpParams : The help parameters.
  *                     Pass NULL to use the default help parameters.
  */
void
dropt_print_help(FILE* f, const dropt_context* context,
                 const dropt_help_params* helpParams)
{
    dropt_char* helpText = dropt_get_help(context, helpParams);
    if (helpText != NULL)
    {
        dropt_fputs(helpText, f);
        free(helpText);
    }
}
#endif /* DROPT_NO_STRING_BUFFERS */


/** set_option_value
  *
  *     Sets the value for a specified option by invoking the option's
  *     handler callback.
  *
  * PARAMETERS:
  *     IN/OUT context    : The options context.
  *     IN option         : The option.
  *     IN optionArgument : The option's value.  May be NULL.
  *
  * RETURNS:
  *     An error code.
  */
static dropt_error
set_option_value(dropt_context* context,
                 const dropt_option* option, const dropt_char* optionArgument)
{
    assert(option != NULL);

    if (option->handler == NULL)
    {
        DROPT_MISUSE_PANIC("No option handler specified.");
        return dropt_error_bad_configuration;
    }

    return option->handler(context, optionArgument, option->handler_data);
}


/** parse_option_arg
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
static dropt_error
parse_option_arg(dropt_context* context, parseState* ps)
{
    dropt_error err;

    bool consumeNextArg = false;

    if (OPTION_TAKES_ARG(ps->option) && ps->optionArgument == NULL)
    {
        /* The option expects an argument, but none was specified with '='.
         * Try using the next item from the command-line.
         */
        if (ps->argsLeft > 0 && *(ps->argNext) != NULL)
        {
            consumeNextArg = true;
            ps->optionArgument = *(ps->argNext);
        }
        else if (!(ps->option->attr & dropt_attr_optional_val))
        {
            err = dropt_error_insufficient_arguments;
            goto exit;
        }
    }

    /* Even for options that don't ask for arguments, always parse and
     * consume an argument that was specified with '='.
     */
    err = set_option_value(context, ps->option, ps->optionArgument);

    if (   err != dropt_error_none
        && (ps->option->attr & dropt_attr_optional_val)
        && consumeNextArg
        && ps->optionArgument != NULL)
    {
        /* The option's handler didn't like the argument we fed it.  If the
         * argument was optional, try again without it.
         */
        consumeNextArg = false;
        ps->optionArgument = NULL;
        err = set_option_value(context, ps->option, NULL);
    }

exit:
    if (err == dropt_error_none && consumeNextArg)
    {
        ps->argNext++;
        ps->argsLeft--;
    }
    return err;
}


/** dropt_parse
  *
  *     Parses command-line options.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *                      Must not be NULL.
  *     IN argc        : The maximum number of arguments to parse from
  *                        argv.
  *                      Pass -1 to parse all arguments up to a NULL
  *                        sentinel value.
  *     IN argv        : The list of command-line arguments, not including
  *                        the initial program name.
  *
  * RETURNS:
  *     A pointer to the first unprocessed element in argv.
  */
dropt_char**
dropt_parse(dropt_context* context,
            int argc, dropt_char** argv)
{
    dropt_error err = dropt_error_none;

    dropt_char* arg;
    parseState ps;

    ps.option = NULL;
    ps.optionArgument = NULL;
    ps.argNext = argv;

    if (argv == NULL)
    {
        /* Nothing to do. */
        goto exit;
    }

    if (context == NULL)
    {
        DROPT_MISUSE_PANIC("No dropt context specified.");
        set_error_details(context, dropt_error_bad_configuration,
                          DROPT_TEXT_LITERAL(""), 0, NULL);
        goto exit;
    }

#ifdef DROPT_NO_STRING_BUFFERS
    if (context->errorHandler == NULL)
    {
        DROPT_MISUSE_PANIC("No error handler specified.");
        set_error_details(context, dropt_error_bad_configuration,
                          DROPT_TEXT_LITERAL(""), 0, NULL);
        goto exit;
    }
#endif

    if (argc == -1)
    {
        argc = 0;
        while (argv[argc] != NULL) { argc++; }
    }

    ps.argsLeft = argc;

    while (   ps.argsLeft-- > 0
           && (arg = *ps.argNext) != NULL
           && arg[0] == DROPT_TEXT_LITERAL('-'))
    {
        assert(err == dropt_error_none);

        if (arg[1] == DROPT_TEXT_LITERAL('\0'))
        {
            /* - */

            /* This intentionally leaves "-" unprocessed for the caller to
             * deal with.  This allows construction of programs that treat
             * "-" to mean "stdin".
             */
            goto exit;
        }

        ps.argNext++;

        if (arg[1] == DROPT_TEXT_LITERAL('-'))
        {
            dropt_char* longName = arg + 2;
            if (longName[0] == DROPT_TEXT_LITERAL('\0'))
            {
                /* -- */

                /* This is used to mark the end of the option processing
                 * to prevent some arguments with leading '-' characters
                 * from being treated as options.
                 *
                 * Don't pass this back to the caller.
                 */
                goto exit;
            }
            else if (longName[0] == DROPT_TEXT_LITERAL('='))
            {
                /* Deal with the pathological case of a user supplying
                 * "--=".
                 */
                err = dropt_error_invalid_option;
                set_error_details(context, err, arg, dropt_strlen(arg), NULL);
                goto exit;
            }
            else
            {
                /* --longName */
                const dropt_char* p = dropt_strchr(longName, DROPT_TEXT_LITERAL('='));
                const dropt_char* longNameEnd;
                if (p != NULL)
                {
                    /* --longName=arg */
                    longNameEnd = p;
                    ps.optionArgument = p + 1;
                }
                else
                {
                    longNameEnd = longName + dropt_strlen(longName);
                    assert(ps.optionArgument == NULL);
                }

                /* Pass the length of the option name so that we don't need
                 * to mutate the original string by inserting a
                 * NUL-terminator.
                 */
                ps.option = find_long_option(context, longName, longNameEnd - longName);
                if (ps.option == NULL)
                {
                    err = dropt_error_invalid_option;
                    set_error_details(context, err, arg, longNameEnd - arg, NULL);
                }
                else
                {
                    err = parse_option_arg(context, &ps);
                    if (err != dropt_error_none)
                    {
                        set_error_details(context, err, arg, longNameEnd - arg,
                                          ps.optionArgument);
                    }
                }

                if (   err != dropt_error_none
                    || ps.option->attr & dropt_attr_halt)
                {
                    goto exit;
                }
            }
        }
        else
        {
            /* Short name. (-x) */
            size_t len;
            size_t j;

            if (arg[1] == DROPT_TEXT_LITERAL('='))
            {
                /* Deal with the pathological case of a user supplying
                 * "-=".
                 */
                err = dropt_error_invalid_option;
                set_error_details(context, err, arg, dropt_strlen(arg), NULL);
                goto exit;
            }
            else
            {
                const dropt_char* p = dropt_strchr(arg, DROPT_TEXT_LITERAL('='));
                if (p != NULL)
                {
                    /* -x=arg */
                    len = p - arg;
                    ps.optionArgument = p + 1;
                }
                else
                {
                    len = dropt_strlen(arg);
                    assert(ps.optionArgument == NULL);
                }
            }

            for (j = 1; j < len; j++)
            {
                ps.option = find_short_option(context, arg[j]);
                if (ps.option == NULL)
                {
                    err = dropt_error_invalid_option;
                    set_short_option_error_details(context, err, arg[j], NULL);
                    goto exit;
                }
                else if (j + 1 == len)
                {
                    /* The last short option in a condensed list gets
                     * to use an argument.
                     */
                    err = parse_option_arg(context, &ps);
                    if (err != dropt_error_none)
                    {
                        set_short_option_error_details(context, err, arg[j],
                                                       ps.optionArgument);
                        goto exit;
                    }
                }
                else if (   context->allowConcatenatedArgs
                         && OPTION_TAKES_ARG(ps.option)
                         && j == 1)
                {
                    err = set_option_value(context, ps.option, &arg[j + 1]);

                    if (   err != dropt_error_none
                        && (ps.option->attr & dropt_attr_optional_val))
                    {
                        err = set_option_value(context, ps.option, NULL);
                    }

                    if (err != dropt_error_none)
                    {
                        set_short_option_error_details(context, err, arg[j], &arg[j + 1]);
                        goto exit;
                    }

                    /* Skip to the next argument. */
                    break;
                }
                else if (   OPTION_TAKES_ARG(ps.option)
                         && !(ps.option->attr & dropt_attr_optional_val))
                {
                    /* Short options with required arguments can't be used
                     * in condensed lists except in the last position.
                     *
                     * e.g. -abcd arg
                     *          ^
                     */
                    err = dropt_error_insufficient_arguments;
                    set_short_option_error_details(context, err, arg[j], NULL);
                    goto exit;
                }
                else
                {
                    err = set_option_value(context, ps.option, NULL);
                    if (err != dropt_error_none)
                    {
                        set_short_option_error_details(context, err, arg[j], NULL);
                        goto exit;
                    }
                }

                if (ps.option->attr & dropt_attr_halt) { goto exit; }
            }
        }

        ps.option = NULL;
        ps.optionArgument = NULL;
    }

exit:
    return ps.argNext;
}


/** dropt_new_context
  *
  *     Creates a new options context.
  *
  * PARAMETERS:
  *     IN options : The list of option specifications.
  *                  Must not be NULL.
  *
  * RETURNS:
  *     An allocated options context.  The caller is responsible for
  *       freeing it with dropt_free_context when no longer needed.
  *     Returns NULL on error.
  */
dropt_context*
dropt_new_context(const dropt_option* options)
{
    dropt_context* context = NULL;

    if (options == NULL)
    {
        DROPT_MISUSE_PANIC("No option list specified.");
        goto exit;
    }

    context = malloc(sizeof *context);
    if (context == NULL) { goto exit; }

    context->options = options;
    context->allowConcatenatedArgs = false;
    context->errorHandler = NULL;
    context->errorHandlerData = NULL;
    context->errorDetails.err = dropt_error_none;
    context->errorDetails.optionName = NULL;
    context->errorDetails.optionArgument = NULL;
    context->errorDetails.message = NULL;
    context->strncmp = NULL;

    /* Sanity-check the options. */
    {
        const dropt_option* option;
        for (option = options; is_valid_option(option); option++)
        {
            if (   option->short_name == DROPT_TEXT_LITERAL('=')
                || (   option->long_name != NULL
                    && dropt_strchr(option->long_name, DROPT_TEXT_LITERAL('=')) != NULL))
            {
                DROPT_MISUSE_PANIC("Invalid option list. '=' may not be used in an option name.");
                free(context);
                context = NULL;
                goto exit;
            }
        }
    }

exit:
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
dropt_free_context(dropt_context* context)
{
    dropt_clear_error(context);
    free(context);
}


/** dropt_get_options
  *
  * PARAMETERS:
  *     IN context : The options context.
  *                  Must not be NULL.
  *
  * RETURNS:
  *     The context's list of option specifications.
  */
const dropt_option*
dropt_get_options(const dropt_context* context)
{
    if (context == NULL)
    {
        DROPT_MISUSE_PANIC("No dropt context specified.");
        return NULL;
    }

    return context->options;
}


/** dropt_init_help_params
  *
  *     Initializes a dropt_help_params structure with the default
  *     values.
  *
  * PARAMETERS:
  *     OUT helpParams : On output, set to the default help parameters.
  *                      Must not be NULL.
  */
void
dropt_init_help_params(dropt_help_params* helpParams)
{
    if (helpParams == NULL)
    {
        DROPT_MISUSE_PANIC("No dropt help parameters specified.");
        return;
    }

    helpParams->indent = default_help_indent;
    helpParams->description_start_column = default_description_start_column;
    helpParams->blank_lines_between_options = true;
}


/** dropt_set_error_handler
  *
  *     Sets the callback function used to generate error strings from
  *     error codes.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *                      Must not be NULL.
  *     IN handler     : The error handler callback.
  *                      Pass NULL to use the default error handler.
  *     IN handlerData : Caller-defined callback data.
  */
void
dropt_set_error_handler(dropt_context* context, dropt_error_handler_func handler, void* handlerData)
{
    if (context == NULL)
    {
        DROPT_MISUSE_PANIC("No dropt context specified.");
        return;
    }

    context->errorHandler = handler;
    context->errorHandlerData = handlerData;
}


/** dropt_set_strncmp
  *
  *     Sets the callback function usde to compare strings.
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *                      Must not be NULL.
  *     IN cmp         : The string comparison function.
  *                      Pass NULL to use the default string comparison
  *                        function.
  */
void
dropt_set_strncmp(dropt_context* context, dropt_strncmp_func cmp)
{
    if (context == NULL)
    {
        DROPT_MISUSE_PANIC("No dropt context specified.");
        return;
    }

    context->strncmp = cmp;
}


/** dropt_misuse_panic
  *
  *     Prints a diagnostic for logical errors caused by external clients
  *     calling into dropt improperly.
  *
  *     In debug builds, terminates the program and prints the filename and
  *     line number of the failure.
  *
  *     For logical errors entirely internal to dropt, use assert()
  *     instead.
  *
  * PARAMETERS:
  *     IN message  : The error message.
  *                   Must not be NULL.
  *     IN filename : The name of the file where the logical error
  *                     occurred.
  *                   Must not be NULL.
  *     IN line     : The line number where the logical error occurred.
  */
void
dropt_misuse_panic(const char* message, const char* filename, int line)
{
#ifdef NDEBUG
    fprintf(stderr, "dropt: %s\n", message);
#else
    fprintf(stderr, "dropt: %s (%s: %d)\n", message, filename, line);
    abort();
#endif
}


/** dropt_allow_concatenated_arguments
  *
  *     Specifies whether "short" options are allowed to have concatenated
  *     arguments (i.e. without space or '=' separators, such as -oARGUMENT).
  *
  *     (Concatenated arguments are disallowed by default.)
  *
  * PARAMETERS:
  *     IN/OUT context : The options context.
  *     IN allow       : Pass 1 if concatenated arguments should be allowed,
  *                        0 otherwise.
  */
void
dropt_allow_concatenated_arguments(dropt_context* context, dropt_bool allow)
{
    if (context == NULL)
    {
        DROPT_MISUSE_PANIC("No dropt context specified.");
        return;
    }

    context->allowConcatenatedArgs = (allow != 0);
}
