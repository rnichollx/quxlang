//
// Created by rpnx on 1/3/25.
//

#ifndef LANG_HPP
#define LANG_HPP

#include <map>
#include <optional>
#include <set>
#include <string>

namespace quxlang
{
    struct localizer
    {
        std::set< std::string > all_keywords;
        std::set< std::string > kw_values;
        std::map< std::string, std::map< std::string, std::string > > tr_from;
        std::map< std::string, std::map< std::string, std::string > > tr_to;

        localizer()
        {
            init_en();
            init_jp();
        }

        bool is_value_kw(std::string const & str)
        {

            return kw_values.contains(str);
        }



        std::optional< std::string > translate_from(std::string const& lang, std::string const& kw)
        {
            auto const& translator = tr_from.at(lang);

            auto it = translator.find(kw);

            if (it != translator.end())
            {
                return it->second;
            }
            else
            {
                return std::nullopt;
            }
        }

        void init_en()
        {

            // kw values are non-literal keywords that can appear as part of a non-type expression

            // clang-format off
            kw_values = {
                "OTHER",
                "THIS",
                "NULLPTR",
                "UNSPECIFIED",
                "POISON",
                "FALSE",
                "TRUE",
                "ARCH_X64",
                "ARCH_X86",
                "ARCH_ARM32",
                "ARCH_ARM64",
                "ARCH_RISCV64",

                "KERNEL_LINUX",
                "KERNEL_NT",
                "KERNEL_BSD",
                "KERNEL_XNU",

                "OS_WINDOWS",
                "OS_LINUX",
                "OS_BSD",
                "OS_MACOS"
            };
            // clang-format on

            // Initialize the set of English keywords
            all_keywords = {"IF", "FOR", "LOOP", "AFTER", "ELSE", "RETURN", "BREAK", "CONTINUE", "VAR", "FUNCTION", "CLASS", "MODULE", "STRUCT", "I", "U"};

            for (const auto& keyword : kw_values)
            {
                all_keywords.insert(keyword);
            }

            // Build identity mapping for English using all_keywords
            for (const auto& keyword : all_keywords)
            {
                tr_from["EN"][keyword] = keyword;
            }
            tr_to["EN"] = tr_from["EN"];
        }

        void init_jp()
        {
            // clang-format off
            tr_to["JP"] = {
                {"IF", "MOSHI"},
                {"FOR", "TAME"},
                {"LOOP", "WA"},
                {"AFTER", "ATO"},
                {"ELSE", "SOREIGAI"},
                {"RETURN", "MODORU"},
                {"BREAK", "KIRU"},
                {"CONTINUE", "TSUZUKERU"},
                {"VAR", "HENNSUU"},
                {"FUNCTION", "KINOU"},
                {"CLASS", "KURASU"},
                {"MODULE", "MODYURU"},
                {"STRUCT", "KOZOU"},
                {"I", "SE"},
                {"U", "MU"},

            };
            // clang-format on

            // Build tr_from by looping over tr_to
            for (const auto& [en, jp] : tr_to["JP"])
            {
                tr_from["JP"][jp] = en;
            }
        }
    };

} // namespace quxlang

#endif // LANG_HPP
