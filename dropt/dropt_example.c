/** dropt_example.c
  *
  *     A simple dropt example.
  *
  * Written by James D. Lin and assigned to the public domain.
  */

#include <stdio.h>
#include "dropt.h"

int
main(int argc, char** argv)
{
    dropt_bool_t showHelp = 0;
    int i = 0;

    dropt_option_t options[] = {
        { 'h',  "help", "Shows help.", NULL, dropt_handle_bool, &showHelp, dropt_attr_halt },
        { 'i',  "int", "Test integer value.", "value", dropt_handle_int, &i },
        { 0 }
    };

    dropt_context_t* droptContext = dropt_new_context(options);
    if (droptContext != NULL)
    {
        dropt_char_t** rest = dropt_parse(droptContext, &argv[1]);
        if (dropt_get_error(droptContext) != dropt_error_none)
        {
            fprintf(stderr, "dropt_example: %s\n", dropt_get_error_message(droptContext));
        }
        else if (showHelp)
        {
            printf("Usage: dropt_example [options] [operands] [--] [arguments]\n\n"
                   "Options:\n");
            dropt_print_help(stdout, options, 0);
        }
        else
        {
            printf("int value: %d\n", i);

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
