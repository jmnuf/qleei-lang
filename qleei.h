#ifndef _QLEEI_H
#define _QLEEI_H

// stdbool.h macros copied straight from the man page of stdbool.h
// Defined manually instead of importing since we disable std libraries in wasm export
#ifndef __bool_true_false_are_defined
#  define __bool_true_false_are_defined 1
#  define bool  _Bool
#  define true  1
#  define false 0
#endif

// Naming integer sizes from stdlib.h since we disable std libraries in wasm builds
typedef __SIZE_TYPE__                        qleei_uisz_t;
typedef signed char                          qleei_si8_t;
typedef unsigned char                        qleei_ui8_t;
typedef signed short int                     qleei_si16_t;
typedef unsigned short int                   qleei_ui16_t;
typedef signed int                           qleei_si32_t;
typedef unsigned int                         qleei_ui32_t;
#if __WORDSIZE == 64
typedef signed long int                      qleei_si64_t;
typedef unsigned long int                    qleei_ui64_t;
#else
__extension__ typedef signed long long int   qleei_si64_t;
__extension__ typedef unsigned long long int qleei_ui64_t;
#endif


#ifndef NULL
#  define NULL ((void*)0)
#endif // NULL

/**
 * A non-owning view into a string, consisting of a pointer and length.
 */
typedef struct {
  const char *data;
  qleei_uisz_t len;
} Qleei_String_View;

#define QLEEI_MAX_WORD_SIZE 64

#define QLEEI_SV_Fmt_Str     "%.*s"
#define QLEEI_SV_Fmt_Arg(sv) (int)sv.len, sv.data

/**
 * Copy a string view to a fixed-size buffer with null termination.
 *
 * @param sv String view to copy.
 * @param buf Destination buffer.
 * @param buf_size Size of the destination buffer.
 * @returns `true` if copied successfully, `false` if sv.len >= buf_size.
 */
bool qleei_cp_sv_to_buf(Qleei_String_View sv, char *buf, qleei_uisz_t buf_size);

/**
 * Create a Qleei_String_View that references a null-terminated C string.
 *
 * @param zstr Null-terminated C string to view. May be NULL.
 * @returns A Qleei_String_View whose `data` points to `zstr` and whose `len` is the number of bytes
 *          before the terminating NUL character. The view does not allocate or copy the string data.
 */
Qleei_String_View qleei_sv_from_zstr(const char *zstr);

/**
 * Determine whether a string view ends with a given suffix.
 *
 * @param sv String view to inspect.
 * @param suffix Suffix string view to check for.
 * @returns `true` if `sv` ends with `suffix`, `false` otherwise.
 */
bool qleei_sv_has_suffix(Qleei_String_View sv, Qleei_String_View suffix);

/**
 * Check whether a string view begins with a given prefix.
 *
 * @param sv The string view to test.
 * @param prefix The prefix to check for at the start of `sv`.
 * @returns `true` if `sv` begins with `prefix`, `false` otherwise.
 */
bool qleei_sv_has_prefix(Qleei_String_View sv, Qleei_String_View prefix);

/**
 * Compare a string view to a null-terminated C string for exact equality.
 * @param sv String view to compare.
 * @param zstr Null-terminated C string to compare against.
 * @returns `true` if `sv` and `zstr` have the same length and identical characters, `false` otherwise.
 */
bool qleei_sv_eq_zstr(Qleei_String_View sv, const char *zstr);

/**
 * Determine whether two string views contain exactly the same bytes.
 *
 * @returns `true` if both `sv_a` and `sv_b` have the same length and identical byte-for-byte contents, `false` otherwise.
 */
bool qleei_sv_eq_sv(Qleei_String_View sv_a, Qleei_String_View sv_b);

#define qleei_sv_iter(it, sv) for (const char *it = (sv).data; it < (sv).data + (sv).len; ++it)

/**
 * Compute the length of a null-terminated C string.
 *
 * @param zstr Pointer to a NUL-terminated string.
 * @returns The number of characters before the terminating NUL.
 */
qleei_uisz_t qleei_zstr_len(const char *zstr);
/**
 * Compare two null-terminated C strings for equality.
 *
 * @param za First null-terminated string to compare.
 * @param zb Second null-terminated string to compare.
 * @returns `true` if the strings contain the same characters and length, `false` otherwise.
 */
bool qleei_zstr_eq(const char *za, const char *zb);

/**
 * Duplicate a null-terminated C string.
 *
 * @param zstr Null-terminated string to duplicate. May be NULL.
 * @returns A newly allocated copy of `zstr`, or `NULL` if `zstr` is `NULL` or allocation fails.
 */
char *qleei_zstr_dup(const char *zstr);

/**
 * Parse a floating-point number from a string view.
 *
 * @param sv String view containing the numeric literal to parse.
 * @returns The parsed `double` value; if `sv` does not contain a valid numeric literal, returns `0.0`.
 */
double qleei_parse_number(Qleei_String_View sv);

/**
 * Ensure a generic dynamic array has capacity for at least `desired_capacity` items by growing (doubling) the allocation as needed.
 * @param items Pointer to the array pointer; updated to point to the reallocated memory on growth.
 * @param item_size Size in bytes of a single item in the array.
 * @param current_capacity Pointer to the current capacity (in number of items); updated to the new capacity on growth.
 * @param desired_capacity Minimum required capacity (in number of items).
 * @returns `true` if the array has capacity for at least `desired_capacity` items after the call, `false` if memory allocation failed.
 */
bool qleei_list_reserve(void **items, qleei_uisz_t item_size, qleei_uisz_t *current_capacity, qleei_uisz_t desired_capacity);

/**
 * Appends a raw item to a dynamically-sized byte array, growing the array if necessary.
 *
 * Ensures the underlying buffer has space for one more element, copies `item_size` bytes
 * from `item` into the buffer at the next position, and increments `*length`.
 *
 * @param items Pointer to the array buffer pointer; may be reallocated and updated.
 * @param item_size Size in bytes of a single item.
 * @param capacity Pointer to the current capacity (in items); updated if the buffer grows.
 * @param length Pointer to the current number of items; incremented on success.
 * @param item Pointer to the source bytes to append.
 * @return `true` if the item was appended successfully, `false` if the buffer could not be grown.
 */
bool qleei_list_append(void **items, qleei_uisz_t item_size, qleei_uisz_t *capacity, qleei_uisz_t *length, void *item);

/**
 * Get a pointer to the last element in a contiguous array of items.
 * @param items Pointer to the start of the array.
 * @param item_size Size in bytes of a single element in the array.
 * @param length Number of elements currently in the array.
 * @returns Pointer to the last element, or `NULL` when `length` is zero.
 */
void* qleei_list_last(void *items, qleei_uisz_t item_size, qleei_uisz_t length);

/**
 * Remove the last element from a raw array buffer.
 *
 * If the list is non-empty, copies the last element into `popped_item` (when non-NULL)
 * and decrements `*length`.
 *
 * @param items Pointer to the array buffer storing items contiguously.
 * @param item_size Size in bytes of each item stored in `items`.
 * @param length Pointer to the current number of items; updated to the new length on success.
 * @param popped_item Optional destination buffer where the popped item will be copied; may be NULL.
 * @returns `true` if an item was popped, `false` if the list was empty.
 */
bool qleei_list_pop(void *items, qleei_uisz_t item_size, qleei_uisz_t *length, void *popped_item);

/**
 * Swap two elements in a contiguous array buffer.
 *
 * Swaps the element at index `i` with the element at index `j` in the buffer pointed
 * to by `items`. The buffer is treated as `length` elements of size `item_size`
 * bytes each; the swap is performed in-place.
 *
 * @param items Pointer to the contiguous array buffer containing elements to swap.
 * @param item_size Size in bytes of a single element in the buffer.
 * @param i Index of the first element to swap.
 * @param j Index of the second element to swap.
 * @param length Number of elements in the buffer.
 * @returns `true` if the swap was performed or `i == j`, `false` if `items` is NULL
 *          or either index is out of range (greater than or equal to `length`).
 */
bool qleei_list_swap(void *items, qleei_uisz_t item_size, qleei_uisz_t i, qleei_uisz_t j, qleei_uisz_t length);

/**
 * Free a dynamic list buffer and reset its metadata.
 *
 * @param items Pointer to the list buffer pointer; if non-NULL the buffer is freed and `*items` is set to NULL.
 * @param capacity Pointer to the list capacity value; set to 0.
 * @param length Pointer to the list length value; set to 0.
 */
void qleei_list_free(void **items, qleei_uisz_t *capacity, qleei_uisz_t *length);

#define qleei_alist_reserve(alist, count) qleei_list_reserve((void**)&(alist)->items, sizeof(*(alist)->items), &(alist)->cap, count)
#define qleei_alist_append(alist, item)   qleei_list_append((void**)&(alist)->items, sizeof(*(alist)->items), &(alist)->cap, &(alist)->len, item)
#define qleei_alist_last(alist, T)        (T*)qleei_list_last((alist)->items, sizeof(*(alist)->items), (alist)->len)
#define qleei_alist_pop(alist, out)       qleei_list_pop((alist)->items, sizeof(*(alist)->items), &(alist)->len, out)
#define qleei_alist_swap(alist, i, j)     qleei_list_swap((alist)->items, sizeof(*(alist)->items), i, j, (alist)->len)
#define qleei_alist_free(alist)           qleei_list_free((void**)&(alist)->items, &(alist)->cap, &(alist)->len)
#define qleei_alist_foreach(T, it, list)  for (T *it = (list)->items; it < (list)->items + (list)->len; ++it)

