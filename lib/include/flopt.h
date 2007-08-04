#ifndef FLOPT_H
#define FLOPT_H

#include "utilChar.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    flopt_error_none,

    /* An option handler can return flopt_error_no_optional_arg to indicate
     * that it chose not to consume the next argument.
     */
    flopt_error_no_optional_arg,

    /* An option handler can return flopt_error_cancel to cancel processing
     * without the action being treated as an error (i.e. no error
     * diagnostics).
     */
    flopt_error_cancel,

    flopt_error_invalid,
    flopt_error_insufficient_args,
    flopt_error_mismatch,
    flopt_error_overflow,
    flopt_error_bad_placement,
    flopt_error_custom,
    flopt_error_unknown,
} flopt_error_t;


enum
{
    flopt_attr_halt = (1 << 0),
    flopt_attr_hidden = (1 << 1),
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


typedef struct flopt_context_t flopt_context_t;


flopt_context_t* flopt_new_context(void);
void flopt_free_context(flopt_context_t* contextP);

void flopt_set_options(flopt_context_t* contextP, const flopt_option_t* optionsP);

TCHAR** flopt_parse(flopt_context_t* contextP, TCHAR** argv);
flopt_error_t flopt_parse_bool(const TCHAR* s, void* valP);
flopt_error_t flopt_parse_optional_bool(const TCHAR* s, void* valP);
flopt_error_t flopt_parse_int(const TCHAR* s, void* valP);
flopt_error_t flopt_parse_uint(const TCHAR* s, void* valP);
flopt_error_t flopt_parse_double(const TCHAR* s, void* valP);
flopt_error_t flopt_parse_string(const TCHAR* s, void* valP);

void flopt_set_error_message(flopt_context_t* contextP, const TCHAR* messageP);
const TCHAR* flopt_get_error_message(const flopt_context_t* contextP);
flopt_error_t flopt_get_error(const flopt_context_t* contextP);

void flopt_print_help(FILE* fp, const flopt_option_t* optionsP, unsigned char compact);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FLOPT_H */
