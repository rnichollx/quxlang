//
// Created by rpnx on 1/3/25.
//

#ifndef LANG_HPP
#define LANG_HPP

#include <string>

namespace quxlang
{
    struct localizer
    {
        std::string kw_if;
        std::string kw_for;
        std::string kw_loop;
        std::string kw_after;
        std::string kw_else;
        std::string kw_return;
        std::string kw_break;
        std::string kw_continue;
        std::string kw_var;
        std::string kw_function;
        std::string kw_class;
        std::string kw_module;
        std::string kw_struct;
        std::string kw_signed_int;
        std::string kw_unsigned_int;


        void init_en()
        {
            kw_if = "IF";
            kw_for = "FOR";
            kw_loop = "LOOP";
            kw_after = "AFTER";
            kw_else = "ELSE";
            kw_return = "RETURN";
            kw_break = "BREAK";
            kw_continue = "CONTINUE";
            kw_var = "VAR";
            kw_function = "FUNCTION";
            kw_class = "CLASS";
            kw_module = "MODULE";
            kw_struct = "STRUCT";
            kw_signed_int = "I";
            kw_unsigned_int = "U";
        }


        void init_jp()
        {
            kw_if = "MOSHI";
            kw_for = "TAME";
            kw_loop = "WA";
            kw_after = "ATO";
            kw_else = "SOREIGAI";
            kw_return = "MODORU";
            kw_break = "KIRU";
            kw_continue = "TSUZUKERU";
            kw_var = "HENNSUU";
            kw_function = "KINOU";
            kw_class = "KURASU";
            kw_module = "MODYURU";
            kw_struct = "KOZOU";
            kw_signed_int = "SE";
            kw_unsigned_int = "MU";
        }
    };


}


#endif //LANG_HPP
