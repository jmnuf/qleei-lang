#include "doc_gen.h"

String_Pool_Index unknown_name = {0};

static String_Pool_Index pool_strdup(const char *str) {
  String_Pool_Index result;
  result.index = string_pool.count;
  result.pool  = &string_pool;
  sb_append_cstr(&string_pool, str);
  result.len = string_pool.count - result.index;
  sb_append_null(&string_pool);
  return result;
}

static const char *group_key(const char *name) {
    if (strncmp(name, "qleei_", 6) == 0) {
        name = name + 6;
    } else if (strncmp(name, "QLeei_", 6) == 0) {
        name = name + 6;
    } else if (strncmp(name, "Qleei_", 6) == 0) {
        name = name + 6;
    }

    const char *underscore = strchr(name, '_');
    if (!underscore) return NULL;

    const char *second = underscore + 1;
    const char *second_end = strchr(second, '_');

    if (second_end && (strncmp(second, "words", 5) == 0 || strncmp(second, "word", 4) == 0)) {
        size_t first_len = underscore - name;
        size_t second_len = second_end - second;
        size_t buf_size = first_len + 1 + second_len + 1;
        char *buf = temp_alloc(buf_size);
        if (buf == NULL) return NULL;
        memcpy(buf, name, first_len);
        buf[first_len] = '_';
        memcpy(buf + first_len + 1, second, second_len);
        buf[first_len + 1 + second_len] = '\0';
        return buf;
    }

    size_t len = underscore - name;
    size_t buf_size = len + 1;
    char *buf = temp_alloc(buf_size);
    if (buf == NULL) return NULL;

    memcpy(buf, name, len);
    buf[len] = '\0';
    return buf;
}

static int group_cmp(const void *a, const void *b) {
    const Group *ga = (const Group*)a;
    const Group *gb = (const Group*)b;
    if (ga->key.pool == NULL && gb->key.pool == NULL) return 0;
    if (ga->key.pool == NULL) return -1;
    if (gb->key.pool == NULL) return 1;
    const char *zsa = Pooled_String(ga->key);
    const char *zsb = Pooled_String(gb->key);
    return strcmp(zsa, zsb);
}

static bool is_function(const char *signature) {
    return strncmp(signature, "static", 6) != 0 &&
           strncmp(signature, "typedef", 7) != 0;
}

static bool is_type(const char *signature) {
    return strncmp(signature, "typedef", 7) == 0;
}

static Group_List group_build(Api_List *api, bool (*filter)(const char*)) {
    Group_List result = {0};

    size_t save_point = temp_save();
    for (size_t i = 0; i < api->count; i++) {
      temp_rewind(save_point);
        if (!filter(Pooled_String(api->items[i].signature))) continue;

        const char *key = group_key(Pooled_String(api->items[i].name));

        size_t g = 0;
        for (; g < result.count; g++) {
            if ((key == NULL && result.items[g].key.pool == NULL) ||
                (key != NULL && result.items[g].key.pool != NULL && strcmp(key, Pooled_String(result.items[g].key)) == 0)) {
                break;
            }
        }

        if (g >= result.count) {
            Group new_group = {0};
            if (key) new_group.key = pool_strdup(key);
            da_append(&result, new_group);
        }

        da_append(&result.items[g], api->items[i]);
    }

    for (size_t i = 0; i < result.count; i++) {
        if (result.items[i].count <= 1) {
            result.items[i].key.index = 0;
            result.items[i].key.pool = NULL;
        }
    }
    qsort(result.items, result.count, sizeof(Group), group_cmp);

    return result;
}

static String_Pool_Index parse_symbol_line(const char *content, size_t len, size_t start) {
    size_t line_start = start;
    size_t typedef_start = 0;
    int brace_count = 0;
    bool is_typedef = false;

    for (size_t i = start; i < len; i++) {
        if (content[i] == '\n') {
            size_t line_end = i;
            size_t line_len = line_end - line_start;

            if (line_len == 0) {
                line_start = i + 1;
                continue;
            }

            char *line = temp_sprintf("%.*s", (int)line_len, content + line_start);

            char *trimmed = line;
            while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

            if (*trimmed == '\0' || *trimmed == '\n' || strncmp(trimmed, "//", 2) == 0 || strncmp(trimmed, "/*", 2) == 0) {
                line_start = i + 1;
                continue;
            }

            if (strncmp(trimmed, "typedef", 7) == 0) {
                is_typedef = true;
                typedef_start = line_start;
            }

            if (is_typedef) {
                for (size_t j = 0; j < line_len; j++) {
                    if (content[line_start + j] == '{') brace_count++;
                    if (content[line_start + j] == '}') brace_count--;
                }
                if (brace_count == 0 && typedef_start > 0) {
                    size_t total_len = i - typedef_start;
                    char *result = temp_sprintf("%.*s", (int)total_len, content + typedef_start);
                    String_Pool_Index pooled = pool_strdup(result);
                    return pooled;
                }
            } else {
                if (strncmp(trimmed, "typedef", 7) == 0 ||
                    strncmp(trimmed, "struct", 6) == 0 ||
                    strncmp(trimmed, "enum", 4) == 0 ||
                    strncmp(trimmed, "static", 6) == 0 ||
                    strncmp(trimmed, "const", 5) == 0 ||
                    strncmp(trimmed, "extern", 6) == 0 ||
                    (trimmed[0] != '#' && (isalpha(trimmed[0]) || trimmed[0] == '_'))) {
                    char *result = temp_sprintf("%.*s", (int)line_len, content + line_start);
                    String_Pool_Index pooled = pool_strdup(result);
                    return pooled;
                }
            }

            line_start = i + 1;
        }
    }
    return Null_String_Pool_Index;
}

