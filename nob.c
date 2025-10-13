#define NOB_FREE(ptr) do { if (ptr) free(ptr); ptr = NULL; } while (0)

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define BUILD_FOLDER "./build"

#define streq(a, b) (strcmp(a, b) == 0)

typedef struct { // Lifetime of the key property is expected to outlive the struct
  const char *key;
  bool ok;
} Map_Result_Item;

typedef struct {
  Map_Result_Item *items;
  size_t count;
  size_t capacity;
} Map;

Map_Result_Item *Map_find_key(Map *m, const char *key) {
  if (m->count == 0) return NULL;
  da_foreach(Map_Result_Item, pair, m) {
    if (streq(pair->key, key)) return pair;
  }
  return NULL;
}

// key should be garantueed to outlive the map
void Map_set_key(Map *m, const char *key, bool ok) {
  Map_Result_Item *existing = Map_find_key(m, key);
  if (existing) {
    existing->ok = ok;
    return;
  }
  Map_Result_Item pair = { .key = key, .ok = ok };
  da_append(m, pair);
}

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

#define within_temp \
for (ssize_t ___save_space = (ssize_t)nob_temp_save(); ___save_space != -1; (nob_temp_rewind((size_t)___save_space), ___save_space = -1))

void usage(const char *program) {
  printf("Usage: %s [run|build]\n", program);
  printf("    run [(input).ql]      ---        Execute interpreter after compiling with an input file\n");
  printf("    build                 ---        Force building of program\n");
  printf("    -etags                ---        Run etags on the C codebase\n");
}

bool build_unit(Cmd *cmd, Unit *u) {
  bool result = true;
  if (u->flags & UNIT_FLAG_FORCE_BUILD || needs_rebuild(u->output_path, u->items, u->count)) {
    cmd_append(cmd, "clang");
    if (u->target == UNIT_TARGET_BROWSER) {
      cmd_append(cmd, "--target=wasm32", "-nostdlib", "-Wl,--allow-undefined", "-Wl,--no-entry");
      cmd_append(cmd, "-Wl,--export=__heap_base", "-Wl,--export=__heap_end", "-Wl,--export=__indirect_function_table");
      for (size_t i = 0; i < u->wasm_exports.count; ++i) {
	      cmd_append(cmd, u->wasm_exports.items[i]);
      }
    } else {
      cmd_append(cmd, "-m32");
    }
    cmd_append(cmd, "-Wall", "-Wextra");

    if (u->flags & UNIT_FLAG_DEBUG_INFO) cmd_append(cmd, "-ggdb");

    nob_cc_output(cmd, u->output_path);
    size_t count = cmd->count;
    da_foreach(const char *, input_path, u) {
      String_View sv = sv_from_cstr(*input_path);
      if (sv_end_with(sv, ".h")) {
	      continue;
      }
      cmd_append(cmd, *input_path);
    }
    if (count == cmd->count) {
      nob_log(ERROR, "Unit %s cannot be built cause all its inputs are header files!", u->output_path);
      unit_clear(u);
      return false;
    }

    result = cmd_run(cmd);
  }

  if (result) nob_log(INFO, "Unit %s is up to date", u->output_path);
  else        nob_log(ERROR, "Unit %s failed to be updated", u->output_path);

  unit_clear(u);
  return result;
}


bool build_etags(Cmd *cmd) {
  File_Paths children = {0};
  if (!read_entire_dir(".", &children)) return false;

  const char *output_path = "TAGS";

  for (ssize_t i = children.count-1; i >= 0; --i) {
    const char *file_path = children.items[i];
    String_View sv = sv_from_cstr(file_path);
    if (sv_end_with(sv, ".c") || sv_end_with(sv, ".h")) continue;
    da_remove_unordered(&children, i);
  }

  bool result = true;
  if (needs_rebuild(output_path, children.items, children.count)) {
    cmd_append(cmd, "etags", "-o", output_path);

    for (size_t i = 0; i < children.count; ++i) {
      cmd_append(cmd, children.items[i]);
    }

    result = cmd_run(cmd);
  }
  if (result) nob_log(INFO, "Tags are up to date");
  else        nob_log(ERROR, "Tags failed to be updated");

  if (children.items) free(children.items);
  return result;
}


