#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "../nob.h"

typedef struct {
    char *name;
    char *signature;
    char *description;
} Api_Item;

typedef struct {
    Api_Item *items;
    size_t count;
    size_t capacity;
} Api_List;



static char *find_next_symbol_line(const char *content, size_t len, size_t start) {
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
            
            char *line = (char*)malloc(line_len + 1);
            memcpy(line, content + line_start, line_len);
            line[line_len] = '\0';
            
            char *trimmed = line;
            while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
            
            if (*trimmed == '\0' || *trimmed == '\n' || strncmp(trimmed, "//", 2) == 0 || strncmp(trimmed, "/*", 2) == 0) {
                free(line);
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
                    char *result = (char*)malloc(total_len + 1);
                    memcpy(result, content + typedef_start, total_len);
                    result[total_len] = '\0';
                    free(line);
                    return result;
                }
            } else {
                if (strncmp(trimmed, "typedef", 7) == 0 ||
                    strncmp(trimmed, "struct", 6) == 0 ||
                    strncmp(trimmed, "enum", 4) == 0 ||
                    strncmp(trimmed, "static", 6) == 0 ||
                    strncmp(trimmed, "const", 5) == 0 ||
                    strncmp(trimmed, "extern", 6) == 0 ||
                    (trimmed[0] != '#' && (isalpha(trimmed[0]) || trimmed[0] == '_'))) {
                    char *result = (char*)malloc(line_len + 1);
                    memcpy(result, line, line_len);
                    result[line_len] = '\0';
                    free(line);
                    return result;
                }
            }
            
            free(line);
            line_start = i + 1;
        }
    }
    return NULL;
}

static char *extract_name_from_signature(const char *signature) {
    const char *paren = strchr(signature, '(');
    
    if (paren) {
        const char *p = paren - 1;
        while (p > signature && (*p == ' ' || *p == '\t')) p--;
        
        const char *name_end = p + 1;
        while (p > signature && (isalnum(*p) || *p == '_')) p--;
        p++;
        
        size_t name_len = name_end - p;
        if (name_len > 0) {
            char *result = (char*)malloc(name_len + 1);
            memcpy(result, p, name_len);
            result[name_len] = '\0';
            return result;
        }
    }
    
    const char *search_end = signature + strlen(signature);
    const char *p = search_end - 1;
    
    while (p > signature && (*p == ' ' || *p == '\t' || *p == '\n' || *p == ';')) p--;
    
    const char *name_end = p + 1;
    while (p > signature && (isalnum(*p) || *p == '_')) p--;
    p++;
    
    if (name_end > p) {
        size_t name_len = name_end - p;
        char *result = (char*)malloc(name_len + 1);
        memcpy(result, p, name_len);
        result[name_len] = '\0';
        return result;
    }
    return NULL;
}

static void api_list_append(Api_List *list, Api_Item item) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        list->items = (Api_Item*)realloc(list->items, list->capacity * sizeof(Api_Item));
    }
    list->items[list->count++] = item;
}

static char *clean_doc_comment(const char *doc, size_t len) {
    size_t start = 0;
    while (start < len && (doc[start] == ' ' || doc[start] == '\t')) start++;
    if (start + 2 < len && doc[start] == '/' && doc[start+1] == '*' && doc[start+2] == '*') {
        start += 3;
    }
    if (start < len && doc[start] == ' ') start++;
    
    size_t cap = 256;
    size_t count = 0;
    char *result = (char*)malloc(cap);
    
    for (size_t i = start; i < len && doc[i] != '\n'; i++) {
        if (count + 1 >= cap) { cap *= 2; result = (char*)realloc(result, cap); }
        result[count++] = doc[i];
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
            
            if (count > 0 && result[count-1] != ' ' && result[count-1] != '\n') {
                if (count + 1 >= cap) { cap *= 2; result = (char*)realloc(result, cap); }
                result[count++] = ' ';
            }
            
            while (line_start < len && doc[line_start] != '\n' && doc[line_start] != '@') {
                if (count + 1 >= cap) { cap *= 2; result = (char*)realloc(result, cap); }
                result[count++] = doc[line_start++];
            }
        }
    }
    
    while (count > 0 && (result[count-1] == ' ' || result[count-1] == '\t')) {
        count--;
    }
    
    result[count] = '\0';
    return result;
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
                char *doc_comment = (char*)malloc(doc_len + 1);
                memcpy(doc_comment, data + doc_start, doc_len);
                doc_comment[doc_len] = '\0';
                
                char *cleaned_doc = clean_doc_comment(doc_comment, doc_len);
                free(doc_comment);
                
                char *signature = find_next_symbol_line(data, len, doc_end + 2);
                
                if (signature) {
                    char *name = extract_name_from_signature(signature);
                    
                    char *desc_copy = (char*)malloc(strlen(cleaned_doc) + 1);
                    strcpy(desc_copy, cleaned_doc);
                    
                    Api_Item item = {
                        .name = name ? strdup(name) : strdup("unknown"),
                        .signature = strdup(signature),
                        .description = desc_copy
                    };
                    api_list_append(api, item);
                    
                    free(signature);
                    free(name);
                } else {
                    free(cleaned_doc);
                }
            }
            
            i = doc_end > 0 ? doc_end + 2 : i + 1;
        } else {
            i++;
        }
    }
    
    sb_free(content);
}

