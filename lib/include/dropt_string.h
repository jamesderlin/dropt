#ifndef DROPT_STRING_H
#define DROPT_STRING_H

#include "dropt.h"

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


typedef struct dropt_stringstream dropt_stringstream;

TCHAR* dropt_strdup(const TCHAR* s);
int dropt_stricmp(const TCHAR* s, const TCHAR* t);

TCHAR* dropt_vaprintf(const TCHAR* fmtP, va_list args);
TCHAR* dropt_aprintf(const TCHAR* fmtP, ...);

dropt_stringstream* dropt_ssopen(void);
void dropt_ssclose(dropt_stringstream* ssP);

void dropt_ssclear(dropt_stringstream* ssP);
TCHAR* dropt_ssfinalize(dropt_stringstream* ssP);
const TCHAR* dropt_ssgetstring(const dropt_stringstream* ssP);

int dropt_vssprintf(dropt_stringstream* ssP, const TCHAR* fmtP, va_list args);
int dropt_ssprintf(dropt_stringstream* ssP, const TCHAR* fmtP, ...);

#endif /* DROPT_STRING_H */
