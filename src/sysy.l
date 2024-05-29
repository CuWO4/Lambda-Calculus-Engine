%option noyywrap

%{
  #define YY_NO_INPUT 1
  #define YY_NO_UNPUT 1

  #include "sysy.tab.hpp"

  #include <string>
%}

WhiteSpace      [ \n\r\t]
Identifier      [^0-9 \n\r\t\\@#.():][^ \n\r\t\\@#.:()]*
Number          [0-9]+

%%

{WhiteSpace}
":="            { return TK_DEFINE; }
{Identifier}    { strcpy(yylval.String, yytext); return TK_IDENTIFIER; }
{Number}        { strcpy(yylval.String, yytext); return TK_NUMBER; }
.               { return yytext[0]; }

%%