/**
 * Allocate a block of memory of the given size.
 *
 * @param size Number of bytes to allocate.
 * @returns Pointer to the allocated memory, or `NULL` if allocation failed.
 */
void *qleei_mem_alloc   (qleei_uisz_t size);
/**
 * Resize an allocated memory block using the C runtime `realloc`.
 *
 * @param ptr Pointer to previously allocated memory block or `NULL`.
 * @param size New size in bytes for the memory block.
 * @returns Pointer to the resized memory block, or `NULL` if allocation failed.
 */
void *qleei_mem_realloc (void *ptr, qleei_uisz_t size);
/**
 * Free memory previously allocated through qleei's allocator.
 *
 * @param ptr Pointer to the block to free. If `ptr` is NULL no action is taken.
 */
void  qleei_mem_free    (void *ptr);
/**
 * Copy `count` bytes from `src` to `dest`.
 *
 * Performs a byte-wise copy of `count` bytes from `src` into `dest`.
 * Behavior is undefined if the source and destination memory regions overlap.
 *
 * @param dest Destination buffer that receives the copied bytes.
 * @param src Source buffer to copy bytes from.
 * @param count Number of bytes to copy.
 * @returns Pointer to `dest`.
 */
void *qleei_mem_copy    (void *dest, const void *src, qleei_uisz_t count);


/**
 * Check whether a character is ASCII whitespace used by the lexer.
 *
 * @param c Character to test.
 * @returns `true` if `c` is a tab (`'\t'`), newline (`'\n'`), carriage return (`'\r'`), or space (`' '`), `false` otherwise.
 */
static inline bool qleei_is_space_char(char c);

/**
 * Checks whether a character is a decimal digit.
 * @param c Character to test.
 * @returns `true` if `c` is between `'0'` and `'9'`, `false` otherwise.
 */
static inline bool qleei_is_number_char(char c);

/**
 * Check whether a character is an ASCII alphabetic letter.
 *
 * @param c Character to test.
 * @return `true` if `c` is in the range 'A'..'Z' or 'a'..'z', `false` otherwise.
 */
static inline bool qleei_is_alphabetic_char(char c);

/**
 * Determine if a character is valid as the first character of an identifier.
 * @param c Character to test.
 * @returns `true` if `c` is an alphabetic character, an underscore (`_`), a numeral (`#`) or an at sign (`@`), `false` otherwise.
 */
static inline bool qleei_is_identifier_start_char(char c);

/**
 * Determine whether a character is valid inside an identifier.
 *
 * @returns `true` if `c` is a letter, underscore, numeral, dash, at sign, or digit (i.e., a valid identifier character), `false` otherwise.
 */
static inline bool qleei_is_identifier_char(char c);


/**
 * A source location within a file, used for error reporting.
 */
typedef struct {
  const char *file_path;
  qleei_uisz_t index;
  qleei_uisz_t line;
  qleei_uisz_t column;
} QLeei_Lex_Location;

#define qleei_loc_printfn(loc, ...) \
do { qleei_printf("%s:%zu:%zu: ", (loc).file_path, (loc).line, (loc).column); qleei_printfn(__VA_ARGS__); } while (0)

/**
 * The kind of a token produced by the lexer.
 */
typedef enum {
  QLEEI_TOKEN_KIND_NONE = 0,
  QLEEI_TOKEN_KIND_EOF,
  QLEEI_TOKEN_KIND_IDENTIFIER,
  QLEEI_TOKEN_KIND_NUMBER,
  QLEEI_TOKEN_KIND_BOOL,
  QLEEI_TOKEN_KIND_SYMBOL,
} Qleei_Token_Kind;

/**
 * A token produced by the lexer, including its kind, source location, and associated data.
 */
typedef struct {
  Qleei_Token_Kind kind;
  QLeei_Lex_Location loc;

  Qleei_String_View string;
  double number;
} QLeei_Token;

const char *qleei_get_token_kind_name(Qleei_Token_Kind kind);


/**
 * The lexer state for tokenizing Qleei source code.
 */
typedef struct {
  const char *input_path;
  const char *buffer;
  qleei_uisz_t buffer_len;

  qleei_uisz_t index;
  qleei_uisz_t line;
  qleei_uisz_t column;

  QLeei_Token token;
} QLeei_Lexer;

/**
 * Initialize a lexer with the provided input buffer and path.
 *
 * Sets the lexer's buffer, buffer length, and resets scanning position and token state.
 *
 * @param l Pointer to the lexer to initialize.
 * @param input_path Human-readable source path used for location reporting (may be NULL).
 * @param buffer Pointer to the input character buffer to tokenize.
 * @param buf_size Length of the input buffer in bytes.
 */
void qleei_lexer_init(QLeei_Lexer *l, const char *input_path, const char *buffer, qleei_uisz_t buf_size);

/**
 * Advance the lexer to the next token and populate lexer->token with its kind, location, text, and numeric value when applicable.
 *
 * @param lexer Lexer state to advance; its index, line, column and token fields will be updated.
 * @returns `true` if a token (including EOF) was successfully produced and stored in `lexer->token`, `false` on a lexing error (for example when `lexer->buffer` is NULL or an unterminated/invalid character literal is encountered).
 */
bool qleei_lexer_next(QLeei_Lexer *lexer);

/**
 * Retrieve the next token without advancing the lexer state.
 *
 * The function inspects the next token produced by the lexer `l` and writes it
 * to `t`, leaving the original lexer `l` unchanged.
 *
 * @param l Lexer to peek into (not modified).
 * @param t Destination for the next token.
 * @returns `true` if the next token was retrieved into `t`, `false` if lexing failed.
 */
bool qleei_lexer_peek(QLeei_Lexer *l, QLeei_Token *t);

/**
 * Capture the lexer's current location so parsing can be resumed later.
 *
 * @param l Lexer whose current position will be saved.
 * @returns A QLeei_Lex_Location containing the lexer's current file_path, index, line, and column.
 */
QLeei_Lex_Location qleei_lexer_save_point(QLeei_Lexer *l);

/**
 * Restore the lexer's current position from a saved location.
 *
 * Sets the lexer's input_path, index, line, and column to the values in save_point.
 *
 * @param l Lexer to restore.
 * @param save_point Saved location providing file_path, index, line, and column.
 * @returns `true` if the lexer was restored (always `true`).
 */
bool qleei_lexer_restore_point(QLeei_Lexer *l, QLeei_Lex_Location save_point);


/**
 * The kind of a value on the Qleei stack.
 */
typedef enum {
  QLEEI_VALUE_KIND_NUMBER,
  QLEEI_VALUE_KIND_POINTER,
  QLEEI_VALUE_KIND_BOOL,
} Qleei_Value_Kind;

/**
 * Get a human-readable name for a value kind.
 *
 * @param kind The value kind to describe.
 * @returns A NUL-terminated string containing one of: "number", "pointer", "bool",
 *          or "<Unknown>" if `kind` is not recognized.
 */
const char *qleei_get_value_kind_name(Qleei_Value_Kind kind);


/**
 * A value that can be stored on the Qleei stack. Contains a kind tag and the actual value data.
 */
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

/**
 * Append a Qleei_Value_Kind to a dynamic array, growing the array if needed.
 *
 * @param items Pointer to the array pointer storing `Qleei_Value_Kind` elements; may be realloced.
 * @param cap Pointer to the current capacity of the array; updated on reallocation.
 * @param len Pointer to the current length (number of used elements); incremented on success.
 * @param item The value kind to append.
 * @returns `true` if the item was appended successfully, `false` on allocation or other failure.
 */
bool qleei_value_kind_list_append(Qleei_Value_Kind **items, qleei_uisz_t *cap, qleei_uisz_t *len, Qleei_Value_Kind item);


/**
 * The Qleei execution stack that holds values during interpretation.
 */
typedef struct {
  Qleei_Value_Item *items;
  qleei_uisz_t len;
  qleei_uisz_t cap;
} Qleei_Stack;

/**
 * Print the stack's contents to the configured output, displaying the top element first.
 *
 * Each item is printed in a human-readable form: Number(value) with four decimal places,
 * Bool(true/false), or Pointer(address). Corrupted or unknown kinds are shown as CorruptedValue(kind, value).
 *
 * @param s Stack whose contents will be printed.
 */
void qleei_print_stack(Qleei_Stack *s);

/**
 * Push an item onto the stack.
 *
 * @param stack Stack to push onto.
 * @param item Item to push.
 * @returns `true` if the item was pushed successfully, `false` if allocation failed.
 */
bool qleei_stack_push(Qleei_Stack *stack, Qleei_Value_Item item);

/**
 * Pop an item from the stack.
 *
 * @param stack Stack to pop from.
 * @param item Destination for the popped item.
 * @returns `true` if an item was popped, `false` if the stack was empty.
 */
bool qleei_stack_pop(Qleei_Stack *stack, Qleei_Value_Item *item);

typedef struct {
  QLeei_Lex_Location body_start;
  QLeei_Lex_Location body_end;

  Qleei_String_View name_sv;

  struct {
    Qleei_Value_Kind *items;
    qleei_uisz_t len;
    qleei_uisz_t cap;
  } inputs;

  struct {
    Qleei_Value_Kind *items;
    qleei_uisz_t len;
    qleei_uisz_t cap;
  } outputs;
} Qleei_Proc;

/**
 * A dynamic array of user-defined procedures.
 */
typedef struct {
  Qleei_Proc *items;
  qleei_uisz_t len;
  qleei_uisz_t cap;
} Qleei_Procs;

/**
 * Find a procedure by name in a list of procedures.
 *
 * @param haystack List of procedures to search.
 * @param needle Name to search for.
 * @returns Pointer to the matching procedure, or `NULL` if not found.
 */
