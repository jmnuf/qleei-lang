#define NOB_DA_INIT_CAP 8
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "../nob.h"

typedef struct {
    const char *name;
    const char *signature;
    const char *description;
} Api_Item;

typedef struct {
    Api_Item *items;
    size_t count;
    size_t capacity;
} Api_List;

typedef struct {
    const char **items;
    size_t count;
    size_t capacity;
} Type_List;

typedef struct {
    const char *key;
    Api_Item *items;
    size_t count;
    size_t capacity;
} Group;

typedef struct {
    Group *items;
    size_t count;
    size_t capacity;
} Group_List;

static String_Builder string_pool = {0};

static const char *pool_strdup(const char *str) {
    size_t len = strlen(str);
    size_t start = string_pool.count;
    da_append_many(&string_pool, str, len);
    da_append(&string_pool, '\0');
    return string_pool.items + start;
}

static const char *group_key(const char *name, char *buf, size_t buf_size) {
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
        if (first_len + 1 + second_len >= buf_size) return NULL;
        memcpy(buf, name, first_len);
        buf[first_len] = '_';
        memcpy(buf + first_len + 1, second, second_len);
        buf[first_len + 1 + second_len] = '\0';
        return buf;
    }
    
    size_t len = underscore - name;
    if (len >= buf_size) return NULL;
    
    memcpy(buf, name, len);
    buf[len] = '\0';
    return buf;
}

static int group_cmp(const void *a, const void *b) {
    const Group *ga = (const Group*)a;
    const Group *gb = (const Group*)b;
    if (ga->key == NULL && gb->key == NULL) return 0;
    if (ga->key == NULL) return -1;
    if (gb->key == NULL) return 1;
    return strcmp(ga->key, gb->key);
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
    char key_buf[64];
    
    for (size_t i = 0; i < api->count; i++) {
        if (!filter(api->items[i].signature)) continue;
        
        const char *key = group_key(api->items[i].name, key_buf, sizeof(key_buf));
        
        size_t g = 0;
        for (; g < result.count; g++) {
            if ((key == NULL && result.items[g].key == NULL) ||
                (key != NULL && result.items[g].key != NULL && strcmp(key, result.items[g].key) == 0)) {
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
            result.items[i].key = NULL;
        }
    }
    qsort(result.items, result.count, sizeof(Group), group_cmp);
    
    return result;
}

static const char *parse_symbol_line(const char *content, size_t len, size_t start) {
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
                    size_t result_len = strlen(result);
                    const char *pooled = pool_strdup(result);
                    (void)result_len;
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
                    const char *pooled = pool_strdup(result);
                    return pooled;
                }
            }
            
            line_start = i + 1;
        }
    }
    return NULL;
}

static const char *parse_name(const char *signature) {
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
    return NULL;
}

static const char *parse_doc(const char *doc, size_t len) {
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
    const char *pooled = pool_strdup(result.items);
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
                
                const char *cleaned_doc = parse_doc(doc_comment, doc_len);
                
                const char *signature = parse_symbol_line(data, len, doc_end + 2);
                
                if (signature) {
                    const char *name = parse_name(signature);
                    
                    Api_Item item = {
                        .name = name ? pool_strdup(name) : pool_strdup("unknown"),
                        .signature = pool_strdup(signature),
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

static bool token_is_keyword(const char *word, size_t len) {
    static const char *keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "inline", "int", "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
        "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary",
        "NULL", "true", "false"
    };
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        if (strncmp(word, keywords[i], len) == 0 && keywords[i][len] == '\0') {
            return true;
        }
    }
    return false;
}

static bool token_is_builtin(const char *word, size_t len) {
    static const char *types[] = {
        "size_t", "ssize_t", "bool", "char", "int", "long", "short", "float",
        "double", "void", "FILE", "ptrdiff_t", "wchar_t", "char16_t", "char32_t",
        "qleei_uisz_t", "qleei_si8_t", "qleei_ui8_t", "qleei_si16_t", "qleei_ui16_t",
        "qleei_si32_t", "qleei_ui32_t", "qleei_si64_t", "qleei_ui64_t"
    };
    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        if (strncmp(word, types[i], len) == 0 && types[i][len] == '\0') {
            return true;
        }
    }
    return false;
}

