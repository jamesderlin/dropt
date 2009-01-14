/** dropt.cpp
  *
  *     A C++ wrapper for dropt.
  *
  * Copyright (c) 2008 James D. Lin <jameslin@csua.berkeley.edu>
  *
  * The latest version of this file can be downloaded from:
  * <http://www.taenarum.com/software/dropt/>
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
  *
  * 2. Altered source versions must be plainly marked as such, and must not be
  *    misrepresented as being the original software.
  *
  * 3. This notice may not be removed or altered from any source distribution.
  */

#include <stdexcept>
#include <cstdlib>
#include "droptxx.hpp"


namespace dropt
{


/** dropt::context::context
  *
  *     dropt::context constructor.
  *
  * PARAMETERS:
  *     IN options : The list of option specifications.
  *                  Must not be NULL.
  */
context::context(const dropt_option_t* options)
: mContext(dropt_new_context(options)),
  mOptions(options)
{
    if (mContext == NULL) { throw std::bad_alloc(); }
}


/** dropt::context::~context
  *
  *     dropt::context destructor.
  */
context::~context()
{
    dropt_free_context(mContext);
}


/** dropt::context::raw
  *
  * RETURNS:
  *     The raw dropt_context_t for this dropt::context.
  */
dropt_context_t*
context::raw()
{
    return mContext;
}


/** dropt::context::set_error_handler
  *
  *     A wrapper around dropt_set_error_handler.
  */
void
context::set_error_handler(dropt_error_handler_t handler, void* handlerData)
{
    dropt_set_error_handler(mContext, handler, handlerData);
}


/** dropt::context::set_strncmp
  *
  *     A wrapper around dropt_set_strncmp.
  */
void context::set_strncmp(dropt_strncmp_t cmp)
{
    dropt_set_strncmp(mContext, cmp);
}


/** dropt::context::parse
  *
  *     Wrappers around dropt_parse.
  */
dropt_char_t**
context::parse(int argc, dropt_char_t** argv)
{
    return dropt_parse(mContext, argc, argv);
}


dropt_char_t**
context::parse(dropt_char_t** argv)
{
    return dropt_parse(mContext, -1, argv);
}


/** dropt::context::get_error
  *
  *     A wrapper around dropt_get_error.
  */
dropt_error_t
context::get_error() const
{
    return dropt_get_error(mContext);
}


/** dropt::context::get_error_details
  *
  *     A wrapper around dropt_get_error_details.
  */
void
context::get_error_details(dropt_char_t** optionName, dropt_char_t** valueString) const
{
    dropt_get_error_details(mContext, optionName, valueString);
}


/** dropt::context::get_error_message
  *
  *     A wrapper around dropt_get_error_message.
  */
const dropt_char_t*
context::get_error_message()
{
    return dropt_get_error_message(mContext);
}


/** dropt::context::clear_error
  *
  *     A wrapper around dropt_clear_error.
  */
void
context::clear_error()
{
    dropt_clear_error(mContext);
}


#ifndef DROPT_NO_STRING_BUFFERS
/** dropt::context::get_help
  *
  *     A wrapper around dropt_get_help.
  *
  * PARAMETERS:
  *     IN help_params : The help parameters.
  *
  * RETURNS:
  *     A string for the available options.
  *     Returns an empty string on error.
  */
string
context::get_help(const help_params& helpParams) const
{
    dropt_char_t* s = NULL;
    try
    {
        s = dropt_get_help(mOptions, &helpParams);
        return (s == NULL)
               ? string()
               : string(s);
    }
    catch (...)
    {
        free(s);
        throw;
    }
}
#endif


/** dropt::convert_exception
  *
  *     Converts the last thrown C++ exception to a dropt_error_t.
  *
  * RETURNS:
  *     An error code.
  */
dropt_error_t
convert_exception()
{
    try
    {
        throw;
    }
    catch (std::bad_alloc&)
    {
        return dropt_error_insufficient_memory;
    }
    catch (std::logic_error&)
    {
        return dropt_error_bad_configuration;
    }
    catch (...)
    {
        return dropt_error_unknown;
    }
}


/** dropt::handle_bool
  *
  *     Parses a C++ bool value from the given string if possible.
  *
  * PARAMETERS:
  *     IN/OUT context  : The options context.
  *     IN valueString  : A string representing a boolean value (0 or 1).
  *                       If NULL, the boolean value is assumed to be
  *                         true.
  *     OUT handlerData : A pointer to a C++ bool.
  *                       On success, set to the interpreted boolean
  *                         value.
  *                       On error, left untouched.
  *
  * RETURNS:
  *     See dropt_handle_bool.
  */
dropt_error_t
handle_bool(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
{
    dropt_bool_t b;
    dropt_error_t err = dropt_handle_bool(context, valueString, &b);
    if (err == dropt_error_none) { *static_cast<bool*>(handlerData) = (b != 0); }
    return err;
}


/** dropt::handle_verbose_bool
  *
  *     Like dropt::handle_bool but accepts "true" and "false" string
  *     values.
  *
  * PARAMETERS:
  *     IN/OUT context  : The options context.
  *     IN valueString  : A string representing a boolean value (0 or 1).
  *                       If NULL, the boolean value is assumed to be
  *                         true.
  *     OUT handlerData : A pointer to a C++ bool.
  *                       On success, set to the interpreted boolean
  *                         value.
  *                       On error, left untouched.
  *
  * RETURNS:
  *     See dropt_handle_bool.
  */
dropt_error_t
handle_verbose_bool(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
{
    dropt_bool_t b;
    dropt_error_t err = dropt_handle_verbose_bool(context, valueString, &b);
    if (err == dropt_error_none) { *static_cast<bool*>(handlerData) = (b != 0); }
    return err;
}


/** dropt::handle_string
  *
  *     Obtains a C++ string.
  *
  * PARAMETERS:
  *     IN/OUT context  : The options context.
  *     IN valueString  : A string.
  *                       If NULL, returns dropt_error_insufficient_args.
  *     OUT handlerData : A pointer to a dropt::string.
  *                       On success, set to the input string.
  *                       On error, left untouched.
  *
  * RETURNS:
  *     dropt_error_none
  *     dropt_error_insufficient_args
  *     dropt_error_insufficient_memory
  */
dropt_error_t
handle_string(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
{
    try
    {
        dropt_char_t* s;
        dropt_error_t err = dropt_handle_string(context, valueString, &s);
        if (err == dropt_error_none) { *static_cast<string*>(handlerData) = s; }
        return err;
    }
    catch (...)
    {
        return convert_exception();
    }
}


/** dropt::handle_int
  *
  *     A wrapper around dropt_handle_int.
  */
dropt_error_t
handle_int(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
{
    return dropt_handle_int(context, valueString, handlerData);
}


/** dropt::handle_uint
  *
  *     A wrapper around dropt_handle_uint.
  */
dropt_error_t
handle_uint(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
{
    return dropt_handle_uint(context, valueString, handlerData);
}


/** dropt::handle_double
  *
  *     A wrapper around dropt_handle_double.
  */
dropt_error_t
handle_double(dropt_context_t* context, const dropt_char_t* valueString, void* handlerData)
{
    return dropt_handle_double(context, valueString, handlerData);
}


} // namespace dropt
