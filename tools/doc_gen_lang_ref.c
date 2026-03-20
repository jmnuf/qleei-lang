#include "doc_gen.h"

typedef struct {
    String_Pool_Index name;
    String_Pool_Index signature;
    String_Pool_Index description;
} Lang_Item;

typedef struct {
    Lang_Item *items;
    size_t count;
    size_t capacity;
} Lang_List;

typedef struct {
    const char *name;
    Lang_List items;
} Lang_Category;

static bool is_qleei_keyword(const char *word, size_t len) {
    static const char *keywords[] = { "while", "begin", "end", "proc" };
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        if (strncmp(word, keywords[i], len) == 0 && keywords[i][len] == '\0') return true;
    }
    return false;
}

static bool is_qleei_intrinsic(const char *word, size_t len) {
    static const char *intrinsics[] = {
        "print_number", "print_ptr", "print_char", "print_zstr", "print_stack",
        "dup", "drop", "rot2", "swap2", "rot3", "swap3", "over",
        "mem_alloc", "mem_free", "mem_load_ui8", "mem_save_ui8",
        "mem_load_ui32", "mem_save_ui32"
    };
    for (size_t i = 0; i < sizeof(intrinsics)/sizeof(intrinsics[0]); i++) {
        if (strncmp(word, intrinsics[i], len) == 0 && intrinsics[i][len] == '\0') return true;
    }
    return false;
}

static void html_qleei_highlight(String_Builder *sb, const char *code, size_t len) {
    size_t i = 0;
    while (i < len) {
        if (code[i] == '/' && i + 1 < len && code[i+1] == '/') {
            sb_append_cstr(sb, "<span class=\"syn-cm\">");
            while (i < len) { da_append(sb, code[i]); if (code[i] == '\n') break; i++; }
            sb_append_cstr(sb, "</span>");
            i++;
        } else if (code[i] == '-' && i + 1 < len && code[i+1] == '-') {
            sb_append_cstr(sb, "<span class=\"syn-cm\">");
            while (i < len) { da_append(sb, code[i]); if (code[i] == '\n') break; i++; }
            sb_append_cstr(sb, "</span>");
            i++;
        } else if (isalpha(code[i]) || code[i] == '_') {
            size_t start = i;
            while (i < len && (isalnum(code[i]) || code[i] == '_')) i++;
            size_t word_len = i - start;
            const char *word = code + start;
            if (is_qleei_keyword(word, word_len)) {
                sb_append_cstr(sb, "<span class=\"syn-kw\">");
                html_escape(sb, word, word_len);
                sb_append_cstr(sb, "</span>");
            } else if (is_qleei_intrinsic(word, word_len)) {
                sb_append_cstr(sb, "<span class=\"syn-fn\">");
                html_escape(sb, word, word_len);
                sb_append_cstr(sb, "</span>");
            } else {
                html_escape(sb, word, word_len);
            }
        } else if (isdigit(code[i])) {
            size_t start = i;
            while (i < len && (isalnum(code[i]) || code[i] == '.')) i++;
            sb_append_cstr(sb, "<span class=\"syn-nm\">");
            html_escape(sb, code + start, i - start);
            sb_append_cstr(sb, "</span>");
        } else {
            switch (code[i]) {
                case '&': case '|': case '!': case '=': case '+': case '*': case '/': case '%':
                case '<': case '>': case '^': case '~': case '?': case ':': case '[': case ']':
                    sb_append_cstr(sb, "<span class=\"syn-cp\">");
                    da_append(sb, code[i]);
                    sb_append_cstr(sb, "</span>");
                    break;
                default: da_append(sb, code[i]);
            }
            i++;
        }
    }
}

