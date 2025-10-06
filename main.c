#include "platform.h"

#define return_defer(value) do { result = value; goto defer; } while(0)

typedef enum {
  TOKEN_KIND_NONE = 0,
  TOKEN_KIND_EOF,
  TOKEN_KIND_IDENTIFIER,
  TOKEN_KIND_NUMBER,
  TOKEN_KIND_SYMBOL,
} Token_Kind;

typedef struct {
  Token_Kind kind;
  uisz line;
  uisz column;

  String_View string;
  double number;
} Token;

typedef struct {
  const char *buffer;
  uisz buffer_len;

  uisz index;
  uisz line;
  uisz column;

  Token token;
} Lexer;

typedef enum {
  VALUE_KIND_NUMBER,
  VALUE_KIND_POINTER,
} Value_Kind;

typedef union {
  Value_Kind kind;
  
  struct {
    Value_Kind kind;
    double value;
  } as_number;

  struct {
    Value_Kind kind;
    char *value;
  } as_pointer;
} Value_Item;

typedef struct {
  Value_Item *items;
  uisz len;
  uisz cap;
} Stack;

define_list_methods(Value_Item, Stack)

typedef struct {
  Token *items;
  uisz len;
  uisz cap;

  String_View name_sv;

  struct {
    Value_Kind *items;
    uisz len;
    uisz cap;
  } inputs;

  struct {
    Value_Kind *items;
    uisz len;
    uisz cap;
  } outputs;
} Proc;

define_list_methods(Token, Proc)

typedef struct {
  Proc *items;
  uisz len;
  uisz cap;
} Procs;

define_list_methods(Proc, Procs)

Proc *find_proc_by_sv_name(Procs *haystack, String_View needle) {
  list_foreach(Proc, it, haystack) {
    if (sv_eq_sv(it->name_sv, needle)) {
      return it;
    }
  }
  return NULL;
}

typedef struct {
  Lexer  lexer;
  Stack  stack;
  Procs  procs;
  bool   done;
} Interpreter;

bool is_space_char(char c) {
  return c == '\t' || c == '\n' || c == '\r' || c == ' ';
}

bool is_number_char(char c) {
  return '0' <= c && c <= '9';
}

