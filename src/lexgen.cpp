#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <sstream>

struct Rule {
    std::string pattern;
    std::string token;
};

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

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "LEX Scanner Generator\n"
                  << "Usage: lexgen rules.lex generated.cpp\n";
        return 1;
    }

    std::ifstream spec(argv[1]);
    if (!spec) {
        std::cerr << "Error: cannot open rules file.\n";
        return 1;
    }

    std::vector<Rule> rules;
    std::string line;

    while (std::getline(spec, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::size_t split = line.find("=>");
        if (split == std::string::npos) {
            std::cerr << "Invalid rule: " << line << "\n"
                      << "Expected: REGEX => TOKEN_NAME\n";
            return 1;
        }

        std::string pattern = line.substr(0, split);
        std::string token = line.substr(split + 2);

        while (!pattern.empty() && pattern.back() == ' ')
            pattern.pop_back();
        while (!token.empty() && token.front() == ' ')
            token.erase(token.begin());

        if (pattern.empty() || token.empty()) {
            std::cerr << "Empty pattern or token name.\n";
            return 1;
        }

        try {
            std::regex check(pattern);
        } catch (const std::regex_error&) {
            std::cerr << "Invalid regular expression: "
                      << pattern << "\n";
            return 1;
        }

        rules.push_back({pattern, token});
    }

    if (rules.empty()) {
        std::cerr << "No rules found.\n";
        return 1;
    }

    std::ofstream out(argv[2]);
    if (!out) {
        std::cerr << "Error: cannot create output file.\n";
        return 1;
    }

    out << R"(#include <iostream>
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
)";

    std::cout << "Scanner generated successfully: "
              << argv[2] << "\n";
    std::cout << "Rules loaded: " << rules.size() << "\n";
    return 0;
}
