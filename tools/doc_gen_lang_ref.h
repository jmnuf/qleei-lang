#include "doc_gen.h"

static const char *types[] = {"number", "bool", "pointer"};
static const size_t types_count = sizeof(types) / sizeof(types[0]);

static const char *printing_intrinsics[] = {"print_number", "print_ptr", "print_char", "print_zstr", "print_stack"};
static const char *stack_intrinsics[] = {"dup", "drop", "rot2", "swap2", "rot3", "swap3", "over"};
static const char *memory_intrinsics[] = {"mem_alloc", "mem_free", "mem_load_ui8", "mem_save_ui8", "mem_load_ui32", "mem_save_ui32"};

static bool is_qleei_keyword(const char *word, size_t len) {
    static const char *keywords[] = { "while", "begin", "end", "proc" };
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        if (strncmp(word, keywords[i], len) == 0 && keywords[i][len] == '\0') return true;
    }
    return false;
}

static bool is_qleei_intrinsic(const char *word, size_t len) {
    for (size_t i = 0; i < sizeof(printing_intrinsics)/sizeof(printing_intrinsics[0]); i++) {
        if (strncmp(word, printing_intrinsics[i], len) == 0 && strlen(printing_intrinsics[i]) == len) return true;
    }
    for (size_t i = 0; i < sizeof(stack_intrinsics)/sizeof(stack_intrinsics[0]); i++) {
        if (strncmp(word, stack_intrinsics[i], len) == 0 && strlen(stack_intrinsics[i]) == len) return true;
    }
    for (size_t i = 0; i < sizeof(memory_intrinsics)/sizeof(memory_intrinsics[0]); i++) {
        if (strncmp(word, memory_intrinsics[i], len) == 0 && strlen(memory_intrinsics[i]) == len) return true;
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

static const char *find_header(const char *data, size_t len, const char *header) {
    size_t header_len = strlen(header);
    for (size_t i = 0; i + header_len <= len; i++) {
        if (strncmp(data + i, header, header_len) == 0) {
            return data + i;
        }
    }
    return NULL;
}

static void unescape_underscores(char *dest, const char *src, size_t src_len) {
    size_t d = 0;
    for (size_t s = 0; s < src_len; s++) {
        if (src[s] == '\\' && s + 1 < src_len && src[s+1] == '_') {
            dest[d++] = '_';
            s++;
        } else {
            dest[d++] = src[s];
        }
    }
    dest[d] = '\0';
}

static int item_matches_name(const char *line, size_t line_len, const char *name) {
    size_t name_len = strlen(name);
    if (line_len < 4 + name_len) return 0;
    if (strncmp(line, "- ", 2) != 0) return 0;
    if (strncmp(line + 2, name, name_len) != 0) return 0;
    char next = line[2 + name_len];
    return (next == ':' || next == ' ' || next == '\0');
}

static String_Pool_Index extract_string(const char *start, size_t len) {
    while (len > 0 && (start[0] == ' ' || start[0] == '\t' || start[0] == '\n' || start[0] == '\r')) { start++; len--; }
    while (len > 0 && (start[len-1] == ' ' || start[len-1] == '\t' || start[len-1] == '\n' || start[len-1] == '\r')) len--;
    if (len == 0) return pool_strdup("");
    char *buf = temp_alloc(len + 1);
    memcpy(buf, start, len);
    buf[len] = '\0';
    char *trimmed = buf;
    while (*trimmed == ' ') trimmed++;
    return pool_strdup(trimmed);
}

static String_Pool_Index extract_paragraph_html(Nob_String_View sv) {
    String_Builder sb = {0};
    bool seen_content = false;

    while (sv.count > 0) {
      Nob_String_View line = sv_chop_by_delim(&sv, '\n');

      Nob_String_View trimmed = sv_trim(line);
      if (trimmed.count == 0) {
        if (!seen_content) {
          sb_append_cstr(&sb, "<br>");
          seen_content = true;
        } else {
          seen_content = false;
        }
        continue;
      }
      seen_content = true;

      if (sv_starts_with(trimmed, sv_from_cstr("```"))) {
        break;
      }

      if (seen_content) {
        da_append(&sb, ' ');
      }

      for (size_t i = 0; i < trimmed.count; i++) {
        da_append(&sb, trimmed.data[i]);
      }
      seen_content = true;
    }

    da_append(&sb, '\0');
    String_Pool_Index result = pool_strdup(sb.items);
    sb_free(sb);
    return result;
}

static String_Pool_Index extract_paragraph_md(Nob_String_View sv) {
    String_Builder sb = {0};
    bool seen_content = false;
    bool pending_newline = false;

    while (sv.count > 0) {
        Nob_String_View line = sv_chop_by_delim(&sv, '\n');

        Nob_String_View trimmed = sv_trim(line);
        if (trimmed.count == 0) {
            if (seen_content) {
                pending_newline = true;
            }
            continue;
        }

        if (trimmed.count >= 3 && strncmp(trimmed.data, "```", 3) == 0) {
            break;
        }

        if (pending_newline) {
            da_append(&sb, '\n');
            pending_newline = false;
        } else if (seen_content) {
            da_append(&sb, ' ');
        }

        for (size_t i = 0; i < trimmed.count; i++) {
            da_append(&sb, trimmed.data[i]);
        }
        seen_content = true;
    }

    da_append(&sb, '\0');
    String_Pool_Index result = pool_strdup(sb.items);
    sb_free(sb);
    return result;
}

static String_Pool_Index extract_type_description(const char *section_start, const char *section_end, const char *type_name) {
    const char *line = section_start;
    while (line < section_end) {
        const char *line_end = line;
        while (line_end < section_end && *line_end != '\n') line_end++;

        char buf[256];
        unescape_underscores(buf, line, line_end - line);

        if (item_matches_name(buf, strlen(buf), type_name)) {
            const char *desc_start = buf + 2 + strlen(type_name);
            while (*desc_start == ' ' || *desc_start == ':') desc_start++;
            return pool_strdup(desc_start);
        }

        line = line_end + 1;
        while (line < section_end && *line == '\n') line++;
    }
    return pool_strdup("");
}

static String_Pool_Index extract_intrinsic_signature(const char *section_start, const char *section_end, const char *intrinsic_name) {
    const char *line = section_start;
    while (line < section_end) {
        const char *line_end = line;
        while (line_end < section_end && *line_end != '\n') line_end++;

        char buf[512];
        unescape_underscores(buf, line, line_end - line);
        size_t buf_len = strlen(buf);

        if (item_matches_name(buf, buf_len, intrinsic_name) && buf_len > strlen(intrinsic_name) + 4 && strstr(buf, "::") != NULL) {
            char *sig_start = strstr(buf, "::");
            if (sig_start) {
                sig_start += 2;
                while (*sig_start == ' ') sig_start++;
                while (sig_start > buf && sig_start[-1] == ' ') sig_start--;

                size_t name_len = strlen(intrinsic_name);
                char *full_sig = temp_alloc(name_len + 4 + strlen(sig_start) + 1);
                snprintf(full_sig, name_len + 4 + strlen(sig_start) + 1, "%s :: %s", intrinsic_name, sig_start);
                return pool_strdup(full_sig);
            }
        }

        line = line_end + 1;
        while (line < section_end && *line == '\n') line++;
    }
    return pool_strdup("");
}

static String_Pool_Index extract_intrinsic_description(const char *section_start, const char *section_end, const char *intrinsic_name) {
    const char *line = section_start;
    while (line < section_end) {
        const char *line_end = line;
        while (line_end < section_end && *line_end != '\n') line_end++;

        char buf[512];
        unescape_underscores(buf, line, line_end - line);
        size_t buf_len = strlen(buf);

        if (item_matches_name(buf, buf_len, intrinsic_name) && buf_len > strlen(intrinsic_name) + 4 && strstr(buf, "::") != NULL) {
            const char *next_line = line_end + 1;
            while (next_line < section_end && (*next_line == ' ' || *next_line == '\t')) next_line++;

            if (next_line < section_end && next_line[0] == '-' && next_line[1] == ' ') {
                next_line += 2;
                while (next_line < section_end && (*next_line == ' ' || *next_line == '\t')) next_line++;

                const char *desc_end = next_line;
                while (desc_end < section_end && *desc_end != '\n') desc_end++;

                while (desc_end > next_line && (desc_end[-1] == ' ' || desc_end[-1] == '\t')) desc_end--;

                return extract_string(next_line, desc_end - next_line);
            }
        }

        line = line_end + 1;
        while (line < section_end && *line == '\n') line++;
    }
    return pool_strdup("");
}

static const char *extract_code_block(const char *section_start, const char *section_end, int index) {
    int count = 0;
    String_View sv = sv_from_parts(section_start, (size_t)(section_end - section_start));
    String_View code_block_prefix = sv_from_cstr("```qleei");
    while (sv.count > 0) {
      if (sv_starts_with(sv, code_block_prefix)) {
        sv_chop_left(&sv, code_block_prefix.count);
        if (count == index) break;
        count++;
        continue;
      }
      sv_chop_left(&sv, 1);
    }
    if (sv.count == 0) return NULL;
    sv = sv_trim_left(sv);

    String_View it = sv;
    while (it.count > 0 && !sv_starts_with(it, sv_from_cstr("```"))) {
      it.data  += 1;
      it.count -= 1;
    }
    if (it.count > 0) sv.count = (size_t)(it.data - sv.data);

    sv = sv_trim_right(sv);

    return temp_sv_to_cstr(sv);
}

static bool doc_gen_lang_ref_html(String_Builder *html, const char *output_path) {
  bool result = true;
  html->count = 0;

    String_Builder content = {0};
    if (!read_entire_file("README.md", &content)) {
        fprintf(stderr, "Failed to read README.md\n");
        return_defer(false);
    }
    const char *data = content.items;
    size_t len = content.count;

    const char *types_start = find_header(data, len, "### Types");
    const char *intrinsics_start = find_header(data, len, "### Intrinsics");
    const char *loops_start = find_header(data, len, "## Loops");
    const char *procs_start = find_header(data, len, "## User Procedures");

    const char *printing_start = find_header(data, len, "Printing:");
    const char *stack_start = find_header(data, len, "General Stack Operations:");
    const char *memory_start = find_header(data, len, "Memory Management:");

    html_doc_open(html);

    sb_append_cstr(html, "<h2>Types</h2>\n");
    for (size_t i = 0; i < types_count; i++) {
        sb_appendf(html, "<a href=\"#%s\">%s</a>\n", types[i], types[i]);
    }

    sb_append_cstr(html, "<h2>Intrinsics</h2>\n");
    sb_appendf(html, "<details>\n<summary>Printing (%zu)</summary>\n<div class=\"group-items\">\n", sizeof(printing_intrinsics)/sizeof(printing_intrinsics[0]));
    for (size_t i = 0; i < sizeof(printing_intrinsics)/sizeof(printing_intrinsics[0]); i++) {
        sb_appendf(html, "<a href=\"#%s\">%s</a>\n", printing_intrinsics[i], printing_intrinsics[i]);
    }
    sb_append_cstr(html, "</div>\n</details>\n");

    sb_appendf(html, "<details>\n<summary>Stack Operations (%zu)</summary>\n<div class=\"group-items\">\n", sizeof(stack_intrinsics)/sizeof(stack_intrinsics[0]));
    for (size_t i = 0; i < sizeof(stack_intrinsics)/sizeof(stack_intrinsics[0]); i++) {
        sb_appendf(html, "<a href=\"#%s\">%s</a>\n", stack_intrinsics[i], stack_intrinsics[i]);
    }
    sb_append_cstr(html, "</div>\n</details>\n");

    sb_appendf(html, "<details>\n<summary>Memory Management (%zu)</summary>\n<div class=\"group-items\">\n", sizeof(memory_intrinsics)/sizeof(memory_intrinsics[0]));
    for (size_t i = 0; i < sizeof(memory_intrinsics)/sizeof(memory_intrinsics[0]); i++) {
        sb_appendf(html, "<a href=\"#%s\">%s</a>\n", memory_intrinsics[i], memory_intrinsics[i]);
    }
    sb_append_cstr(html, "</div>\n</details>\n");

    sb_append_cstr(html, "<h2>Loops</h2>\n");
    sb_append_cstr(html, "<a href=\"#while\">while</a>\n");

    sb_append_cstr(html, "<h2>User Procedures</h2>\n");
    sb_append_cstr(html, "<a href=\"#proc\">proc</a>\n");

    html_main_open(html);

    html_section_open(html, "types", "Types", "The building blocks of data in QLeei");
    for (size_t i = 0; i < types_count; i++) {
        String_Pool_Index desc = extract_type_description(types_start, intrinsics_start, types[i]);
        sb_appendf(html, "<div class=\"item\" id=\"%s\">\n", types[i]);
        sb_appendf(html, "<h2>%s</h2>\n", types[i]);
        sb_appendf(html, "<div class=\"description\">%s</div>\n", Pooled_String(desc));
        sb_appendf(html, "</div>\n\n");
    }
    html_section_close(html);

    html_section_open(html, "intrinsics", "Intrinsics", "Built-in procedures for stack operations, memory, and I/O");

    if (printing_start && printing_start < (stack_start ? stack_start : loops_start)) {
        const char *printing_end = stack_start ? stack_start : (loops_start ? loops_start : intrinsics_start + 100);
        sb_append_cstr(html, "<h3>Printing</h3>\n");
        for (size_t i = 0; i < sizeof(printing_intrinsics)/sizeof(printing_intrinsics[0]); i++) {
            String_Pool_Index sig = extract_intrinsic_signature(printing_start, printing_end, printing_intrinsics[i]);
            String_Pool_Index desc = extract_intrinsic_description(printing_start, printing_end, printing_intrinsics[i]);
            sb_appendf(html, "<div class=\"item\" id=\"%s\">\n", printing_intrinsics[i]);
            sb_appendf(html, "<h2>%s</h2>\n", printing_intrinsics[i]);
            if (sig.pool && strlen(Pooled_String(sig)) > 0) {
                sb_appendf(html, "<pre class=\"signature\"><code>");
                html_escape(html, Pooled_String(sig), sig.len);
                sb_appendf(html, "</code></pre>\n");
            }
            if (desc.pool && strlen(Pooled_String(desc)) > 0) {
                sb_appendf(html, "<div class=\"description\">%s</div>\n", Pooled_String(desc));
            }
            sb_appendf(html, "</div>\n\n");
        }
    }

    if (stack_start && stack_start < (memory_start ? memory_start : loops_start)) {
        const char *stack_end = memory_start ? memory_start : (loops_start ? loops_start : intrinsics_start + 100);
        sb_append_cstr(html, "<h3>Stack Operations</h3>\n");
        for (size_t i = 0; i < sizeof(stack_intrinsics)/sizeof(stack_intrinsics[0]); i++) {
            String_Pool_Index sig = extract_intrinsic_signature(stack_start, stack_end, stack_intrinsics[i]);
            String_Pool_Index desc = extract_intrinsic_description(stack_start, stack_end, stack_intrinsics[i]);
            sb_appendf(html, "<div class=\"item\" id=\"%s\">\n", stack_intrinsics[i]);
            sb_appendf(html, "<h2>%s</h2>\n", stack_intrinsics[i]);
            if (sig.pool && strlen(Pooled_String(sig)) > 0) {
                sb_appendf(html, "<pre class=\"signature\"><code>");
                html_escape(html, Pooled_String(sig), sig.len);
                sb_appendf(html, "</code></pre>\n");
            }
            if (desc.pool && strlen(Pooled_String(desc)) > 0) {
                sb_appendf(html, "<div class=\"description\">%s</div>\n", Pooled_String(desc));
            }
            sb_appendf(html, "</div>\n\n");
        }
    }

    if (memory_start && memory_start < loops_start) {
        sb_append_cstr(html, "<h3>Memory Management</h3>\n");
        for (size_t i = 0; i < sizeof(memory_intrinsics)/sizeof(memory_intrinsics[0]); i++) {
            String_Pool_Index sig = extract_intrinsic_signature(memory_start, loops_start, memory_intrinsics[i]);
            String_Pool_Index desc = extract_intrinsic_description(memory_start, loops_start, memory_intrinsics[i]);
            sb_appendf(html, "<div class=\"item\" id=\"%s\">\n", memory_intrinsics[i]);
            sb_appendf(html, "<h2>%s</h2>\n", memory_intrinsics[i]);
            if (sig.pool && strlen(Pooled_String(sig)) > 0) {
                sb_appendf(html, "<pre class=\"signature\"><code>");
                html_escape(html, Pooled_String(sig), sig.len);
                sb_appendf(html, "</code></pre>\n");
            }
            if (desc.pool && strlen(Pooled_String(desc)) > 0) {
                sb_appendf(html, "<div class=\"description\">%s</div>\n", Pooled_String(desc));
            }
            sb_appendf(html, "</div>\n\n");
        }
    }

    html_section_close(html);

    html_section_open(html, "control-flow", "Control Flow", "Who needs more than while loops anyways?");
    sb_appendf(html, "<div class=\"item\" id=\"while\">\n");
    sb_appendf(html, "<h2>while</h2>\n");
    Nob_String_View loop_sv = sv_from_parts(loops_start, procs_start - loops_start);
    sv_chop_by_delim(&loop_sv, '\n');
    String_Pool_Index loop_desc_idx = extract_paragraph_html(loop_sv);
    if (loop_desc_idx.pool && strlen(Pooled_String(loop_desc_idx)) > 0) {
        sb_appendf(html, "<div class=\"description\">%s</div>\n", Pooled_String(loop_desc_idx));
    }
    const char *loop_sig = extract_code_block(loops_start, procs_start, 0);
    if (loop_sig) {
        sb_appendf(html, "<pre class=\"signature\"><code>");
        html_qleei_highlight(html, loop_sig, strlen(loop_sig));
        sb_appendf(html, "</code></pre>\n");
    }
    const char *loop_example = extract_code_block(loops_start, procs_start, 1);
    if (loop_example) {
        sb_appendf(html, "<pre class=\"example\"><code>");
        html_qleei_highlight(html, loop_example, strlen(loop_example));
        sb_appendf(html, "</code></pre>\n");
    }
    sb_appendf(html, "</div>\n\n");
    html_section_close(html);

    html_section_open(html, "procedures", "User Procedures", "Defining your own procedures");
    sb_appendf(html, "<div class=\"item\" id=\"proc\">\n");
    sb_appendf(html, "<h2>proc</h2>\n");
    Nob_String_View proc_sv = sv_from_parts(procs_start, len - (procs_start - data));
    sv_chop_by_delim(&proc_sv, '\n');
    String_Pool_Index proc_desc_idx = extract_paragraph_html(proc_sv);
    if (proc_desc_idx.pool && strlen(Pooled_String(proc_desc_idx)) > 0) {
        sb_appendf(html, "<div class=\"description\">%s</div>\n", Pooled_String(proc_desc_idx));
    }
    const char *proc_sig = extract_code_block(procs_start, data + len, 0);
    if (proc_sig) {
        sb_appendf(html, "<pre class=\"signature\"><code>");
        html_qleei_highlight(html, proc_sig, strlen(proc_sig));
        sb_appendf(html, "</code></pre>\n");
    }
    const char *proc_example = extract_code_block(procs_start, data + len, 1);
    if (proc_example) {
        sb_appendf(html, "<pre class=\"example\"><code>");
        html_qleei_highlight(html, proc_example, strlen(proc_example));
        sb_appendf(html, "</code></pre>\n");
    }
    sb_appendf(html, "</div>\n\n");
    html_section_close(html);

    sb_append_cstr(html, "</main>\n</div>\n</body>\n</html>\n");

    if (!write_entire_file(output_path, html->items, html->count)) return_defer(false);

  defer:
    sb_free(content);
    return result;
}

static bool doc_gen_lang_ref_md(String_Builder *md, const char *output_path) {
  bool result = true;
  md->count = 0;
  String_Builder content = {0};

    if (!read_entire_file("README.md", &content)) {
        fprintf(stderr, "Failed to read README.md\n");
        return_defer(false);
    }
    const char *data = content.items;
    size_t len = content.count;

    const char *types_start = find_header(data, len, "### Types");
    const char *intrinsics_start = find_header(data, len, "### Intrinsics");
    const char *loops_start = find_header(data, len, "## Loops");
    const char *procs_start = find_header(data, len, "## User Procedures");

    const char *printing_start = find_header(data, len, "Printing:");
    const char *stack_start = find_header(data, len, "General Stack Operations:");
    const char *memory_start = find_header(data, len, "Memory Management:");

    sb_append_cstr(md, "# QLeii Language Reference\n\n");
    sb_append_cstr(md, "QLeei is a simple interpreted stack-based language.\n\n");

    sb_append_cstr(md, "## Types\n\n");
    for (size_t i = 0; i < types_count; i++) {
        String_Pool_Index desc = extract_type_description(types_start, intrinsics_start, types[i]);
        sb_appendf(md, "### %s\n\n", types[i]);
        sb_appendf(md, "%s\n\n", Pooled_String(desc));
    }

    sb_append_cstr(md, "## Intrinsics\n\n");

    if (printing_start && printing_start < (stack_start ? stack_start : loops_start)) {
        const char *printing_end = stack_start ? stack_start : (loops_start ? loops_start : intrinsics_start + 100);
        sb_append_cstr(md, "### Printing\n\n");
        for (size_t i = 0; i < sizeof(printing_intrinsics)/sizeof(printing_intrinsics[0]); i++) {
            String_Pool_Index sig = extract_intrinsic_signature(printing_start, printing_end, printing_intrinsics[i]);
            String_Pool_Index desc = extract_intrinsic_description(printing_start, printing_end, printing_intrinsics[i]);
            sb_appendf(md, "#### %s\n\n", printing_intrinsics[i]);
            if (sig.pool && strlen(Pooled_String(sig)) > 0) {
                sb_appendf(md, "```\n%s\n```\n\n", Pooled_String(sig));
            }
            if (desc.pool && strlen(Pooled_String(desc)) > 0) {
                sb_appendf(md, "%s\n\n", Pooled_String(desc));
            }
        }
    }

    if (stack_start && stack_start < (memory_start ? memory_start : loops_start)) {
        const char *stack_end = memory_start ? memory_start : (loops_start ? loops_start : intrinsics_start + 100);
        sb_append_cstr(md, "### Stack Operations\n\n");
        for (size_t i = 0; i < sizeof(stack_intrinsics)/sizeof(stack_intrinsics[0]); i++) {
            String_Pool_Index sig = extract_intrinsic_signature(stack_start, stack_end, stack_intrinsics[i]);
            String_Pool_Index desc = extract_intrinsic_description(stack_start, stack_end, stack_intrinsics[i]);
            sb_appendf(md, "#### %s\n\n", stack_intrinsics[i]);
            if (sig.pool && strlen(Pooled_String(sig)) > 0) {
                sb_appendf(md, "```\n%s\n```\n\n", Pooled_String(sig));
            }
            if (desc.pool && strlen(Pooled_String(desc)) > 0) {
                sb_appendf(md, "%s\n\n", Pooled_String(desc));
            }
        }
    }

    if (memory_start && memory_start < loops_start) {
        sb_append_cstr(md, "### Memory Management\n\n");
        for (size_t i = 0; i < sizeof(memory_intrinsics)/sizeof(memory_intrinsics[0]); i++) {
            String_Pool_Index sig = extract_intrinsic_signature(memory_start, loops_start, memory_intrinsics[i]);
            String_Pool_Index desc = extract_intrinsic_description(memory_start, loops_start, memory_intrinsics[i]);
            sb_appendf(md, "#### %s\n\n", memory_intrinsics[i]);
            if (sig.pool && strlen(Pooled_String(sig)) > 0) {
                sb_appendf(md, "```\n%s\n```\n\n", Pooled_String(sig));
            }
            if (desc.pool && strlen(Pooled_String(desc)) > 0) {
                sb_appendf(md, "%s\n\n", Pooled_String(desc));
            }
        }
    }

    sb_append_cstr(md, "## Loops\n\n");
    sb_appendf(md, "### while\n\n");
    Nob_String_View loop_sv_md = sv_from_parts(loops_start + 9, procs_start - loops_start - 9);
    String_Pool_Index loop_desc_idx = extract_paragraph_md(loop_sv_md);
    if (loop_desc_idx.pool && strlen(Pooled_String(loop_desc_idx)) > 0) {
        sb_appendf(md, "%s\n\n", Pooled_String(loop_desc_idx));
    }
    const char *loop_sig_md = extract_code_block(loops_start, procs_start, 0);
    if (loop_sig_md) {
        sb_appendf(md, "```qleei\n%s\n```\n\n", loop_sig_md);
    }
    const char *loop_example_md = extract_code_block(loops_start, procs_start, 1);
    if (loop_example_md) {
        sb_appendf(md, "```qleei\n%s\n```\n\n", loop_example_md);
    }

    sb_append_cstr(md, "## User Procedures\n\n");
    sb_appendf(md, "### proc\n\n");
    Nob_String_View proc_sv_md = sv_from_parts(procs_start + 19, len - (procs_start - data) - 19);
    String_Pool_Index proc_desc_idx = extract_paragraph_md(proc_sv_md);
    if (proc_desc_idx.pool && strlen(Pooled_String(proc_desc_idx)) > 0) {
        sb_appendf(md, "%s\n\n", Pooled_String(proc_desc_idx));
    }
    const char *proc_sig_md = extract_code_block(procs_start, data + len, 0);
    if (proc_sig_md) {
        sb_appendf(md, "```qleei\n%s\n```\n\n", proc_sig_md);
    }
    const char *proc_example_md = extract_code_block(procs_start, data + len, 1);
    if (proc_example_md) {
        sb_appendf(md, "```qleei\n%s\n```\n\n", proc_example_md);
    }

  if (!write_entire_file(output_path, md->items, md->count)) return_defer(false);

defer:
  return result;
}

bool doc_gen_lang_ref(String_Builder *sb, const char *output_dir) {
    if (!mkdir_if_not_exists(output_dir)) return false;

    char html_path[512];
    char md_path[512];
    snprintf(html_path, sizeof(html_path), "%s/index.html", output_dir);
    snprintf(md_path, sizeof(md_path), "%s/llm.md", output_dir);

    if (!doc_gen_lang_ref_html(sb, html_path)) return false;
    if (!doc_gen_lang_ref_md(sb, md_path)) return false;
    return true;
}