bool is_alphabetic_char(char c) {
  return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

bool is_identifier_start_char(char c) {
  return is_alphabetic_char(c) || c == '_';
}

bool is_identifier_char(char c) {
  return is_identifier_start_char(c) || is_number_char(c);
}

Lexer l = {0};
Stack stack = {0};

const char *get_token_kind_name(Token_Kind kind) {
  switch(kind) {
  case TOKEN_KIND_NONE:
    return "None";
  case TOKEN_KIND_EOF:
    return "End of File";
  case TOKEN_KIND_IDENTIFIER:
    return "Identifier";
  case TOKEN_KIND_NUMBER:
    return "Number";
  case TOKEN_KIND_SYMBOL:
    return "Symbol";
  }
  return "<Unknown>";
}

const char *get_value_kind_name(Value_Kind kind) {
  switch (kind) {
  case VALUE_KIND_NUMBER:
    return "number";
  case VALUE_KIND_POINTER:
    return "pointer";
  }
  return "<Unknown>";
}

void lexer_init(Lexer *l, const char *buffer, uisz buf_size) {
  l->buffer = buffer;
  l->buffer_len = buf_size;
  l->index = 0;
  l->line = 1;
  l->column = 0;
  l->token = (Token) { .string = { .data = buffer, .len = 0 } };
  l->token.string.data = buffer;
}

bool lexer_next(Lexer *lexer) {
  lexer->token.kind = TOKEN_KIND_NONE;
  if (lexer->buffer == NULL) {
    platform_printfn("[ERROR] Attempting to lex into a NULL buffer");
    return false;
  }

  while (lexer->index < lexer->buffer_len) {
    char c = lexer->buffer[lexer->index++];
    while (is_space_char(c)) {
      if (c == '\n') {
	lexer->line++;
	lexer->column = 0;
      } else {
	lexer->column++;
      }
      c = lexer->buffer[lexer->index++];
    }

    lexer->token.line = lexer->line;
    lexer->token.column = lexer->column;

    if (is_number_char(c)) {
      lexer->token.string.data = lexer->buffer + (lexer->index - 1);
      lexer->token.string.len = 0;
      bool decimal = false;
      while (is_number_char(c)) {
	lexer->column++;
	lexer->token.string.len++;
	c = lexer->buffer[lexer->index++];
	if (c == '.' && !decimal) {
	  decimal = true;
	  c = lexer->buffer[lexer->index++];
	}
      }
      lexer->token.kind = TOKEN_KIND_NUMBER;
      lexer->token.number = parse_number(lexer->token.string);
      return true;
    }

    if (is_identifier_start_char(c)) {
      lexer->token.string.data = lexer->buffer + (lexer->index-1);
      lexer->token.string.len = 0;
      while (is_identifier_char(c)) {
	lexer->token.string.len++;
	c = lexer->buffer[lexer->index++];
	lexer->column++;
      }
      lexer->index -= 1;
      lexer->column--;
      lexer->token.kind = TOKEN_KIND_IDENTIFIER;
      return true;
    }

    // If somehow the last char is a null terminator we just ignore it
    if (lexer->index >= lexer->buffer_len) {
      if (c == 0) break;
    }
    
    lexer->token.kind = TOKEN_KIND_SYMBOL;
    lexer->token.string.data = lexer->buffer + (lexer->index - 1);
    lexer->token.string.len = 1;

    if (c == '-') {
      if (lexer->index < lexer->buffer_len && lexer->buffer[lexer->index] == '>') {
	lexer->index += 2;
	lexer->column += 2;
	lexer->token.string.len = 2;
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

  lexer->token.kind = TOKEN_KIND_EOF;
  return true;
}

bool lexer_peek(Lexer *l, Token *t) {
  Lexer peeker = *l;
  if (!lexer_next(&peeker)) return false;
  *t = peeker.token;
  return true;
}

void print_stack(Stack *s) {
  platform_printf("[ ");
  for (uisz i = 0; i < s->len; ++i) {
    if (i > 0) platform_printf(", ");
    switch (s->items[i].kind) {
    case VALUE_KIND_NUMBER:
      platform_printf("Number(%.4f)", s->items[i].as_number.value);
      break;
    case VALUE_KIND_POINTER:
      platform_printf("Pointer(%p)", s->items[i].as_pointer.value);
      break;
    }
  }
  platform_printf(" ]\n");
}

bool stack_operation_requires_n_items(Stack *s, String_View sv, uisz n) {
  if (s->len < n) {
    platform_printfn("[ERROR] "SV_Fmt_Str" requires %zu items in the stack to execute", SV_Fmt_Arg(sv), n);
    return false;
  }
  return true;
}

bool action_expects_value_kind(String_View sv, Value_Kind got, Value_Kind exp) {
  if (got == exp) return true;
  platform_printfn("[ERROR] Invalid type passed to "SV_Fmt_Str" expected pointer", SV_Fmt_Arg(sv));
  return false;
}


bool parse_proc(Interpreter *it) {
  Lexer *l = &it->lexer;
  if (!lexer_next(l)) return false;
  if (l->token.kind != TOKEN_KIND_IDENTIFIER) {
    platform_printfn("%zu:%zu: [ERROR] Procedure is required to be given a name after 'proc' keyword", l->token.line, l->token.column);
    return false;
  }

  String_View name_sv = l->token.string;
  Proc proc = {0};
  proc.name_sv = name_sv;

  if (!lexer_next(l)) return false;

  // ==================================================
  // Parse Inputs
  // --------------------------------------------------
  if (!sv_eq_zstr(l->token.string, "[")) {
    platform_printfn("%zu:%zu: [ERROR] After procedure name must specify inputs & outputs like `[] -> []`", l->token.line, l->token.column);
    platform_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
    return false;
  }

  while (!sv_eq_zstr(l->token.string, "]")) {
    if (!lexer_next(l)) {
      platform_printfn("%zu:%zu: [ERROR] After procedure name must specify inputs & outputs like `[] -> []`", l->token.line, l->token.column);
      platform_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
      return false;
    }
    if (sv_eq_zstr(l->token.string, "]")) break;

    if (l->token.kind != TOKEN_KIND_IDENTIFIER) {
      platform_printfn("%zu:%zu: [ERROR] Unexpected %s token when expecting identifier for type name", l->token.line, l->token.column, get_token_kind_name(l->token.kind));
      platform_printfn("[NOTE] Token source: '"SV_Fmt_Str"'", SV_Fmt_Arg(l->token.string));
      return false;
    }

    if (sv_eq_zstr(l->token.string, "pointer") || sv_eq_zstr(l->token.string, "ptr")) {
      alist_append(&proc.inputs, VALUE_KIND_POINTER);
    } else if (sv_eq_zstr(l->token.string, "number")) {
      alist_append(&proc.inputs, VALUE_KIND_NUMBER);
    } else {
      platform_printfn("%zu:%zu: [ERROR] Invalid type name only 'pointer', 'ptr', and 'number' types exist", l->token.line, l->token.column);
      return false;
    }

    Token t = {0};
    if (!lexer_peek(l, &t)) return false;

    if (sv_eq_zstr(t.string, ",") || sv_eq_zstr(t.string, "]")) {
      lexer_next(l);
    }
  }

  if (!sv_eq_zstr(l->token.string, "]")) {
    platform_printfn("%zu:%zu: [ERROR] After procedure name must specify inputs & outputs like `[] -> []`", l->token.line, l->token.column);
    platform_printfn("[NOTE] Token source: '"SV_Fmt_Str"'", SV_Fmt_Arg(l->token.string));
    platform_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
    return false;
  }
  if (!lexer_next(l)) return false;


  if (!sv_eq_zstr(l->token.string, "->")) {
    platform_printfn("%zu:%zu: [ERROR] Expected arrow symbol '->' between proc inputs and outputs", l->token.line, l->token.column);
    return false;
  }
  if (!lexer_next(l)) return false;
  
  // ==================================================
  // Parse Outputs
  // --------------------------------------------------
  if (!sv_eq_zstr(l->token.string, "[")) {
    platform_printfn("%zu:%zu: [ERROR] After procedure inputs outputs must be specified: <input> -> <output>", l->token.line, l->token.column);
    platform_printfn("[NOTE] Found '"SV_Fmt_Str"' but expected '['", SV_Fmt_Arg(l->token.string));
    return false;
  }

  while (l->token.kind != TOKEN_KIND_SYMBOL || !sv_eq_zstr(l->token.string, "]")) {
    if (!lexer_next(l)) {
      platform_printfn("%zu:%zu: [ERROR] After procedure inputs outputs must be specified", l->token.line, l->token.column);
      return false;
    }
    if (sv_eq_zstr(l->token.string, "]")) break;

    if (l->token.kind != TOKEN_KIND_IDENTIFIER) {
      platform_printfn("%zu:%zu: [ERROR] Unexpected %s token when expecting identifier for type name", l->token.line, l->token.column, get_token_kind_name(l->token.kind));
      return false;
    }

    if (sv_eq_zstr(l->token.string, "pointer") || sv_eq_zstr(l->token.string, "ptr")) {
      alist_append(&proc.outputs, VALUE_KIND_POINTER);
    } else if (sv_eq_zstr(l->token.string, "number")) {
      alist_append(&proc.outputs, VALUE_KIND_NUMBER);
    } else {
      platform_printfn("%zu:%zu: [ERROR] Invalid type name only 'pointer', 'ptr', and 'number' types exist", l->token.line, l->token.column);
      return false;
    }

    Token t = {0};
    if (!lexer_peek(l, &t)) return false;

    if (sv_eq_zstr(t.string, ",")) {
      lexer_next(l);
    }
  }

  if (!sv_eq_zstr(l->token.string, "]")) {
    platform_printfn("%zu:%zu: [ERROR] After procedure inputs outputs must be specified", l->token.line, l->token.column);
    platform_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
    return false;
  }


  // ==================================================
  // Parse Body
  // --------------------------------------------------
  while (true) {
    if (!lexer_next(l)) return false;
    if (l->token.kind == TOKEN_KIND_NONE) {
      platform_printfn("[UNREACHABLE] Lexer.Token.kind == TOKEN_KIND_NONE in parse_proc");
      return false;
    }
    if (l->token.kind == TOKEN_KIND_EOF) {
      platform_printfn("[ERROR] Procedure is missing to finish with 'end' keyword");
      return false;
    }
    if (l->token.kind == TOKEN_KIND_IDENTIFIER) {
      if (sv_eq_zstr(l->token.string, "end")) {
	break;
      }
    }

    Proc_append(&proc, l->token);
  }

  Procs_append(&it->procs, proc);

  return true;
}

bool execute_proc(Interpreter *it, Proc *proc);

bool execute_token(Interpreter *it, bool inside_of_proc, Token t) {
  Stack *stack = &it->stack;
  String_View sv = t.string;

  switch (t.kind) {
  case TOKEN_KIND_NONE:
    platform_printfn("[UNREACHABLE] interpreter_step switch (lexer.kind) case TOKEN_KIND_NONE");
    return false;

  case TOKEN_KIND_EOF:
    it->done = true;
    return true;

  case TOKEN_KIND_IDENTIFIER:
    if (sv_eq_zstr(sv, "proc")) {
      if (inside_of_proc) {
	platform_printfn("[ERROR] Cannot define a procedure while inside of a procedure");
	return false;
      }
      if (!parse_proc(it)) return false;
      return true;
    }

    // [number] -> []
    if (sv_eq_zstr(sv, "print_number")) {
      if (!stack_operation_requires_n_items(stack, sv, 1)) return false;
      Value_Item item;
      Stack_pop(stack, &item);
      switch (item.kind) {
      case VALUE_KIND_NUMBER:
	platform_printfn("%.4f", item.as_number.value);
	break;
      case VALUE_KIND_POINTER:
	platform_printfn("%zu", (uisz)item.as_pointer.value);
	break;
      }
      return true;
    }

    // [number] -> []
    if (sv_eq_zstr(sv, "print_char")) {
      if (!stack_operation_requires_n_items(stack, sv, 1)) return false;
      Value_Item item;
      Stack_pop(stack, &item);
      if (item.kind != VALUE_KIND_NUMBER) {
	platform_printfn("[ERROR] Invalid item type passed to ");
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

    // [] -> []
    if (sv_eq_zstr(sv, "print_stack")) {
      print_stack(stack);
      return true;
    }

    // [pointer] -> []
    if (sv_eq_zstr(sv, "print_zstr")) {
      if (!stack_operation_requires_n_items(stack, sv, 1)) return false;
      Value_Item item;
      Stack_pop(stack, &item);
      if (item.kind != VALUE_KIND_POINTER) {
	platform_printfn("[ERROR] Invalid type passed to "SV_Fmt_Str" expected pointer", SV_Fmt_Arg(sv));
	return false;
      }
      char *zstr = item.as_pointer.value;
      platform_printfn("%s", zstr);
      return true;
    }

    // [T] -> [T, T]
    if (sv_eq_zstr(sv, "dup")) {
      if (!stack_operation_requires_n_items(stack, sv, 1)) return false;
      Value_Item n = *Stack_last(stack);
      Stack_append(stack, n);
      return true;
    }

    // [a, b] -> [b, a]
    if (sv_eq_zstr(sv, "rot2")) {
      if (!stack_operation_requires_n_items(stack, sv, 2)) return false;
      Stack_swap(stack, stack->len - 1, stack->len - 2);
      return true;
    }

    // [a, b, c] -> [c, a, b]
    if (sv_eq_zstr(sv, "rot3")) {
      if (!stack_operation_requires_n_items(stack, sv, 3)) return false;
      Stack_swap(stack, stack->len - 1, stack->len - 3);
      Stack_swap(stack, stack->len - 1, stack->len - 2);
      return true;
    }

    // [] -> - | [T] -> !
    if (sv_eq_zstr(sv, "assert_empty")) {
      if (stack->len != 0) {
	platform_printfn("[ERROR] Expected stack to be empty but %zu elements remain in it", stack->len);
	print_stack(stack);
      }
      return true;
    }

    // [number(uisz)] -> [ptr]
    if (sv_eq_zstr(sv, "mem_alloc")) {
      if (!stack_operation_requires_n_items(stack, sv, 1)) return false;
      Value_Item item;
      Stack_pop(stack, &item);
      if (item.kind != VALUE_KIND_NUMBER) {
	platform_printfn("[ERROR] Invalid type passed to "SV_Fmt_Str" expected number", SV_Fmt_Arg(sv));
	return false;
      }
      uisz n = (uisz)item.as_number.value;
      void *ptr = platform_mem_alloc(n);
      item.as_pointer.kind = VALUE_KIND_POINTER;
      item.as_pointer.value = ptr;
      Stack_append(stack, item);
      return true;
    }

    // [ptr] -> -
    if (sv_eq_zstr(sv, "mem_free")) {
      if (!stack_operation_requires_n_items(stack, sv, 1)) return false;
      Value_Item item;
      Stack_pop(stack, &item);
      if (item.kind != VALUE_KIND_POINTER) {
	platform_printfn("[ERROR] Invalid type passed to "SV_Fmt_Str" expected pointer", SV_Fmt_Arg(sv));
	return false;
      }
      void *ptr = item.as_pointer.value;
      platform_mem_free(ptr);
      return true;
    }

    // [ptr, ui8] -> -
    if (sv_eq_zstr(sv, "mem_save_ui8")) {
      if (!stack_operation_requires_n_items(stack, sv, 2)) return false;
      Value_Item ptr_item, val_item;

      Stack_pop(stack, &ptr_item);
      if (ptr_item.kind != VALUE_KIND_POINTER) {
	platform_printfn("[ERROR] "SV_Fmt_Str" requires a pointer at the top of the stack", SV_Fmt_Arg(sv));
	return false;
      }

      Stack_pop(stack, &val_item);
      if (val_item.kind != VALUE_KIND_NUMBER) {
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
      if (!stack_operation_requires_n_items(stack, sv, 1)) return false;
      Value_Item item;
      Stack_pop(stack, &item);
      if (!action_expects_value_kind(sv, item.kind, VALUE_KIND_POINTER)) return false;
      char *ptr = item.as_pointer.value;
      unsigned char val = (unsigned char)*ptr;
      item.as_number.kind  = VALUE_KIND_NUMBER;
      item.as_number.value = (double)val;
      Stack_append(stack, item);
      return true;
    }

    // [ptr, ui32] -> -
    if (sv_eq_zstr(sv, "mem_save_ui32")) {
      if (!stack_operation_requires_n_items(stack, sv, 2)) return false;
      Value_Item ptr_item, val_item;
      if (!action_expects_value_kind(sv, ptr_item.kind, VALUE_KIND_POINTER)) return false;
      if (!action_expects_value_kind(sv, val_item.kind, VALUE_KIND_NUMBER)) return false;
      ui32 *ptr = (ui32*)ptr_item.as_pointer.value;
      ui32  val = (ui32)val_item.as_number.value;
      *ptr = val;
      return true;
    }

    // [ptr] -> [ui32]
    if (sv_eq_zstr(sv, "mem_load_ui32")) {
      if (!stack_operation_requires_n_items(stack, sv, 1)) return false;
      Value_Item item;
      if (!action_expects_value_kind(sv, item.kind, VALUE_KIND_POINTER)) return false;
      ui32 val = *(ui32*)item.as_pointer.value;
      item.as_number.kind  = VALUE_KIND_NUMBER;
      item.as_number.value = val;
      Stack_append(stack, item);
      return true;
    }


    {
      Proc *proc = find_proc_by_sv_name(&it->procs, sv);
      if (proc != NULL) {
	if (!stack_operation_requires_n_items(stack, sv, proc->inputs.len)) return false;
	for (uisz i = 0; i < proc->inputs.len; ++i) {
	  Value_Kind received = (stack->items[(proc->inputs.len - (i + 1))]).kind;
	  Value_Kind expected = proc->inputs.items[i];
	  if (received != expected) {
	    platform_printfn("[ERROR] Proc "SV_Fmt_Str" expected %s but got %s", SV_Fmt_Arg(proc->name_sv), get_value_kind_name(expected), get_value_kind_name(received));
	    return false;
	  }
	}
	execute_proc(it, proc);
	return true;
      }
    }

    platform_printfn("[ERROR] Unknown command/identifier provided: '%.*s'", (int)sv.len, sv.data);
    return false;

  case TOKEN_KIND_NUMBER:
    Stack_append(stack, (Value_Item){ .as_number = { .kind = VALUE_KIND_NUMBER, .value = t.number } });
    return true;

  case TOKEN_KIND_SYMBOL:
    if (sv_eq_zstr(sv, "+")) {
      if (!stack_operation_requires_n_items(stack, sv, 2)) return false;
      Value_Item a, b;
      Stack_pop(stack, &a);
      Stack_pop(stack, &b);

      if (a.kind == VALUE_KIND_POINTER && b.kind == VALUE_KIND_NUMBER) {
	char *ptr = a.as_pointer.value;
	uisz  n   = (uisz)b.as_number.value;
	ptr += n;
	a.as_pointer.value = ptr;
	Stack_append(stack, a);
	return true;
      } else if (b.kind == VALUE_KIND_POINTER && a.kind == VALUE_KIND_NUMBER) {
	char *ptr = b.as_pointer.value;
	uisz  n   = (uisz)a.as_number.value;
	ptr += n;
	b.as_pointer.value = ptr;
	Stack_append(stack, b);
	return true;
      }
      
      a.as_number.value = a.as_number.value + b.as_number.value;
      Stack_append(stack, a);
      return true;
    }

    if (sv_eq_zstr(sv, "-")) {
      if (!stack_operation_requires_n_items(stack, sv, 2)) return false;
      Value_Item a, b;
      Stack_pop(stack, &a);
      Stack_pop(stack, &b);

      if (a.kind == VALUE_KIND_POINTER && b.kind == VALUE_KIND_NUMBER) {
	char *ptr = a.as_pointer.value;
	uisz  n   = (uisz)b.as_number.value;
	ptr += n;
	a.as_pointer.value = ptr;
	Stack_append(stack, a);
	return true;
      } else if (b.kind == VALUE_KIND_POINTER && a.kind == VALUE_KIND_NUMBER) {
	char *ptr = b.as_pointer.value;
	uisz  n   = (uisz)a.as_number.value;
	ptr += n;
	b.as_pointer.value = ptr;
	Stack_append(stack, b);
	return true;
      }

      a.as_number.value = a.as_number.value - b.as_number.value;
      Stack_append(stack, a);
      return true;
    }

    if (sv_eq_zstr(sv, "/")) {
      if (!stack_operation_requires_n_items(stack, sv, 2)) return false;
      Value_Item a, b;
      Stack_pop(stack, &a);
      Stack_pop(stack, &b);
      
      if (a.kind == VALUE_KIND_POINTER || b.kind == VALUE_KIND_POINTER) {
	platform_printfn("[ERROR] Cannot do division with pointers");
	return false;
      }

      a.as_number.value = a.as_number.value / b.as_number.value;
      Stack_append(stack, a);
      return true;
    }

    if (sv_eq_zstr(sv, "*")) {
      if (!stack_operation_requires_n_items(stack, sv, 2)) return false;
      Value_Item a, b;
      Stack_pop(stack, &a);
      Stack_pop(stack, &b);

      if (a.kind == VALUE_KIND_POINTER || b.kind == VALUE_KIND_POINTER) {
	platform_printfn("[ERROR] Cannot do division with pointers");
	return false;
      }

      a.as_number.value = a.as_number.value * b.as_number.value;
      Stack_append(stack, a);
      return true;
    }

    if (sv_eq_zstr(sv, "!")) {
      print_stack(stack);
      platform_printfn("[SYSTEM] ABORTED");
      return false;
    }

    platform_printfn("proc.qll:%zu:%zu: [ERROR] Unsupported symbol '"SV_Fmt_Str"'", t.line, t.column, SV_Fmt_Arg(sv));
    return false;

  }

  platform_printfn("[UNREACHABLE] interpreter_step switch (lexer.kind) case %s", get_token_kind_name(t.kind));
  return false;
}


bool execute_proc(Interpreter *it, Proc *proc) {
  list_foreach(Token, pToken, proc) {
    if (!execute_token(it, true, *pToken)) return false;
  }
  return true;
}

bool interpreter_step(Interpreter *it) {
  if (!lexer_next(&it->lexer)) return false;

  if (!execute_token(it, false, it->lexer.token)) return false;

  return true;
}

void interpreter_lexer_init(Interpreter *it, const char *buffer, uisz buf_size) {
  lexer_init(&it->lexer, buffer, buf_size);
  it->stack.len = 0;
}

Interpreter *alloc_new_interpreter(const char *buffer, uisz buf_size) {
  Interpreter it = {0};
  if (buffer != NULL) lexer_init(&it.lexer, buffer, buf_size);

  return platform_mem_copy(
    platform_mem_alloc(sizeof(Interpreter)),
    &it,
    sizeof(Interpreter)
  );
}

bool interpreter_exec(Interpreter *it) {
  while (interpreter_step(it)) {
    if (it->done) return true;
  }
  return false;
}


bool interpret_buffer(const char *buffer, uisz buf_size) {
  bool result = false;

  Interpreter it = {0};
  interpreter_lexer_init(&it, buffer, buf_size);

  while (interpreter_step(&it)) {
    if (it.done) return_defer(true);
  }

defer:
  Stack_free(&it.stack);
  return result;
}

#ifdef PLATFORM_DESKTOP

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

  if (!interpret_buffer(sb.items, sb.count)) return 1;

  return 0;
}

#endif // PLATFORM_DESKTOP

#define PLATFORM_IMPLEMENTATION
#include "platform.h"

