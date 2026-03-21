#define NOB_IMPLEMENTATION
#include "doc_gen.h"

#include "doc_gen_c_ref.h"
#include "doc_gen_lang_ref.h"

int main(void) {
    mkdir_if_not_exists("docs");

    doc_gen_c_ref("docs/c_ref");
    printf("Generated docs/c_ref/index.html and docs/c_ref/llm.md\n");

    doc_gen_lang_ref("docs/lang_ref");
    printf("Generated docs/lang_ref/index.html and docs/lang_ref/llm.md\n");

    String_Builder html = {0};
    da_reserve(&html, 4 * 1024);

    html_index_open(&html, "Documentation");

    sb_append_cstr(&html, "<p class=\"tagline\">A simple interpreted stack-based language</p>\n");

    sb_append_cstr(&html, "<div class=\"nav\">\n");
    html_index_card(&html, "c_ref/", "C", "C API Reference",
        "Documentation for the C API of the QLeei interpreter library.");
    html_index_card(&html, "lang_ref/", "QL", "Language Reference",
        "Documentation for the QLeei programming language syntax and intrinsics.");
    sb_append_cstr(&html, "</div>\n");

    html_index_close(&html);

    write_entire_file("docs/index.html", html.items, html.count);
    printf("Generated docs/index.html\n");

    sb_free(html);

    return 0;
}
