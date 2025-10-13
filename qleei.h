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

typedef struct {
  const char *data;
  qleei_uisz_t len;
} Qleei_String_View;

#define QLEEI_SV_Fmt_Str     "%.*s"
#define QLEEI_SV_Fmt_Arg(sv) (int)sv.len, sv.data

Qleei_String_View qleei_sv_from_zstr(const char *zstr);
bool qleei_sv_has_suffix(Qleei_String_View sv, Qleei_String_View suffix);
bool qleei_sv_has_prefix(Qleei_String_View sv, Qleei_String_View prefix);
bool qleei_sv_eq_zstr(Qleei_String_View sv, const char *zstr);
bool qleei_sv_eq_sv(Qleei_String_View sv_a, Qleei_String_View sv_b);

#define qleei_sv_iter(it, sv) for (const char *it = (sv).data; it < (sv).data + (sv).len; ++it)

qleei_uisz_t qleei_zstr_len(const char *zstr);
bool qleei_zstr_eq(const char *za, const char *zb);

double qleei_parse_number(Qleei_String_View sv);


#define qleei_alist_reserve(alist, count)                            \
do {                                                                 \
  qleei_uisz_t ncap = (alist)->cap == 0 ? 256 : ((alist)->cap*2);    \
  while (ncap < count) ncap *= 2;                                    \
  qleei_uisz_t bytes = ncap*sizeof(*(alist)->items);                 \
  (alist)->items = qleei_mem_realloc((alist)->items, bytes);         \
  (alist)->cap = ncap;                                               \
} while(0)

#define qleei_alist_append(alist, item)                              \
do {                                                                 \
  qleei_alist_reserve(alist, (alist)->len + 1);                      \
  (alist)->items[(alist)->len++] = item;                             \
} while (0)

#define qleei_define_list_reserve(T_Item, T_List)                    \
bool T_List ## _reserve(T_List *list, qleei_uisz_t count) {          \
  if (list->cap >= count) return true;                               \
  qleei_uisz_t ncap = list->cap == 0 ? 256 : (list->cap*2);          \
  while (ncap < count) ncap = ncap * 2;                              \
  qleei_uisz_t bytes = sizeof(T_Item)*ncap;                          \
  T_Item *ptr = (T_Item*)qleei_mem_realloc(list->items, bytes);      \
  if (ptr == NULL) return false;                                     \
  list->items = ptr;                                                 \
  list->cap = ncap;                                                  \
  return true;                                                       \
}

#define qleei_define_list_append(T_Item, T_List)                     \
bool T_List ## _append (T_List *list, T_Item item) {                 \
  if (!T_List ## _reserve(list, list->len+1)) return false;          \
  list->items[list->len] = item;                                     \
  list->len++;                                                       \
  return true;                                                       \
}

#define qleei_define_list_last(T_Item, T_List)                       \
T_Item * T_List ## _last (T_List *list) {                            \
  if (list->len == 0) return NULL;                                   \
  return list->items + (list->len - 1);                              \
}

#define qleei_define_list_pop(T_Item, T_List)                        \
bool T_List ## _pop (T_List *list, T_Item *out) {                    \
  if (list->len == 0) return false;                                  \
  T_Item item = list->items[--list->len];                            \
  if (out) *out = item;                                              \
  return true;                                                       \
}

#define qleei_define_list_swap(T_Item, T_List)                       \
bool T_List ## _swap(T_List *list, qleei_uisz_t i, qleei_uisz_t j) { \
  if (i > list->len || j > list->len) return false;                  \
  T_Item t = list->items[i];                                         \
  list->items[i] = list->items[j];                                   \
  list->items[j] = t;                                                \
  return true;                                                       \
}

#define qleei_define_list_clear(T_List)                              \
void T_List ## _clear(T_List *list) {                                \
  list->len = 0;                                                     \
}

