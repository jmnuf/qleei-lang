#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define BUILD_FOLDER "./build"

#define streq(a, b) (strcmp(a, b) == 0)

typedef enum {
  UNIT_FLAG_FORCE_BUILD = 1 << 0,
  UNIT_FLAG_DEBUG_INFO  = 1 << 1,
} UNIT_FLAGS;

typedef enum {
  UNIT_TARGET_DESKTOP,
  UNIT_TARGET_BROWSER,
} Unit_Target;

typedef struct {
  const char *output_path;
  const char **items;
  size_t count;
  size_t capacity;
  Unit_Target target;
  struct {
    const char **items;
    size_t count;
    size_t capacity;
  } wasm_exports;
  uint8_t flags;
} Unit;

#define unit_clear(u) do { (u)->count = 0; (u)->output_path = NULL; (u)->flags = 0; } while (0)
#define unit_input(u, input) da_append(u, input)
#define unit_output(u, output)  (u)->output_path = (output);
#define unit_outputf(u, ...)    (u)->output_path = temp_sprintf(__VA_ARGS__)
#define unit_force_build(u)     (u)->flags |= UNIT_FLAG_FORCE_BUILD;
#define unit_debug_info(u)      (u)->flags |= UNIT_FLAG_DEBUG_INFO;

#define unit_wasm_export(u, fn_name) da_append(&(u)->wasm_exports, temp_sprintf("-Wl,--export=%s", fn_name))

#define unit_target_browser(u)       (u)->target = UNIT_TARGET_BROWSER;
#define unit_target_desktop(u)       (u)->target = UNIT_TARGET_DESKTOP;

#define within_temp for (ssize_t ___save_space = (ssize_t)nob_temp_save(); ___save_space != -1; (nob_temp_rewind((size_t)___save_space), ___save_space = -1))

void usage(const char *program) {
  printf("Usage: %s [run|build]\n", program);
  printf("    run        ---        Execute program after compiling\n", program);
  printf("    build      ---        Force building of program\n", program);
}

bool build_unit(Cmd *cmd, Unit *u) {
  bool result = true;
  if (u->flags & UNIT_FLAG_FORCE_BUILD || needs_rebuild(u->output_path, u->items, u->count)) {
    cmd_append(cmd, "clang");
    if (u->target == UNIT_TARGET_BROWSER) {
      cmd_append(cmd, "-DPLATFORM_BROWSER");
      cmd_append(cmd, "--target=wasm32", "-nostdlib", "-Wl,--allow-undefined", "-Wl,--no-entry");
      cmd_append(cmd, "-Wl,--export=__heap_base", "-Wl,--export=__heap_end", "-Wl,--export=__indirect_function_table");
      for (size_t i = 0; i < u->wasm_exports.count; ++i) {
	cmd_append(cmd, u->wasm_exports.items[i]);
      }
    } else {
      cmd_append(cmd, "-DPLATFORM_DESKTOP");
      cmd_append(cmd, "-m32");
    }
    cmd_append(cmd, "-Wall", "-Wextra");

    if (u->flags & UNIT_FLAG_DEBUG_INFO) cmd_append(cmd, "-ggdb");

    nob_cc_output(cmd, u->output_path);
    da_foreach(const char *, input_path, u) {
      String_View sv = sv_from_cstr(*input_path);
      if (sv_end_with(sv, ".h")) {
	continue;
      }
      cmd_append(cmd, *input_path);
    }

    result = cmd_run(cmd);
  }

  nob_log(INFO, "Unit %s is updated", u->output_path);

  unit_clear(u);
  return result;
}


int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  const char *program_name = shift(argv, argc);
  bool run_requested = false;
  bool build_demanded = false;
  while (argc > 0) {
    const char *arg = shift(argv, argc);
    if (streq(arg, "run")) {
      run_requested = true;
      continue;
    }
    if (streq(arg, "build")) {
      build_demanded = true;
      continue;
    }
    
    nob_log(ERROR, "Unknown argument provided to build system: %s", arg);
    usage(program_name);
    return 1;
  }

  Cmd cmd = {0};
  Unit unit = {0};
  if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;

  const char *native_output = BUILD_FOLDER"/qleei";

  within_temp {
    unit_target_desktop(&unit);
    unit_output(&unit, native_output);
    unit_input(&unit, "./main.c");
    unit_input(&unit, "./platform.h");
    if (build_demanded) unit_force_build(&unit);
    unit_debug_info(&unit);

    if (!build_unit(&cmd, &unit)) return 1;
  }

  within_temp {
    unit_target_browser(&unit);
    const char *output_path = BUILD_FOLDER"/qleei.wasm";
    unit_output(&unit, output_path);
    unit_input(&unit, "./main.c");
    unit_input(&unit, "./platform.h");

    unit_wasm_export(&unit, "get_token_kind_name");
    // Export interpreter functions
    unit_wasm_export(&unit, "interpret_buffer");
    unit_wasm_export(&unit, "alloc_new_interpreter");
    unit_wasm_export(&unit, "interpreter_step");
    unit_wasm_export(&unit, "interpreter_lexer_init");
    if (build_demanded) unit_force_build(&unit);
    if (!build_unit(&cmd, &unit)) return 1;

    if (needs_rebuild1("./web/qleei.wasm", output_path)) {
      String_Builder sb = {0};
      if (read_entire_file(output_path, &sb)) {
	if (write_entire_file("./web/qleei.wasm", sb.items, sb.count)) {
	  nob_log(INFO, "Updated ./web/qleei.wasm");
	}
      }
      sb_free(sb);
    }
  }

  if (run_requested) {
    cmd_append(&cmd, native_output);
    cmd_append(&cmd, "./examples/math.qll");
    if (!cmd_run(&cmd)) return 1;
    cmd_append(&cmd, native_output);
    cmd_append(&cmd, "./examples/hello_world.qll");
    if (!cmd_run(&cmd)) return 1;
  }

  return 0;
}
