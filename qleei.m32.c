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

bool word_handler_sub(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 2)) return false;

  Qleei_Value_Item item_a, item_b;
  qleei_stack_pop(opt.stack, &item_a);
  qleei_stack_pop(opt.stack, &item_b);
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, item_a.kind, QLEEI_VALUE_KIND_NUMBER)) return false;
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, item_b.kind, QLEEI_VALUE_KIND_NUMBER)) return false;
  item_a.as_number.value = item_b.as_number.value - item_a.as_number.value;
  return qleei_stack_push(opt.stack, item_a);
}

bool word_handler_at_hello_world(Qleei_Word_Handler_Opt opt) {
  Qleei_Value_Item item = { .as_pointer = { .kind = QLEEI_VALUE_KIND_POINTER, .value = (char*)opt.user_data } };
  return qleei_stack_push(opt.stack, item);
}

bool word_handler_at_zstr_pound_ascci_upper(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item_ptr;
  qleei_stack_pop(opt.stack, &item_ptr);
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, item_ptr.kind, QLEEI_VALUE_KIND_POINTER)) return false;
  for (char *ptr = item_ptr.as_pointer.value; *ptr != 0; ++ptr) {
    char c = *ptr;
    if ('a' <= c && c <= 'z') *ptr = c - 32;
  }
  return true;
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

  Qleei_Interpreter it = {0};
  qleei_interpreter_lexer_init(&it, input_path, sb.items, sb.count);
  qleei_interpreter_register_word(&it, "sub", word_handler_sub);
  qleei_interpreter_register_word_with_data(&it, "@hello_world", word_handler_at_hello_world, qleei_zstr_dup("Hello, World!"));
  qleei_interpreter_register_word(&it, "@zstr#ascii_upper", word_handler_at_zstr_pound_ascci_upper);
  if (!qleei_interpreter_exec(&it)) return 1;

  return 0;
}
