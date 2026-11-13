#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <iterator>
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
        {std::regex("float"), "KEYWORD_FLOAT"},
        {std::regex("double"), "KEYWORD_DOUBLE"},
        {std::regex("char"), "KEYWORD_CHAR"},
        {std::regex("bool"), "KEYWORD_BOOL"},
        {std::regex("void"), "KEYWORD_VOID"},
        {std::regex("if"), "KEYWORD_IF"},
        {std::regex("else"), "KEYWORD_ELSE"},
        {std::regex("while"), "KEYWORD_WHILE"},
        {std::regex("for"), "KEYWORD_FOR"},
        {std::regex("return"), "KEYWORD_RETURN"},
        {std::regex("break"), "KEYWORD_BREAK"},
        {std::regex("continue"), "KEYWORD_CONTINUE"},
        {std::regex("true"), "BOOLEAN_LITERAL"},
        {std::regex("false"), "BOOLEAN_LITERAL"},
        {std::regex("[0-9]+\\.[0-9]+"), "FLOAT"},
        {std::regex("[0-9]+"), "INTEGER"},
        {std::regex("\"([^\"\\\\\\r\\n]|\\\\.)*\""), "STRING_LITERAL"},
        {std::regex("'([^'\\\\\\r\\n]|\\\\.)'"), "CHARACTER_LITERAL"},
        {std::regex("'([^'\\\\\\r\\n]|\\\\.)*'"), "ERROR_CHARACTER_LITERAL"},
        {std::regex("'([^'\\\\\\r\\n]|\\\\.)*"), "ERROR_CHARACTER_LITERAL"},
        {std::regex("\"([^\"\\\\\\r\\n]|\\\\.)*"), "ERROR_UNTERMINATED_STRING"},
        {std::regex("/\\*([^*]|\\*+[^*/])*\\*+/"), "SKIP"},
        {std::regex("/\\*([^*]|\\*+[^*/])*(\\*+)?$"), "ERROR_UNTERMINATED_COMMENT"},
        {std::regex("//[^\\r\\n]*"), "SKIP"},
        {std::regex("[a-zA-Z_][a-zA-Z0-9_]*"), "IDENTIFIER"},
        {std::regex(">="), "GREATER_EQUAL"},
        {std::regex("<="), "LESS_EQUAL"},
        {std::regex("=="), "EQUAL"},
        {std::regex("!="), "NOT_EQUAL"},
        {std::regex("&&"), "LOGICAL_AND"},
        {std::regex("\\|\\|"), "LOGICAL_OR"},
        {std::regex("\\+="), "PLUS_ASSIGN"},
        {std::regex("-="), "MINUS_ASSIGN"},
        {std::regex("\\*="), "MULTIPLY_ASSIGN"},
        {std::regex("/="), "DIVIDE_ASSIGN"},
        {std::regex("[%]="), "MODULO_ASSIGN"},
        {std::regex("\\+\\+"), "INCREMENT"},
        {std::regex("--"), "DECREMENT"},
        {std::regex(">"), "GREATER_THAN"},
        {std::regex("<"), "LESS_THAN"},
        {std::regex("="), "ASSIGN"},
        {std::regex("\\+"), "PLUS"},
        {std::regex("-"), "MINUS"},
        {std::regex("\\*"), "MULTIPLY"},
        {std::regex("/"), "DIVIDE"},
        {std::regex("[%]"), "MODULO"},
        {std::regex("!"), "LOGICAL_NOT"},
        {std::regex(";"), "SEMICOLON"},
        {std::regex(","), "COMMA"},
        {std::regex("\\."), "DOT"},
        {std::regex("[(]"), "LEFT_PAREN"},
        {std::regex("[)]"), "RIGHT_PAREN"},
        {std::regex("\\x7B"), "LEFT_BRACE"},
        {std::regex("\\x7D"), "RIGHT_BRACE"},
        {std::regex("[\\[]"), "LEFT_BRACKET"},
        {std::regex("[\\]]"), "RIGHT_BRACKET"},
        {std::regex(":"), "COLON"},
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
                if (static_cast<std::size_t>(match.length()) > bestLength) {
                    bestLength = static_cast<std::size_t>(match.length());
                    bestToken = rule.token;
                }
            }
        }

        static std::size_t line = 1;
        static std::size_t column = 1;

        if (bestLength == 0) {
            std::cout << "LEXICAL_ERROR: unexpected character '"
                      << input[pos] << "' at line " << line
                      << ", column " << column
                      << " (position " << pos << ")\n";

            if (input[pos] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
            ++pos;
        } else {
            std::string lexeme = input.substr(pos, bestLength);

            if (bestToken == "ERROR_UNTERMINATED_STRING") {
                std::cout << "LEXICAL_ERROR: unterminated string literal"
                          << " at line " << line << ", column " << column
                          << " (position " << pos << ")\n";
            } else if (bestToken == "ERROR_CHARACTER_LITERAL") {
                const bool closed = lexeme.size() >= 2
                    && lexeme.back() == '\'';
                std::cout << "LEXICAL_ERROR: "
                          << (closed ? "malformed character literal"
                                     : "unterminated character literal")
                          << " at line " << line << ", column " << column
                          << " (position " << pos << ")\n";
            } else if (bestToken == "ERROR_UNTERMINATED_COMMENT") {
                std::cout << "LEXICAL_ERROR: unterminated block comment"
                          << " at line " << line << ", column " << column
                          << " (position " << pos << ")\n";
            } else if (bestToken != "SKIP") {
                std::cout << bestToken << ": " << lexeme << "\n";
            }

            for (char ch : lexeme) {
                if (ch == '\n') {
                    ++line;
                    column = 1;
                } else {
                    ++column;
                }
            }

            pos += bestLength;
        }
    }
    return 0;
}
