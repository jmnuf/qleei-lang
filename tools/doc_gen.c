#define NOB_IMPLEMENTATION
#include "doc_gen.h"

#include "doc_gen_c_ref.h"
#include "doc_gen_lang_ref.h"

int main(void) {
  int result = 0;
  String_Builder sb = {0};
  da_reserve(&string_pool, 1024);
  da_reserve(&sb, 6 * 1024);

  if (!mkdir_if_not_exists("docs")) return_defer(1);


  string_pool.count = 0;
  if (!doc_gen_c_ref(&sb, "docs/c_ref")) {
    fprintf(stderr, "Failed while generating docs/c_ref/index.html and docs/c_ref/llm.md\n");
    return_defer(1);
  }
  printf("Generated docs/c_ref/index.html and docs/c_ref/llm.md\n");

  string_pool.count = 0;
  if (!doc_gen_lang_ref(&sb, "docs/lang_ref")) {
    fprintf(stderr, "Failed while generating docs/lang_ref/index.html and docs/lang_ref/llm.md\n");
    return_defer(1);
  }
  printf("Generated docs/lang_ref/index.html and docs/lang_ref/llm.md\n");

  string_pool.count = 0;
  sb.count = 0;

  html_index_open(&sb, "Documentation");

  sb_append_cstr(&sb, "<p class=\"tagline\">A simple interpreted stack-based language</p>\n");

  sb_append_cstr(&sb, "<div class=\"nav\">\n");
  html_index_card(&sb, "c_ref/", "C", "C API Reference",
  "Documentation for the C API of the QLeei interpreter library.");
  html_index_card(&sb, "lang_ref/", "QL", "Language Reference",
  "Documentation for the QLeei programming language syntax and intrinsics.");
  sb_append_cstr(&sb, "</div>\n");

  html_index_close(&sb);

  write_entire_file("docs/index.html", sb.items, sb.count);
  printf("Generated docs/index.html\n");


defer:
  if (sb.items) sb_free(sb);
  if (string_pool.items) sb_free(string_pool);

  return result;
}
