#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
struct Rule { std::regex pattern; std::string token; };
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: scanner input_file\n";
        return 1;
    }
    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "Cannot open input file.\n";
        return 1;
    }
    std::string input((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
    std::vector<Rule> rules = {
        {std::regex("int"), "KEYWORD_INT"},
        {std::regex("if"), "KEYWORD_IF"},
        {std::regex("return"), "KEYWORD_RETURN"},
        {std::regex("[A-Za-z_][A-Za-z0-9_]*"), "IDENTIFIER"},
        {std::regex("[0-9]+(\\.[0-9]+)?"), "NUMBER"},
        {std::regex("=="), "EQUALS"},
        {std::regex("="), "ASSIGN"},
        {std::regex("\\+"), "PLUS"},
        {std::regex("-"), "MINUS"},
        {std::regex("\\*"), "MULTIPLY"},
        {std::regex("/"), "DIVIDE"},
        {std::regex(";"), "SEMICOLON"},
        {std::regex("\\("), "LEFT_PAREN"},
        {std::regex("\\)"), "RIGHT_PAREN"},
        {std::regex("\\{"), "LEFT_BRACE"},
        {std::regex("\\}"), "RIGHT_BRACE"},
        {std::regex("[ \\t\\r\\n]+"), "SKIP"},
    };
    std::size_t pos = 0;
    while (pos < input.size()) {
        std::string remaining = input.substr(pos);
        std::size_t bestLength = 0;
        std::string bestToken;
        for (const Rule& rule : rules) {
            std::smatch match;
            if (std::regex_search(remaining, match, rule.pattern,
                                  std::regex_constants::match_continuous)) {
                if (match.length() > bestLength) {
                    bestLength = match.length();
                    bestToken = rule.token;
                }
            }
        }
        if (bestLength == 0) {
            std::cout << "LEXICAL_ERROR: unexpected character '"
                      << input[pos] << "' at position " << pos << "\n";
            ++pos;
        } else {
            if (bestToken != "SKIP")
                std::cout << bestToken << ": "
                          << input.substr(pos, bestLength) << "\n";
            pos += bestLength;
        }
    }
    return 0;
}
