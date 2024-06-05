#include <iostream>

#include <getopt.h>
#include <unistd.h>

#include "inter.h"
#include "niko.h"

// repl

INLINE void stack_print_repl(stack_t& stack) {
  DO(i, stack.len()) {
    if (i > 0) print(cout, " ");
    print(cout, "{:25}", stack[i]);
  }
}

void v() {}

void repl(inter_t* inter) {
  size_t input_size = 0;
  own(char) input   = NULL;
  bool prompt       = isatty(STDIN_FILENO);

  if (prompt) printf(VERSION_STRING "\n");

next:
  while (true) {
    if (prompt) {
      if (inter->arr_level) {
        printf("%ld> ", inter->arr_level);
      } else {
        stack_print_repl(inter->stack);
        printf(" > ");
      }
    }

    if (getline(&input, &input_size, stdin) <= 0) return;

    try {
      inter_line(inter, input);
    } catch (std::exception& e) {
      cerr << "ERROR: " << e.what() << "\n";
      goto next;
    }
  }
}

// test runner

int test(inter_t* inter, const char* fname, bool v, bool f) {
  int ret        = 0;
  own(FILE) file = fopen(fname, "r");
  CHECK(file, "test: can't open file: %s", fname);

  size_t len     = 0;
  own(char) line = NULL;

  size_t read;
  size_t line_no = 0;

  std::string      out;
  size_t           out_size;
  std::string_view rest_out;
  size_t           in_line_no = 0;

  const str_t nkt_start       = "```nkt\n";
  const str_t nk_start        = "```nk\n";
  const str_t code_end        = "```";

  bool in_nk                  = false;
  bool in_nkt                 = false;

  while ((read = getline(&line, &len, file)) != -1) {
    if (f && ret) return ret;
    line_no++;
    if (read == 0) continue;

    str_t l(line);

    if (in_nk) {
      if (str_starts_with(l, code_end)) {
        inter_reset(inter);
        in_nk = false;
      } else {
        inter_line(inter, line);
      }
    } else if (in_nkt) {
      if (str_starts_with(l, code_end)) {
        inter_reset(inter);
        if (rest_out.size()) {
          cerr << "ERROR " << fname << ":" << in_line_no << " : unmatched output: '" << rest_out << "'\n";
          ret = 1;
        }
        rest_out = "";
        in_nkt   = false;
      } else if (*line == '>') {
        if (v) fprintf(stderr, "%s", line);
        if (rest_out.size()) {
          cerr << "ERROR " << fname << ":" << in_line_no << " : unmatched output: '" << rest_out << "'\n";
          ret = 1;
        }
        in_line_no = line_no;
        out        = inter_line_capture_out(inter, line + 1);
        rest_out   = out;
        continue;
      } else if (!rest_out.size() || memcmp(line, rest_out.begin(), read)) {
        cerr << "ERROR " << fname << ":" << in_line_no << " : mismatched output, expected: '" << line << "' actual: '"
             << rest_out << "'\n";
        ret      = 1;
        rest_out = "";
      } else {
        rest_out = rest_out.substr(read);
      }
    }
    if (str_ends_with(l, nkt_start)) in_nkt = true;
    else if (str_ends_with(l, nk_start)) in_nk = true;
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

  inter_t inter;
  inter_set_current(&inter);

  if (!z) inter_load_prelude(&inter);

  try {
    if (t) return test(&inter, t, v, f);
    else if (e) inter_line(&inter, e);
    else repl(&inter);
    return 0;
  } catch (std::exception& e) {
    cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
}