/** dropt.hpp
  *
  *     A C++ wrapper for dropt.
  *
  * Copyright (C) 2008 James D. Lin <jameslin@csua.berkeley.edu>
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

#ifndef DROPT_HPP
#define DROPT_HPP

#include <string>
#include <iostream>
#include "dropt.h"


namespace dropt
{


typedef std::basic_string<dropt_char_t> string;
typedef std::basic_ostream<dropt_char_t> ostream;


class context
{
public:
    context(const dropt_option_t* options);
    ~context();

    dropt_context_t* raw();

    void set_error_handler(dropt_error_handler_t handler, void* handlerData);
    void set_strncmp(dropt_strncmp_t cmp);

    dropt_char_t** parse(dropt_char_t** argv);

    dropt_error_t get_error() const;
    void get_error_details(dropt_char_t** optionName, dropt_char_t** valueString) const;
    const dropt_char_t* get_error_message();
    void clear_error();

#ifndef DROPT_NO_STRING_BUFFERS
    string get_help(bool compact = false) const;
#endif

private:
    dropt_context_t* mContext;
    const dropt_option_t* mOptions;

private:
    // Intentionally unimplemented to be non-copyable.
    context(const context&);
    context& operator=(const context&);
};


dropt_error_t convert_exception();

DROPT_HANDLER_DECL(handle_bool);
DROPT_HANDLER_DECL(handle_verbose_bool);
DROPT_HANDLER_DECL(handle_string);

DROPT_HANDLER_DECL(handle_int);
DROPT_HANDLER_DECL(handle_uint);
DROPT_HANDLER_DECL(handle_double);


} // namespace dropt


#endif // DROPT_HPP