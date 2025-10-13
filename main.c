#include "qleei.h"

typedef struct {
  const char *file_path;
  uisz index;
  uisz line;
  uisz column;
} QLeei_Lex_Location;

#define qleei_loc_printfn(loc, ...) \
do { platform_printf("%s:%zu:%zu: ", (loc).file_path, (loc).line, (loc).column); platform_printfn(__VA_ARGS__); } while (0)

typedef enum {
  QLEEI_TOKEN_KIND_NONE = 0,
  QLEEI_TOKEN_KIND_EOF,
  QLEEI_TOKEN_KIND_IDENTIFIER,
  QLEEI_TOKEN_KIND_NUMBER,
  QLEEI_TOKEN_KIND_BOOL,
  QLEEI_TOKEN_KIND_SYMBOL,
} Qleei_Token_Kind;

typedef struct {
  Qleei_Token_Kind kind;
  QLeei_Lex_Location loc;

  String_View string;
  double number;
} QLeei_Token;

typedef struct {
  const char *input_path;
  const char *buffer;
  uisz buffer_len;

  uisz index;
  uisz line;
  uisz column;

  QLeei_Token token;
} QLeei_Lexer;

typedef enum {
  QLEEI_VALUE_KIND_NUMBER,
  QLEEI_VALUE_KIND_POINTER,
  QLEEI_VALUE_KIND_BOOL,
} Qleei_Value_Kind;

typedef union {
  Qleei_Value_Kind kind;
  
  struct {
    Qleei_Value_Kind kind;
    double value;
  } as_number;

  struct {
    Qleei_Value_Kind kind;
    char *value;
  } as_pointer;

  struct {
    Qleei_Value_Kind kind;
    bool value;
  } as_bool;
} Qleei_Value_Item;

typedef struct {
  Qleei_Value_Item *items;
  uisz len;
  uisz cap;
} Qleei_Stack;

define_list_methods(Qleei_Value_Item, Qleei_Stack)

typedef struct {
  QLeei_Lex_Location body_start;
  QLeei_Lex_Location body_end;

  String_View name_sv;

  struct {
    Qleei_Value_Kind *items;
    uisz len;
    uisz cap;
  } inputs;

  struct {
    Qleei_Value_Kind *items;
    uisz len;
    uisz cap;
  } outputs;
} Qleei_Proc;

typedef struct {
  Qleei_Proc *items;
  uisz len;
  uisz cap;
} Qleei_Procs;

define_list_methods(Qleei_Proc, Qleei_Procs)

Qleei_Proc *qleei_find_proc_by_sv_name(Qleei_Procs *haystack, String_View needle) {
  list_foreach(Qleei_Proc, it, haystack) {
    if (sv_eq_sv(it->name_sv, needle)) {
      return it;
    }
  }
  return NULL;
}

typedef struct {
  QLeei_Lexer  lexer;
  Qleei_Stack  stack;
  Qleei_Procs  procs;
  bool   done;
} Qleei_Interpreter;

bool qleei_is_space_char(char c) {
  return c == '\t' || c == '\n' || c == '\r' || c == ' ';
}

bool qleei_is_number_char(char c) {
  return '0' <= c && c <= '9';
}

