#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

// ---------------- TOKEN ----------------
struct Token {
    string type, value;
    int line;
};

const set<string> keywords = {
    "int", "float", "double", "char", "bool", "void",
    "if", "else", "while", "return", "true", "false"
};

const set<string> twoCharOps = {
    "==", "!=", "<=", ">=", "++", "--", "+=", "-=",
    "*=", "/=", "&&", "||"
};

// ---------------- LEXER ----------------
vector<Token> tokenize(const string& source) {
    vector<Token> out;
    size_t i = 0;
    int line = 1;

    auto add = [&](string type, string value) {
        out.push_back({type, value, line});
    };

    while (i < source.size()) {
        char c = source[i];

        if (c == '\n') { ++line; ++i; continue; }
        if (isspace(static_cast<unsigned char>(c))) { ++i; continue; }

        if (c == '/' && i + 1 < source.size() &&
            source[i + 1] == '/') {
            while (i < source.size() && source[i] != '\n') ++i;
            continue;
        }

        if (c == '/' && i + 1 < source.size() &&
            source[i + 1] == '*') {
            int start = line;
            i += 2;
            bool closed = false;
            while (i < source.size()) {
                if (source[i] == '\n') ++line;
                if (source[i] == '*' && i + 1 < source.size() &&
                    source[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                ++i;
            }
            if (!closed) out.push_back(
                {"ERROR", "Unclosed comment", start});
            continue;
        }

        if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t start = i++;
            while (i < source.size() &&
                   (isalnum(static_cast<unsigned char>(source[i])) ||
                    source[i] == '_')) ++i;
            string word = source.substr(start, i - start);
            add(keywords.count(word) ? "KEYWORD" : "IDENTIFIER", word);
            continue;
        }

        if (isdigit(static_cast<unsigned char>(c))) {
            size_t start = i++;
            while (i < source.size() &&
                   (isdigit(static_cast<unsigned char>(source[i])) ||
                    (source[i] == '.' && i + 1 < source.size() &&
                     isdigit(static_cast<unsigned char>(source[i + 1])))))
                ++i;
            add("NUMBER", source.substr(start, i - start));
            continue;
        }

        if (c == '"' || c == '\'') {
            char quote = c;
            size_t start = i++;
            bool closed = false;
            while (i < source.size() && source[i] != '\n') {
                if (source[i] == '\\' && i + 1 < source.size()) {
                    i += 2;
                    continue;
                }
                if (source[i++] == quote) {
                    closed = true;
                    break;
                }
            }
            if (closed)
                add(quote == '"' ? "STRING" : "CHARACTER",
                    source.substr(start, i - start));
            else
                add("ERROR", "Unclosed string or character literal");
            continue;
        }

        string two = i + 1 < source.size()
            ? source.substr(i, 2) : "";
        if (twoCharOps.count(two)) {
            add("OPERATOR", two);
            i += 2;
            continue;
        }

        if (string("+-*/%=<>!&|").find(c) != string::npos) {
            add("OPERATOR", string(1, c));
            ++i;
            continue;
        }

        if (string("(){}[];,.:").find(c) != string::npos) {
            add("PUNCTUATION", string(1, c));
            ++i;
            continue;
        }

        add("ERROR", string("Invalid character: ") + c);
        ++i;
    }

    out.push_back({"EOF", "EOF", line});
    return out;
}

// ---------------- SYMBOL TABLE ----------------
struct Symbol {
    string type;
    int line;
};

class SymbolTable {
    vector<map<string, Symbol>> scopes;

public:
    SymbolTable() { enterScope(); }

    void enterScope() { scopes.push_back({}); }

    void exitScope() {
        if (!scopes.empty()) scopes.pop_back();
    }

    bool declare(const string& name, const string& type, int line) {
        if (scopes.empty()) enterScope();
        auto& current = scopes.back();

        if (current.count(name)) return false;
        current[name] = {type, line};
        return true;
    }

    const Symbol* lookup(const string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return &found->second;
        }
        return nullptr;
    }
};