#define qleei_define_list_free(T_List)                               \
void T_List ## _free(T_List *list) {                                 \
  if (list->items != NULL) {                                         \
    qleei_mem_free(list->items);                                     \
  }                                                                  \
  list->items = NULL;                                                \
  list->len = 0;                                                     \
  list->cap = 0;                                                     \
}

#define qleei_define_list_methods(T_Item, T_List)  \
qleei_define_list_reserve(T_Item, T_List)          \
qleei_define_list_append(T_Item, T_List)           \
qleei_define_list_pop(T_Item, T_List)              \
qleei_define_list_last(T_Item, T_List)             \
qleei_define_list_swap(T_Item, T_List)             \
qleei_define_list_clear(T_List)                    \
qleei_define_list_free(T_List)


#define qleei_list_foreach(T, it, list) for (T *it = (list)->items; it < (list)->items + (list)->len; ++it)

void *qleei_mem_alloc   (qleei_uisz_t size);
void *qleei_mem_realloc (void *ptr, qleei_uisz_t size);
void  qleei_mem_free    (void *ptr);
void *qleei_mem_copy    (void *dest, const void *src, qleei_uisz_t count);


#ifndef QLEEI_TEMP_BUF_SIZE
#  define QLEEI_TEMP_BUF_SIZE (1024*8*8)
#endif

qleei_uisz_t _qleei_temporary_buffer_index = 0;
char _qleei_temporary_buffer[QLEEI_TEMP_BUF_SIZE];

void *qleei_temp_alloc(qleei_uisz_t bytes_count);
qleei_uisz_t   qleei_temp_save();
void  qleei_temp_rewind(qleei_uisz_t save_point);


#ifdef PLATFORM_BROWSER

extern double qleei_wasm_parse_number(const char *buf, qleei_uisz_t buf_size);
extern void   qleei_wasm_printf(const char *fmt, ...);
extern void   qleei_wasm_printfn(const char *fmt, ...);
extern void   qleei_wasm_temp_sprintfn(char *temp_buffer, const char *fmt, ...);
extern void   qleei_wasm_mfree(void *ptr);
extern void*  qleei_wasm_malloc(qleei_uisz_t bytes_count);
extern void*  qleei_wasm_mrealloc(void *base_ptr, qleei_uisz_t bytes_count);

#define qleei_printf  qleei_wasm_printf
#define qleei_printfn qleei_wasm_printfn
#define qleei_temp_sprintf(...) qleei_wasm_temp_sprintfn(__VA_ARGS__)

#endif // PLATFORM_BROWSER


#ifdef PLATFORM_DESKTOP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>


#define qleei_printf printf
void qleei_printfn(const char *fmt, ...);
const char *qleei_temp_sprintf(char *fmt, ...);

#endif // PLATFORM_DESKTOP


#endif // _PLATFORM_H

#ifdef QLEEI_IMPLEMENTATION

Qleei_String_View qleei_sv_from_zstr(const char *zstr) {
  Qleei_String_View sv = {0};
  sv.data = (char*) zstr;
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

void *qleei_temp_alloc(qleei_uisz_t bytes_count) {
  if (_qleei_temporary_buffer_index >= QLEEI_TEMP_BUF_SIZE) {
    qleei_printfn("[ERROR] Temp buffer size exceeded");
    return NULL;
  }
  void *ptr = &_qleei_temporary_buffer[_qleei_temporary_buffer_index];
  _qleei_temporary_buffer_index += bytes_count;
  return ptr;
}

qleei_uisz_t qleei_temp_save() {
  return _qleei_temporary_buffer_index;
}
void  qleei_temp_rewind(qleei_uisz_t save_point) {
  _qleei_temporary_buffer_index = save_point;
}


#ifdef PLATFORM_DESKTOP
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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

const char *qleei_temp_sprintf(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n_int = vsprintf(NULL, fmt, ap);
  va_end(ap);
  if (n_int < 0) return NULL;
  qleei_uisz_t n = (qleei_uisz_t)n_int;
  char *buffer = qleei_temp_alloc(n);
  if (buffer == NULL) return NULL;
  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);
  return buffer;
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