bool qleei_is_alphabetic_char(char c) {
  return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

bool qleei_is_identifier_start_char(char c) {
  return qleei_is_alphabetic_char(c) || c == '_';
}

bool qleei_is_identifier_char(char c) {
  return qleei_is_identifier_start_char(c) || qleei_is_number_char(c);
}

const char *qleei_get_token_kind_name(Qleei_Token_Kind kind) {
  switch(kind) {
  case QLEEI_TOKEN_KIND_NONE:
    return "None";
  case QLEEI_TOKEN_KIND_EOF:
    return "End of File";
  case QLEEI_TOKEN_KIND_IDENTIFIER:
    return "Identifier";
  case QLEEI_TOKEN_KIND_NUMBER:
    return "Number";
  case QLEEI_TOKEN_KIND_BOOL:
    return "Bool";
  case QLEEI_TOKEN_KIND_SYMBOL:
    return "Symbol";
  }
  return "<Unknown>";
}

const char *qleei_get_value_kind_name(Qleei_Value_Kind kind) {
  switch (kind) {
  case QLEEI_VALUE_KIND_NUMBER:
    return "number";
  case QLEEI_VALUE_KIND_POINTER:
    return "pointer";
  case QLEEI_VALUE_KIND_BOOL:
    return "bool";
  }
  return "<Unknown>";
}

void qleei_lexer_init(QLeei_Lexer *l, const char *input_path, const char *buffer, uisz buf_size) {
  l->input_path = input_path;
  l->buffer = buffer;
  l->buffer_len = buf_size;
  l->index = 0;
  l->line = 1;
  l->column = 1;
  l->token = (QLeei_Token) { .string = { .data = buffer, .len = 0 } };
}

bool qleei_lexer_next(QLeei_Lexer *lexer) {
  lexer->token.loc.file_path = lexer->input_path;
  lexer->token.kind = QLEEI_TOKEN_KIND_NONE;
  if (lexer->buffer == NULL) {
    platform_printfn("[ERROR] Attempting to lex into a NULL buffer");
    return false;
  }

  while (lexer->index < lexer->buffer_len) {
    char c = lexer->buffer[lexer->index++];
    while (qleei_is_space_char(c)) {
      if (c == '\n') {
	      lexer->line++;
	      lexer->column = 1;
      } else {
	      lexer->column++;
      }
      c = lexer->buffer[lexer->index++];
    }

    // If somehow the last char is a null terminator we just ignore it
    if (lexer->index >= lexer->buffer_len) {
      if (c == 0) break;
    }

    // Ignore any unmanaged ASCII control character
    if (c < 32) {
      platform_printfn("[WARN] Unexpected control character found in input: %d", (int)c);
      continue;
    }

    QLeei_Token *token = &lexer->token;
    token->loc.line = lexer->line;
    token->loc.column = lexer->column;
    QLeei_Lex_Location loc = token->loc;

    if (qleei_is_number_char(c)) {
      token->string.data = lexer->buffer + (lexer->index - 1);
      token->string.len = 0;
      bool decimal = false;
      while (qleei_is_number_char(c)) {
	      lexer->column++;
	      token->string.len++;
        if (lexer->index >= lexer->buffer_len) break;

	      c = lexer->buffer[lexer->index++];
	      if (c == '.' && !decimal) {
	        decimal = true;
          if (lexer->index >= lexer->buffer_len) {
            decimal = false;
            break;
          }
	        c = lexer->buffer[lexer->index++];
	      }
      }
      lexer->index--;
      // lexer->column++;
      token->kind = QLEEI_TOKEN_KIND_NUMBER;
      token->number = parse_number(token->string);
      return true;
    }

    if (qleei_is_identifier_start_char(c)) {
      token->string.data = lexer->buffer + (lexer->index-1);
      token->string.len = 0;
      while (qleei_is_identifier_char(c)) {
	      token->string.len++;
        if (lexer->index >= lexer->buffer_len) break;
	      c = lexer->buffer[lexer->index++];
	      lexer->column++;
      }
      lexer->index -= 1;

      if (sv_eq_zstr(token->string, "true")) {
	      token->kind = QLEEI_TOKEN_KIND_BOOL;
	      token->number = 1;
      } else if (sv_eq_zstr(token->string, "false")) {
	      token->kind = QLEEI_TOKEN_KIND_BOOL;
	      token->number = 0;
      } else {
	      token->kind = QLEEI_TOKEN_KIND_IDENTIFIER;
      }

      return true;
    }

    if (c == '\'') {
      token->kind = QLEEI_TOKEN_KIND_NUMBER;
      token->string.data = lexer->buffer + (lexer->index - 1);
      token->string.len = 3;

      if (lexer->index >= lexer->buffer_len) {
        qleei_loc_printfn(loc, "[LexError] Unterminated ASCII char literal");
        return false;
      }
      lexer->column++;
      loc.column++;

      c = lexer->buffer[lexer->index++];
      token->number = (double)c;
      if (lexer->index >= lexer->buffer_len) {
        qleei_loc_printfn(loc, "[LexError] Unterminated ASCII char literal");
        return false;
      }

      loc.column++;
      lexer->column++;
      c = lexer->buffer[lexer->index++];
      if (c != '\'') {
        qleei_loc_printfn(loc, "[LexError] ASCII char literal does not end with ' or takes more than 1 byte");
        return false;
      }
      lexer->column++;
      return true;
    }
    
    token->kind = QLEEI_TOKEN_KIND_SYMBOL;
    token->string.data = lexer->buffer + (lexer->index - 1);
    token->string.len = 1;
    lexer->column++;

    if (c == '-') {
      if (lexer->index < lexer->buffer_len && lexer->buffer[lexer->index] == '>') {
	      lexer->index++;
	      lexer->column++;
	      token->string.len = 2;
	      return true;
      }
    }

    if (*(lexer->buffer + (lexer->index - 1)) == '/') {
      if (lexer->buffer_len > lexer->index && lexer->buffer[lexer->index] == '/') {
	      while (lexer->index < lexer->buffer_len && lexer->buffer[lexer->index] != '\n') {
	        lexer->column++;
	        lexer->index++;
	      }
	      continue;
      }
    }
    
    return true;
  }

  lexer->token.kind = QLEEI_TOKEN_KIND_EOF;
  return true;
}

bool qleei_lexer_peek(QLeei_Lexer *l, QLeei_Token *t) {
  QLeei_Lexer peeker = *l;
  if (!qleei_lexer_next(&peeker)) return false;
  *t = peeker.token;
  return true;
}

QLeei_Lex_Location qleei_lexer_save_point(QLeei_Lexer *l) {
  QLeei_Lex_Location save_point = {
    .file_path = l->input_path,
    .index = l->index,
    .line = l->line,
    .column = l->column,
  };
  return save_point;
}

bool qleei_lexer_restore_point(QLeei_Lexer *l, QLeei_Lex_Location save_point) {
  l->input_path = save_point.file_path;
  l->index = save_point.index;
  l->line = save_point.line;
  l->column = save_point.column;
  return true;
}

void qleei_print_stack(Qleei_Stack *s) {
  platform_printf("[ ");
  for (uisz i = 0; i < s->len; ++i) {
    uisz index = s->len - (i + 1);
    Qleei_Value_Item item = s->items[index];
    if (i > 0) platform_printf(", ");
    switch (item.kind) {
    case QLEEI_VALUE_KIND_NUMBER:
      platform_printf("Number(%.4f)", item.as_number.value);
      break;
    case QLEEI_VALUE_KIND_BOOL:
      platform_printf("Bool(%s)", item.as_bool.value ? "true" : "false");
      break;
    case QLEEI_VALUE_KIND_POINTER:
      platform_printf("Pointer(%p)", item.as_pointer.value);
      break;
    }
  }
  platform_printf(" ]\n");
}

bool qleei_stack_operation_requires_n_items(QLeei_Lex_Location loc, Qleei_Stack *s, String_View sv, uisz n) {
  if (s->len < n) {
    qleei_loc_printfn(loc, "[ERROR] "SV_Fmt_Str" requires %zu items in the stack to execute", SV_Fmt_Arg(sv), n);
    return false;
  }
  return true;
}

bool qleei_action_expects_value_kind(String_View sv, Qleei_Value_Kind got, Qleei_Value_Kind exp) {
  if (got == exp) return true;
  platform_printfn("[TYPE_ERROR] Invalid type passed to "SV_Fmt_Str" expected pointer", SV_Fmt_Arg(sv));
  return false;
}

double qleei_value_item_as_number(Qleei_Value_Item item) {
  switch (item.kind) {
  case QLEEI_VALUE_KIND_NUMBER:
    return item.as_number.value;
  case QLEEI_VALUE_KIND_BOOL:
    return (double)item.as_bool.value;
  case QLEEI_VALUE_KIND_POINTER:
    return (double)(uisz)item.as_pointer.value;
  }
  return 0.0;
}

bool qleei_value_item_as_bool(Qleei_Value_Item item) {
  switch (item.kind) {
  case QLEEI_VALUE_KIND_NUMBER:
    return item.as_number.value != 0;
  case QLEEI_VALUE_KIND_BOOL:
    return item.as_bool.value;
  case QLEEI_VALUE_KIND_POINTER:
    return item.as_pointer.value != NULL;
  }
  return false;
}

bool qleei_execute_token(Qleei_Interpreter *it, bool inside_of_proc, QLeei_Token t);


// Expects word 'while' to have been consumed already
bool qleei_execute_while(Qleei_Interpreter *it, bool inside_of_proc) {
  QLeei_Lexer *l = &it->lexer;
  QLeei_Lex_Location start_point = qleei_lexer_save_point(l);
  QLeei_Lex_Location end_point = {0};
  bool end_point_found = false;
  while (qleei_lexer_next(l)) {
    QLeei_Token t = l->token;
    if (sv_eq_zstr(t.string, "begin")) {
      if (it->stack.len == 0) {
	      platform_printfn("[ERROR] While loop requires at least one element on the stack to do evaluation but nothing is on the stack");
	      return false;
      }

      Qleei_Value_Item item;
      Qleei_Stack_pop(&it->stack, &item);
      if (!qleei_value_item_as_bool(item)) {
	      if (end_point_found) {
	        qleei_lexer_restore_point(l, end_point);
	        return true;
	      }
	      uisz level = 1;
	      while (level > 0) {
	        if (!qleei_lexer_next(l)) return false;
	        t = l->token;
	        if (sv_eq_zstr(t.string, "while")) {
	          level++;
	        } else if (sv_eq_zstr(t.string, "end")) {
	          level--;
	        }
	      }
	      return true;
      }

      continue;
    }

    if (sv_eq_zstr(t.string, "end")) {
      if (!end_point_found) {
	      end_point_found = true;
	      end_point = qleei_lexer_save_point(l);
      }
      qleei_lexer_restore_point(l, start_point);
      continue;
    }

    if (!qleei_execute_token(it, inside_of_proc, t)) return false;
  }

  return false;
}


bool qleei_parse_proc(Qleei_Interpreter *it) {
  QLeei_Lexer *l = &it->lexer;
  if (!qleei_lexer_next(l)) return false;
  if (l->token.kind != QLEEI_TOKEN_KIND_IDENTIFIER) {
    platform_printfn("%zu:%zu: [ERROR] Procedure is required to be given a name after 'proc' keyword", l->token.loc.line, l->token.loc.column);
    return false;
  }

  String_View name_sv = l->token.string;
  Qleei_Proc proc = {0};
  proc.name_sv = name_sv;

  if (!qleei_lexer_next(l)) return false;

  // ==================================================
  // Parse Inputs
  // --------------------------------------------------
  if (!sv_eq_zstr(l->token.string, "[")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] After procedure name must specify inputs & outputs like `[] -> []`");
    platform_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
    return false;
  }

  while (!sv_eq_zstr(l->token.string, "]")) {
    if (!qleei_lexer_next(l)) {
      qleei_loc_printfn(l->token.loc, "[ERROR] After procedure name must specify inputs & outputs like `[] -> []`");
      platform_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
      return false;
    }
    if (sv_eq_zstr(l->token.string, "]")) break;

    if (l->token.kind != QLEEI_TOKEN_KIND_IDENTIFIER) {
      qleei_loc_printfn(l->token.loc, "[ERROR] Unexpected %s token when expecting identifier for type name", qleei_get_token_kind_name(l->token.kind));
      platform_printfn("[NOTE] Token source: '"SV_Fmt_Str"'", SV_Fmt_Arg(l->token.string));
      return false;
    }

    if (sv_eq_zstr(l->token.string, "pointer") || sv_eq_zstr(l->token.string, "ptr")) {
      alist_append(&proc.inputs, QLEEI_VALUE_KIND_POINTER);
    } else if (sv_eq_zstr(l->token.string, "number")) {
      alist_append(&proc.inputs, QLEEI_VALUE_KIND_NUMBER);
    } else if (sv_eq_zstr(l->token.string, "bool")) {
      alist_append(&proc.outputs, QLEEI_VALUE_KIND_BOOL);
    } else {
      qleei_loc_printfn(l->token.loc, "[ERROR] Invalid type name only 'pointer', 'ptr', and 'number' types exist");
      return false;
    }

    QLeei_Token t = {0};
    if (!qleei_lexer_peek(l, &t)) return false;

    if (sv_eq_zstr(t.string, ",") || sv_eq_zstr(t.string, "]")) {
      qleei_lexer_next(l);
    }
  }

  if (!sv_eq_zstr(l->token.string, "]")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] After procedure name must specify inputs & outputs like `[] -> []`");
    platform_printfn("[NOTE] Token source: '"SV_Fmt_Str"'", SV_Fmt_Arg(l->token.string));
    platform_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
    return false;
  }
  if (!qleei_lexer_next(l)) return false;


  if (!sv_eq_zstr(l->token.string, "->")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] Expected arrow symbol '->' between proc inputs and outputs");
    return false;
  }
  if (!qleei_lexer_next(l)) return false;
  
  // ==================================================
  // Parse Outputs
  // --------------------------------------------------
  if (!sv_eq_zstr(l->token.string, "[")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] After procedure inputs outputs must be specified: <input> -> <output>");
    platform_printfn("[NOTE] Found '"SV_Fmt_Str"' but expected '['", SV_Fmt_Arg(l->token.string));
    return false;
  }

  while (l->token.kind != QLEEI_TOKEN_KIND_SYMBOL || !sv_eq_zstr(l->token.string, "]")) {
    if (!qleei_lexer_next(l)) {
      qleei_loc_printfn(l->token.loc, "[ERROR] After procedure inputs outputs must be specified");
      return false;
    }
    if (sv_eq_zstr(l->token.string, "]")) break;

    if (l->token.kind != QLEEI_TOKEN_KIND_IDENTIFIER) {
      qleei_loc_printfn(l->token.loc, "[ERROR] Unexpected %s token when expecting identifier for type name", qleei_get_token_kind_name(l->token.kind));
      return false;
    }

    if (sv_eq_zstr(l->token.string, "pointer") || sv_eq_zstr(l->token.string, "ptr")) {
      alist_append(&proc.outputs, QLEEI_VALUE_KIND_POINTER);
    } else if (sv_eq_zstr(l->token.string, "number")) {
      alist_append(&proc.outputs, QLEEI_VALUE_KIND_NUMBER);
    } else if (sv_eq_zstr(l->token.string, "bool")) {
      alist_append(&proc.outputs, QLEEI_VALUE_KIND_BOOL);
    } else {
      qleei_loc_printfn(l->token.loc, "[ERROR] Invalid type name only 'pointer', 'ptr', and 'number' types exist");
      return false;
    }

    QLeei_Token t = {0};
    if (!qleei_lexer_peek(l, &t)) return false;

    if (sv_eq_zstr(t.string, ",")) {
      qleei_lexer_next(l);
    }
  }

  if (!sv_eq_zstr(l->token.string, "]")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] After procedure inputs outputs must be specified");
    platform_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
    return false;
  }


  // ==================================================
  // Parse Body
  // --------------------------------------------------
  proc.body_start = qleei_lexer_save_point(l);
  uisz level = 1;
  while (true) {
    if (!qleei_lexer_next(l)) return false;
    if (l->token.kind == QLEEI_TOKEN_KIND_NONE) {
      platform_printfn("[UNREACHABLE] Lexer.Token.kind == QLEEI_TOKEN_KIND_NONE in parse_proc");
      return false;
    }
    if (l->token.kind == QLEEI_TOKEN_KIND_EOF) {
      platform_printfn("[ERROR] Procedure is missing to finish with 'end' keyword");
      return false;
    }
    if (l->token.kind == QLEEI_TOKEN_KIND_IDENTIFIER) {
      if (sv_eq_zstr(l->token.string, "while")) {
	      level++;
      } else if (sv_eq_zstr(l->token.string, "end")) {
	      level--;
	      if (level == 0) {
          proc.body_end = qleei_lexer_save_point(l);
          break;
        }
      }
    }
  }

  Qleei_Procs_append(&it->procs, proc);

  return true;
}

