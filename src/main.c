#include <getopt.h>
#include <unistd.h>

#include "inter.h"
#include "niko.h"

// repl

INLINE void stack_print_repl(stack_t* stack) {
  DO(i, stack->l) {
    if (i > 0) printf(" ");
    printf("%25pA", stack->data[i]);
  }
}

void v() {
  
}

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
    CATCH(e) {
      fprintf(stderr, "ERROR: %pS\n", &e);
      goto next;
    }
    inter_line(inter, input);
  }
}

// test runner

void test(inter_t* inter, const char* fname, bool v) {
  own(FILE) file = fopen(fname, "r");
  CHECK(file, "test: can't open file: %s", fname);

  size_t len     = 0;
  own(char) line = NULL;

  size_t read;
  size_t line_no = 0;

  own(char) out = NULL;
  size_t out_size;
  char*  rest_out   = NULL;
  size_t in_line_no = 0;

  const str_t nkt_start = str_from_c("```nkt\n");
  const str_t nk_start  = str_from_c("```nk\n");
  const str_t code_end  = str_from_c("```");

  bool in_nk  = false;
  bool in_nkt = false;

  while ((read = getline(&line, &len, file)) != -1) {
    line_no++;
    if (read == 0) continue;

    str_t l = str_from_c(line);

    if (in_nk) {
      if (str_starts_with(l, code_end)) {
        inter_reset(inter);
        in_nk = false;
      } else {
        if (out) free(out);
        inter_line_capture_out(inter, line, &out, &out_size);
        if (out_size > 0) fprintf(stderr, "ERROR %s:%ld : unexpected output: '%s'\n", fname, line_no, out);
      }
    } else if (in_nkt) {
      if (str_starts_with(l, code_end)) {
        inter_reset(inter);
        if (rest_out && *rest_out)
          fprintf(stderr, "ERROR %s:%ld : unmatched output: '%s'\n", fname, in_line_no, rest_out);
        rest_out = NULL;
        in_nkt   = false;
      } else if (*line == '>') {
        if (v) fprintf(stderr, "%s", line);
        if (rest_out && *rest_out)
          fprintf(stderr, "ERROR %s:%ld : unmatched output: '%s'\n", fname, in_line_no, rest_out);
        in_line_no = line_no;
        if (out) free(out);
        inter_line_capture_out(inter, line + 1, &out, &out_size);
        rest_out = out;
        continue;
      } else if (memcmp(line, rest_out, read)) {
        fprintf(stderr, "ERROR %s:%ld : mismatched output, expected: '%s' actual: '%s' \n", fname, in_line_no, line,
                rest_out);
        rest_out = NULL;
      } else {
        rest_out += read;
      }
    }
    if (str_ends_with(l, nkt_start)) in_nkt = true;
    else if (str_ends_with(l, nk_start)) in_nk = true;
  }
}

void h(FILE* f, char* argv_0) {
  fprintf(f, VERSION_STRING "\n");
  fprintf(f, "\nUSAGE:\n");
  fprintf(f, "    %s [FLAGS] [OPTIONS]\n", argv_0);
  fprintf(f, "\nFLAGS:\n");
  fprintf(f, "    -z               Do not load the prelude\n");
  fprintf(f, "    -v               Verbose test execution\n");
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

  int opt;
  while ((opt = getopt(argc, argv, "vzht:e:m:")) != -1) {
    switch (opt) {
      case 't': t = optarg; break;
      case 'v': v = true; break;
      case 'e': e = optarg; break;
      case 'z': z = true; break;
      case 'h': {
        h(stdout, argv[0]);
        exit(0);
      }
      default: {
        h(stderr, argv[0]);
        exit(1);
      }
    }
  }

  own(inter_t) inter = inter_new();
  if (!z) inter_load_prelude(inter);

  CATCH(e) {
    fprintf(stderr, "ERROR: %pS", &e);
    return 1;
  }

  if (t) test(inter, t, v);
  else if (e) inter_line(inter, e);
  else repl(inter);

  return 0;
}