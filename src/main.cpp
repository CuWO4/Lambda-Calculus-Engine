#include <iostream>

extern FILE* yyin;
extern int yyparse(FILE* ast);
FILE* out = stdout;

void handle_args(int argc, char** argv) {
  if (argc < 2) { throw std::runtime_error("no input file specify"); }
  yyin = fopen(argv[1], "r");
  if (yyin == nullptr) { throw std::runtime_error("cannot open file"); }

  if (argc >= 3) {
    out = fopen(argv[2], "w");
  }
}

int main(int argc, char** argv) {
  try {
    handle_args(argc, argv);

    yyparse(out);
  } 
  catch (std::runtime_error& s) {
    std::cout << s.what() << std::endl;
    return 0;
  }

  return 0;
}