static bool token_is_known(const char *word, size_t len, Type_List *types) {
    for (size_t i = 0; i < types->count; i++) {
        size_t type_len = strlen(types->items[i]);
        if (len == type_len && strncmp(word, types->items[i], len) == 0) {
            return true;
        }
    }
    return false;
}

static void html_escape(String_Builder *sb, const char *text, size_t len) {
    for (size_t i = 0; i < len; i++) {
        switch (text[i]) {
            case '&': sb_append_cstr(sb, "&amp;"); break;
            case '<': sb_append_cstr(sb, "&lt;"); break;
            case '>': sb_append_cstr(sb, "&gt;"); break;
            case '"': sb_append_cstr(sb, "&quot;"); break;
            default: da_append(sb, text[i]);
        }
    }
}

static void html_code_highlight(String_Builder *sb, const char *code, size_t len, Type_List *types, const char *current_name) {
    size_t i = 0;
    while (i < len) {
        if (code[i] == '/' && i + 1 < len && code[i+1] == '/') {
            sb_append_cstr(sb, "<span class=\"syn-cm\">");
            while (i < len) { da_append(sb, code[i]); if (code[i] == '\n') break; i++; }
            sb_append_cstr(sb, "</span>");
            i++;
        } else if (code[i] == '/' && i + 1 < len && code[i+1] == '*') {
            sb_append_cstr(sb, "<span class=\"syn-cm\">");
            da_append(sb, code[i]); da_append(sb, code[i+1]); i += 2;
            while (i < len - 1) {
                if (code[i] == '*' && code[i+1] == '/') {
                    da_append(sb, code[i]); da_append(sb, code[i+1]); i += 2;
                    break;
                }
                da_append(sb, code[i]); i++;
            }
            sb_append_cstr(sb, "</span>");
        } else if (code[i] == '"') {
            sb_append_cstr(sb, "<span class=\"syn-st\">");
            da_append(sb, code[i]); i++;
            while (i < len && code[i] != '"') {
                if (code[i] == '\\' && i + 1 < len) {
                    da_append(sb, code[i]); da_append(sb, code[i+1]); i += 2;
                } else {
                    da_append(sb, code[i]); i++;
                }
            }
            if (i < len) { da_append(sb, code[i]); i++; }
            sb_append_cstr(sb, "</span>");
        } else if (code[i] == '\'' && i + 1 < len) {
            sb_append_cstr(sb, "<span class=\"syn-st\">");
            da_append(sb, code[i]); i++;
            while (i < len && code[i] != '\'') {
                if (code[i] == '\\' && i + 1 < len) {
                    da_append(sb, code[i]); da_append(sb, code[i+1]); i += 2;
                } else {
                    da_append(sb, code[i]); i++;
                }
            }
            if (i < len) { da_append(sb, code[i]); i++; }
            sb_append_cstr(sb, "</span>");
        } else if (isalpha(code[i]) || code[i] == '_') {
            size_t start = i;
            while (i < len && (isalnum(code[i]) || code[i] == '_')) i++;
            size_t word_len = i - start;
            const char *word = code + start;
            
            if (token_is_keyword(word, word_len)) {
                sb_append_cstr(sb, "<span class=\"syn-kw\">");
                html_escape(sb, word, word_len);
                sb_append_cstr(sb, "</span>");
            } else if (token_is_builtin(word, word_len)) {
                sb_append_cstr(sb, "<span class=\"syn-tp\">");
                html_escape(sb, word, word_len);
                sb_append_cstr(sb, "</span>");
            } else if (token_is_known(word, word_len, types) && 
                       (current_name == NULL || strncmp(word, current_name, word_len) != 0 || strlen(current_name) != word_len)) {
                sb_appendf(sb, "<a href=\"#%.*s\" class=\"type-link\">", (int)word_len, word);
                html_escape(sb, word, word_len);
                sb_append_cstr(sb, "</a>");
            } else if (i < len && code[i] == '(') {
                sb_append_cstr(sb, "<span class=\"syn-fn\">");
                html_escape(sb, word, word_len);
                sb_append_cstr(sb, "</span>");
            } else {
                html_escape(sb, word, word_len);
            }
        } else if (isdigit(code[i])) {
            size_t start = i;
            while (i < len && (isalnum(code[i]) || code[i] == '.' || code[i] == '_' || code[i] == 'x' || code[i] == 'X' ||
                             (code[i] >= 'a' && code[i] <= 'f') || (code[i] >= 'A' && code[i] <= 'F'))) i++;
            sb_append_cstr(sb, "<span class=\"syn-nm\">");
            html_escape(sb, code + start, i - start);
            sb_append_cstr(sb, "</span>");
        } else if (code[i] == '#') {
            size_t start = i;
            while (i < len && !isspace(code[i])) i++;
            sb_append_cstr(sb, "<span class=\"syn-cp\">");
            html_escape(sb, code + start, i - start);
            sb_append_cstr(sb, "</span>");
        } else {
            switch (code[i]) {
                case '&': case '|': case '!': case '=': case '+': case '-': case '*': case '/': case '%':
                case '<': case '>': case '^': case '~': case '?': case ':':
                    sb_append_cstr(sb, "<span class=\"syn-cp\">");
                    da_append(sb, code[i]);
                    sb_append_cstr(sb, "</span>");
                    break;
                default:
                    da_append(sb, code[i]);
            }
            i++;
        }
    }
}

