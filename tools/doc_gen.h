#ifndef DOC_GEN_H
#define DOC_GEN_H

#define NOB_DA_INIT_CAP 8
#define NOB_STRIP_PREFIX
#include "../nob.h"

typedef struct {
  String_Builder *pool;
  size_t index;
  size_t len;
} String_Pool_Index;

#define Null_String_Pool_Index ((String_Pool_Index){.pool = NULL})
#define Pooled_String(spi) (&(spi).pool->items[(spi).index])

typedef struct {
    String_Pool_Index name;
    String_Pool_Index signature;
    String_Pool_Index description;
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
    String_Pool_Index key;
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

extern String_Pool_Index unknown_name;

static void html_escape(String_Builder *sb, const char *text, size_t len) {
    for (size_t i = 0; i < len; i++) {
        switch (text[i]) {
            case '&': sb_append_cstr(sb, "&amp;"); break;
            case '<': sb_append_cstr(sb, "&lt;"); break;
            case '>': sb_append_cstr(sb, "&gt;"); break;
            case '"': sb_append_cstr(sb, "&quot;"); break;
            case '\'': sb_append_cstr(sb, "&#39;"); break;
            default: da_append(sb, text[i]);
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
        ".item .example { background: #0d0d14; padding: 12px; border-radius: 4px; font-family: 'Courier New', monospace; font-size: 0.85rem; color: #e0e0e0; overflow-x: auto; margin-bottom: 12px; border: 1px solid #6272a4; white-space: pre; }\n"
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

static void html_item(String_Builder *sb, Api_Item *item) {
    sb_appendf(sb, "<div class=\"item\" id=\"%s\">\n", Pooled_String(item->name));
    sb_appendf(sb, "<h2>%s</h2>\n", Pooled_String(item->name));
    sb_appendf(sb, "<pre class=\"signature\"><code>");
    html_escape(sb, Pooled_String(item->signature), item->signature.len);
    sb_appendf(sb, "</code></pre>\n");
    sb_appendf(sb, "<div class=\"description\">%s</div>\n", Pooled_String(item->description));
    sb_appendf(sb, "</div>\n\n");
}

static void html_sidebar_group(String_Builder *sb, Group *group) {
    if (group->key.pool) {
        sb_appendf(sb, "<details>\n");
        sb_appendf(sb, "<summary>%s (%zu)</summary>\n", Pooled_String(group->key), group->count);
        sb_appendf(sb, "<div class=\"group-items\">\n");
        for (size_t j = 0; j < group->count; j++) {
            sb_appendf(sb, "<a href=\"#%s\">%s</a>\n", Pooled_String(group->items[j].name), Pooled_String(group->items[j].name));
        }
        sb_appendf(sb, "</div>\n");
        sb_appendf(sb, "</details>\n");
    } else {
        sb_appendf(sb, "<a href=\"#%s\">%s</a>\n", Pooled_String(group->items[0].name), Pooled_String(group->items[0].name));
    }
}

static void html_content_group(String_Builder *sb, Group *group) {
    if (group->key.pool) {
        sb_appendf(sb, "<details class=\"group\">\n");
        sb_appendf(sb, "<summary>%s</summary>\n", Pooled_String(group->key));
        for (size_t j = 0; j < group->count; j++) {
            html_item(sb, &group->items[j]);
        }
        sb_appendf(sb, "</details>\n");
    } else {
        html_item(sb, &group->items[0]);
    }
}

static void md_header(String_Builder *sb) {
    sb_append_cstr(sb, "# QLeii API Reference\n\n");
    sb_append_cstr(sb, "QLeei is a simple interpreted stack-based language.\n\n");
}

static void md_item(String_Builder *sb, Api_Item *item) {
    sb_appendf(sb, "## %s\n\n", Pooled_String(item->name));
    sb_appendf(sb, "%s\n\n", Pooled_String(item->description));
    sb_appendf(sb, "```c\n%.*s\n```\n\n", (int)item->signature.len, Pooled_String(item->signature));
}

static void md_group(String_Builder *sb, Group *group) {
    if (group->key.pool) {
        sb_appendf(sb, "### %s\n\n", Pooled_String(group->key));
    }
    for (size_t j = 0; j < group->count; j++) {
        md_item(sb, &group->items[j]);
    }
}

static void html_index_open(String_Builder *sb, const char *title) {
    sb_append_cstr(sb,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    );
    sb_appendf(sb, "<title>QLeii - %s</title>\n", title);
    sb_append_cstr(sb,
        "<style>\n"
        "* { box-sizing: border-box; margin: 0; padding: 0; }\n"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; line-height: 1.6; color: #333; background: #fff0f5; min-height: 100vh; display: flex; flex-direction: column; }\n"
        ".container { max-width: 800px; margin: 0 auto; padding: 40px 20px; flex: 1; }\n"
        "h1 { color: #4a1259; font-size: 2.5rem; margin-bottom: 10px; text-align: center; }\n"
        ".tagline { text-align: center; color: #666; font-size: 1.1rem; margin-bottom: 40px; }\n"
        ".nav { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin-bottom: 40px; }\n"
        ".card { background: #fff; border-radius: 8px; padding: 30px; box-shadow: 0 2px 4px rgba(74,18,89,0.1); border: 1px solid #ffb6c1; transition: transform 0.2s, box-shadow 0.2s; text-decoration: none; color: inherit; display: block; }\n"
        ".card:hover { transform: translateY(-2px); box-shadow: 0 4px 8px rgba(74,18,89,0.15); }\n"
        ".card h2 { color: #4a1259; font-size: 1.3rem; margin-bottom: 8px; }\n"
        ".card p { color: #666; font-size: 0.95rem; }\n"
        ".card-icon { font-size: 2rem; margin-bottom: 10px; }\n"
        "footer { text-align: center; padding: 20px; color: #888; font-size: 0.9rem; border-top: 1px solid #ffb6c1; margin-top: auto; }\n"
        "footer a { color: #4a1259; text-decoration: none; }\n"
        "footer a:hover { color: #ff69b4; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<div class=\"container\">\n"
    );
    sb_appendf(sb, "<h1>%s</h1>\n", title);
}

static void html_index_close(String_Builder *sb) {
    sb_append_cstr(sb,
        "<footer>\n"
        "<a href=\"https://github.com/jmnuf/qleei-lang\">GitHub Repository</a> &middot;\n"
        "<a href=\"playground/\">Playground</a>\n"
        "</footer>\n"
        "</div>\n"
        "</body>\n"
        "</html>\n"
    );
}

static void html_index_card(String_Builder *sb, const char *href, const char *icon, const char *title, const char *desc) {
    sb_appendf(sb, "<a href=\"%s\" class=\"card\">\n", href);
    sb_appendf(sb, "<div class=\"card-icon\">%s</div>\n", icon);
    sb_appendf(sb, "<h2>%s</h2>\n", title);
    sb_appendf(sb, "<p>%s</p>\n", desc);
    sb_appendf(sb, "</a>\n");
}

#endif
