%code requires {
  #include "lambda.h"
  
  #include <memory>

  int yylex();
  void yyerror(FILE* out, const char* s);
}

%parse-param  { FILE* out }

%{
  #include "lambda.h"

  lambda::Reducer reducer;
%}

%union {
  char String[4096];
  lambda::Expression* LambdaExpression;
  lambda::Variable*   LambdaVariable;
}

%token  <String> TK_IDENTIFIER TK_NUMBER
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
    auto [path_string, _] = reducer.reduce($2); 
    fprintf(out, "%s\n", path_string.c_str());
  }
;

expression: abstraction ;

abstraction
  : '\\' variable '.' expression {
    $$ = lambda::Abstraction::get_instance(*$2, $4);
    delete $2;
  } 
  | application 
;

application
  : application atomic {
    $$ = lambda::Application::get_instance($1, $2);
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
  | TK_NUMBER {
    reducer.register_symbol($1, lambda::generate_church_number(atoi($1)));
    $$ = new lambda::Variable($1);
  }
;

%%

void yyerror(FILE* out, const char* s) {
  throw std::runtime_error(s);
}