bool qleei_execute_proc(Qleei_Interpreter *it, Qleei_Proc *proc);

bool qleei_execute_token(Qleei_Interpreter *it, bool inside_of_proc, QLeei_Token t) {
  Qleei_Stack *stack = &it->stack;
  String_View sv = t.string;

  switch (t.kind) {
  case QLEEI_TOKEN_KIND_NONE:
    platform_printfn("[UNREACHABLE] qleei_interpreter_step switch (lexer.kind) case QLEEI_TOKEN_KIND_NONE");
    return false;

  case QLEEI_TOKEN_KIND_EOF:
    it->done = true;
    return true;

  case QLEEI_TOKEN_KIND_IDENTIFIER:
    if (sv_eq_zstr(sv, "proc")) {
      if (inside_of_proc) {
	      platform_printfn("[ERROR] Cannot define a procedure while inside of a procedure");
	      return false;
      }
      if (!qleei_parse_proc(it)) return false;
      return true;
    }

    if (sv_eq_zstr(sv, "if")) {
      platform_printfn("[TODO] if expressions are not implemented yet");
      return false;
    }

    if (sv_eq_zstr(sv, "while")) {
      if (!qleei_execute_while(it, inside_of_proc)) return false;
      return true;
    }

    // [number] -> []
    if (sv_eq_zstr(sv, "print_number")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      Qleei_Stack_pop(stack, &item);
      switch (item.kind) {
      case QLEEI_VALUE_KIND_NUMBER:
	      platform_printfn("%.4f", item.as_number.value);
	      break;
      case QLEEI_VALUE_KIND_BOOL:
	      platform_printfn("%d", (int)item.as_bool.value);
	      break;
      case QLEEI_VALUE_KIND_POINTER:
	      platform_printfn("%zu", (uisz)item.as_pointer.value);
	      break;
      }
      return true;
    }

    // [number] -> []
    if (sv_eq_zstr(sv, "print_uisz")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      Qleei_Stack_pop(stack, &item);
      switch (item.kind) {
      case QLEEI_VALUE_KIND_NUMBER:
	      platform_printfn("%zu", (uisz)item.as_number.value);
	      break;
      case QLEEI_VALUE_KIND_BOOL:
	      platform_printfn("%zu", (uisz)item.as_bool.value);
	      break;
      case QLEEI_VALUE_KIND_POINTER:
	      platform_printfn("%zu", (uisz)item.as_pointer.value);
	      break;
      }
      return true;
    }

    // [pointer] -> []
    if (sv_eq_zstr(sv, "print_ptr")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      Qleei_Stack_pop(stack, &item);
      if (!qleei_action_expects_value_kind(sv, item.kind, QLEEI_VALUE_KIND_POINTER)) return false;
      void *ptr = item.as_pointer.value;
      platform_printfn("%p", ptr);
      return true;
    }

    // [number] -> []
    if (sv_eq_zstr(sv, "print_char")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      Qleei_Stack_pop(stack, &item);
      if (item.kind != QLEEI_VALUE_KIND_NUMBER) {
	      platform_printfn("[ERROR] Invalid item type passed to print_char");
	      return false;
      }
      double n = item.as_number.value;
      if (n > 255) {
	      platform_printfn("[ERROR] Attempting to read a number as a char that exceeds the char limit of 255: %zu", (uisz)n);
	      return false;
      }
      char c = (char)n;
      platform_printfn("%c", c);
      return true;
    }

    // [bool] -> []
    if (sv_eq_zstr(sv, "print_bool")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      Qleei_Stack_pop(stack, &item);
      if (item.kind != QLEEI_VALUE_KIND_BOOL) {
	      platform_printfn("[ERROR] Invalid item type passed to print_bool");
	      return false;
      }
      platform_printfn(item.as_bool.value ? "true" : "false");
      return true;
    }

    // [] -> []
    if (sv_eq_zstr(sv, "print_stack")) {
      qleei_print_stack(stack);
      return true;
    }

    // [pointer] -> []
    if (sv_eq_zstr(sv, "print_zstr")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      Qleei_Stack_pop(stack, &item);
      if (item.kind != QLEEI_VALUE_KIND_POINTER) {
	      platform_printfn("[ERROR] Invalid type passed to "SV_Fmt_Str" expected pointer", SV_Fmt_Arg(sv));
	      return false;
      }
      char *zstr = item.as_pointer.value;
      platform_printfn("%s", zstr);
      return true;
    }

    // [T] -> [T, T]
    if (sv_eq_zstr(sv, "dup")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item n = *Qleei_Stack_last(stack);
      Qleei_Stack_append(stack, n);
      return true;
    }

    // [a, b] -> [b, a, b]
    if (sv_eq_zstr(sv, "over")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item n = stack->items[stack->len-2];
      Qleei_Stack_append(stack, n);
      return true;
    }

    // [T] -> []
    if (sv_eq_zstr(sv, "drop")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Stack_pop(stack, NULL);
      return true;
    }

    // [a, b] -> [b, a]
    if (sv_eq_zstr(sv, "rot2") || sv_eq_zstr(sv, "swap2")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Stack_swap(stack, stack->len - 1, stack->len - 2);
      return true;
    }

    // [a, b, c] -> [c, b, a]
    if (sv_eq_zstr(sv, "swap3")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 3)) return false;
      Qleei_Stack_swap(stack, stack->len - 1, stack->len - 3);
      return true;
    }

    // [a, b, c] -> [a, b, c]
    if (sv_eq_zstr(sv, "rot3")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 3)) return false;
      Qleei_Stack_swap(stack, stack->len - 1, stack->len - 3); // [a, b, c] -> [c, b, a]
      Qleei_Stack_swap(stack, stack->len - 1, stack->len - 2); // [c, b, a] -> [b, c, a]
      return true;
    }

    // [] -> - | [T] -> !
    if (sv_eq_zstr(sv, "assert_empty")) {
      if (stack->len != 0) {
	      platform_printfn("[ERROR] Expected stack to be empty but %zu elements remain in it", stack->len);
	      qleei_print_stack(stack);
      }
      return true;
    }

    // [number(uisz)] -> [ptr]
    if (sv_eq_zstr(sv, "mem_alloc")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      Qleei_Stack_pop(stack, &item);
      if (item.kind != QLEEI_VALUE_KIND_NUMBER) {
	      platform_printfn("[ERROR] Invalid type passed to "SV_Fmt_Str" expected number", SV_Fmt_Arg(sv));
	      return false;
      }
      uisz n = (uisz)item.as_number.value;
      void *ptr = platform_mem_alloc(n);
      item.as_pointer.kind = QLEEI_VALUE_KIND_POINTER;
      item.as_pointer.value = ptr;
      Qleei_Stack_append(stack, item);
      return true;
    }

    // [ptr] -> -
    if (sv_eq_zstr(sv, "mem_free")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      Qleei_Stack_pop(stack, &item);
      if (item.kind != QLEEI_VALUE_KIND_POINTER) {
	      platform_printfn("[ERROR] Invalid type passed to "SV_Fmt_Str" expected pointer", SV_Fmt_Arg(sv));
	      return false;
      }
      void *ptr = item.as_pointer.value;
      platform_mem_free(ptr);
      return true;
    }

    if (sv_eq_zstr(sv, "mem_save_si8")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item ptr_item, val_item;

      Qleei_Stack_pop(stack, &ptr_item);
      if (!qleei_action_expects_value_kind(sv, ptr_item.kind, QLEEI_VALUE_KIND_POINTER)) return false;

      Qleei_Stack_pop(stack, &val_item);
      if (!qleei_action_expects_value_kind(sv, val_item.kind, QLEEI_VALUE_KIND_NUMBER)) return false;

      si8 *ptr = (si8*)ptr_item.as_pointer.value;
      si8  val =  (si8)val_item.as_number.value;

      *ptr = val;
      return true;
    }

    // [ptr, ui8] -> -
    if (sv_eq_zstr(sv, "mem_save_ui8")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item ptr_item, val_item;

      Qleei_Stack_pop(stack, &ptr_item);
      if (ptr_item.kind != QLEEI_VALUE_KIND_POINTER) {
	      qleei_loc_printfn(t.loc, "[ERROR] "SV_Fmt_Str" requires a pointer at the top of the stack", SV_Fmt_Arg(sv));
        platform_printf("[NOTE] Current stack: ");
        qleei_print_stack(stack);
	      return false;
      }

      Qleei_Stack_pop(stack, &val_item);
      if (val_item.kind != QLEEI_VALUE_KIND_NUMBER) {
	      platform_printfn("[ERROR] "SV_Fmt_Str" requires a number second to the top of the stack", SV_Fmt_Arg(sv));
	      return false;
      }

      ui8 *ptr = (ui8*)ptr_item.as_pointer.value;
      ui8  val = (ui8)val_item.as_number.value;

      *ptr = val;
      return true;
    }

    // [ptr] -> [number]
    if (sv_eq_zstr(sv, "mem_load_ui8")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      Qleei_Stack_pop(stack, &item);
      if (!qleei_action_expects_value_kind(sv, item.kind, QLEEI_VALUE_KIND_POINTER)) return false;
      char *ptr = item.as_pointer.value;
      unsigned char val = (unsigned char)*ptr;
      item.as_number.kind  = QLEEI_VALUE_KIND_NUMBER;
      item.as_number.value = (double)val;
      Qleei_Stack_append(stack, item);
      return true;
    }

    // [ptr, ui32] -> -
    if (sv_eq_zstr(sv, "mem_save_ui32")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item ptr_item, val_item;
      if (!qleei_action_expects_value_kind(sv, ptr_item.kind, QLEEI_VALUE_KIND_POINTER)) return false;
      if (!qleei_action_expects_value_kind(sv, val_item.kind, QLEEI_VALUE_KIND_NUMBER)) return false;
      ui32 *ptr = (ui32*)ptr_item.as_pointer.value;
      ui32  val = (ui32)val_item.as_number.value;
      *ptr = val;
      return true;
    }

    // [ptr] -> [ui32]
    if (sv_eq_zstr(sv, "mem_load_ui32")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 1)) return false;
      Qleei_Value_Item item;
      if (!qleei_action_expects_value_kind(sv, item.kind, QLEEI_VALUE_KIND_POINTER)) return false;
      ui32 val = *(ui32*)item.as_pointer.value;
      item.as_number.kind  = QLEEI_VALUE_KIND_NUMBER;
      item.as_number.value = val;
      Qleei_Stack_append(stack, item);
      return true;
    }


    {
      Qleei_Proc *proc = qleei_find_proc_by_sv_name(&it->procs, sv);
      if (proc != NULL) {
	      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, proc->inputs.len)) return false;
	      for (uisz i = 0; i < proc->inputs.len; ++i) {
	        Qleei_Value_Kind received = (stack->items[(proc->inputs.len - (i + 1))]).kind;
	        Qleei_Value_Kind expected = proc->inputs.items[i];
	        if (received != expected) {
	          platform_printfn("[ERROR] Proc "SV_Fmt_Str" expected %s but got %s", SV_Fmt_Arg(proc->name_sv), qleei_get_value_kind_name(expected), qleei_get_value_kind_name(received));
	          return false;
	        }
	      }
	      if (!qleei_execute_proc(it, proc)) return false;
	      return true;
      }
    }

    platform_printfn("[ERROR] Unknown command/identifier provided: '%.*s'", (int)sv.len, sv.data);
    return false;

  case QLEEI_TOKEN_KIND_NUMBER:
    Qleei_Stack_append(stack, (Qleei_Value_Item){ .as_number = { .kind = QLEEI_VALUE_KIND_NUMBER, .value = t.number } });
    return true;

  case QLEEI_TOKEN_KIND_BOOL:
    Qleei_Stack_append(stack, (Qleei_Value_Item){ .as_bool = { .kind = QLEEI_VALUE_KIND_BOOL, .value = t.number == 1.0 } });
    return true;

  case QLEEI_TOKEN_KIND_SYMBOL:
    if (sv_eq_zstr(sv, "+")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item a, b;
      Qleei_Stack_pop(stack, &a);
      Qleei_Stack_pop(stack, &b);
      
      if (a.kind == QLEEI_VALUE_KIND_POINTER && b.kind == QLEEI_VALUE_KIND_POINTER) {
	      platform_printfn("[ERROR] Cannot add 2 pointers together");
	      return false;
      }

      if (a.kind == QLEEI_VALUE_KIND_POINTER || b.kind == QLEEI_VALUE_KIND_POINTER) {
	      char *ptr;
	      uisz n;
	      if (a.kind == QLEEI_VALUE_KIND_POINTER) {
	        ptr = a.as_pointer.value;
	        n = (uisz)qleei_value_item_as_number(b);
	      } else {
	        ptr = b.as_pointer.value;
	        n = (uisz)qleei_value_item_as_number(a);
	      }
	      a.as_pointer.kind = QLEEI_VALUE_KIND_POINTER;
	      a.as_pointer.value = ptr + n;
	      Qleei_Stack_append(stack, a);
	      return true;
      }

      a.as_number.value = qleei_value_item_as_number(a) + qleei_value_item_as_number(b);
      Qleei_Stack_append(stack, a);
      return true;
    }

    if (sv_eq_zstr(sv, "-")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item a, b;
      Qleei_Stack_pop(stack, &a);
      Qleei_Stack_pop(stack, &b);

      if (a.kind == QLEEI_VALUE_KIND_POINTER && b.kind == QLEEI_VALUE_KIND_POINTER) {
	      platform_printfn("[ERROR] Cannot do subtraction between 2 pointers");
	      return false;
      }

      if (a.kind == QLEEI_VALUE_KIND_POINTER || b.kind == QLEEI_VALUE_KIND_POINTER) {
	      char *ptr;
	      uisz n;
	      if (a.kind == QLEEI_VALUE_KIND_POINTER) {
	        ptr = a.as_pointer.value;
	        n = (uisz)qleei_value_item_as_number(b);
	      } else {
	        ptr = b.as_pointer.value;
	        n = (uisz)qleei_value_item_as_number(a);
	      }

	      if (n > (uisz)ptr) {
	        platform_printfn("[ERROR] Pointer arithmetic ends results in a negative value");
	        return false;
	      }

	      a.as_pointer.kind  = QLEEI_VALUE_KIND_POINTER;
	      a.as_pointer.value = ptr - n;
	      Qleei_Stack_append(stack, a);
	      return true;
      }

      a.as_number.value = qleei_value_item_as_number(a) - qleei_value_item_as_number(b);
      Qleei_Stack_append(stack, a);
      return true;
    }

    if (sv_eq_zstr(sv, "/")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item a, b;
      Qleei_Stack_pop(stack, &a);
      Qleei_Stack_pop(stack, &b);
      
      if (a.kind == QLEEI_VALUE_KIND_POINTER || b.kind == QLEEI_VALUE_KIND_POINTER) {
	      platform_printfn("[ERROR] Cannot do division with pointers");
	      return false;
      }

      a.as_number.value = qleei_value_item_as_number(a) / qleei_value_item_as_number(b);
      Qleei_Stack_append(stack, a);
      return true;
    }

    if (sv_eq_zstr(sv, "*")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item a, b;
      Qleei_Stack_pop(stack, &a);
      Qleei_Stack_pop(stack, &b);

      if (a.kind == QLEEI_VALUE_KIND_POINTER || b.kind == QLEEI_VALUE_KIND_POINTER) {
	      qleei_loc_printfn(t.loc, "[ERROR] Cannot do division with pointers");
	      return false;
      }

      a.as_number.value = qleei_value_item_as_number(a) * qleei_value_item_as_number(b);
      Qleei_Stack_append(stack, a);
      return true;
    }

    if (sv_eq_zstr(sv, "!")) {
      qleei_loc_printfn(t.loc, "[SYSTEM] ABORTED");
      platform_printf("[NOTE] Stack state: ");
      qleei_print_stack(stack);
      return false;
    }

    qleei_loc_printfn(t.loc, "[ERROR] Unsupported symbol '"SV_Fmt_Str"'", SV_Fmt_Arg(sv));
    platform_printf("[INFO] Symbol bytes: [");
    sv_iter(c, sv) {
      if (c > sv.data) platform_printf(", ");
      platform_printf("%d", (int)*c);
    }
    platform_printfn("]");
    return false;

  }

  platform_printfn("[UNREACHABLE] qleei_interpreter_step switch (lexer.kind) case %s", qleei_get_token_kind_name(t.kind));
  return false;
}


