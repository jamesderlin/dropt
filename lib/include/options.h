#ifndef JDL_UTIL_OPTIONS_H
#define JDL_UTIL_OPTIONS_H

#include "utilChar.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    optionsErrorNone,
    optionsErrorNoOptionalArg,
    optionsErrorCancel,
    optionsErrorInvalid,
    optionsErrorInsufficientArgs,
    optionsErrorMismatch,
    optionsErrorOverflow,
    optionsErrorBadPlacement,
    optionsErrorCustom,
    optionsErrorUnknown,
} optionsError_t;


const unsigned int optionsAttrHalt = (1 << 0);
const unsigned int optionsAttrHidden = (1 << 1);


typedef struct option_t option_t;

typedef optionsError_t (*handler_t)(const TCHAR* valP, void* handlerDataP);

struct option_t
{
    const TCHAR* longName;       /* May be NULL. */
    TCHAR shortName;             /* May be '\0'. */
    const TCHAR* description;    /* May be NULL.*/
    const TCHAR* argDescription; /* NULL if no argument. */
    handler_t handler;
    void* handlerDataP;
    unsigned int attr;
};

typedef struct optionsContext_t optionsContext_t;


optionsContext_t* optionsNewContext(void);
void optionsFreeContext(optionsContext_t* contextP);

void optionsSet(optionsContext_t* contextP, const option_t* optionsP);

TCHAR** optionsParse(optionsContext_t* contextP, TCHAR** argv);
optionsError_t optionsParseBool(const TCHAR* s, void* valP);
optionsError_t optionsParseInt(const TCHAR* s, void* valP);
optionsError_t optionsParseUInt(const TCHAR* s, void* valP);
optionsError_t optionsParseDouble(const TCHAR* s, void* valP);
optionsError_t optionsParseString(const TCHAR* s, void* valP);

void optionsSetErrorMessage(optionsContext_t* contextP, const TCHAR* messageP);
const TCHAR* optionsGetErrorMessage(const optionsContext_t* contextP);
optionsError_t optionsGetError(const optionsContext_t* contextP);

void optionsPrintHelp(FILE* fp, const option_t* optionsP, unsigned char compact);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* JDL_UTIL_OPTIONS_H */