Qleei_Proc *qleei_procs_find_by_sv_name(Qleei_Procs *haystack, Qleei_String_View needle);


typedef struct {
  void *user_data;
  Qleei_Stack *stack;
  Qleei_Procs *procs;
  QLeei_Token token;
  bool inside_proc;
} Qleei_Word_Handler_Opt;
typedef bool (*Qleei_Word_Handler)(Qleei_Word_Handler_Opt opt);

typedef struct {
  const char *key;
  struct {
    Qleei_Word_Handler handler;
    void *user_data;
  } val;
} Qleei_Custom_Word;

typedef struct {
  Qleei_Custom_Word *items;
  qleei_uisz_t len;
  qleei_uisz_t cap;
} Qleei_Custom_Words;

/**
 * Add or update a word in the custom words registry.
 *
 * @param w Registry to insert into or update.
 * @param word Null-terminated word name (must not be a builtin).
 * @param handler Function to invoke when the word is encountered.
 * @param user_data Arbitrary pointer forwarded to the handler; may be NULL.
 * @returns `true` on success, `false` if allocation failed or word is NULL/handler is NULL.
 */
bool qleei_custom_words_add(Qleei_Custom_Words *w, const char *word, Qleei_Word_Handler handler, void *user_data);

/**
 * Remove a word from the custom words registry.
 *
 * @param w Registry to modify.
 * @param word Word to remove.
 * @returns `true` if the word was removed, `false` if not found.
 */
bool qleei_custom_words_remove(Qleei_Custom_Words *w, const char *word);

/**
 * The main Qleei interpreter state, containing the lexer, stack, custom words, and user procedures.
 */
typedef struct {
  QLeei_Lexer  lexer;
  Qleei_Stack  stack;
  Qleei_Custom_Words words;
  Qleei_Procs  procs;
  bool   done;
} Qleei_Interpreter;

/**
 * Initialize the interpreter's lexer.
 *
 * @param it Interpreter to initialize.
 * @param input_path Path for error messages (may be NULL).
 * @param buffer Input buffer to lex.
 * @param buf_size Size of the input buffer.
 */
void qleei_interpreter_lexer_init(Qleei_Interpreter *it, const char *input_path, const char *buffer, qleei_uisz_t buf_size);

/**
 * Register a word with a handler.
 *
 * @param it Interpreter to register with.
 * @param word Word to register (must not be a builtin).
 * @param handler Function to call when the word is encountered.
 * @returns `true` if registered successfully, `false` if word is a builtin or on error.
 */
bool qleei_interpreter_register_word(Qleei_Interpreter *it, const char *word, Qleei_Word_Handler handler);

/**
 * Register a word with a handler and user data.
 *
 * @param it Interpreter to register with.
 * @param word Word to register (must not be a builtin).
 * @param handler Function to call when the word is encountered.
 * @param user_data Data to pass to the handler.
 * @returns `true` if registered successfully, `false` if word is a builtin or on error.
 */
bool qleei_interpreter_register_word_with_data(Qleei_Interpreter *it, const char *word, Qleei_Word_Handler handler, void *user_data);

/**
 * Unregister a word.
 *
 * @param it Interpreter to unregister from.
 * @param word Word to remove.
 * @returns `true` if removed successfully, `false` if not found.
 */
bool qleei_interpreter_unregister_word(Qleei_Interpreter *it, const char *word);

/**
 * Execute a single token.
 *
 * @param it Interpreter to step.
 * @returns `true` if the token was executed successfully, `false` on error.
 */
bool qleei_interpreter_step(Qleei_Interpreter *it);

/**
 * Run the interpreter until completion or error.
 *
 * @param it Interpreter to run.
 * @returns `true` if execution completed, `false` on error.
 */
bool qleei_interpreter_exec(Qleei_Interpreter *it);

/**
 * Reset the interpreter to its initial state.
 *
 * @param it Interpreter to reset.
 * @param input_path Path for error messages (may be NULL).
 * @param buffer Input buffer to lex.
 * @param buf_size Size of the input buffer.
 */
void qleei_interpreter_reset(Qleei_Interpreter *it, const char *input_path, const char *buffer, qleei_uisz_t buf_size);

/**
 * Clear all procedures and registered words from the interpreter.
 *
 * @param it Interpreter to clear.
 */
void qleei_interpreter_clear(Qleei_Interpreter *it);

/**
 * Free all memory allocated by the interpreter.
 *
 * @param it Interpreter to free.
 */
void qleei_interpreter_free(Qleei_Interpreter *it);

/**
 * Interpret a buffer as Qleei source and execute it until the program completes or an error occurs.
 *
 * @param buffer_source_path Path used for location reporting (may be NULL).
 * @param buffer Pointer to the source buffer to interpret.
 * @param buf_size Size in bytes of the source buffer.
 * @returns `true` if interpretation ran to completion without error, `false` otherwise.
 */
bool qleei_interpret_buffer(const char *buffer_source_path, const char *buffer, qleei_uisz_t buf_size);

/**
 * Verify the stack contains at least `n` items and report an error if it does not.
 *
 * If the stack has fewer than `n` items, prints a location-prefixed error mentioning
 * the action named by `sv` and the required item count.
 *
 * @param loc Location used for the error message.
 * @param s Stack to validate.
 * @param sv String view naming the action that requires items on the stack.
 * @param n Required number of items.
 * @returns `true` if the stack has at least `n` items, `false` otherwise.
 */
bool qleei_stack_operation_requires_n_items(QLeei_Lex_Location loc, Qleei_Stack *s, Qleei_String_View sv, qleei_uisz_t n);

/**
 * Validate that a value kind matches the expected kind and report a type error at the given source location if it does not.
 *
 * If `got` and `exp` differ, prints a location-prefixed type error referencing `sv` that states the expected and actual kinds.
 *
 * @param loc Source location used for the error message.
 * @param sv String view identifying the action or value being checked.
 * @param got The actual value kind observed.
 * @param exp The value kind expected.
 * @returns `true` if `got` equals `exp`, `false` otherwise.
 */
bool qleei_action_expects_value_kind(QLeei_Lex_Location loc, Qleei_String_View sv, Qleei_Value_Kind got, Qleei_Value_Kind exp);

/**
 * Convert a Qleei_Value_Item to a numeric double representation.
 *
 * @param item Value item to convert.
 * @returns The numeric representation of `item`: the stored number for `QLEEI_VALUE_KIND_NUMBER`,
 * `1.0` if a `QLEEI_VALUE_KIND_BOOL` is true and `0.0` if false, or the pointer's address cast
 * to an unsigned integer then to `double` for `QLEEI_VALUE_KIND_POINTER`. Returns `0.0` for any
 * unrecognized kind.
 */
double qleei_value_item_as_number(Qleei_Value_Item item);

/**
 * Convert a Qleei_Value_Item to its boolean interpretation.
 * @param item Value item to interpret as a boolean.
 * @returns `true` if the item is a number not equal to 0, a boolean `true`, or a non-NULL pointer; `false` otherwise.
 */
bool qleei_value_item_as_bool(Qleei_Value_Item item);

/**
 * Execute a while loop whose `while` token has already been consumed, using the interpreter state.
 *
 * @param it Interpreter instance whose lexer and stack are used and modified during loop execution.
 * @param inside_of_proc True if the loop is being executed while parsing/executing inside a procedure; affects execution context.
 * @returns `true` if the loop completed successfully, `false` on error.
 */
bool qleei_execute_while(Qleei_Interpreter *it, bool inside_of_proc);

/**
 * Execute a single token within the given interpreter, performing stack, memory,
 * I/O, control-flow, and procedure-related actions and updating interpreter state.
 *
 * @param it Interpreter instance to operate on; its stack, procs, lexer state and
 *           completion flag may be modified.
 * @param inside_of_proc Set to true when the token is being executed while
 *                       parsing/executing a procedure body (affects allowed actions).
 * @param t Token to execute.
 * @returns `true` if the token was handled successfully, `false` on error.
 */
bool qleei_execute_token(Qleei_Interpreter *it, bool inside_of_proc, QLeei_Token t);

/**
 * Parse a procedure declaration starting at the current lexer position and register it with the interpreter.
 *
 * Parses a procedure name, its input and output type lists (e.g. `[] -> []`), and the procedure body (captures nested
 * `while`/`end` depth). On success the constructed Qleei_Proc is appended to the interpreter's procs list.
 *
 * @param it Interpreter whose embedded lexer is positioned at the `proc` keyword; the function advances the lexer
 *           as it consumes tokens.
 * @returns `true` if the procedure was successfully parsed and registered; `false` on lexer failure or syntax errors.
 */
bool qleei_parse_proc(Qleei_Interpreter *it);

/**
 * Execute the procedure body defined by `proc` within the interpreter `it`.
 *
 * The lexer and interpreter state are advanced through the procedure body until an `end`
 * token is encountered; the interpreter's stack and other runtime state may be modified
 * by executing the procedure's tokens. On successful completion the original lexer
 * position is restored.
 *
 * @param it Interpreter instance whose lexer and runtime state will be used and modified.
 * @param proc Procedure descriptor containing the saved body start location to execute.
 * @returns `true` if the procedure body completed successfully and the original lexer
 *          position was restored, `false` if execution failed (lexer/state remains at
 *          the failure point).
 */
bool qleei_execute_proc(Qleei_Interpreter *it, Qleei_Proc *proc);

#ifdef PLATFORM_BROWSER

