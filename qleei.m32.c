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

  if (!qleei_interpret_buffer(input_path, sb.items, sb.count)) return 1;

  return 0;
}