static void html_doc_open(String_Builder *sb) {
    sb_append_cstr(sb,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "<title>QLeii - Documentation</title>\n"
        "<style>\n"
        "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "html { overflow-x: hidden; }\n"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; line-height: 1.6; color: #333; background: #fff0f5; overflow-x: hidden; }\n"
        ".container { display: flex; min-height: 100vh; max-width: 100vw; }\n"
        ".sidebar { width: 280px; background: #4a1259; color: #fff; padding: 20px; overflow-y: auto; position: fixed; height: 100vh; overflow-x: hidden; }\n"
        ".sidebar h1 { font-size: 1.5rem; margin-bottom: 20px; color: #fff; }\n"
        ".sidebar h2 { font-size: 0.9rem; text-transform: uppercase; color: #ffb6c1; margin: 20px 0 10px; letter-spacing: 1px; }\n"
        ".sidebar a { color: #fff; text-decoration: none; display: block; padding: 4px 0; font-size: 0.9rem; overflow-wrap: break-word; }\n"
        ".sidebar a:hover { color: #ff69b4; }\n"
        ".sidebar details { margin: 4px 0; }\n"
        ".sidebar summary { cursor: pointer; padding: 4px 0; font-size: 0.9rem; color: #ffb6c1; user-select: none; }\n"
        ".sidebar summary:hover { color: #ff69b4; }\n"
        ".sidebar .group-items { padding-left: 12px; }\n"
        ".sidebar .group-items a { font-size: 0.8rem; color: #d4a5d4; }\n"
        ".main { flex: 1; margin-left: 280px; padding: 40px; max-width: calc(100vw - 280px); }\n"
        ".section-header { color: #4a1259; font-size: 2rem; margin: 40px 0 8px; padding-bottom: 8px; border-bottom: 2px solid #ffb6c1; }\n"
        ".section-tagline { color: #888; font-style: italic; margin-bottom: 24px; font-size: 0.95rem; }\n"
        ".section-content { margin-bottom: 20px; }\n"
        ".item { background: #fff; border-radius: 8px; padding: 24px; margin-bottom: 24px; box-shadow: 0 2px 4px rgba(74,18,89,0.1); border: 1px solid #ffb6c1; overflow-x: hidden; }\n"
        ".item h2 { color: #4a1259; font-size: 1.3rem; margin-bottom: 8px; font-family: 'Courier New', monospace; }\n"
        ".item .signature { background: #0d0d14; padding: 12px; border-radius: 4px; font-family: 'Courier New', monospace; font-size: 0.85rem; color: #e0e0e0; overflow-x: auto; margin-bottom: 12px; border: 1px solid #4a1259; white-space: pre; }\n"
        ".item .description { color: #444; word-wrap: break-word; overflow-wrap: break-word; }\n"
        ".item .description p { margin-bottom: 8px; word-wrap: break-word; overflow-wrap: break-word; }\n"
        ".item .description code { background: #1a1a2e; padding: 2px 6px; border-radius: 3px; font-family: 'Courier New', monospace; font-size: 0.85rem; color: #f8f0f5; word-wrap: break-word; }\n"
        ".item .description pre { background: #0d0d14; color: #e0e0e0; padding: 12px; border-radius: 4px; overflow-x: auto; margin: 12px 0; white-space: pre-wrap; word-wrap: break-word; }\n"
        ".item .description pre code { background: none; padding: 0; color: inherit; }\n"
        ".group { margin-bottom: 32px; }\n"
        ".group > summary { cursor: pointer; color: #4a1259; font-size: 1.2rem; font-weight: 600; margin-bottom: 16px; list-style-position: outside; }\n"
        ".group > summary:hover { color: #ff69b4; }\n"
        ".syn-kw { color: #ff79c6; font-weight: bold; }\n"
        ".syn-tp { color: #bd93f9; }\n"
        ".syn-fn { color: #50fa7b; }\n"
        ".syn-nm { color: #ffb86c; }\n"
        ".syn-st { color: #f1fa8c; }\n"
        ".syn-cm { color: #6272a4; font-style: italic; }\n"
        ".syn-cp { color: #8be9fd; }\n"
        ".type-link { color: #c792ea; text-decoration: underline; cursor: pointer; }\n"
        ".type-link:hover { color: #ff69b4; }\n"
        "@media (max-width: 768px) { html, body { overflow-x: hidden; } .sidebar { width: 100%; height: auto; position: relative; } .main { margin-left: 0; max-width: 100vw; } }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<div class=\"container\">\n"
        "<nav class=\"sidebar\">\n"
        "<h1>QLeii Docs</h1>\n"
    );
}