extern double qleei_wasm_parse_number(const char *buf, qleei_uisz_t buf_size);
extern void   qleei_wasm_printf(const char *fmt, ...);
extern void   qleei_wasm_printfn(const char *fmt, ...);
extern void   qleei_wasm_mfree(void *ptr);
extern void*  qleei_wasm_malloc(qleei_uisz_t bytes_count);
extern void*  qleei_wasm_mrealloc(void *base_ptr, qleei_uisz_t bytes_count);

#define qleei_printf  qleei_wasm_printf
#define qleei_printfn qleei_wasm_printfn

#endif // PLATFORM_BROWSER


#ifdef PLATFORM_DESKTOP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void qleei_printf(const char *fmt, ...);
void qleei_printfn(const char *fmt, ...);

#endif // PLATFORM_DESKTOP


#endif // _QLEEI_H

#ifdef QLEEI_IMPLEMENTATION


Qleei_String_View qleei_sv_from_zstr(const char *zstr) {
  Qleei_String_View sv = {0};
  sv.data = zstr;
  if (zstr == NULL) return sv;
  while (sv.data[sv.len] != 0) sv.len++;
  return sv;
}

bool qleei_sv_has_suffix(Qleei_String_View sv, Qleei_String_View suffix) {
  if (sv.len < suffix.len) return false;
  for (qleei_uisz_t i = 0; i < suffix.len; ++i) {
    char a = sv.data[sv.len - suffix.len + i];
    char b = suffix.data[i];
    if (a != b) return false;
  }
  return true;
}

bool qleei_sv_has_prefix(Qleei_String_View sv, Qleei_String_View prefix) {
  if (sv.len < prefix.len) return false;
  for (qleei_uisz_t i = 0; i < prefix.len; ++i) {
    char a = sv.data[i];
    char b = prefix.data[i];
    if (a != b) return false;
  }
  return true;
}

bool qleei_sv_eq_zstr(Qleei_String_View sv, const char *zstr) {
  qleei_uisz_t zstr_sz = qleei_zstr_len(zstr);
  if (sv.len != zstr_sz) return false;
  for (qleei_uisz_t i = 0; i < sv.len; ++i) {
    char a = sv.data[i];
    char b = zstr[i];
    if (a != b) return false;
  }
  return true;
}

bool qleei_sv_eq_sv(Qleei_String_View sv_a, Qleei_String_View sv_b) {
  if (sv_a.len != sv_b.len) return false;
  for (qleei_uisz_t i = 0; i < sv_a.len; ++i) {
    char a = sv_a.data[i];
    char b = sv_b.data[i];
    if (a != b) return false;
  }
  return true;
}

bool qleei_zstr_eq(const char *za, const char *zb) {
#ifdef PLATFORM_DESKTOP
  return strcmp(za, zb) == 0;
#else // PLATFORM_BROWSER
  qleei_uisz_t len = qleei_zstr_len(za);
  if (len != qleei_zstr_len(zb)) return false;
  for (qleei_uisz_t i = 0; i < len; ++i) {
    if (za[i] != zb[i]) return false;
  }
  return true;
#endif // PLATFORM_DESKTOP
}

char *qleei_zstr_dup(const char *zstr) {
  if (zstr == NULL) return NULL;
#ifdef PLATFORM_DESKTOP
  return strdup(zstr);
#else
  qleei_uisz_t len = qleei_zstr_len(zstr);
  char *dup = qleei_mem_alloc(len + 1);
  if (dup == NULL) return NULL;
  qleei_mem_copy(dup, zstr, len);
  dup[len] = 0;
  return dup;
#endif // PLATFORM_DESKTOP
}

bool qleei_list_reserve(void **items, qleei_uisz_t item_size, qleei_uisz_t *current_capacity, qleei_uisz_t desired_capacity) {
  qleei_uisz_t n = *current_capacity;
  if (desired_capacity <= n) return true;

  if (n == 0) n = 256;
  while (desired_capacity > n) n *= 2;

  void *new_ptr = qleei_mem_realloc(*items, n*item_size);
  if (new_ptr == 0) return false;

  *items = new_ptr;
  *current_capacity = n;
  return true;
}

bool qleei_list_append(void **items, qleei_uisz_t item_size, qleei_uisz_t *capacity, qleei_uisz_t *length, void *item) {
  qleei_uisz_t len = *length;
  if (!qleei_list_reserve(items, item_size, capacity, len + 1)) return false;
  char *bytes = (char*)*items;
  qleei_mem_copy(bytes + (len*item_size), item, item_size);
  *length = len + 1;
  return true;
}

void* qleei_list_last(void *items, qleei_uisz_t item_size, qleei_uisz_t length) {
  if (length == 0) return NULL;
  char *bytes = (char*)items;
  qleei_uisz_t offset = (length - 1) * item_size;
  return (void*)(bytes + offset);
}

bool qleei_list_pop(void *items, qleei_uisz_t item_size, qleei_uisz_t *length, void *popped_item) {
  qleei_uisz_t len = *length;
  if (len == 0) return false;
  len--;
  if (popped_item) {
    char *bytes = (char*)items;
    char *item  = bytes + (len * item_size);
    qleei_mem_copy(popped_item, item, item_size);
  }
  *length = len;
  return true;
}

bool qleei_list_swap(void *items, qleei_uisz_t item_size, qleei_uisz_t i, qleei_uisz_t j, qleei_uisz_t length) {
  if (items == NULL) return false;
  if (i >= length || j >= length) return false;

  if (i == j) return true;

  char *bytes = (char*)items;
  char  item_t[item_size];
  char *item_i = bytes + (i * item_size);
  char *item_j = bytes + (j * item_size);
  qleei_mem_copy(item_t, item_i, item_size);
  qleei_mem_copy(item_i, item_j, item_size);
  qleei_mem_copy(item_j, item_t, item_size);
  return true;
}

void qleei_list_free(void **items, qleei_uisz_t *capacity, qleei_uisz_t *length) {
  if (*items != NULL) {
    qleei_mem_free(*items);
    *items = NULL;
  }
  *capacity = 0;
  *length = 0;
}

static bool qleei__word_print_number(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  switch (item.kind) {
  case QLEEI_VALUE_KIND_NUMBER:   qleei_printfn("%.4f", item.as_number.value); break;
  case QLEEI_VALUE_KIND_BOOL:     qleei_printfn("%d", (int)item.as_bool.value); break;
  case QLEEI_VALUE_KIND_POINTER:  qleei_printfn("%zu", (qleei_uisz_t)item.as_pointer.value); break;
  }
  return true;
}

static bool qleei__word_print_uisz(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  switch (item.kind) {
  case QLEEI_VALUE_KIND_NUMBER:   qleei_printfn("%zu", (qleei_uisz_t)item.as_number.value); break;
  case QLEEI_VALUE_KIND_BOOL:    qleei_printfn("%zu", (qleei_uisz_t)item.as_bool.value); break;
  case QLEEI_VALUE_KIND_POINTER:  qleei_printfn("%zu", (qleei_uisz_t)item.as_pointer.value); break;
  }
  return true;
}

static bool qleei__word_print_ptr(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, item.kind, QLEEI_VALUE_KIND_POINTER)) return false;
  qleei_printfn("%p", item.as_pointer.value);
  return true;
}

static bool qleei__word_print_char(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  if (item.kind != QLEEI_VALUE_KIND_NUMBER) { qleei_printfn("[ERROR] Invalid item type passed to print_char"); return false; }
  double n = item.as_number.value;
  if (0 > n || n > 255) { qleei_printfn("[ERROR] Attempting to read a number as a char that exceeds the char limit of 255: %zu", (qleei_uisz_t)n); return false; }
  qleei_printfn("%c", (char)n);
  return true;
}

static bool qleei__word_print_bool(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  if (item.kind != QLEEI_VALUE_KIND_BOOL) { qleei_printfn("[ERROR] Invalid item type passed to print_bool"); return false; }
  qleei_printfn(item.as_bool.value ? "true" : "false");
  return true;
}

static bool qleei__word_print_stack(Qleei_Word_Handler_Opt opt) {
  qleei_print_stack(opt.stack);
  return true;
}

static bool qleei__word_print_zstr(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  if (item.kind != QLEEI_VALUE_KIND_POINTER) { qleei_printfn("[ERROR] Invalid type passed to "QLEEI_SV_Fmt_Str" expected pointer", QLEEI_SV_Fmt_Arg(opt.token.string)); return false; }
  qleei_printfn("%s", (char*)item.as_pointer.value);
  return true;
}

static bool qleei__word_dup(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item *n = qleei_alist_last(opt.stack, Qleei_Value_Item);
  qleei_alist_append(opt.stack, n);
  return true;
}

static bool qleei__word_over(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 2)) return false;
  Qleei_Value_Item n = opt.stack->items[opt.stack->len-2];
  qleei_alist_append(opt.stack, &n);
  return true;
}

static bool qleei__word_drop(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  qleei_alist_pop(opt.stack, NULL);
  return true;
}

static bool qleei__word_rot2(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 2)) return false;
  qleei_alist_swap(opt.stack, opt.stack->len - 1, opt.stack->len - 2);
  return true;
}

static bool qleei__word_swap2(Qleei_Word_Handler_Opt opt) {
  return qleei__word_rot2(opt);
}

static bool qleei__word_swap3(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 3)) return false;
  qleei_alist_swap(opt.stack, opt.stack->len - 1, opt.stack->len - 3);
  return true;
}

static bool qleei__word_rot3(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 3)) return false;
  qleei_alist_swap(opt.stack, opt.stack->len - 1, opt.stack->len - 3);
  qleei_alist_swap(opt.stack, opt.stack->len - 1, opt.stack->len - 2);
  return true;
}

