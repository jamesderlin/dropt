#ifndef JDL_UTIL_OPTIONS_H
#define JDL_UTIL_OPTIONS_H

#include "utilChar.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    optionsErrorNone,
    optionsErrorInvalid,
    optionsErrorInsufficientArgs,
    optionsErrorMismatch,
    optionsErrorOverflow,
    optionsErrorBadPlacement,
    optionsErrorUnknown,
} optionsError_t;


typedef enum
{
    optionsTypeUnchecked,
    optionsTypeBool,
    optionsTypeInt,
    optionsTypeUInt,
    optionsTypeDouble,
} optionsValue_t;

const unsigned int optionsAttrHalt = (1 << 0);


typedef struct option_t option_t;

typedef optionsError_t (*handler_t)(const option_t* optionP, const void* valP);

struct option_t
{
    const TCHAR* longName; /* May be NULL. */
    TCHAR shortName;       /* May be '\0'. */
    optionsValue_t type;
    void* varP;
    const TCHAR* description; /* May be NULL.*/
    const TCHAR* argDescription;
    unsigned int attr;
    handler_t handler;
    void* handlerDataP;
};

typedef struct optionsContext_t optionsContext_t;


optionsContext_t* optionsNewContext(void);
void optionsFreeContext(optionsContext_t* contextP);

void optionsSet(optionsContext_t* contextP, const option_t* optionsP);

TCHAR** optionsParse(optionsContext_t* contextP, TCHAR** argv);

optionsError_t optionsGetError(const optionsContext_t* contextP);
const TCHAR* optionsGetErrorMessage(const optionsContext_t* contextP);

void optionsPrintHelp(FILE* fp, const option_t* optionsP, unsigned char compact);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* JDL_UTIL_OPTIONS_H */