static String_Pool_Index parse_name(const char *signature) {
    if (strncmp(signature, "typedef", 7) == 0) {
        const char *ptrn = strstr(signature, "(*");
        if (ptrn) {
            const char *p = ptrn + 2;
            while (*p == ' ' || *p == '\t') p++;

            if (isalnum(*p) || *p == '_') {
                const char *name_start = p;
                while (isalnum(*p) || *p == '_') p++;
                size_t name_len = p - name_start;
                char *result = temp_sprintf("%.*s", (int)name_len, name_start);
                return pool_strdup(result);
            }
        }

        const char *last_paren = strrchr(signature, ')');
        if (last_paren) {
            const char *p = last_paren - 1;
            while (p > signature && (*p == ' ' || *p == '\t')) p--;

            if (*p == ')') {
                int depth = 1;
                while (p > signature && depth > 0) {
                    p--;
                    if (*p == ')') depth++;
                    else if (*p == '(') depth--;
                }
                if (p > signature) p--;
                while (p > signature && (*p == ' ' || *p == '\t' || *p == '*')) p--;

                if (isalnum(*p) || *p == '_') {
                    const char *name_start = p;
                    while (name_start > signature && (isalnum(name_start[-1]) || name_start[-1] == '_')) name_start--;
                    size_t name_len = p - name_start + 1;
                    char *result = temp_sprintf("%.*s", (int)name_len, name_start);
                    return pool_strdup(result);
                }
            }
        }
    }

    const char *paren = strchr(signature, '(');
    if (paren) {
        const char *p = paren - 1;
        while (p > signature && (*p == ' ' || *p == '\t')) p--;

        const char *name_end = p + 1;
        while (p > signature && (isalnum(*p) || *p == '_')) p--;
        p++;

        size_t name_len = name_end - p;
        if (name_len > 0) {
            char *result = temp_sprintf("%.*s", (int)name_len, p);
            return pool_strdup(result);
        }
    }

    const char *p = signature + strlen(signature) - 1;
    while (p > signature && (*p == ' ' || *p == '\t' || *p == '\n' || *p == ';')) p--;

    const char *name_end = p + 1;
    while (p > signature && (isalnum(*p) || *p == '_')) p--;
    p++;

    if (name_end > p) {
        size_t name_len = name_end - p;
        char *result = temp_sprintf("%.*s", (int)name_len, p);
        return pool_strdup(result);
    }
    return Null_String_Pool_Index;
}

static String_Pool_Index parse_doc(const char *doc, size_t len) {
    size_t start = 0;
    while (start < len && (doc[start] == ' ' || doc[start] == '\t')) start++;
    if (start + 2 < len && doc[start] == '/' && doc[start+1] == '*' && doc[start+2] == '*') {
        start += 3;
    }
    if (start < len && doc[start] == ' ') start++;

    String_Builder result = {0};

    for (size_t i = start; i < len && doc[i] != '\n'; i++) {
        da_append(&result, doc[i]);
    }

    for (size_t i = start; i < len; i++) {
        if (doc[i] == '\n') {
            size_t line_start = i + 1;
            if (line_start >= len) break;

            while (line_start < len && doc[line_start] == ' ') line_start++;
            if (line_start < len && doc[line_start] == '*') line_start++;
            if (line_start < len && doc[line_start] == ' ') line_start++;

            if (line_start >= len || doc[line_start] == '\n') continue;
            if (doc[line_start] == '@') break;

            if (result.count > 0 && result.items[result.count-1] != ' ' && result.items[result.count-1] != '\n') {
                da_append(&result, ' ');
            }

            while (line_start < len && doc[line_start] != '\n' && doc[line_start] != '@') {
                da_append(&result, doc[line_start++]);
            }
        }
    }

    while (result.count > 0 && (result.items[result.count-1] == ' ' || result.items[result.count-1] == '\t')) {
        result.count--;
    }

    da_append(&result, '\0');
    String_Pool_Index pooled = pool_strdup(result.items);
    sb_free(result);
    return pooled;
}