static void html_main_open(String_Builder *sb) {
    sb_append_cstr(sb,
        "</nav>\n"
        "<main class=\"main\">\n"
    );
}

static void html_section_open(String_Builder *sb, const char *id, const char *title, const char *tagline) {
    sb_appendf(sb, "<section id=\"%s\">\n", id);
    sb_appendf(sb, "<h1 class=\"section-header\">%s</h1>\n", title);
    sb_appendf(sb, "<p class=\"section-tagline\">%s</p>\n", tagline);
}

static void html_section_close(String_Builder *sb) {
    sb_append_cstr(sb, "</section>\n");
}

static void html_item(String_Builder *sb, Api_Item *item, Type_List *types) {
    sb_appendf(sb, "<div class=\"item\" id=\"%s\">\n", item->name);
    sb_appendf(sb, "<h2>%s</h2>\n", item->name);
    sb_appendf(sb, "<pre class=\"signature\"><code>");
    html_code_highlight(sb, item->signature, strlen(item->signature), types, item->name);
    sb_appendf(sb, "</code></pre>\n");
    sb_appendf(sb, "<div class=\"description\">%s</div>\n", item->description);
    sb_appendf(sb, "</div>\n\n");
}

static void html_sidebar_group(String_Builder *sb, Group *group) {
    if (group->key) {
        sb_appendf(sb, "<details>\n");
        sb_appendf(sb, "<summary>%s (%zu)</summary>\n", group->key, group->count);
        sb_appendf(sb, "<div class=\"group-items\">\n");
        for (size_t j = 0; j < group->count; j++) {
            sb_appendf(sb, "<a href=\"#%s\">%s</a>\n", group->items[j].name, group->items[j].name);
        }
        sb_appendf(sb, "</div>\n");
        sb_appendf(sb, "</details>\n");
    } else {
        sb_appendf(sb, "<a href=\"#%s\">%s</a>\n", group->items[0].name, group->items[0].name);
    }
}