static void doc_gen_lang_ref_html(const char *output_path) {
    String_Builder html = {0};
    da_reserve(&html, 64 * 1024);

    html_doc_open(&html);

    sb_append_cstr(&html, "<h2>Types</h2>\n");
    sb_append_cstr(&html, "<details>\n<summary>Types</summary>\n<div class=\"group-items\">\n");
    sb_append_cstr(&html, "<a href=\"#number\">number</a>\n");
    sb_append_cstr(&html, "<a href=\"#bool\">bool</a>\n");
    sb_append_cstr(&html, "<a href=\"#pointer\">pointer</a>\n");
    sb_append_cstr(&html, "</div>\n</details>\n");

    sb_append_cstr(&html, "<h2>Intrinsics</h2>\n");
    sb_append_cstr(&html, "<details>\n<summary>Printing</summary>\n<div class=\"group-items\">\n");
    sb_append_cstr(&html, "<a href=\"#print_number\">print_number</a>\n");
    sb_append_cstr(&html, "<a href=\"#print_ptr\">print_ptr</a>\n");
    sb_append_cstr(&html, "<a href=\"#print_char\">print_char</a>\n");
    sb_append_cstr(&html, "<a href=\"#print_zstr\">print_zstr</a>\n");
    sb_append_cstr(&html, "<a href=\"#print_stack\">print_stack</a>\n");
    sb_append_cstr(&html, "</div>\n</details>\n");

    html_main_open(&html);

    html_section_open(&html, "types", "Types", "The building blocks of data in QLeei");

    sb_appendf(&html, "<div class=\"item\" id=\"number\">\n");
    sb_appendf(&html, "<h2>number</h2>\n");
    sb_appendf(&html, "<div class=\"description\">Represented as a 64 bit floating point number</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"bool\">\n");
    sb_appendf(&html, "<h2>bool</h2>\n");
    sb_appendf(&html, "<div class=\"description\">Whatever is the bool type in stdbool.h</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"pointer\">\n");
    sb_appendf(&html, "<h2>pointer</h2>\n");
    sb_appendf(&html, "<div class=\"description\">Some address to the heap</div>\n");
    sb_appendf(&html, "</div>\n\n");

    html_section_close(&html);

    html_section_open(&html, "intrinsics", "Intrinsics", "Built-in procedures for stack operations, memory, and I/O");

    sb_appendf(&html, "<div class=\"item\" id=\"print_number\">\n");
    sb_appendf(&html, "<h2>print_number</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>print_number :: [number]  -> []</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Consumes a number and prints it to stdout with a newline</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"print_ptr\">\n");
    sb_appendf(&html, "<h2>print_ptr</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>print_ptr :: [pointer]  -> []</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Consumes a pointer and prints it to stdout with a newline</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"print_char\">\n");
    sb_appendf(&html, "<h2>print_char</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>print_char :: [number]  -> []</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Consumes a number, casts it to a C char and prints it to stdout with a newline</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"print_zstr\">\n");
    sb_appendf(&html, "<h2>print_zstr</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>print_zstr :: [pointer] -> []</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Consumes a pointer expecting it to be a null terminated string and prints it to stdout with a newline</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"print_stack\">\n");
    sb_appendf(&html, "<h2>print_stack</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>print_stack :: [] -> []</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Prints the current stack to stdout read as top to bottom from left to right</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"dup\">\n");
    sb_appendf(&html, "<h2>dup</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>dup :: [a] -> [a, a]</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Duplicates the element at the top of the stack</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"drop\">\n");
    sb_appendf(&html, "<h2>drop</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>drop :: [a] -> []</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Drops the first element of the stack</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"rot2\">\n");
    sb_appendf(&html, "<h2>rot2</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>rot2 :: [a, b] -> [b, a]</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Swaps the top and second to top elements of the stack</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"rot3\">\n");
    sb_appendf(&html, "<h2>rot3</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>rot3 :: [a, b, c] -> [b, c, a]</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Rotates the top 3 elements of the stack</div>\n");
    sb_appendf(&html, "</div>\n\n");

    sb_appendf(&html, "<div class=\"item\" id=\"over\">\n");
    sb_appendf(&html, "<h2>over</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>over :: [a, b] -> [b, a, b]</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">Duplicates the 2nd element of the stack at the top of the stack</div>\n");
    sb_appendf(&html, "</div>\n\n");

    html_section_close(&html);

    html_section_open(&html, "loops", "Loops", "Control flow with while loops");

    sb_appendf(&html, "<div class=\"item\" id=\"while\">\n");
    sb_appendf(&html, "<h2>while</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>while <condition> begin <body> end</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">QLeei only supports while loops. The condition is evaluated before each iteration.</div>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>");
    html_qleei_highlight(&html, "while dup 0 < begin\n    // body\nend", 32);
    sb_appendf(&html, "</code></pre>\n");
    sb_appendf(&html, "</div>\n\n");

    html_section_close(&html);

    html_section_open(&html, "procedures", "User Procedures", "Defining your own procedures");

    sb_appendf(&html, "<div class=\"item\" id=\"proc\">\n");
    sb_appendf(&html, "<h2>proc</h2>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>proc &lt;name&gt; [&lt;inputs&gt;] -&gt; [&lt;outputs&gt;] &lt;body&gt; end</code></pre>\n");
    sb_appendf(&html, "<div class=\"description\">You can define your own procedures in QLeei by using the proc keyword. Input is type checked at RUNTIME. Output types are not type checked.</div>\n");
    sb_appendf(&html, "<pre class=\"signature\"><code>");
    html_qleei_highlight(&html, "proc ascii_E [] -> [number] 69 end", 36);
    sb_appendf(&html, "</code></pre>\n");
    sb_appendf(&html, "</div>\n\n");

    html_section_close(&html);

    sb_append_cstr(&html, "</main>\n</div>\n</body>\n</html>\n");

    write_entire_file(output_path, html.items, html.count);
    sb_free(html);
}

static void doc_gen_lang_ref_md(const char *output_path) {
    String_Builder md = {0};
    da_reserve(&md, 32 * 1024);

    sb_append_cstr(&md, "# QLeii Language Reference\n\n");
    sb_append_cstr(&md, "QLeei is a simple interpreted stack-based language.\n\n");

    sb_append_cstr(&md, "## Types\n\n");
    sb_append_cstr(&md, "### number\n\nRepresented as a 64 bit floating point number\n\n");
    sb_append_cstr(&md, "### bool\n\nWhatever is the bool type in stdbool.h\n\n");
    sb_append_cstr(&md, "### pointer\n\nSome address to the heap\n\n");

    sb_append_cstr(&md, "## Intrinsics\n\n");
    sb_append_cstr(&md, "### print_number\n\n```\nprint_number :: [number] -> []\n```\n\nConsumes a number and prints it to stdout with a newline\n\n");

    sb_append_cstr(&md, "### print_ptr\n\n```\nprint_ptr :: [pointer] -> []\n```\n\nConsumes a pointer and prints it to stdout with a newline\n\n");

    sb_append_cstr(&md, "### print_char\n\n```\nprint_char :: [number] -> []\n```\n\nConsumes a number, casts it to a C char and prints it to stdout with a newline\n\n");

    sb_append_cstr(&md, "### print_zstr\n\n```\nprint_zstr :: [pointer] -> []\n```\n\nConsumes a pointer expecting it to be a null terminated string and prints it to stdout with a newline\n\n");

    sb_append_cstr(&md, "### print_stack\n\n```\nprint_stack :: [] -> []\n```\n\nPrints the current stack to stdout read as top to bottom from left to right\n\n");

    sb_append_cstr(&md, "### dup\n\n```\ndup :: [a] -> [a, a]\n```\n\nDuplicates the element at the top of the stack\n\n");

    sb_append_cstr(&md, "### drop\n\n```\ndrop :: [a] -> []\n```\n\nDrops the first element of the stack\n\n");

    sb_append_cstr(&md, "### rot2\n\n```\nrot2 :: [a, b] -> [b, a]\n```\n\nSwaps the top and second to top elements of the stack\n\n");

    sb_append_cstr(&md, "### rot3\n\n```\nrot3 :: [a, b, c] -> [b, c, a]\n```\n\nRotates the top 3 elements of the stack\n\n");

    sb_append_cstr(&md, "### over\n\n```\nover :: [a, b] -> [b, a, b]\n```\n\nDuplicates the 2nd element of the stack at the top of the stack\n\n");

    sb_append_cstr(&md, "## Loops\n\n### while\n\n```\nwhile <condition> begin <body> end\n```\n\nQLeei only supports while loops. The condition is evaluated before each iteration.\n\n");

    sb_append_cstr(&md, "## User Procedures\n\n### proc\n\n```\nproc <name> [<inputs>] -> [<outputs>] <body> end\n```\n\nYou can define your own procedures in QLeei by using the proc keyword. Input is type checked at RUNTIME. Output types are not type checked.\n\n");

    sb_append_null(&md);
    write_entire_file(output_path, md.items, md.count - 1);
    sb_free(md);
}

void doc_gen_lang_ref(const char *output_dir) {
    mkdir_if_not_exists(output_dir);

    char html_path[512];
    char md_path[512];
    snprintf(html_path, sizeof(html_path), "%s/index.html", output_dir);
    snprintf(md_path, sizeof(md_path), "%s/llm.md", output_dir);

    doc_gen_lang_ref_html(html_path);
    doc_gen_lang_ref_md(md_path);
}
