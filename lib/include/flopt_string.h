#ifndef FLOPT_STRING_H
#define FLOPT_STRING_H

#include "flopt.h"

#ifdef _UNICODE
    #define T(s) (L ## s)
    #define tcslen wcslen
    #define tcsncpy wcscpy
    #define tcscmp wcscmp
    #define tcschr wcschr
    #define tcstol wcstol
    #define tcstoul wcstoul
    #define tcstod wcstod
    #define tclower wclower
#else
    #define T(s) s
    #define tcslen strlen
    #define tcsncpy strncpy
    #define tcscmp strcmp
    #define tcschr strchr
    #define tcstol strtol
    #define tcstoul strtoul
    #define tcstod strtod
    #define tclower tolower
#endif


typedef struct flopt_stringstream flopt_stringstream;

TCHAR* flopt_strdup(const TCHAR* s);
int flopt_stricmp(const TCHAR* s, const TCHAR* t);

TCHAR* flopt_vaprintf(const TCHAR* fmtP, va_list args);
TCHAR* flopt_aprintf(const TCHAR* fmtP, ...);

flopt_stringstream* flopt_ssopen(void);
void flopt_ssclose(flopt_stringstream* ssP);

void flopt_ssclear(flopt_stringstream* ssP);
TCHAR* flopt_ssfinalize(flopt_stringstream* ssP);
const TCHAR* flopt_ssgetstring(const flopt_stringstream* ssP);

int flopt_vssprintf(flopt_stringstream* ssP, const TCHAR* fmtP, va_list args);
int flopt_ssprintf(flopt_stringstream* ssP, const TCHAR* fmtP, ...);

#endif /* FLOPT_STRING_H */