// ---------------- PARSER + SEMANTIC CHECKS ----------------
class Parser {
    vector<Token> tokens;
    size_t pos = 0;
    SymbolTable symbols;
    vector<string> syntaxErrors, semanticErrors;
    vector<string> tac;
    int temp = 0;

    Token current() const { return tokens[pos]; }

    bool check(const string& value) const {
        return current().value == value;
    }

    Token advance() {
        Token t = current();
        if (pos + 1 < tokens.size()) ++pos;
        return t;
    }

    bool match(const string& value) {
        if (!check(value)) return false;
        advance();
        return true;
    }

    void syntax(const string& message) {
        syntaxErrors.push_back(
            "Line " + to_string(current().line) + ": " + message);
    }

    void semantic(const string& message, int line) {
        semanticErrors.push_back(
            "Line " + to_string(line) + ": " + message);
    }

    bool expect(const string& value) {
        if (match(value)) return true;
        syntax("Expected '" + value + "', found '" +
               current().value + "'");
        return false;
    }

    bool isType() const {
        return current().value == "int" ||
               current().value == "float" ||
               current().value == "double" ||
               current().value == "char" ||
               current().value == "bool" ||
               current().value == "void";
    }

    string newTemp() { return "t" + to_string(++temp); }

    void synchronize() {
        while (current().type != "EOF" &&
               !check(";") && !check("}")) advance();
        if (check(";")) advance();
    }

    // Expression precedence: primary -> unary -> multiply -> add
    // -> comparison -> equality -> logical AND -> logical OR.
    string primary() {
        if (match("(")) {
            string value = expression();
            expect(")");
            return value;
        }

        if (current().type == "NUMBER" ||
            current().type == "STRING" ||
            current().type == "CHARACTER" ||
            check("true") || check("false")) {
            return advance().value;
        }

        if (current().type == "IDENTIFIER") {
            Token id = advance();
            if (!symbols.lookup(id.value))
                semantic("Use of undeclared variable '" +
                         id.value + "'", id.line);
            return id.value;
        }

        syntax("Expected an expression");
        if (current().type != "EOF") advance();
        return "0";
    }

    string unary() {
        if (check("-") || check("+") || check("!")) {
            string op = advance().value;
            string value = unary();
            string result = newTemp();
            tac.push_back(result + " = " + op + value);
            return result;
        }
        return primary();
    }

    string binaryLevel(int level) {
        static const vector<vector<string>> operators = {
            {"*", "/", "%"},
            {"+", "-"},
            {"<", ">", "<=", ">="},
            {"==", "!="},
            {"&&"},
            {"||"}
        };

        if (level == 6) return unary();

        string left = binaryLevel(level + 1);
        while (find(operators[level].begin(), operators[level].end(),
                    current().value) != operators[level].end()) {
            string op = advance().value;
            string right = binaryLevel(level + 1);
            string result = newTemp();
            tac.push_back(result + " = " + left + " " +
                          op + " " + right);
            left = result;
        }
        return left;
    }

    string expression() { return binaryLevel(0); }

    void declaration() {
        string type = advance().value;

        do {
            if (current().type != "IDENTIFIER") {
                syntax("Expected a variable name");
                synchronize();
                return;
            }

            Token id = advance();
            if (!symbols.declare(id.value, type, id.line))
                semantic("Duplicate declaration of '" +
                         id.value + "'", id.line);

            if (match("=")) {
                string value = expression();
                tac.push_back(id.value + " = " + value);
            }
        } while (match(","));

        expect(";");
    }