bool qleei_execute_proc(Qleei_Interpreter *it, Qleei_Proc *proc) {
  QLeei_Lexer *l = &it->lexer;
  QLeei_Lex_Location save_point = qleei_lexer_save_point(l);
  
  qleei_lexer_restore_point(l, proc->body_start);

  bool ok = true;
  while ((ok = qleei_lexer_next(l))) {
    if (sv_eq_zstr(l->token.string, "end")) break;
    if (!qleei_execute_token(it, true, l->token)) return false;
  }

  if (ok) qleei_lexer_restore_point(l, save_point);

  return ok;
}

bool qleei_interpreter_step(Qleei_Interpreter *it) {
  if (!qleei_lexer_next(&it->lexer)) return false;

  if (!qleei_execute_token(it, false, it->lexer.token)) return false;

  return true;
}

void qleei_interpreter_lexer_init(Qleei_Interpreter *it, const char *input_path, const char *buffer, uisz buf_size) {
  qleei_lexer_init(&it->lexer, input_path, buffer, buf_size);
  it->stack.len = 0;
}

bool qleei_interpreter_exec(Qleei_Interpreter *it) {
  while (qleei_interpreter_step(it)) {
    if (it->done) return true;
  }
  return false;
}


bool qleei_interpret_buffer(const char *buffer_source_path, const char *buffer, uisz buf_size) {
  bool result = false;

  Qleei_Interpreter it = {0};
  qleei_interpreter_lexer_init(&it, buffer_source_path, buffer, buf_size);

  while (qleei_interpreter_step(&it)) {
    if (it.done) {
      result = true;
      break;
    }
  }

  Qleei_Stack_free(&it.stack);
  return result;
}

#if defined(PLATFORM_DESKTOP) //&& defined(QLEEI_INTERPRETER)

#include <stddef.h>
#include <stdalign.h>
#include <stdint.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

void usage(FILE *f, const char *program) {
  fprintf(f, "Usage: %s <input-file>\n", program);
}

int main(int argc, char **argv) {
  const char *program = nob_shift(argv, argc);

  if (argc == 0) {
    nob_log(NOB_ERROR, "No input was provided");
    usage(stderr, program);
    return 1;
  }
  const char *input_path = nob_shift(argv, argc);
  if (strcmp(input_path, "-h") == 0 || strcmp(input_path, "--help") == 0) {
    usage(stdout, program);
    return 0;
  }

  Nob_String_Builder sb = {0};

  if (!nob_read_entire_file(input_path, &sb)) return 1;

  if (sb.count > 0 && sb.items[sb.count - 1] == 0) sb.count--;

  if (!qleei_interpret_buffer(input_path, sb.items, sb.count)) return 1;

  return 0;
}

#endif // PLATFORM_DESKTOP

#define PLATFORM_IMPLEMENTATION
#include "platform.h"

