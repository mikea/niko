#include <getopt.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

#include "inter.h"
#include "niko.h"

// repl

INLINE void stack_print_repl(stack& stack) {
  DO(i, stack.len()) {
    if (i > 0) print(cout, " ");
    print(cout, "{:25}", stack[i]);
  }
}

void v() {}

void repl(inter_t& inter) {
  std::string input;
  bool        prompt = isatty(STDIN_FILENO);

  if (prompt) printf(VERSION_STRING "\n");

next:
  while (true) {
    if (prompt) {
      if (inter.arr_level) {
        printf("%ld> ", inter.arr_level);
      } else {
        stack_print_repl(inter.stack);
        printf(" > ");
      }
    }

    if (!std::getline(std::cin, input)) return;

    try {
      inter.line(input.c_str());
    } catch (std::exception& e) {
      cerr << "ERROR: " << e.what() << "\n";
      goto next;
    }
  }
}

// test runner

int test(inter_t& inter, const char* fname, bool v, bool f) {
  int           ret = 0;
  std::ifstream file(fname);

  std::string line;
  size_t      line_no = 0;

  std::string      out;
  std::string_view rest_out;
  size_t           in_line_no = 0;

  bool in_nk                  = false;
  bool in_nkt                 = false;

  while (std::getline(file, line)) {
    if (f && ret) return ret;
    line_no++;

    if (in_nk) {
      if (line.starts_with("```")) {
        inter.reset();
        in_nk = false;
      } else {
        inter.line(line.c_str());
      }
    } else if (in_nkt) {
      if (line.starts_with("```")) {
        inter.reset();
        if (rest_out.size()) {
          ERROR("ERROR {}:{} : unmatched output: '{}'", fname, in_line_no, rest_out);
          ret = 1;
        }
        rest_out = "";
        in_nkt   = false;
      } else if (line[0] == '>') {
        if (v) ERROR("{}", line);
        if (rest_out.size()) {
          ERROR("ERROR {}:{} : unmatched output: '{}'", fname, in_line_no, rest_out);
          ret = 1;
        }
        in_line_no = line_no;
        out        = inter.line_capture_out(line.c_str() + 1);
        rest_out   = out;
        continue;
      } else if (!rest_out.starts_with(line + "\n")) {
        cerr << "ERROR " << fname << ":" << in_line_no << " : mismatched output, expected: '" << line << "' actual: '"
             << rest_out << "'\n";
        ret      = 1;
        rest_out = "";
      } else {
        rest_out = rest_out.substr(line.size() + 1);
      }
    }
    if (line.ends_with("```nkt")) in_nkt = true;
    else if (line.ends_with("```nk")) in_nk = true;
  }

  return ret;
}

void h(FILE* f, char* argv_0) {
  fprintf(f, VERSION_STRING "\n");
  fprintf(f, "\nUSAGE:\n");
  fprintf(f, "    %s [FLAGS] [OPTIONS]\n", argv_0);
  fprintf(f, "\nFLAGS:\n");
  fprintf(f, "    -z               Do not load the prelude\n");
  fprintf(f, "    -v               Verbose test execution\n");
  fprintf(f, "    -f               Fail fast\n");
  fprintf(f, "    -h               Print help information\n");
  fprintf(f, "\nOPTIONS:\n");
  fprintf(f, "    -e <expr>        Evaluate niko expression\n");
  fprintf(f, "    -t <test.md>     Run markdown test\n");
  fprintf(f, "\nEnters the repl if no options are specified (using rlwrap is recommended).\n");
  fprintf(f, "\n");
}

// main

int main(int argc, char* argv[]) {
  char *t = NULL, *e = NULL;
  bool  v = false;
  bool  z = false;
  bool  f = false;

  int opt;
  while ((opt = getopt(argc, argv, "fvzht:e:m:")) != -1) {
    switch (opt) {
      case 't': t = optarg; break;
      case 'v': v = true; break;
      case 'e': e = optarg; break;
      case 'z': z = true; break;
      case 'f': f = true; break;
      case 'h': {
        h(stdout, argv[0]);
        return 0;
      }
      default: {
        h(stderr, argv[0]);
        return 1;
      }
    }
  }

  inter_t inter(!z);

  try {
    if (t) return test(inter, t, v, f);
    else if (e) inter.line(e);
    else repl(inter);
    return 0;
  } catch (std::exception& e) {
    cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
}