    void statement() {
        if (isType()) {
            declaration();
            return;
        }

        if (match("{")) {
            symbols.enterScope();
            while (!check("}") && current().type != "EOF")
                statement();
            expect("}");
            symbols.exitScope();
            return;
        }

        if (match("if")) {
            expect("(");
            string condition = expression();
            expect(")");
            tac.push_back("IF " + condition + " GOTO IF_TRUE");
            statement();
            if (match("else")) statement();
            return;
        }

        if (match("while")) {
            expect("(");
            string condition = expression();
            expect(")");
            tac.push_back("WHILE condition: " + condition);
            statement();
            return;
        }

        if (match("return")) {
            if (!check(";")) {
                string value = expression();
                tac.push_back("RETURN " + value);
            } else {
                tac.push_back("RETURN");
            }
            expect(";");
            return;
        }

        if (current().type == "IDENTIFIER") {
            Token id = advance();

            if (!symbols.lookup(id.value))
                semantic("Use of undeclared variable '" +
                         id.value + "'", id.line);

            if (match("=")) {
                string value = expression();
                tac.push_back(id.value + " = " + value);
                expect(";");
                return;
            }

            syntax("Expected '=' after variable name");
            synchronize();
            return;
        }

        if (match(";")) return;

        syntax("Unexpected token '" + current().value + "'");
        synchronize();
        if (check("}")) advance();
    }

public:
    explicit Parser(vector<Token> input) : tokens(input) {}

    void run() {
        while (current().type != "EOF") {
            if (!isType()) {
                syntax("Expected a type or function declaration");
                synchronize();
                if (check("}")) advance();
                continue;
            }

            string type = advance().value;

            if (current().type != "IDENTIFIER") {
                syntax("Expected a function or variable name");
                synchronize();
                continue;
            }

            Token name = advance();

            // Function definition, e.g. int main() { ... }
            if (match("(")) {
                while (!check(")") && current().type != "EOF")
                    advance(); // Skip simple parameter lists.
                expect(")");

                if (match("{")) {
                    symbols.enterScope();
                    while (!check("}") && current().type != "EOF")
                        statement();
                    expect("}");
                    symbols.exitScope();
                } else {
                    syntax("Expected function body");
                }
            } else {
                // Top-level variable declaration.
                if (!symbols.declare(name.value, type, name.line))
                    semantic("Duplicate declaration of '" +
                             name.value + "'", name.line);

                if (match("=")) {
                    string value = expression();
                    tac.push_back(name.value + " = " + value);
                }
                expect(";");
            }
        }
    }

    void report() const {
        cout << "\n=== SYNTAX ANALYSIS ===\n";
        if (syntaxErrors.empty()) cout << "No syntax errors found.\n";
        else for (const auto& e : syntaxErrors) cout << e << '\n';

        cout << "\n=== SEMANTIC ANALYSIS ===\n";
        if (semanticErrors.empty())
            cout << "No basic semantic errors found.\n";
        else for (const auto& e : semanticErrors) cout << e << '\n';

        cout << "\n=== INTERMEDIATE CODE (TAC) ===\n";
        if (tac.empty()) cout << "No intermediate instructions.\n";
        else for (const auto& instruction : tac)
            cout << instruction << '\n';
    }

    bool hasErrors() const {
        return !syntaxErrors.empty() || !semanticErrors.empty();
    }
};

// ---------------- MAIN ----------------
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: CD_LEX_Project <source-file>\n";
        return 1;
    }

    ifstream file(argv[1], ios::binary);
    if (!file) {
        cerr << "Cannot open input file: " << argv[1] << '\n';
        return 1;
    }

    string source((istreambuf_iterator<char>(file)),
                   istreambuf_iterator<char>());

    // Ignore a UTF-8 BOM if the input file has one.
    if (source.size() >= 3 &&
        (unsigned char)source[0] == 0xEF &&
        (unsigned char)source[1] == 0xBB &&
        (unsigned char)source[2] == 0xBF)
        source.erase(0, 3);

    vector<Token> tokens = tokenize(source);

    cout << "=== LEXICAL ANALYSIS ===\n";
    cout << "LINE\tTYPE\t\tVALUE\n";
    bool lexicalErrors = false;

    for (const auto& t : tokens) {
        cout << t.line << '\t' << t.type << "\t\t"
             << t.value << '\n';
        if (t.type == "ERROR") lexicalErrors = true;
    }

    if (lexicalErrors) {
        cout << "\nLexical errors found. Fix them before parsing.\n";
        return 2;
    }

    Parser parser(tokens);
    parser.run();
    parser.report();

    return parser.hasErrors() ? 2 : 0;
}