static bool qleei__word_assert_empty(Qleei_Word_Handler_Opt opt) {
  if (opt.stack->len != 0) {
    qleei_printfn("[ERROR] Expected stack to be empty but %zu elements remain in it", opt.stack->len);
    qleei_print_stack(opt.stack);
    return false;
  }
  return true;
}

static bool qleei__word_mem_alloc(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  if (item.kind != QLEEI_VALUE_KIND_NUMBER) { qleei_printfn("[ERROR] Invalid type passed to "QLEEI_SV_Fmt_Str" expected number", QLEEI_SV_Fmt_Arg(opt.token.string)); return false; }
  void *ptr = qleei_mem_alloc((qleei_uisz_t)item.as_number.value);
  item.as_pointer.kind = QLEEI_VALUE_KIND_POINTER;
  item.as_pointer.value = ptr;
  qleei_alist_append(opt.stack, &item);
  return true;
}

static bool qleei__word_mem_free(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  if (item.kind != QLEEI_VALUE_KIND_POINTER) { qleei_printfn("[ERROR] Invalid type passed to "QLEEI_SV_Fmt_Str" expected pointer", QLEEI_SV_Fmt_Arg(opt.token.string)); return false; }
  qleei_mem_free(item.as_pointer.value);
  return true;
}

static bool qleei__word_mem_save_si8(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 2)) return false;
  Qleei_Value_Item ptr_item, val_item;
  qleei_alist_pop(opt.stack, &ptr_item);
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, ptr_item.kind, QLEEI_VALUE_KIND_POINTER)) return false;
  qleei_alist_pop(opt.stack, &val_item);
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, val_item.kind, QLEEI_VALUE_KIND_NUMBER)) return false;
  *(qleei_si8_t*)ptr_item.as_pointer.value = (qleei_si8_t)val_item.as_number.value;
  return true;
}

static bool qleei__word_mem_save_ui8(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 2)) return false;
  Qleei_Value_Item ptr_item, val_item;
  qleei_alist_pop(opt.stack, &ptr_item);
  if (ptr_item.kind != QLEEI_VALUE_KIND_POINTER) { qleei_loc_printfn(opt.token.loc, "[ERROR] "QLEEI_SV_Fmt_Str" requires a pointer at the top of the stack", QLEEI_SV_Fmt_Arg(opt.token.string)); qleei_printf("[NOTE] Current stack: "); qleei_print_stack(opt.stack); return false; }
  qleei_alist_pop(opt.stack, &val_item);
  if (val_item.kind != QLEEI_VALUE_KIND_NUMBER) { qleei_printfn("[ERROR] "QLEEI_SV_Fmt_Str" requires a number second to the top of the stack", QLEEI_SV_Fmt_Arg(opt.token.string)); return false; }
  *(qleei_ui8_t*)ptr_item.as_pointer.value = (qleei_ui8_t)val_item.as_number.value;
  return true;
}

static bool qleei__word_mem_load_ui8(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, item.kind, QLEEI_VALUE_KIND_POINTER)) return false;
  item.as_number.kind = QLEEI_VALUE_KIND_NUMBER;
  item.as_number.value = *(qleei_ui8_t*)item.as_pointer.value;
  qleei_alist_append(opt.stack, &item);
  return true;
}

static bool qleei__word_mem_save_ui32(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 2)) return false;
  Qleei_Value_Item ptr_item, val_item;
  qleei_alist_pop(opt.stack, &ptr_item);
  qleei_alist_pop(opt.stack, &val_item);
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, ptr_item.kind, QLEEI_VALUE_KIND_POINTER)) return false;
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, val_item.kind, QLEEI_VALUE_KIND_NUMBER)) return false;
  *(qleei_ui32_t*)ptr_item.as_pointer.value = (qleei_ui32_t)val_item.as_number.value;
  return true;
}

static bool qleei__word_mem_load_ui32(Qleei_Word_Handler_Opt opt) {
  if (!qleei_stack_operation_requires_n_items(opt.token.loc, opt.stack, opt.token.string, 1)) return false;
  Qleei_Value_Item item;
  qleei_alist_pop(opt.stack, &item);
  if (!qleei_action_expects_value_kind(opt.token.loc, opt.token.string, item.kind, QLEEI_VALUE_KIND_POINTER)) return false;
  item.as_number.kind = QLEEI_VALUE_KIND_NUMBER;
  item.as_number.value = *(qleei_ui32_t*)item.as_pointer.value;
  qleei_alist_append(opt.stack, &item);
  return true;
}

static bool qleei__word_proc(Qleei_Word_Handler_Opt opt) {
  if (opt.inside_proc) {
    qleei_printfn("[ERROR] Cannot define a procedure while inside of a procedure");
    return false;
  }
  return qleei_parse_proc((Qleei_Interpreter*)opt.user_data);
}

static bool qleei__word_while(Qleei_Word_Handler_Opt opt) {
  return qleei_execute_while((Qleei_Interpreter*)opt.user_data, opt.inside_proc);
}

static bool qleei__word_if(Qleei_Word_Handler_Opt opt) {
  (void)opt;
  qleei_printfn("[TODO] if expressions are not implemented yet");
  return false;
}

static Qleei_Custom_Word QLEEI_BUILTIN_WORDS[] = {
  { "proc",           { .handler = qleei__word_proc,          .user_data = NULL } },
  { "while",          { .handler = qleei__word_while,         .user_data = NULL } },
  { "if",             { .handler = qleei__word_if,            .user_data = NULL } },
  { "print_number",   { .handler = qleei__word_print_number,  .user_data = NULL } },
  { "print_uisz",     { .handler = qleei__word_print_uisz,    .user_data = NULL } },
  { "print_ptr",      { .handler = qleei__word_print_ptr,     .user_data = NULL } },
  { "print_char",     { .handler = qleei__word_print_char,    .user_data = NULL } },
  { "print_bool",     { .handler = qleei__word_print_bool,    .user_data = NULL } },
  { "print_stack",    { .handler = qleei__word_print_stack,   .user_data = NULL } },
  { "print_zstr",     { .handler = qleei__word_print_zstr,    .user_data = NULL } },
  { "dup",            { .handler = qleei__word_dup,           .user_data = NULL } },
  { "over",           { .handler = qleei__word_over,          .user_data = NULL } },
  { "drop",           { .handler = qleei__word_drop,          .user_data = NULL } },
  { "rot2",           { .handler = qleei__word_rot2,          .user_data = NULL } },
  { "swap2",          { .handler = qleei__word_swap2,         .user_data = NULL } },
  { "swap3",          { .handler = qleei__word_swap3,         .user_data = NULL } },
  { "rot3",           { .handler = qleei__word_rot3,          .user_data = NULL } },
  { "assert_empty",   { .handler = qleei__word_assert_empty,  .user_data = NULL } },
  { "mem_alloc",      { .handler = qleei__word_mem_alloc,     .user_data = NULL } },
  { "mem_free",       { .handler = qleei__word_mem_free,      .user_data = NULL } },
  { "mem_save_si8",   { .handler = qleei__word_mem_save_si8,  .user_data = NULL } },
  { "mem_save_ui8",   { .handler = qleei__word_mem_save_ui8,  .user_data = NULL } },
  { "mem_load_ui8",   { .handler = qleei__word_mem_load_ui8,  .user_data = NULL } },
  { "mem_save_ui32",  { .handler = qleei__word_mem_save_ui32, .user_data = NULL } },
  { "mem_load_ui32",  { .handler = qleei__word_mem_load_ui32, .user_data = NULL } },
};
static const qleei_uisz_t QLEEI_BUILTIN_WORD_COUNT = sizeof(QLEEI_BUILTIN_WORDS) / sizeof(QLEEI_BUILTIN_WORDS[0]);

bool qleei_stack_push(Qleei_Stack *stack, Qleei_Value_Item item) {
  return qleei_alist_append(stack, &item);
}

bool qleei_stack_pop(Qleei_Stack *stack, Qleei_Value_Item *item) {
  return qleei_alist_pop(stack, item);
}

static inline bool qleei_is_space_char(char c) {
  return c == '\t' || c == '\n' || c == '\r' || c == ' ';
}

static inline bool qleei_is_number_char(char c) {
  return '0' <= c && c <= '9';
}