int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  const char *program_name = shift(argv, argc);
  Cmd cmd = {0};

  bool run_requested = false;
  bool build_demanded = false;
  bool uses_etags = false;
  bool run_all = false;
  const char *run_input_file = NULL;

  while (argc > 0) {
    const char *arg = shift(argv, argc);
    if (streq(arg, "run")) {
      run_requested = true;
      if (argc > 0) {
        String_View sv = sv_from_cstr(*argv);
        if (sv_end_with(sv, ".ql")) {
          run_input_file = shift(argv, argc);
        } else if (sv_eq(sv, sv_from_cstr("all"))) {
          shift(argv, argc);
          run_all = true;
        }
      }
      continue;
    }

    if (streq(arg, "build")) {
      build_demanded = true;
      continue;
    }

    if (streq(arg, "-etags")) {
      uses_etags = true;
      continue;
    }
    
    nob_log(ERROR, "Unknown argument provided to build system: %s", arg);
    usage(program_name);
    return 1;
  }

  Unit unit = {0};
  if (!mkdir_if_not_exists(BUILD_FOLDER)) return 1;

  const char *native_output = BUILD_FOLDER"/qleei";

  build_etags(&cmd);

  within_temp {
    unit_target_desktop(&unit);
    unit_output(&unit, native_output);
    unit_input(&unit, "./qleei.m32.c");
    unit_input(&unit, "./qleei.h");
    if (build_demanded) unit_force_build(&unit);
    unit_debug_info(&unit);

    if (!build_unit(&cmd, &unit)) return 1;
  }

  within_temp {
    unit_target_browser(&unit);
    const char *output_path = BUILD_FOLDER"/qleei.wasm";
    unit_output(&unit, output_path);
    unit_input(&unit, "./qleei.wasm.c");
    unit_input(&unit, "./qleei.h");

    unit_wasm_export(&unit, "qleei_get_token_kind_name");
    // Export interpreter functions
    unit_wasm_export(&unit, "qleei_interpret_buffer");
    unit_wasm_export(&unit, "qleei_alloc_new_interpreter");
    unit_wasm_export(&unit, "qleei_interpreter_step");
    unit_wasm_export(&unit, "qleei_interpreter_lexer_init");
    if (build_demanded) unit_force_build(&unit);
    if (!build_unit(&cmd, &unit)) return 1;

    if (needs_rebuild1("./playground/qleei.wasm", output_path)) {
      String_Builder sb = {0};
      if (read_entire_file(output_path, &sb)) {
	      if (write_entire_file("./playground/qleei.wasm", sb.items, sb.count)) {
	        nob_log(INFO, "Updated ./playground/qleei.wasm");
	      }
      }
      sb_free(sb);
    }
  }

  if (run_requested) {
    if (run_all) {
      Map m = {0};
      File_Paths paths = {0};
      const char *folder_path = "./examples";
      if (!read_entire_dir(folder_path, &paths)) return 1;

      da_foreach(const char *, it, &paths) within_temp {
        const char *file_path = *it;
        if (streq(file_path, ".") || streq(file_path, "..")) continue;
        if (!sv_end_with(sv_from_cstr(file_path), ".ql")) continue;
        cmd_append(&cmd, native_output);
        cmd_append(&cmd, nob_temp_sprintf("%s/%s", folder_path, file_path));
        bool ok = cmd_run(&cmd);
        Map_set_key(&m, file_path, ok);
      }

      printf("\n==================================================\n");
      printf("| Results:\n");
      bool ok = true;
      da_foreach(Map_Result_Item, pair, &m) {
        printf("|    ");
        if (pair->ok) {
          printf("✅");
        } else {
          ok = false;
          printf("❎");
        }
        printf(" - %s\n", pair->key);
      }
      printf("==================================================\n");
      if (m.items) da_free(&m);
      if (!ok) return 1;
    } else {
      cmd_append(&cmd, native_output);
      if (run_input_file) cmd_append(&cmd, run_input_file);
      if (!cmd_run(&cmd)) return 1;
    }
  }

  return 0;
}