static void html_content_group(String_Builder *sb, Group *group, Type_List *types) {
    if (group->key) {
        sb_appendf(sb, "<details class=\"group\">\n");
        sb_appendf(sb, "<summary>%s</summary>\n", group->key);
        for (size_t j = 0; j < group->count; j++) {
            html_item(sb, &group->items[j], types);
        }
        sb_appendf(sb, "</details>\n");
    } else {
        html_item(sb, &group->items[0], types);
    }
}

static void html_write(const char *output_path, Group_List *funcs, Group_List *types, Type_List *type_registry) {
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
        html_content_group(&html, &funcs->items[i], type_registry);
    }
    html_section_close(&html);
    
    html_section_open(&html, "types", "Types", "What does this mean?! What does this mean?! WHAT DOES THIS MEAN?!");
    for (size_t i = 0; i < types->count; i++) {
        html_content_group(&html, &types->items[i], type_registry);
    }
    html_section_close(&html);
    
    sb_append_cstr(&html, "</main>\n</div>\n</body>\n</html>\n");
    
    write_entire_file(output_path, html.items, html.count);
    sb_free(html);
}

static void md_header(String_Builder *sb) {
    sb_append_cstr(sb, "# QLeii API Reference\n\n");
    sb_append_cstr(sb, "QLeei is a simple interpreted stack-based language.\n\n");
}

static void md_item(String_Builder *sb, Api_Item *item) {
    sb_appendf(sb, "## %s\n\n", item->name);
    sb_appendf(sb, "%s\n\n", item->description);
    sb_appendf(sb, "```c\n%s\n```\n\n", item->signature);
}

static void md_group(String_Builder *sb, Group *group) {
    if (group->key) {
        sb_appendf(sb, "### %s\n\n", group->key);
    }
    for (size_t j = 0; j < group->count; j++) {
        md_item(sb, &group->items[j]);
    }
}

static void md_write(const char *output_path, Group_List *funcs, Group_List *types) {
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

static void cleanup(Api_List *api, Group_List *funcs, Group_List *types) {
    for (size_t i = 0; i < funcs->count; i++) {
        free(funcs->items[i].items);
    }
    free(funcs->items);
    
    for (size_t i = 0; i < types->count; i++) {
        free(types->items[i].items);
    }
    free(types->items);
    
    free(api->items);
}

int main(void) {
    Api_List api = {0};
    parse_header("qleei.h", &api);
    
    Type_List types = {0};
    for (size_t i = 0; i < api.count; i++) {
        if (is_type(api.items[i].signature)) {
            da_append(&types, api.items[i].name);
        }
    }
    
    mkdir_if_not_exists("docs");
    
    Group_List funcs = group_build(&api, is_function);
    Group_List type_groups = group_build(&api, is_type);
    
    html_write("docs/index.html", &funcs, &type_groups, &types);
    md_write("docs/llm.md", &funcs, &type_groups);
    
    cleanup(&api, &funcs, &type_groups);
    free(types.items);
    sb_free(string_pool);
    
    printf("Generated docs/index.html and docs/llm.md\n");
    return 0;
}
