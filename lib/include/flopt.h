/** flopt.h
  *
  *     A fairly lame command-line option parser.
  *
  * Last modified: 2007-08-04
  *
  * Copyright (C) 2006-2007 James D. Lin
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

#ifndef FLOPT_H
#define FLOPT_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _UNICODE
    typedef wchar_t TCHAR;
#else
    typedef char TCHAR;
#endif


typedef enum
{
    flopt_error_none,

    /* An option handler can return flopt_error_cancel to cancel processing
     * without the action being treated as an error (i.e. no error
     * diagnostics).
     */
    flopt_error_cancel,

    flopt_error_invalid,
    flopt_error_insufficient_args,
    flopt_error_mismatch,
    flopt_error_overflow,
    flopt_error_custom,
    flopt_error_unknown,
} flopt_error_t;


enum
{
    flopt_attr_halt = (1 << 0),
    flopt_attr_hidden = (1 << 1),
    flopt_attr_optional = (1 << 2),
};


typedef flopt_error_t (*flopt_option_handler_t)(const TCHAR* valP, void* handlerDataP);

typedef struct flopt_option_t
{
    const TCHAR* longName;       /* May be NULL. */
    TCHAR shortName;             /* May be '\0'. */
    const TCHAR* description;    /* May be NULL.*/
    const TCHAR* argDescription; /* NULL if no argument. */
    flopt_option_handler_t handler;
    void* handlerDataP;
    unsigned int attr;
} flopt_option_t;


typedef unsigned char flopt_bool_t;

/* Opaque. */
typedef struct flopt_context_t flopt_context_t;


flopt_context_t* flopt_new_context(void);
void flopt_free_context(flopt_context_t* contextP);

void flopt_set_options(flopt_context_t* contextP, const flopt_option_t* optionsP);
void flopt_set_case_sensitive(flopt_context_t* contextP, flopt_bool_t caseSensitive);


TCHAR** flopt_parse(flopt_context_t* contextP, TCHAR** argv);
flopt_error_t flopt_parse_bool(const TCHAR* s, void* valP);
flopt_error_t flopt_parse_int(const TCHAR* s, void* valP);
flopt_error_t flopt_parse_uint(const TCHAR* s, void* valP);
flopt_error_t flopt_parse_double(const TCHAR* s, void* valP);
flopt_error_t flopt_parse_string(const TCHAR* s, void* valP);

flopt_error_t flopt_get_error(const flopt_context_t* contextP);
void flopt_get_error_details(const flopt_context_t* contextP,
                             TCHAR** optionNamePP,
                             TCHAR** valPP);

void flopt_set_error_message(flopt_context_t* contextP, const TCHAR* messageP);
const TCHAR* flopt_get_error_message(const flopt_context_t* contextP);

TCHAR* flopt_get_help(const flopt_option_t* optionsP, flopt_bool_t compact);
void flopt_print_help(FILE* fp, const flopt_option_t* optionsP, flopt_bool_t compact);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FLOPT_H */
