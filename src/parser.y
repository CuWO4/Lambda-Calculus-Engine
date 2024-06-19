%code requires {
  #include "lambda.h"
  
  #include <memory>

  int yylex();
  void yyerror(FILE* out, bool display_process, const char* s);
}

%parse-param  { FILE* out }
%parse-param  { bool display_process }

%{
  #include "lambda.h"

  lambda::Reducer reducer;
%}

%union {
  char String[4096];
  lambda::Expression* LambdaExpression;
  lambda::Variable*   LambdaVariable;
}

%token  <String> TK_IDENTIFIER
%token  TK_DEFINE

%type <LambdaExpression> expression abstraction application atomic
%type <LambdaVariable> variable

%%

comp_unit
  : commands
;

commands
  : commands command
  |
;

command
  : definition
  | solution
;

definition
  : '#' TK_IDENTIFIER TK_DEFINE expression {
    reducer.register_symbol($2, $4);
  }
;

solution
  : '@' expression { 
    reducer.reduce($2, out, display_process); 
  }
;

expression: abstraction ;

abstraction
  : '\\' variable '.' expression {
    $$ = new lambda::Abstraction(*$2, $4);
    delete $2;
  } 
  | application 
;

application
  : application atomic {
    $$ = new lambda::Application($1, $2);
  }
  | atomic
;

atomic
  : variable            { $$ = $1; }
  | '(' expression ')' { $$ = $2; }
  | '{' expression '}' { 
    $$ = $2; 
    $$->set_computational_priority(lambda::ComputationalPriority::Eager);
  }
  | '$' atomic { 
    $$ = $2; 
    $$->set_computational_priority(lambda::ComputationalPriority::Lazy);
  }
;

variable
  : TK_IDENTIFIER { $$ = new lambda::Variable($1); }
;

%%

void yyerror(FILE* out, bool display_process, const char* s) {
  throw std::runtime_error(s);
}
