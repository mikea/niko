#include <getopt.h>
#include <unistd.h>

#include "niko.h"

// repl

INLINE void stack_print_repl(stack_t* stack) {
  DO(i, stack->l) {
    if (i > 0) printf(" ");
    printf("%25pA", stack->bottom[i]);
  }
}

STATUS_T repl(inter_t* inter) {
  size_t input_size = 0;
  own(char) input = NULL;

  bool prompt = isatty(STDIN_FILENO);

  if (prompt) printf(VERSION_STRING "\n");

  while (true) {
    if (prompt) {
      if (inter->arr_level) {
        printf("%ld> ", inter->arr_level);
      } else {
        stack_print_repl(inter->stack);
        printf(" > ");
      }
    }

    if (getline(&input, &input_size, stdin) <= 0) STATUS_OK;
    status_t result = inter_line(inter, input);
    if (status_is_err(result)) {
      str_t msg = status_msg(result);
      fprintf(stderr, "ERROR: %pS\n", &msg);
      status_free(result);
    }
  }
}

// test runner

STATUS_T test(inter_t* inter, const char* fname, bool v) {
  own(FILE) file = fopen(fname, "r");
  STATUS_CHECK(file, "test: can't open file: %s", fname);

  size_t len = 0;
  own(char) line = NULL;

  size_t read;
  size_t line_no = 0;

  own(char) out = NULL;
  size_t out_size;
  char* rest_out = NULL;
  size_t in_line_no = 0;

  const str_t nkt_start = str_from_c("```nkt\n");
  const str_t nk_start = str_from_c("```nk\n");
  const str_t code_end = str_from_c("```");

  bool in_nk = false;
  bool in_nkt = false;

  while ((read = getline(&line, &len, file)) != -1) {
    line_no++;
    if (read == 0) continue;

    str_t l = str_from_c(line);

    if (in_nk) {
      if (str_starts_with(l, code_end)) {
        in_nk = false;
      } else {
        if (out) free(out);
        inter_line_capture_out(inter, line, &out, &out_size);
        if (out_size > 0) fprintf(stderr, "ERROR %s:%ld : unexpected output: '%s'\n", fname, line_no, out);
      }
    } else if (in_nkt) {
      if (str_starts_with(l, code_end)) {
        if (rest_out && *rest_out)
          fprintf(stderr, "ERROR %s:%ld : unmatched output: '%s'\n", fname, in_line_no, rest_out);
        rest_out = NULL;
        in_nkt = false;
      } else if (*line == '>') {
        if (v) fprintf(stderr, "%s", line);
        if (rest_out && *rest_out)
          fprintf(stderr, "ERROR %s:%ld : unmatched output: '%s'\n", fname, in_line_no, rest_out);
        if (out) free(out);
        in_line_no = line_no;
        inter_line_capture_out(inter, line + 1, &out, &out_size);
        stack_clear(inter->stack);
        rest_out = out;
        continue;
      } else if (memcmp(line, rest_out, read)) {
        fprintf(stderr, "ERROR %s:%ld : mismatched output, expected: '%s' actual: '%s' \n", fname, in_line_no, line,
                rest_out);
        rest_out = NULL;
      } else {
        rest_out += read;
      }
    } else {
      if (str_ends_with(l, nkt_start)) in_nkt = true;
      else if (str_ends_with(l, nk_start)) in_nk = true;
    }
  }

  STATUS_OK;
}

// main

int main(int argc, char* argv[]) {
  char *t = NULL, *e = NULL;
  bool v = false;
  bool z = false;

  int opt;
  while ((opt = getopt(argc, argv, "vzht:e:m:")) != -1) {
    switch (opt) {
      case 't': t = optarg; break;
      case 'v': v = true; break;
      case 'e': e = optarg; break;
      case 'z': z = true; break;
      case 'h':
      default:
        fprintf(stderr, "Usage: %s | %s -t test_file [-v] | %s -e stmt | %s -h \n", argv[0], argv[0], argv[0], argv[0]);
        exit(1);
    }
  }

  if (!z) inter_load_prelude();
  own(inter_t) inter = inter_new();

  status_t s;
  if (t) s = test(inter, t, v);
  else if (e) s = inter_line(inter, e);
  else s = repl(inter);

  if (status_is_ok(s)) return 0;
  status_print_error(s);
  return 1;
}