%option noinput

%{
  #define YY_INPUT(buf, result, max_size) \
  { \
    int c = fgetc(yyin); \
    result = (c == EOF) ? YY_NULL : (buf[0] = c, 1); \
  }

  #define YY_NO_UNPUT 1

  #include "sysy.tab.hpp"

  #include <string>
  #include <stack>

  std::stack<FILE*> include_file_stack;
  std::stack<std::string> include_path_stack;
%}

WhiteSpace      [ \n\r\t]
Identifier      [^0-9 \n\r\t\\@#.():][^ \n\r\t\\@#.:()]*
Number          [0-9]+
Path            [^:*?"<>|\n\r]+

%x  IMPORT_STATE
%x  PATH_STATE

%%
"import"              { BEGIN(IMPORT_STATE); return TK_IMPORT; }
<IMPORT_STATE>{WhiteSpace}
<IMPORT_STATE>\"      { BEGIN(PATH_STATE); }
<PATH_STATE>\"        { BEGIN(INITIAL); }
<PATH_STATE>{Path}    { strcpy(yylval.String, yytext); return TK_PATH; }
<PATH_STATE>{WhiteSpace}
<PATH_STATE>.         { return yytext[0]; }


{WhiteSpace}
":="            { return TK_DEFINE; }
{Identifier}    { strcpy(yylval.String, yytext); return TK_IDENTIFIER; }
{Number}        { strcpy(yylval.String, yytext); return TK_NUMBER; }
.               { return yytext[0]; }

%%

int yywrap() { 
  if (include_file_stack.empty()) { return 1; }

  yyin = include_file_stack.top();
  include_file_stack.pop();
  include_path_stack.pop();
  return 0;
}