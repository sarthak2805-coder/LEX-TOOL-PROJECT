#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <iterator>
#include <cctype>

struct Rule {
    std::string pattern;
    std::string token;
};

std::string trim(const std::string& s) {
    const std::string ws = " \t\r\n";
    std::size_t first = s.find_first_not_of(ws);
    if (first == std::string::npos) return "";
    return s.substr(first, s.find_last_not_of(ws) - first + 1);
}

std::string escapeString(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\\' || c == '"') out += '\\';
        if (c == '\n') { out += "\\n"; continue; }
        if (c == '\r') { out += "\\r"; continue; }
        if (c == '\t') { out += "\\t"; continue; }
        out += c;
    }
    return out;
}

bool readRules(const std::string& path, std::vector<Rule>& rules) {
    std::ifstream spec(path);
    if (!spec) {
        std::cerr << "Cannot open rules file: " << path << "\n";
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    bool lexStyle = false;
    int separators = 0;

    while (std::getline(spec, line)) {
        lines.push_back(line);
        if (trim(line) == "%%") ++separators;
    }
    lexStyle = separators > 0;

    bool inDefinitions = false;
    for (const std::string& raw : lines) {
        std::string s = trim(raw);

        if (s == "%{") { inDefinitions = true; continue; }
        if (s == "%}") { inDefinitions = false; continue; }
        if (inDefinitions) continue;
        if (s == "%%") continue;
        if (s.empty() || s[0] == '#') continue;
        if (s[0] == '%') continue;

        // A line-comment token rule also begins with //, so keep its action.
        if (s.rfind("/*", 0) == 0
            || (s.rfind("//", 0) == 0
                && s.find(" {") == std::string::npos
                && s.find("=>") == std::string::npos))
            continue;

        std::string pattern, token;
        std::size_t split = s.find("=>");

        if (split != std::string::npos) {
            pattern = trim(s.substr(0, split));
            token = trim(s.substr(split + 2));
        } else if (lexStyle) {
            // The action starts at the final " { " delimiter.
            // This preserves spaces inside patterns such as [ \t\r\n]+.
            std::size_t open = s.find(" {");
            std::size_t close = s.rfind('}');

            if (open == std::string::npos || close == std::string::npos
                || close < open) {
                std::cerr << "Invalid LEX-style rule: " << s << "\n"
                          << "Expected: PATTERN { TOKEN_NAME; }\n";
                return false;
            }

            pattern = trim(s.substr(0, open));
            token = trim(s.substr(open + 2, close - open - 2));

            std::size_t semi = token.find(';');
            if (semi != std::string::npos)
                token = trim(token.substr(0, semi));
        } else {
            std::cerr << "Invalid rule: " << s << "\n"
                      << "Expected: REGEX => TOKEN_NAME\n";
            return false;
        }

        if (pattern.empty() || token.empty()) {
            std::cerr << "Empty pattern or token name.\n";
            return false;
        }

        try {
            std::regex check(pattern);
        } catch (const std::regex_error&) {
            std::cerr << "Invalid regular expression: " << pattern << "\n";
            return false;
        }

        rules.push_back({pattern, token});
    }

    if (rules.empty()) {
        std::cerr << "No rules found.\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Simplified LEX-Style Scanner Generator\n"
                  << "Usage: lexgen_v2 rules_file generated.cpp\n";
        return 1;
    }

    std::vector<Rule> rules;
    if (!readRules(argv[1], rules)) return 1;

    std::ofstream out(argv[2]);
    if (!out) {
        std::cerr << "Cannot create output file.\n";
        return 1;
    }

    out << R"(#include <iostream>
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
)";

    for (const Rule& r : rules) {
        out << "        {std::regex(\""
            << escapeString(r.pattern) << "\"), \""
            << escapeString(r.token) << "\"},\n";
    }

    out << R"(    };
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
)";

    if (!out) {
        std::cerr << "Failed to write generated scanner.\n";
        return 1;
    }

    std::cout << "Scanner generated successfully: " << argv[2] << "\n";
    std::cout << "Rules loaded: " << rules.size() << "\n";
    return 0;
}

