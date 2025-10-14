#define NOB_IMPLEMENTATION
#include "nob.h"

#define QLEEI_IMPLEMENTATION
#define PLATFORM_DESKTOP
#include "qleei.h"

/**
 * Print the program usage message to the specified stream.
 *
 * Writes "Usage: <program> <input-file>\n" to the provided FILE stream,
 * substituting the given program name.
 *
 * @param f Output stream to receive the usage message.
 * @param program Program name to display in the usage message.
 */
void usage(FILE *f, const char *program) {
  fprintf(f, "Usage: %s <input-file>\n", program);
}

bool word_handler_sub(QLeei_Token token, Qleei_Stack *stack, Qleei_Procs *procs, bool inside_proc) {
  (void)procs; (void)inside_proc;

  if (!qleei_stack_operation_requires_n_items(token.loc, stack, token.string, 2)) return false;

  Qleei_Value_Item item_a, item_b;
  qleei_stack_pop(stack, &item_a);
  qleei_stack_pop(stack, &item_b);
  if (!qleei_action_expects_value_kind(token.loc, token.string, item_a.kind, QLEEI_VALUE_KIND_NUMBER)) return false;
  if (!qleei_action_expects_value_kind(token.loc, token.string, item_b.kind, QLEEI_VALUE_KIND_NUMBER)) return false;
  item_a.as_number.value = item_b.as_number.value - item_a.as_number.value;
  return qleei_stack_push(stack, item_a);
}

char *at_hello_world_ptr;
bool word_handler_at_hello_world(QLeei_Token token, Qleei_Stack *stack, Qleei_Procs *procs, bool inside_proc) {
  (void)token; (void)procs; (void)inside_proc;
  Qleei_Value_Item item = { .as_pointer = { .kind = QLEEI_VALUE_KIND_POINTER, .value = at_hello_world_ptr } };
  return qleei_stack_push(stack, item);
}

/**
 * Program entry point that reads a source file and invokes the QLEEI interpreter on its contents.
 *
 * Handles "-h" and "--help" by printing usage and exiting successfully. If no input path is provided,
 * prints usage to stderr and exits with an error.
 *
 * @returns 0 on successful interpretation; 1 on error (missing input, file read failure, or interpreter failure).
 */
int main(int argc, char **argv) {
  const char *program = nob_shift(argv, argc);

  if (argc == 0) {
    nob_log(NOB_ERROR, "No input was provided");
    usage(stderr, program);
    return 1;
  }
  const char *input_path = nob_shift(argv, argc);
  if (strcmp(input_path, "-h") == 0 || strcmp(input_path, "--help") == 0) {
    usage(stdout, program);
    return 0;
  }

  Nob_String_Builder sb = {0};

  if (!nob_read_entire_file(input_path, &sb)) return 1;

  if (sb.count > 0 && sb.items[sb.count - 1] == 0) sb.count--;

  at_hello_world_ptr = qleei_zstr_dup("Hello, World!");

  Qleei_Interpreter it = {0};
  qleei_interpreter_lexer_init(&it, input_path, sb.items, sb.count);
  qleei_interpreter_register_word(&it, "sub", word_handler_sub);
  qleei_interpreter_register_word(&it, "@hello_world", word_handler_at_hello_world);
  if (!qleei_interpreter_exec(&it)) return 1;

  return 0;
}