static inline bool qleei_is_alphabetic_char(char c) {
  return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

static inline bool qleei_is_identifier_start_char(char c) {
  return qleei_is_alphabetic_char(c) || c == '_' || c == '#' || c == '@';
}

static inline bool qleei_is_identifier_char(char c) {
  return qleei_is_identifier_start_char(c) || c == '-' || qleei_is_number_char(c);
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

bool qleei_value_kind_list_append(Qleei_Value_Kind **items, qleei_uisz_t *cap, qleei_uisz_t *len, Qleei_Value_Kind item) {
  return qleei_list_append((void**)items, sizeof(Qleei_Value_Kind), cap, len, &item);
}

/**
 * Finds a procedure with the given name in a list of procedures.
 *
 * @param haystack List of procedures to search.
 * @param needle Name of the procedure to find as a Qleei_String_View.
 * @returns Pointer to the matching Qleei_Proc if found, NULL otherwise.
 */
Qleei_Proc *qleei_procs_find_by_sv_name(Qleei_Procs *haystack, Qleei_String_View needle) {
  qleei_alist_foreach(Qleei_Proc, it, haystack) {
    if (qleei_sv_eq_sv(it->name_sv, needle)) {
      return it;
    }
  }
  return NULL;
}

bool qleei_cp_sv_to_buf(Qleei_String_View sv, char *buf, qleei_uisz_t buf_size) {
  if (sv.len+1 > buf_size) return false;
  qleei_mem_copy(buf, sv.data, sv.len);
  buf[sv.len] = 0;
  return true;
}

static bool qleei_builtin_word_exists(Qleei_String_View sv) {
  char buf[QLEEI_MAX_WORD_SIZE+1];// +1 for NULL terminator
  if (!qleei_cp_sv_to_buf(sv, buf, sizeof(buf))) return false;
  for (qleei_uisz_t i = 0; i < QLEEI_BUILTIN_WORD_COUNT; i++) {
    if (qleei_zstr_eq(QLEEI_BUILTIN_WORDS[i].key, buf)) return true;
  }
  return false;
}

Qleei_Word_Handler qleei__get_word_handler(Qleei_Interpreter *it, Qleei_String_View sv, void **user_data_out) {
  char buf[QLEEI_MAX_WORD_SIZE];
  if (!qleei_cp_sv_to_buf(sv, buf, sizeof(buf))) return NULL;

  for (qleei_uisz_t i = 0; i < QLEEI_BUILTIN_WORD_COUNT; i++) {
    if (qleei_zstr_eq(QLEEI_BUILTIN_WORDS[i].key, buf)) {
      if (user_data_out) {
        *user_data_out = QLEEI_BUILTIN_WORDS[i].val.user_data;
        if (*user_data_out == NULL) *user_data_out = it;
      }
      return QLEEI_BUILTIN_WORDS[i].val.handler;
    }
  }

  for (qleei_uisz_t i = 0; i < it->words.len; i++) {
    if (qleei_zstr_eq(it->words.items[i].key, buf)) {
      if (user_data_out) {
        *user_data_out = it->words.items[i].val.user_data;
        if (*user_data_out == NULL) *user_data_out = it;
      }
      return it->words.items[i].val.handler;
    }
  }

  return NULL;
}

bool qleei_custom_words_add(Qleei_Custom_Words *w, const char *word, Qleei_Word_Handler handler, void *user_data) {
  if (w == NULL || word == NULL || handler == NULL) return false;
  for (qleei_uisz_t i = 0; i < w->len; i++) {
    if (qleei_zstr_eq(w->items[i].key, word)) {
      w->items[i].val.handler = handler;
      w->items[i].val.user_data = user_data;
      return true;
    }
  }
  Qleei_Custom_Word item = { .key = word, .val = { .handler = handler, .user_data = user_data } };
  return qleei_alist_append(w, &item);
}

bool qleei_custom_words_remove(Qleei_Custom_Words *w, const char *word) {
  if (w == NULL || word == NULL) return false;
  for (qleei_uisz_t i = w->len; i > 0; i--) {
    if (qleei_zstr_eq(w->items[i - 1].key, word)) {
      if (i < w->len) w->items[i - 1] = w->items[w->len - 1];
      w->len--;
      return true;
    }
  }
  return false;
}


void qleei_lexer_init(QLeei_Lexer *l, const char *input_path, const char *buffer, qleei_uisz_t buf_size) {
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
    qleei_printfn("[ERROR] Attempting to lex into a NULL buffer");
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
      if (lexer->index >= lexer->buffer_len) {
        c = 0;
        break;
      }
      c = lexer->buffer[lexer->index++];
    }

    // If somehow the last char is a null terminator we just ignore it
    if (lexer->index >= lexer->buffer_len) {
      if (c == 0) break;
    }

    // Ignore any unmanaged ASCII control character
    if (c < 32) {
      qleei_printfn("[WARN] Unexpected control character found in input: %d", (int)c);
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
      if (lexer->index < lexer->buffer_len) lexer->index -= 1;
      token->kind = QLEEI_TOKEN_KIND_NUMBER;
      token->number = qleei_parse_number(token->string);
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
      if (lexer->index < lexer->buffer_len) lexer->index -= 1;

      if (qleei_sv_eq_zstr(token->string, "true")) {
	      token->kind = QLEEI_TOKEN_KIND_BOOL;
	      token->number = 1;
      } else if (qleei_sv_eq_zstr(token->string, "false")) {
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
  qleei_printf("[ ");
  for (qleei_uisz_t i = 0; i < s->len; ++i) {
    qleei_uisz_t index = s->len - (i + 1);
    Qleei_Value_Item item = s->items[index];
    if (i > 0) qleei_printf(", ");
    switch (item.kind) {
    case QLEEI_VALUE_KIND_NUMBER:
      qleei_printf("Number(%.4f)", item.as_number.value);
      break;
    case QLEEI_VALUE_KIND_BOOL:
      qleei_printf("Bool(%s)", item.as_bool.value ? "true" : "false");
      break;
    case QLEEI_VALUE_KIND_POINTER:
      qleei_printf("Pointer(%p)", item.as_pointer.value);
      break;
    default:
      qleei_printf("CorruptedValue(%d, %.4f)", item.kind, item.as_number.value);
      break;
    }
  }
  qleei_printf(" ]\n");
}

bool qleei_stack_operation_requires_n_items(QLeei_Lex_Location loc, Qleei_Stack *s, Qleei_String_View sv, qleei_uisz_t n) {
  if (s->len < n) {
    qleei_loc_printfn(loc, "[ERROR] "QLEEI_SV_Fmt_Str" requires %zu items in the stack to execute", QLEEI_SV_Fmt_Arg(sv), n);
    return false;
  }
  return true;
}

bool qleei_action_expects_value_kind(QLeei_Lex_Location loc, Qleei_String_View sv, Qleei_Value_Kind got, Qleei_Value_Kind exp) {
  if (got == exp) return true;
  qleei_loc_printfn(
    loc,
    "[TYPE_ERROR] Invalid type passed to "QLEEI_SV_Fmt_Str" expected %s but got %s",
    QLEEI_SV_Fmt_Arg(sv),
    qleei_get_value_kind_name(exp),
    qleei_get_value_kind_name(got)
  );
  return false;
}


double qleei_value_item_as_number(Qleei_Value_Item item) {
  switch (item.kind) {
  case QLEEI_VALUE_KIND_NUMBER:
    return item.as_number.value;
  case QLEEI_VALUE_KIND_BOOL:
    return (double)item.as_bool.value;
  case QLEEI_VALUE_KIND_POINTER:
    return (double)(qleei_uisz_t)item.as_pointer.value;
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

bool qleei_execute_while(Qleei_Interpreter *it, bool inside_of_proc) {
  QLeei_Lexer *l = &it->lexer;
  QLeei_Lex_Location start_point = qleei_lexer_save_point(l);
  QLeei_Lex_Location end_point = {0};
  bool end_point_found = false;
  while (qleei_lexer_next(l)) {
    QLeei_Token t = l->token;
    if (qleei_sv_eq_zstr(t.string, "begin")) {
      if (it->stack.len == 0) {
	      qleei_printfn("[ERROR] While loop requires at least one element on the stack to do evaluation but nothing is on the stack");
	      return false;
      }

      Qleei_Value_Item item;
      qleei_alist_pop(&it->stack, &item);
      if (!qleei_value_item_as_bool(item)) {
	      if (end_point_found) {
	        qleei_lexer_restore_point(l, end_point);
	        return true;
	      }
	      qleei_uisz_t level = 1;
	      while (level > 0) {
	        if (!qleei_lexer_next(l)) return false;
	        t = l->token;
	        if (qleei_sv_eq_zstr(t.string, "while")) {
	          level++;
	        } else if (qleei_sv_eq_zstr(t.string, "end")) {
	          level--;
	        }
	      }
	      return true;
      }

      continue;
    }

    if (qleei_sv_eq_zstr(t.string, "end")) {
      if (!end_point_found) {
	      end_point_found = true;
	      end_point = qleei_lexer_save_point(l);
      }
      qleei_lexer_restore_point(l, start_point);
      continue;
    }

    if (t.kind == QLEEI_TOKEN_KIND_EOF) {
      qleei_loc_printfn(start_point, "[ERROR] Unterminated while loop hit: missing 'end' at the end of the loop's body");
      return false;
    }

    if (!qleei_execute_token(it, inside_of_proc, t)) return false;
  }

  return false;
}

bool qleei_parse_proc(Qleei_Interpreter *it) {
  QLeei_Lexer *l = &it->lexer;
  if (!qleei_lexer_next(l)) return false;
  if (l->token.kind != QLEEI_TOKEN_KIND_IDENTIFIER) {
    qleei_printfn("%zu:%zu: [ERROR] Procedure is required to be given a name after 'proc' keyword", l->token.loc.line, l->token.loc.column);
    return false;
  }

  Qleei_String_View name_sv = l->token.string;
  if (qleei__get_word_handler(it, name_sv, NULL) != NULL) {
    qleei_loc_printfn(l->token.loc, "[ERROR] Cannot define procedure with name '"QLEEI_SV_Fmt_Str"': name conflicts with built-in or custom word", QLEEI_SV_Fmt_Arg(name_sv));
    return false;
  }
  Qleei_Proc proc = {0};
  proc.name_sv = name_sv;

  if (!qleei_lexer_next(l)) return false;

  // ==================================================
  // Parse Inputs
  // --------------------------------------------------
  if (!qleei_sv_eq_zstr(l->token.string, "[")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] After procedure name must specify inputs & outputs like `[] -> []`");
    qleei_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
    return false;
  }

  while (!qleei_sv_eq_zstr(l->token.string, "]")) {
    if (!qleei_lexer_next(l)) {
      qleei_loc_printfn(l->token.loc, "[ERROR] After procedure name must specify inputs & outputs like `[] -> []`");
      qleei_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
      return false;
    }
    if (qleei_sv_eq_zstr(l->token.string, "]")) break;

    if (l->token.kind != QLEEI_TOKEN_KIND_IDENTIFIER) {
      qleei_loc_printfn(l->token.loc, "[ERROR] Unexpected %s token when expecting identifier for type name", qleei_get_token_kind_name(l->token.kind));
      qleei_printfn("[NOTE] Token source: '"QLEEI_SV_Fmt_Str"'", QLEEI_SV_Fmt_Arg(l->token.string));
      return false;
    }

    if (qleei_sv_eq_zstr(l->token.string, "pointer") || qleei_sv_eq_zstr(l->token.string, "ptr")) {
      qleei_value_kind_list_append(&proc.inputs.items, &proc.inputs.cap, &proc.inputs.len, QLEEI_VALUE_KIND_POINTER);
    } else if (qleei_sv_eq_zstr(l->token.string, "number")) {
      qleei_value_kind_list_append(&proc.inputs.items, &proc.inputs.cap, &proc.inputs.len, QLEEI_VALUE_KIND_NUMBER);
    } else if (qleei_sv_eq_zstr(l->token.string, "bool")) {
      qleei_value_kind_list_append(&proc.inputs.items, &proc.inputs.cap, &proc.inputs.len, QLEEI_VALUE_KIND_BOOL);
    } else {
      qleei_loc_printfn(l->token.loc, "[ERROR] Invalid type name only 'pointer'/'ptr', 'number', and 'bool' types exist");
      return false;
    }

    QLeei_Token t = {0};
    if (!qleei_lexer_peek(l, &t)) return false;

    if (qleei_sv_eq_zstr(t.string, ",") || qleei_sv_eq_zstr(t.string, "]")) {
      qleei_lexer_next(l);
    }
  }

  if (!qleei_sv_eq_zstr(l->token.string, "]")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] After procedure name must specify inputs & outputs like `[] -> []`");
    qleei_printfn("[NOTE] Token source: '"QLEEI_SV_Fmt_Str"'", QLEEI_SV_Fmt_Arg(l->token.string));
    qleei_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
    return false;
  }
  if (!qleei_lexer_next(l)) return false;


  if (!qleei_sv_eq_zstr(l->token.string, "->")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] Expected arrow symbol '->' between proc inputs and outputs");
    return false;
  }
  if (!qleei_lexer_next(l)) return false;

  // ==================================================
  // Parse Outputs
  // --------------------------------------------------
  if (!qleei_sv_eq_zstr(l->token.string, "[")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] After procedure inputs outputs must be specified: <input> -> <output>");
    qleei_printfn("[NOTE] Found '"QLEEI_SV_Fmt_Str"' but expected '['", QLEEI_SV_Fmt_Arg(l->token.string));
    return false;
  }

  while (l->token.kind != QLEEI_TOKEN_KIND_SYMBOL || !qleei_sv_eq_zstr(l->token.string, "]")) {
    if (!qleei_lexer_next(l)) {
      qleei_loc_printfn(l->token.loc, "[ERROR] After procedure inputs outputs must be specified");
      return false;
    }
    if (qleei_sv_eq_zstr(l->token.string, "]")) break;

    if (l->token.kind != QLEEI_TOKEN_KIND_IDENTIFIER) {
      qleei_loc_printfn(l->token.loc, "[ERROR] Unexpected %s token when expecting identifier for type name", qleei_get_token_kind_name(l->token.kind));
      return false;
    }

    if (qleei_sv_eq_zstr(l->token.string, "pointer") || qleei_sv_eq_zstr(l->token.string, "ptr")) {
      qleei_value_kind_list_append(&proc.outputs.items, &proc.outputs.cap, &proc.outputs.len, QLEEI_VALUE_KIND_POINTER);
    } else if (qleei_sv_eq_zstr(l->token.string, "number")) {
      qleei_value_kind_list_append(&proc.outputs.items, &proc.outputs.cap, &proc.outputs.len, QLEEI_VALUE_KIND_NUMBER);
    } else if (qleei_sv_eq_zstr(l->token.string, "bool")) {
      qleei_value_kind_list_append(&proc.outputs.items, &proc.outputs.cap, &proc.outputs.len, QLEEI_VALUE_KIND_BOOL);
    } else {
      qleei_loc_printfn(l->token.loc, "[ERROR] Invalid type name only 'pointer'/'ptr', 'number', and 'bool' types exist");
      return false;
    }

    QLeei_Token t = {0};
    if (!qleei_lexer_peek(l, &t)) return false;

    if (qleei_sv_eq_zstr(t.string, ",")) {
      qleei_lexer_next(l);
    }
  }

  if (!qleei_sv_eq_zstr(l->token.string, "]")) {
    qleei_loc_printfn(l->token.loc, "[ERROR] After procedure inputs outputs must be specified");
    qleei_printfn("[NOTE] The return type is just imaginary, we don't check if you honor it KEKW");
    return false;
  }


  // ==================================================
  // Parse Body
  // --------------------------------------------------
  proc.body_start = qleei_lexer_save_point(l);
  qleei_uisz_t level = 1;
  while (true) {
    if (!qleei_lexer_next(l)) return false;
    if (l->token.kind == QLEEI_TOKEN_KIND_NONE) {
      qleei_printfn("[UNREACHABLE] Lexer.Token.kind == QLEEI_TOKEN_KIND_NONE in parse_proc");
      return false;
    }
    if (l->token.kind == QLEEI_TOKEN_KIND_EOF) {
      qleei_printfn("[ERROR] Procedure is missing to finish with 'end' keyword");
      return false;
    }
    if (l->token.kind == QLEEI_TOKEN_KIND_IDENTIFIER) {
      if (qleei_sv_eq_zstr(l->token.string, "while")) {
	      level++;
      } else if (qleei_sv_eq_zstr(l->token.string, "end")) {
	      level--;
	      if (level == 0) {
          proc.body_end = qleei_lexer_save_point(l);
          break;
        }
      }
    }
  }

  qleei_list_append((void**)&it->procs.items, sizeof(Qleei_Proc), &it->procs.cap, &it->procs.len, &proc);

  return true;
}

bool qleei_execute_token(Qleei_Interpreter *it, bool inside_of_proc, QLeei_Token t) {
  Qleei_Stack *stack = &it->stack;
  Qleei_String_View sv = t.string;

  switch (t.kind) {
  case QLEEI_TOKEN_KIND_NONE:
    qleei_printfn("[UNREACHABLE] qleei_interpreter_step switch (lexer.kind) case QLEEI_TOKEN_KIND_NONE");
    return false;

  case QLEEI_TOKEN_KIND_EOF:
    it->done = true;
    return true;

  case QLEEI_TOKEN_KIND_IDENTIFIER:
    {
      void *user_data = NULL;
      Qleei_Word_Handler h = qleei__get_word_handler(it, sv, &user_data);
      if (h != NULL) {
        Qleei_Word_Handler_Opt handler_opt = {
          .token = t,
          .stack = stack,
          .procs = &it->procs,
          .inside_proc = inside_of_proc,
          .user_data = user_data,
        };
        if (!h(handler_opt)) return false;
        return true;
      }
    }

    {
      Qleei_Proc *proc = qleei_procs_find_by_sv_name(&it->procs, sv);
      if (proc != NULL) {
	      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, proc->inputs.len)) return false;
	      for (qleei_uisz_t i = 0; i < proc->inputs.len; ++i) {
	        Qleei_Value_Kind received = (stack->items[(proc->inputs.len - (i + 1))]).kind;
	        Qleei_Value_Kind expected = proc->inputs.items[i];
	        if (received != expected) {
	          qleei_printfn("[ERROR] Proc "QLEEI_SV_Fmt_Str" expected %s but got %s", QLEEI_SV_Fmt_Arg(proc->name_sv), qleei_get_value_kind_name(expected), qleei_get_value_kind_name(received));
	          return false;
	        }
	      }
	      if (!qleei_execute_proc(it, proc)) return false;
	      return true;
      }
    }

    qleei_loc_printfn(t.loc, "[ERROR] Unknown command/identifier provided: '%.*s'", (int)sv.len, sv.data);
    return false;

  case QLEEI_TOKEN_KIND_NUMBER:
    {
      Qleei_Value_Item item = { .as_number = { .kind = QLEEI_VALUE_KIND_NUMBER, .value = t.number } };
      qleei_alist_append(stack, &item);
    }
    return true;

  case QLEEI_TOKEN_KIND_BOOL:
    {
      Qleei_Value_Item item = { .as_bool = { .kind = QLEEI_VALUE_KIND_BOOL, .value = t.number == 1.0 } };
      qleei_alist_append(stack, &item);
    }
    return true;

  case QLEEI_TOKEN_KIND_SYMBOL:
    if (qleei_sv_eq_zstr(sv, "+")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item a, b;
      qleei_alist_pop(stack, &a);
      qleei_alist_pop(stack, &b);

      if (a.kind == QLEEI_VALUE_KIND_POINTER && b.kind == QLEEI_VALUE_KIND_POINTER) {
	      qleei_printfn("[ERROR] Cannot add 2 pointers together");
	      return false;
      }

      if (a.kind == QLEEI_VALUE_KIND_POINTER || b.kind == QLEEI_VALUE_KIND_POINTER) {
	      char *ptr;
	      qleei_uisz_t n;
	      if (a.kind == QLEEI_VALUE_KIND_POINTER) {
	        ptr = a.as_pointer.value;
	        n = (qleei_uisz_t)qleei_value_item_as_number(b);
	      } else {
	        ptr = b.as_pointer.value;
	        n = (qleei_uisz_t)qleei_value_item_as_number(a);
	      }
	      a.as_pointer.kind = QLEEI_VALUE_KIND_POINTER;
	      a.as_pointer.value = ptr + n;
	      qleei_alist_append(stack, &a);
	      return true;
      }

      a.as_number.value = qleei_value_item_as_number(a) + qleei_value_item_as_number(b);
      qleei_alist_append(stack, &a);
      return true;
    }

    if (qleei_sv_eq_zstr(sv, "-")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item a, b;
      qleei_alist_pop(stack, &a);
      qleei_alist_pop(stack, &b);

      if (a.kind == QLEEI_VALUE_KIND_POINTER && b.kind == QLEEI_VALUE_KIND_POINTER) {
	      qleei_printfn("[ERROR] Cannot do subtraction between 2 pointers");
	      return false;
      }

      if (a.kind == QLEEI_VALUE_KIND_POINTER || b.kind == QLEEI_VALUE_KIND_POINTER) {
	      char *ptr;
	      qleei_uisz_t n;
	      if (a.kind == QLEEI_VALUE_KIND_POINTER) {
	        ptr = a.as_pointer.value;
	        n = (qleei_uisz_t)qleei_value_item_as_number(b);
	      } else {
	        ptr = b.as_pointer.value;
	        n = (qleei_uisz_t)qleei_value_item_as_number(a);
	      }

	      if (n > (qleei_uisz_t)ptr) {
	        qleei_printfn("[ERROR] Pointer arithmetic ends results in a negative value");
	        return false;
	      }

	      a.as_pointer.kind  = QLEEI_VALUE_KIND_POINTER;
	      a.as_pointer.value = ptr - n;
	      qleei_alist_append(stack, &a);
	      return true;
      }

      a.as_number.value = qleei_value_item_as_number(a) - qleei_value_item_as_number(b);
      qleei_alist_append(stack, &a);
      return true;
    }

    if (qleei_sv_eq_zstr(sv, "/")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item a, b;
      qleei_alist_pop(stack, &a);
      qleei_alist_pop(stack, &b);

      if (a.kind == QLEEI_VALUE_KIND_POINTER || b.kind == QLEEI_VALUE_KIND_POINTER) {
	      qleei_loc_printfn(t.loc, "[ERROR] Cannot do division with pointers");
	      return false;
      }

      a.as_number.value = qleei_value_item_as_number(a) / qleei_value_item_as_number(b);
      qleei_alist_append(stack, &a);
      return true;
    }

    if (qleei_sv_eq_zstr(sv, "*")) {
      if (!qleei_stack_operation_requires_n_items(t.loc, stack, sv, 2)) return false;
      Qleei_Value_Item a, b;
      qleei_alist_pop(stack, &a);
      qleei_alist_pop(stack, &b);

      if (a.kind == QLEEI_VALUE_KIND_POINTER || b.kind == QLEEI_VALUE_KIND_POINTER) {
	      qleei_loc_printfn(t.loc, "[ERROR] Cannot do multiplication with pointers");
	      return false;
      }

      a.as_number.value = qleei_value_item_as_number(a) * qleei_value_item_as_number(b);
      qleei_alist_append(stack, &a);
      return true;
    }

    if (qleei_sv_eq_zstr(sv, "!")) {
      qleei_loc_printfn(t.loc, "[SYSTEM] ABORTED");
      qleei_printf("[NOTE] Stack state: ");
      qleei_print_stack(stack);
      return false;
    }

    qleei_loc_printfn(t.loc, "[ERROR] Unsupported symbol '"QLEEI_SV_Fmt_Str"'", QLEEI_SV_Fmt_Arg(sv));
    qleei_printf("[INFO] Symbol bytes: [");
    qleei_sv_iter(c, sv) {
      if (c > sv.data) qleei_printf(", ");
      qleei_printf("%d", (int)*c);
    }
    qleei_printfn("]");
    return false;

  }

  qleei_printfn("[UNREACHABLE] qleei_interpreter_step switch (lexer.kind) case %s", qleei_get_token_kind_name(t.kind));
  return false;
}

