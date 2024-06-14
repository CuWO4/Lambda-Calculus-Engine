#include <iostream>
#include <stack>
#include <string.h>

extern FILE* yyin;
extern int yyparse(FILE*, bool);
extern std::stack<std::string> include_path_stack;
FILE* out = stdout;
bool display_process = false;

void handle_args(int argc, char** argv) {
  FILE* in = nullptr;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-o")) {
      if (++i >= argc) {
        throw std::runtime_error(std::string(argv[0]) + "argument missing for -o option");
      }
      out = fopen(argv[i], "w");
    }
    else if (!strcmp(argv[i], "-i")) {
      display_process = true;
    }
    else {
      include_path_stack.push(argv[1]);
      in = fopen(argv[1], "r");
      if (in == nullptr) { 
        throw std::runtime_error(std::string(argv[0]) + "cannot open file"); 
      }
    }
  }

  if (in == nullptr) { 
    throw std::runtime_error(std::string(argv[0]) + "no input file specify"); 
  }
  yyin = in;
}

int main(int argc, char** argv) {
  try {
    handle_args(argc, argv);

    yyparse(out, display_process);
  } 
  catch (std::runtime_error& s) {
    std::cout << s.what() << std::endl;
    return 0;
  }

  return 0;
}