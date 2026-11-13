$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "build"
$generator = Join-Path $buildDir "lexgen_v2.exe"
$scannerSource = Join-Path $root "src\lex_style_scanner.cpp"
$scanner = Join-Path $buildDir "lex_style_scanner.exe"
$rules = Join-Path $PSScriptRoot "lex_style_sample.lex"
$validInput = Join-Path $PSScriptRoot "lex_style_input.txt"
$invalidInput = Join-Path $PSScriptRoot "lex_style_error_input.txt"
$featureInput = Join-Path $buildDir ("feature_test_{0}.txt" -f [Guid]::NewGuid())
$literalInput = Join-Path $buildDir ("literal_test_{0}.txt" -f [Guid]::NewGuid())
$commentInput = Join-Path $buildDir ("comment_test_{0}.txt" -f [Guid]::NewGuid())
$continuationInput = Join-Path $buildDir ("location_test_{0}.txt" -f [Guid]::NewGuid())

try {
    if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
        throw "g++ was not found on PATH. Install a C++17-capable MinGW toolchain."
    }

    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

    Write-Host "[BUILD] Generator"
    & g++ -std=c++17 -Wall -Wextra -pedantic `
        (Join-Path $root "src\lexgen_v2.cpp") -o $generator
    if ($LASTEXITCODE -ne 0) {
        throw "Generator compilation failed with exit code $LASTEXITCODE."
    }

    Write-Host "[GENERATE] Scanner from tests\lex_style_sample.lex"
    & $generator $rules $scannerSource
    if ($LASTEXITCODE -ne 0) {
        throw "Scanner generation failed with exit code $LASTEXITCODE."
    }

    Write-Host "[BUILD] Generated scanner"
    & g++ -std=c++17 -Wall -Wextra -pedantic $scannerSource -o $scanner
    if ($LASTEXITCODE -ne 0) {
        throw "Generated scanner compilation failed with exit code $LASTEXITCODE."
    }

    Write-Host "[TEST] Valid fixture"
    $validOutput = (& $scanner $validInput 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "Valid fixture scanner exited with code $LASTEXITCODE."
    }
    if ($validOutput.Contains("LEXICAL_ERROR:")) {
        throw "Valid fixture unexpectedly produced a lexical error."
    }
    foreach ($expected in @(
        "KEYWORD_INT: int",
        "IDENTIFIER: x",
        "ASSIGN: =",
        "INTEGER: 1",
        "SEMICOLON: ;",
        "INCREMENT: ++"
    )) {
        if (-not $validOutput.Contains($expected)) {
            throw "Valid fixture output did not contain '$expected'."
        }
    }
    Write-Host "PASS: valid fixture has expected tokens and no lexical errors."

    if ($validOutput -match "(?m)^SKIP:") {
        throw "A SKIP token appeared in the emitted valid-fixture tokens."
    }
    Write-Host "PASS: whitespace SKIP rules are not emitted as tokens."

    Write-Host "[TEST] Existing invalid fixture"
    $invalidOutput = (& $scanner $invalidInput 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "Invalid fixture scanner exited with code $LASTEXITCODE."
    }
    if (-not $invalidOutput.Contains("LEXICAL_ERROR: unexpected character '@'")) {
        throw "Existing invalid fixture did not report '@' as a lexical error."
    }
    if (-not $invalidOutput.Contains("at line 1, column 16")) {
        throw "Existing invalid fixture did not report the expected line 1, column 16."
    }
    Write-Host "PASS: existing invalid fixture reports a lexical error at line 1, column 16."

    [System.IO.File]::WriteAllText(
        $continuationInput,
        "int total = 25;`n  @`nreturn total;",
        [System.Text.Encoding]::ASCII
    )

    Write-Host "[TEST] Multiline location and continuation"
    $continuationOutput = (& $scanner $continuationInput 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "Multiline scanner exited with code $LASTEXITCODE."
    }
    if (-not $continuationOutput.Contains("at line 2, column 3")) {
        throw "Multiline invalid character did not report line 2, column 3."
    }
    if (-not $continuationOutput.Contains("KEYWORD_RETURN: return")) {
        throw "Scanner did not continue and recognize 'return' after the invalid character."
    }
    if ($continuationOutput -match "(?m)^SKIP:") {
        throw "A SKIP token appeared in the emitted multiline tokens."
    }
    Write-Host "PASS: multiline location is correct, scanning continues, and SKIP is omitted."

    $featureSource = @'
int total = 12;
float ratio = 3.25;
char letter = 'Q';
char newline = '\n';
ifx = 2;
text = "say \"hi\"; // text /* not a comment */";
if (total >= 10 && total <= 12 && total == 12 && total > 0 && ratio != 0.0 || !false) {
    total += 1;
    total -= 1;
    total *= 2;
    total /= 2;
    total %= 2;
    total++;
    total--;
    total = total / 2;
}
for (;;) { break; }
continue;
total = [1, 2].0;
label:;
'@
    [System.IO.File]::WriteAllText($featureInput, $featureSource, [System.Text.Encoding]::ASCII)
    Write-Host "[TEST] Supported token categories and comments"
    $featureOutput = (& $scanner $featureInput 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "Feature input scanner exited with code $LASTEXITCODE."
    }
    if ($featureOutput.Contains("LEXICAL_ERROR:")) {
        throw "Feature input unexpectedly produced a lexical error: $featureOutput"
    }
    foreach ($expected in @(
        "KEYWORD_INT: int",
        "KEYWORD_FLOAT: float",
        "KEYWORD_CHAR: char",
        "INTEGER: 12",
        "IDENTIFIER: ifx",
        "FLOAT: 3.25",
        "CHARACTER_LITERAL: 'Q'",
        "CHARACTER_LITERAL: '\n'",
        'STRING_LITERAL: "say \"hi\"; // text /* not a comment */"',
        "BOOLEAN_LITERAL: false",
        "GREATER_EQUAL: >=",
        "LESS_EQUAL: <=",
        "EQUAL: ==",
        "GREATER_THAN: >",
        "NOT_EQUAL: !=",
        "LOGICAL_AND: &&",
        "LOGICAL_OR: ||",
        "LOGICAL_NOT: !",
        "PLUS_ASSIGN: +=",
        "MINUS_ASSIGN: -=",
        "MULTIPLY_ASSIGN: *=",
        "DIVIDE_ASSIGN: /=",
        "MODULO_ASSIGN: %=",
        "INCREMENT: ++",
        "DECREMENT: --",
        "DIVIDE: /",
        "LEFT_BRACKET: [",
        "RIGHT_BRACKET: ]",
        "COMMA: ,",
        "DOT: .",
        "COLON: :"
    )) {
        if (-not $featureOutput.Contains($expected)) {
            throw "Feature output did not contain '$expected'. Output: $featureOutput"
        }
    }
    if ($featureOutput -match "(?m)^IDENTIFIER: (line|comment)\r?$") {
        throw "A comment was emitted instead of skipped."
    }
    if ([regex]::Matches($featureOutput, "(?m)^DIVIDE: /\r?$").Count -ne 1) {
        throw "Line comments interfered with the division operator or were emitted."
    }
    if ($featureOutput -match "(?m)^SKIP:") {
        throw "A SKIP token appeared in the feature output."
    }
    Write-Host "PASS: keywords, literals, operators, punctuation, and comment skipping."

    [System.IO.File]::WriteAllText(
        $literalInput,
        "`"unterminated`nint after = 1;`n'abc' @`n'open",
        [System.Text.Encoding]::ASCII
    )
    Write-Host "[TEST] Malformed literals and recovery"
    $literalOutput = (& $scanner $literalInput 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "Malformed-literal scanner exited with code $LASTEXITCODE."
    }
    if (-not $literalOutput.Contains("unterminated string literal at line 1, column 1")) {
        throw "Unterminated string was not reported at its opening location."
    }
    if (-not $literalOutput.Contains("KEYWORD_INT: int")) {
        throw "Scanner did not resume after an unterminated string at newline."
    }
    if (-not $literalOutput.Contains("malformed character literal at line 3, column 1")) {
        throw "Malformed character literal was not reported at line 3, column 1."
    }
    if (-not $literalOutput.Contains("unexpected character '@' at line 3, column 7")) {
        throw "Scanner did not continue after a malformed character literal."
    }
    if (-not $literalOutput.Contains("unterminated character literal at line 4, column 1")) {
        throw "Unterminated character literal was not reported at line 4, column 1."
    }
    Write-Host "PASS: malformed literals are reported clearly and scanning recovers."

    [System.IO.File]::WriteAllText(
        $commentInput,
        "int n = 8 / 2; // line comment`n/* block`ncomment */ n++;`n/* never closed",
        [System.Text.Encoding]::ASCII
    )
    Write-Host "[TEST] Division, comments, and unterminated block comment"
    $commentOutput = (& $scanner $commentInput 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "Comment scanner exited with code $LASTEXITCODE."
    }
    if (-not $commentOutput.Contains("DIVIDE: /")) {
        throw "Division operator was not recognized next to comments."
    }
    if (-not $commentOutput.Contains("IDENTIFIER: n")) {
        throw "Scanner did not resume after a closed multiline comment."
    }
    if (-not $commentOutput.Contains("unterminated block comment at line 4, column 1")) {
        throw "Unterminated block comment was not reported at line 4, column 1."
    }
    if ($commentOutput -match "(?m)^IDENTIFIER: (line|comment)$") {
        throw "Comment text was emitted as tokens."
    }
    Write-Host "PASS: line/block comments skip correctly, division remains an operator, and unclosed blocks report."

    Write-Host "All scanner pipeline tests passed."
}
catch {
    [Console]::Error.WriteLine("TEST FAILURE: {0}" -f $_.Exception.Message)
    throw
}
finally {
    foreach ($path in @($featureInput, $literalInput, $commentInput, $continuationInput)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -ErrorAction SilentlyContinue
        }
    }
}
