/** dropt_example.c
  *
  *     A simple dropt example.
  *
  * Written by James D. Lin and assigned to the public domain.
  *
  * The latest version of this file can be downloaded from:
  * <http://www.taenarum.com/software/dropt/>
  */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "dropt.h"


typedef enum { UNKNOWN, HEADS, TAILS } face_t;
static face_t face = UNKNOWN;


/** handle_face
  *
  *     An example of a custom option handler.  Usually the stock callbacks
  *     (e.g. dropt_handle_bool, dropt_handle_int, dropt_handle_string,
  *     etc.) should be sufficient for most purposes.
  */
static dropt_error_t
handle_face(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
{
    dropt_error_t err = dropt_error_none;
    face_t* face = handlerData;
    assert(face != NULL);

    /*
     * Option handlers should handle 'valueString' being NULL or the empty
     * string.  This can happen if the option's argument is optional or
     * if a user explicitly passed an empty string (e.g. --face="").
     */
    if (valueString == NULL || valueString[0] == '\0')
    {
        err = dropt_error_insufficient_args;
    }
    else if (strcmp(valueString, "heads") == 0)
    {
        *face = HEADS;
    }
    else if (strcmp(valueString, "tails") == 0)
    {
        *face = TAILS;
    }
    else
    {
        /* Reject the argument as being inappropriate for this handler. */
        err = dropt_error_mismatch;
    }

    return err;
}


int
main(int argc, char** argv)
{
    dropt_bool_t showHelp = 0;
    dropt_bool_t showVersion = 0;
    int i = 0;

    dropt_option_t options[] = {
        { 'h',  "help", "Shows help.", NULL, dropt_handle_bool, &showHelp, dropt_attr_halt },
        { '?', NULL, NULL, NULL, dropt_handle_bool, &showHelp, dropt_attr_halt | dropt_attr_hidden },
        { '\0', "version", "Shows version information.", NULL, dropt_handle_bool, &showVersion, dropt_attr_halt },
        { 'i',  "int", "Sample integer option.", "value", dropt_handle_int, &i },
        { 'f',  "face", "Sample custom option.", "{heads, tails}", handle_face, &face },
        { 0 } /* Sentinel value. */
    };

    dropt_context_t* droptContext = dropt_new_context(options);
    if (droptContext != NULL)
    {
        dropt_char_t** rest = dropt_parse(droptContext, -1, &argv[1]);
        if (dropt_get_error(droptContext) != dropt_error_none)
        {
            fprintf(stderr, "dropt_example: %s\n", dropt_get_error_message(droptContext));
        }
        else if (showHelp)
        {
            printf("Usage: dropt_example [options] [operands] [--] [arguments]\n\n"
                   "Options:\n");
            dropt_print_help(stdout, options, NULL);
        }
        else if (showVersion)
        {
            printf("dropt_example 1.0\n");
        }
        else
        {
            printf("int value: %d\n", i);
            printf("face value: %d\n", face);

            printf("Arguments: ");
            while (*rest != NULL)
            {
                printf("%s ", *rest);
                rest++;
            }
            printf("\n");
        }

        dropt_free_context(droptContext);
    }

    return 0;
}