static bool is_c_keyword(const char *word, size_t len) {
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

static bool is_c_type(const char *word, size_t len) {
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

static void highlight_c_code(String_Builder *sb, const char *code, size_t len) {
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
            
            if (is_c_keyword(word, word_len)) {
                sb_append_cstr(sb, "<span class=\"syn-kw\">");
                html_escape(sb, word, word_len);
                sb_append_cstr(sb, "</span>");
            } else if (is_c_type(word, word_len)) {
                sb_append_cstr(sb, "<span class=\"syn-tp\">");
                html_escape(sb, word, word_len);
                sb_append_cstr(sb, "</span>");
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

static void write_html_header(String_Builder *sb) {
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
        ".syn-kw { color: #ff79c6; font-weight: bold; }\n"
        ".syn-tp { color: #bd93f9; }\n"
        ".syn-fn { color: #50fa7b; }\n"
        ".syn-nm { color: #ffb86c; }\n"
        ".syn-st { color: #f1fa8c; }\n"
        ".syn-cm { color: #6272a4; font-style: italic; }\n"
        ".syn-cp { color: #8be9fd; }\n"
        "@media (max-width: 768px) { html, body { overflow-x: hidden; } .sidebar { width: 100%; height: auto; position: relative; } .main { margin-left: 0; max-width: 100vw; } }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<div class=\"container\">\n"
        "<nav class=\"sidebar\">\n"
        "<h1>QLeii Docs</h1>\n"
    );
}

static void write_html_footer(String_Builder *sb) {
    sb_append_cstr(sb,
        "</nav>\n"
        "<main class=\"main\">\n"
    );
}

static void write_section_header(String_Builder *sb, const char *id, const char *title, const char *tagline) {
    sb_appendf(sb, "<section id=\"%s\">\n", id);
    sb_appendf(sb, "<h1 class=\"section-header\">%s</h1>\n", title);
    sb_appendf(sb, "<p class=\"section-tagline\">%s</p>\n", tagline);
}

static void write_section_footer(String_Builder *sb) {
    sb_append_cstr(sb, "</section>\n");
}

static void write_item_html(String_Builder *sb, Api_Item *item) {
    sb_appendf(sb, "<div class=\"item\" id=\"%s\">\n", item->name);
    sb_appendf(sb, "<h2>%s</h2>\n", item->name);
    sb_appendf(sb, "<pre class=\"signature\"><code>");
    highlight_c_code(sb, item->signature, strlen(item->signature));
    sb_appendf(sb, "</code></pre>\n");
    sb_appendf(sb, "<div class=\"description\">%s</div>\n", item->description);
    sb_appendf(sb, "</div>\n\n");
}

static void write_llm_header(String_Builder *sb) {
    sb_append_cstr(sb, "# QLeii API Reference\n\n");
    sb_append_cstr(sb, "QLeii is a simple interpreted stack-based language.\n\n");
}

static void write_item_llm(String_Builder *sb, Api_Item *item) {
    sb_appendf(sb, "## %s\n\n", item->name);
    sb_appendf(sb, "%s\n\n", item->description);
    sb_appendf(sb, "```c\n%s\n```\n\n", item->signature);
}

int main(void) {
    Api_List api = {0};
    
    parse_header("qleei.h", &api);
    
    mkdir_if_not_exists("docs");
    
    {
        String_Builder html = {0};
        write_html_header(&html);
        
        sb_append_cstr(&html, "<h2>Functions</h2>\n");
        for (size_t i = 0; i < api.count; i++) {
            if (strncmp(api.items[i].signature, "static", 6) != 0 &&
                strncmp(api.items[i].signature, "typedef", 7) != 0) {
                sb_appendf(&html, "<a href=\"#%s\">%s</a>\n", api.items[i].name, api.items[i].name);
            }
        }
        
        sb_append_cstr(&html, "<h2>Types</h2>\n");
        for (size_t i = 0; i < api.count; i++) {
            if (strncmp(api.items[i].signature, "typedef", 7) == 0) {
                sb_appendf(&html, "<a href=\"#%s\">%s</a>\n", api.items[i].name, api.items[i].name);
            }
        }
        
        write_html_footer(&html);
        
        write_section_header(&html, "functions", "Functions", "I am error. Just kidding, these actually work.");
        for (size_t i = 0; i < api.count; i++) {
            if (strncmp(api.items[i].signature, "static", 6) != 0 &&
                strncmp(api.items[i].signature, "typedef", 7) != 0) {
                write_item_html(&html, &api.items[i]);
            }
        }
        write_section_footer(&html);
        
        write_section_header(&html, "types", "Types", "What does this mean?! What does this mean?! WHAT DOES THIS MEAN?!");
        for (size_t i = 0; i < api.count; i++) {
            if (strncmp(api.items[i].signature, "typedef", 7) == 0) {
                write_item_html(&html, &api.items[i]);
            }
        }
        write_section_footer(&html);
        
        sb_append_cstr(&html, "</main>\n</div>\n</body>\n</html>\n");
        
        write_entire_file("docs/index.html", html.items, html.count);
        sb_free(html);
    }
    
    {
        String_Builder llm = {0};
        write_llm_header(&llm);
        
        for (size_t i = 0; i < api.count; i++) {
            write_item_llm(&llm, &api.items[i]);
        }
        
        sb_append_null(&llm);
        write_entire_file("docs/llm.md", llm.items, llm.count - 1);
        sb_free(llm);
    }
    
    for (size_t i = 0; i < api.count; i++) {
        free(api.items[i].name);
        free(api.items[i].signature);
        free(api.items[i].description);
    }
    free(api.items);
    
    printf("Generated docs/index.html and docs/llm.md\n");
    return 0;
}
