/** dropt.h
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

#ifndef DROPT_H
#define DROPT_H

#include <stdio.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif


#if defined _UNICODE || defined UNICODE
    typedef wchar_t dropt_char_t;
#else
    typedef char dropt_char_t;
#endif


typedef enum
{
    /* Errors in the range [0x00, 0x7F] are reserved for dropt. */
    dropt_error_none,
    dropt_error_unknown,
    dropt_error_bad_configuration,
    dropt_error_invalid,
    dropt_error_insufficient_args,
    dropt_error_mismatch,
    dropt_error_overflow,
    dropt_error_underflow,
    dropt_error_insufficient_memory,

    /* Errors in the range [0x80, 0xFFFF] are free for clients to use. */
    dropt_error_custom_start = 0x80,
    dropt_error_custom_last = 0xFFFF
} dropt_error_t;


enum
{
    dropt_attr_halt = (1 << 0),
    dropt_attr_hidden = (1 << 1),
    dropt_attr_optional_val = (1 << 2),
};


typedef unsigned char dropt_bool_t;

/* Opaque. */
typedef struct dropt_context_t dropt_context_t;


typedef dropt_error_t (*dropt_option_handler_t)(dropt_context_t* context,
                                                const dropt_char_t* valueString,
                                                void* handlerData);
typedef dropt_char_t* (*dropt_error_handler_t)(dropt_error_t error,
                                               const dropt_char_t* optionName,
                                               const dropt_char_t* valueString,
                                               void* handlerData);


typedef struct dropt_option_t
{
    const dropt_char_t* longName;       /* May be NULL. */
    dropt_char_t shortName;             /* May be '\0'. */
    const dropt_char_t* description;    /* May be NULL. */
    const dropt_char_t* argDescription; /* Set to NULL if no argument. */
    dropt_option_handler_t handler;
    void* handlerData;
    unsigned int attr;
} dropt_option_t;


dropt_context_t* dropt_new_context(void);
void dropt_free_context(dropt_context_t* context);

dropt_error_t dropt_set_options(dropt_context_t* context, const dropt_option_t* options);
void dropt_set_case_sensitive(dropt_context_t* context, dropt_bool_t caseSensitive);
void dropt_set_error_handler(dropt_context_t* context, dropt_error_handler_t handler, void* handlerData);

dropt_char_t** dropt_parse(dropt_context_t* context, dropt_char_t** argv);

dropt_error_t dropt_get_error(const dropt_context_t* context);
void dropt_get_error_details(const dropt_context_t* context,
                             dropt_char_t** optionName,
                             dropt_char_t** valueString);
const dropt_char_t* dropt_get_error_message(dropt_context_t* context);

#ifndef DROPT_NO_STRING_BUFFERS
dropt_char_t* dropt_default_error_handler(dropt_error_t error,
                                          const dropt_char_t* optionName,
                                          const dropt_char_t* valueString);

dropt_char_t* dropt_get_help(const dropt_option_t* options, dropt_bool_t compact);
void dropt_print_help(FILE* f, const dropt_option_t* options, dropt_bool_t compact);
#endif

#define DROPT_HANDLER_DECL(func) \
    dropt_error_t func(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
DROPT_HANDLER_DECL(dropt_handle_bool);
DROPT_HANDLER_DECL(dropt_handle_verbose_bool);
DROPT_HANDLER_DECL(dropt_handle_int);
DROPT_HANDLER_DECL(dropt_handle_uint);
DROPT_HANDLER_DECL(dropt_handle_double);
DROPT_HANDLER_DECL(dropt_handle_string);
#undef DROPT_HANDLER_DECL

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DROPT_H */