bool qleei_execute_proc(Qleei_Interpreter *it, Qleei_Proc *proc) {
  QLeei_Lexer *l = &it->lexer;
  QLeei_Lex_Location save_point = qleei_lexer_save_point(l);

  qleei_lexer_restore_point(l, proc->body_start);

  bool ok = true;
  while ((ok = qleei_lexer_next(l))) {
    if (qleei_sv_eq_zstr(l->token.string, "end")) break;
    if (l->token.kind == QLEEI_TOKEN_KIND_EOF) {
      qleei_loc_printfn(save_point, "[ERROR] Unterminated procedure: Missing 'end' at end of the procedure's body");
      ok = false;
      break;
    }
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

void qleei_interpreter_lexer_init(Qleei_Interpreter *it, const char *input_path, const char *buffer, qleei_uisz_t buf_size) {
  qleei_lexer_init(&it->lexer, input_path, buffer, buf_size);
  it->stack.len = 0;
  it->done = false;
}

void qleei_interpreter_clear(Qleei_Interpreter *it) {
  // Don't feel like adding a qleei_mem_set and this is the answer I came up with to reset the lexer
  it->lexer = (QLeei_Lexer){0};
  it->stack.len = 0;
  it->words.len = 0;
  qleei_alist_foreach(Qleei_Proc, proc, &it->procs) {
    proc->inputs.len = 0;
    proc->outputs.len = 0;
  }
  it->procs.len = 0;
  it->done = false;
}

void qleei_interpreter_reset(Qleei_Interpreter *it, const char *input_path, const char *buffer, qleei_uisz_t buf_size) {
  qleei_lexer_init(&it->lexer, input_path, buffer, buf_size);
  it->stack.len = 0;
  it->done = false;
  // words registry is intentionally preserved across resets
}

void qleei_interpreter_free(Qleei_Interpreter *it) {
  // Free each proc's input/output type lists
  qleei_alist_foreach(Qleei_Proc, proc, &it->procs) {
    qleei_list_free((void**)&proc->inputs.items,  &proc->inputs.cap,  &proc->inputs.len);
    qleei_list_free((void**)&proc->outputs.items, &proc->outputs.cap, &proc->outputs.len);
  }
  qleei_alist_free(&it->stack);
  qleei_alist_free(&it->words);
  qleei_alist_free(&it->procs);
}

bool qleei_interpreter_register_word(Qleei_Interpreter *it, const char *word, Qleei_Word_Handler handler) {
  if (qleei_builtin_word_exists(qleei_sv_from_zstr(word))) return false;
  return qleei_custom_words_add(&it->words, word, handler, NULL);
}

bool qleei_interpreter_register_word_with_data(Qleei_Interpreter *it, const char *word, Qleei_Word_Handler handler, void *user_data) {
  if (qleei_builtin_word_exists(qleei_sv_from_zstr(word))) return false;
  return qleei_custom_words_add(&it->words, word, handler, user_data);
}

bool qleei_interpreter_unregister_word(Qleei_Interpreter *it, const char *word) {
  return qleei_custom_words_remove(&it->words, word);
}

bool qleei_interpreter_exec(Qleei_Interpreter *it) {
  while (qleei_interpreter_step(it)) {
    if (it->done) return true;
  }
  return false;
}

bool qleei_interpret_buffer(const char *buffer_source_path, const char *buffer, qleei_uisz_t buf_size) {
  bool result = false;

  Qleei_Interpreter it = {0};
  qleei_interpreter_lexer_init(&it, buffer_source_path, buffer, buf_size);

  while (qleei_interpreter_step(&it)) {
    if (it.done) {
      result = true;
      break;
    }
  }

  qleei_interpreter_free(&it);
  return result;
}


#ifdef PLATFORM_DESKTOP
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void qleei_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void qleei_printfn(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  putchar('\n');
}

double qleei_parse_number(Qleei_String_View sv) {
  char buffer[sv.len+1];
  memcpy(buffer, sv.data, sv.len);
  buffer[sv.len] = 0;
  return atof((const char *)buffer);
}

void *qleei_mem_alloc(qleei_uisz_t size) {
  return malloc(size);
}

void *qleei_mem_realloc(void *ptr, qleei_uisz_t size) {
  return realloc(ptr, size);
}

void qleei_mem_free(void * ptr) {
  free(ptr);
}

void *qleei_mem_copy(void *dest, const void *src, qleei_uisz_t count) {
  return memcpy(dest, src, count);
}

qleei_uisz_t qleei_zstr_len(const char *zstr) {
  return strlen(zstr);
}


#endif // PLATFORM_DESKTOP

#ifdef PLATFORM_BROWSER

qleei_uisz_t qleei_zstr_len(const char *zstr) {
  qleei_uisz_t len = 0;
  char c;
  while ((c = zstr[len++]) != 0) { }
  len--;
  return len;
}

double qleei_parse_number(Qleei_String_View sv) {
  return qleei_wasm_parse_number((const char *)sv.data, sv.len);
}

void *qleei_mem_alloc(qleei_uisz_t size) {
  return qleei_wasm_malloc(size);
}

void *qleei_mem_realloc(void *ptr, qleei_uisz_t size) {
  return qleei_wasm_mrealloc(ptr, size);
}

void qleei_mem_free(void *ptr) {
  qleei_wasm_mfree(ptr);
}

void *qleei_mem_copy(void *dest, const void *src, qleei_uisz_t count) {
  char *dbuf = (char*)dest;
  const char *sbuf = (const char *)src;
  for (qleei_uisz_t i = 0; i < count; ++i) {
    *dbuf = *sbuf;
    dbuf++;
    sbuf++;
  }
  return dest;
}

#endif // PLATFORM_BROWSER

#endif
