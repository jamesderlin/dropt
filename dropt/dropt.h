/** dropt.h
  *
  * A deliberately rudimentary command-line option parser.
  *
  * Version 1.0
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

#ifndef DROPT_H
#define DROPT_H

#include <stdio.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef DROPT_USE_WCHAR
#if defined _UNICODE && (defined _MSC_VER || defined DROPT_NO_STRING_BUFFERS)
#define DROPT_USE_WCHAR 1
#endif
#endif

#ifdef DROPT_USE_WCHAR
    /* This may be used for both char and string literals. */
    #define DROPT_TEXT_LITERAL(s) L ## s

    typedef wchar_t dropt_char_t;
#else
    #define DROPT_TEXT_LITERAL(s) s

    typedef char dropt_char_t;
#endif


typedef enum
{
    /* Errors in the range [0x00, 0x7F] are reserved for dropt. */
    dropt_error_none,
    dropt_error_unknown,
    dropt_error_bad_configuration,
    dropt_error_insufficient_memory,
    dropt_error_invalid_option,
    dropt_error_insufficient_arguments,
    dropt_error_mismatch,
    dropt_error_overflow,
    dropt_error_underflow,

    /* Errors in the range [0x80, 0xFFFF] are free for clients to use. */
    dropt_error_custom_start = 0x80,
    dropt_error_custom_last = 0xFFFF
} dropt_error_t;


typedef unsigned char dropt_bool_t;

/* Opaque. */
typedef struct dropt_context_t dropt_context_t;


typedef dropt_error_t (*dropt_option_handler_t)(dropt_context_t* context,
                                                const dropt_char_t* optionArgument,
                                                void* handlerData);

/** dropt_error_handler_t callbacks are responsible for generating error
  * messages.  The returned string must be allocated on the heap and must
  * be freeable with free().
  */
typedef dropt_char_t* (*dropt_error_handler_t)(dropt_error_t error,
                                               const dropt_char_t* optionName,
                                               const dropt_char_t* optionArgument,
                                               void* handlerData);

/** dropt_strncmp_t callbacks allow callers to provide their own (possibly
  * case-insensitive) string comparison function.
  */
typedef int (*dropt_strncmp_t)(const dropt_char_t* s, const dropt_char_t* t, size_t n);


/** Properties defining each option:
  *
  * short_name:
  *     The option's short name (e.g. the 'h' in -h).
  *     Use '\0' if the option has no short name.
  *
  * long_name:
  *     The option's long name (e.g. "help" in --help).
  *     Use NULL if the option has no long name.
  *
  * description:
  *     The description shown when generating help.
  *
  * arg_description:
  *     The description for the option's argument (e.g. --option=argument
  *     or --option argument), printed when generating help.  If NULL, the
  *     option does not take an argument.
  *
  * handler:
  *     The handler callback and data invoked in response to encountering
  *     the option.
  *
  * handler_data:
  *     Callback data for the handler.  For typical handlers, this is
  *     usually the address of a variable for the handler to modify.
  *
  * attr:
  *     Miscellaneous attributes.  See below.
  */
typedef struct dropt_option_t
{
    dropt_char_t short_name;
    const dropt_char_t* long_name;
    const dropt_char_t* description;
    const dropt_char_t* arg_description;
    dropt_option_handler_t handler;
    void* handler_data;
    unsigned int attr;
} dropt_option_t;


/** Bitwise flags for option attributes:
  *
  * dropt_attr_halt:
  *     Stop processing when this option is encountered.
  *
  * dropt_attr_hidden:
  *     Don't list the option when generating help.  Use this for
  *     undocumented options.
  *
  * dropt_attr_optional_val:
  *     The option's argument is optional.  If an option has this
  *     attribute, the handler callback may be invoked twice (once with a
  *     potential argument, and if that fails, again with a NULL argument).
  */
enum
{
    dropt_attr_halt = (1 << 0),
    dropt_attr_hidden = (1 << 1),
    dropt_attr_optional_val = (1 << 2)
};


typedef struct dropt_help_params_t
{
    unsigned int indent;
    unsigned int description_start_column;
    dropt_bool_t blank_lines_between_options;
} dropt_help_params_t;


dropt_context_t* dropt_new_context(const dropt_option_t* options);
void dropt_free_context(dropt_context_t* context);

const dropt_option_t* dropt_get_options(const dropt_context_t* context);

void dropt_set_error_handler(dropt_context_t* context,
                             dropt_error_handler_t handler, void* handlerData);
void dropt_set_strncmp(dropt_context_t* context, dropt_strncmp_t cmp);

/* Use this only for backward compatibility purposes. */
void dropt_allow_concatenated_arguments(dropt_context_t* context, dropt_bool_t allow);

dropt_char_t** dropt_parse(dropt_context_t* context, int argc, dropt_char_t** argv);

dropt_error_t dropt_get_error(const dropt_context_t* context);
void dropt_get_error_details(const dropt_context_t* context,
                             dropt_char_t** optionName,
                             dropt_char_t** optionArgument);
const dropt_char_t* dropt_get_error_message(dropt_context_t* context);
void dropt_clear_error(dropt_context_t* context);

#ifndef DROPT_NO_STRING_BUFFERS
dropt_char_t* dropt_default_error_handler(dropt_error_t error,
                                          const dropt_char_t* optionName,
                                          const dropt_char_t* optionArgument);

void dropt_init_help_params(dropt_help_params_t* helpParams);
dropt_char_t* dropt_get_help(const dropt_context_t* context,
                             const dropt_help_params_t* helpParams);
void dropt_print_help(FILE* f, const dropt_context_t* context,
                      const dropt_help_params_t* helpParams);
#endif

#define DROPT_HANDLER_DECL(func) \
    dropt_error_t func(dropt_context_t* context, const dropt_char_t* optionArgument, \
                       void* handlerData)
DROPT_HANDLER_DECL(dropt_handle_bool);
DROPT_HANDLER_DECL(dropt_handle_verbose_bool);
DROPT_HANDLER_DECL(dropt_handle_int);
DROPT_HANDLER_DECL(dropt_handle_uint);
DROPT_HANDLER_DECL(dropt_handle_double);
DROPT_HANDLER_DECL(dropt_handle_string);

#define DROPT_MISUSE_PANIC(message) dropt_misuse_panic(message, __FILE__, __LINE__)
void dropt_misuse_panic(const char* message, const char* filename, int line);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DROPT_H */
