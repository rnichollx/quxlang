#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <regex>
#include <string>

/** Counts DNA patterns and performs the prescribed sequential regex replacements. */
int main()
{
    std::string sequence{std::istreambuf_iterator< char >(std::cin), std::istreambuf_iterator< char >()};
    if (std::cin.bad())
    {
        return EXIT_FAILURE;
    }
    std::size_t original_length = sequence.size();
    sequence = std::regex_replace(sequence, std::regex(">.*\n|\n"), "");
    std::size_t sequence_length = sequence.size();
    std::array< char const*, 9 > patterns{"agggtaaa|tttaccct", "[cgt]gggtaaa|tttaccc[acg]", "a[act]ggtaaa|tttacc[agt]t", "ag[act]gtaaa|tttac[agt]ct", "agg[act]taaa|ttta[agt]cct", "aggg[acg]aaa|ttt[cgt]ccct", "agggt[cgt]aa|tt[acg]accct", "agggta[cgt]a|t[acg]taccct", "agggtaa[cgt]|[acg]ttaccct"};
    for (char const* pattern : patterns)
    {
        std::regex expression(pattern);
        std::size_t count = 0;
        for (std::sregex_iterator current(sequence.begin(), sequence.end(), expression), end; current != end; ++current)
        {
            ++count;
        }
        std::cout << pattern << ' ' << count << '\n';
    }
    std::array< char const*, 5 > substitutions{"tHa[Nt]", "aND|caN|Ha[DS]|WaS", "a[NSt]|BY", "<[^>]*>", "\\|[^|][^|]*\\|"};
    std::array< char const*, 5 > replacements{"<4>", "<3>", "<2>", "|", "-"};
    for (std::size_t i = 0; i < substitutions.size(); ++i)
    {
        sequence = std::regex_replace(sequence, std::regex(substitutions[i]), replacements[i]);
    }
    std::cout << '\n' << original_length << '\n' << sequence_length << '\n' << sequence.size() << '\n';
    return std::cout ? EXIT_SUCCESS : EXIT_FAILURE;
}
