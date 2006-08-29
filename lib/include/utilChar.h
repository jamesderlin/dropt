#ifndef JDL_UTIL_CHAR_H
#define JDL_UTIL_CHAR_H

#if defined _WIN32

    #include <tchar.h>

#elif defined _UNICODE

    #include <wchar.h>

    typedef wchar_t TCHAR;

    #define _T(s) L ## s
    #define _tcscmp wcscmp
    #define _tcslen wcslen
    #define _tcstol wcstol
    #define _tcstoul wcstoul
    #define _tcstod wcstod
    #define _tcschr wcschr
    #define _tcsncat wcsncat
    #define _ftprintf fwprintf
    #define _istdigit iswdigit

#else

    typedef char TCHAR;

    #define _T(s) s
    #define _tcscmp strcmp
    #define _tcslen strlen
    #define _tcstol strtol
    #define _tcstoul strtoul
    #define _tcstod strtod
    #define _tcschr strchr
    #define _tcsncat strncat
    #define _ftprintf fprintf
    #define _istdigit isdigit

#endif


#endif /* JDL_UTIL_CHAR_H */
