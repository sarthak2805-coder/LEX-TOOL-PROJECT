# Pattern => TOKEN_NAME
int => KEYWORD_INT
if => KEYWORD_IF
return => KEYWORD_RETURN
[A-Za-z_][A-Za-z0-9_]* => IDENTIFIER
[0-9]+(\.[0-9]+)? => NUMBER
== => EQUALS
= => ASSIGN
\+ => PLUS
- => MINUS
\* => MULTIPLY
/ => DIVIDE
; => SEMICOLON
\( => LEFT_PAREN
\) => RIGHT_PAREN
\{ => LEFT_BRACE
\} => RIGHT_BRACE
[ \t\r\n]+ => SKIP