static void parse_header(const char *path, Api_List *api) {
    String_Builder content = {0};
    if (!read_entire_file(path, &content)) {
        fprintf(stderr, "Failed to read %s\n", path);
        return;
    }

    char *data = content.items;
    size_t len = content.count;

    size_t i = 0;
    while (i < len - 3) {
        if (data[i] == '/' && data[i+1] == '*' && data[i+2] == '*') {
            size_t doc_start = i;
            size_t doc_end = 0;

            for (size_t j = i + 3; j < len - 1; j++) {
                if (data[j] == '*' && data[j+1] == '/') {
                    doc_end = j;
                    break;
                }
            }

            if (doc_end > doc_start) {
                size_t doc_len = doc_end - doc_start;
                char *doc_comment = temp_sprintf("%.*s", (int)doc_len, data + doc_start);

                String_Pool_Index cleaned_doc = parse_doc(doc_comment, doc_len);

                String_Pool_Index signature = parse_symbol_line(data, len, doc_end + 2);

                if (signature.pool != NULL) {
                    String_Pool_Index name = parse_name(Pooled_String(signature));

                    Api_Item item = {
                        .name = name.pool ? name : unknown_name,
                        .signature = signature,
                        .description = cleaned_doc
                    };
                    da_append(api, item);
                }
            }

            i = doc_end > 0 ? doc_end + 2 : i + 1;
        } else {
            i++;
        }
    }

    sb_free(content);
}

static void doc_gen_c_ref_html(const char *output_path, Group_List *funcs, Group_List *types) {
    String_Builder html = {0};
    da_reserve(&html, 64 * 1024);

    html_doc_open(&html);

    sb_append_cstr(&html, "<h2>Functions</h2>\n");
    for (size_t i = 0; i < funcs->count; i++) {
        html_sidebar_group(&html, &funcs->items[i]);
    }

    sb_append_cstr(&html, "<h2>Types</h2>\n");
    for (size_t i = 0; i < types->count; i++) {
        html_sidebar_group(&html, &types->items[i]);
    }

    html_main_open(&html);

    html_section_open(&html, "functions", "Functions", "Can you feel? Can you hear me?");
    for (size_t i = 0; i < funcs->count; i++) {
        html_content_group(&html, &funcs->items[i]);
    }
    html_section_close(&html);

    html_section_open(&html, "types", "Types", "What does this mean?! What does this mean?! WHAT DOES THIS MEAN?!");
    for (size_t i = 0; i < types->count; i++) {
        html_content_group(&html, &types->items[i]);
    }
    html_section_close(&html);

    sb_append_cstr(&html, "</main>\n</div>\n</body>\n</html>\n");

    write_entire_file(output_path, html.items, html.count);
    sb_free(html);
}

static void doc_gen_c_ref_md(const char *output_path, Group_List *funcs, Group_List *types) {
    String_Builder md = {0};
    da_reserve(&md, 32 * 1024);

    md_header(&md);

    sb_append_cstr(&md, "## Functions\n\n");
    for (size_t i = 0; i < funcs->count; i++) {
        md_group(&md, &funcs->items[i]);
    }

    sb_append_cstr(&md, "\n## Types\n\n");
    for (size_t i = 0; i < types->count; i++) {
        md_group(&md, &types->items[i]);
    }

    sb_append_null(&md);
    write_entire_file(output_path, md.items, md.count - 1);
    sb_free(md);
}

void doc_gen_c_ref(const char *output_dir) {
    mkdir_if_not_exists(output_dir);

    Api_List api = {0};
    unknown_name = pool_strdup("unknown");
    da_reserve(&string_pool, 64 * 1024);
    parse_header("qleei.h", &api);

    Type_List types = {0};
    for (size_t i = 0; i < api.count; i++) {
        if (is_type(Pooled_String(api.items[i].signature))) {
            da_append(&types, Pooled_String(api.items[i].name));
        }
    }

    Group_List funcs = group_build(&api, is_function);
    Group_List type_groups = group_build(&api, is_type);

    char html_path[512];
    char md_path[512];
    snprintf(html_path, sizeof(html_path), "%s/index.html", output_dir);
    snprintf(md_path, sizeof(md_path), "%s/llm.md", output_dir);

    doc_gen_c_ref_html(html_path, &funcs, &type_groups);
    doc_gen_c_ref_md(md_path, &funcs, &type_groups);

    for (size_t i = 0; i < funcs.count; i++) {
        free(funcs.items[i].items);
    }
    free(funcs.items);

    for (size_t i = 0; i < type_groups.count; i++) {
        free(type_groups.items[i].items);
    }
    free(type_groups.items);

    free(api.items);
    free(types.items);
}
