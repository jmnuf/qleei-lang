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
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; line-height: 1.6; color: #333; background: #fff0f5; }\n"
        ".container { display: flex; min-height: 100vh; }\n"
        ".sidebar { width: 280px; background: #4a1259; color: #fff; padding: 20px; overflow-y: auto; position: fixed; height: 100vh; }\n"
        ".sidebar h1 { font-size: 1.5rem; margin-bottom: 20px; color: #fff; }\n"
        ".sidebar h2 { font-size: 0.9rem; text-transform: uppercase; color: #ffb6c1; margin: 20px 0 10px; letter-spacing: 1px; }\n"
        ".sidebar a { color: #fff; text-decoration: none; display: block; padding: 4px 0; font-size: 0.9rem; }\n"
        ".sidebar a:hover { color: #ff69b4; }\n"
        ".main { flex: 1; margin-left: 280px; padding: 40px; }\n"
        ".item { background: #fff; border-radius: 8px; padding: 24px; margin-bottom: 24px; box-shadow: 0 2px 4px rgba(74,18,89,0.1); border: 1px solid #ffb6c1; }\n"
        ".item h2 { color: #4a1259; font-size: 1.3rem; margin-bottom: 8px; font-family: 'Courier New', monospace; }\n"
        ".item .signature { background: #f8f0f8; padding: 12px; border-radius: 4px; font-family: 'Courier New', monospace; font-size: 0.85rem; color: #4a1259; overflow-x: auto; margin-bottom: 12px; border: 1px solid #e0d0e0; }\n"
        ".item .description { color: #444; }\n"
        ".item .description p { margin-bottom: 8px; }\n"
        ".item .description code { background: #f8f0f8; padding: 2px 6px; border-radius: 3px; font-family: 'Courier New', monospace; font-size: 0.85rem; color: #8b008b; }\n"
        ".item .description pre { background: #4a1259; color: #fff; padding: 12px; border-radius: 4px; overflow-x: auto; margin: 12px 0; }\n"
        ".item .description pre code { background: none; padding: 0; color: inherit; }\n"
        "@media (max-width: 768px) { .sidebar { width: 100%; height: auto; position: relative; } .main { margin-left: 0; } }\n"
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

static void write_item_html(String_Builder *sb, Api_Item *item) {
    sb_appendf(sb, "<div class=\"item\" id=\"%s\">\n", item->name);
    sb_appendf(sb, "<h2>%s</h2>\n", item->name);
    sb_appendf(sb, "<pre class=\"signature\">%s</pre>\n", item->signature);
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
        
        for (size_t i = 0; i < api.count; i++) {
            write_item_html(&html, &api.items[i]);
        }
        
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
