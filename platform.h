#ifndef _PLATFORM_H
#define _PLATFORM_H

// stdbool.h macros copied straight from the man page of stdbool.h
#ifndef bool
#  define bool  _Bool
#  define true  1
#  define false 0
#endif

// Renaming of some integer sizes from stdlib.h
typedef __SIZE_TYPE__                        uisz;

typedef signed char                          si8;
typedef unsigned char                        ui8;
typedef signed short int                     si16;
typedef unsigned short int                   ui16;
typedef signed int                           si32;
typedef unsigned int                         ui32;
#if __WORDSIZE == 64
typedef signed long int                      si64;
typedef unsigned long int                    ui64;
#else
__extension__ typedef signed long long int   si64;
__extension__ typedef unsigned long long int ui64;
#endif


#ifndef NULL
#  define NULL ((void*)0)
#endif // NULL

typedef struct {
  const char *data;
  uisz len;
} String_View;

#define SV_Fmt_Str     "%.*s"
#define SV_Fmt_Arg(sv) (int)sv.len, sv.data

String_View sv_from_zstr(const char *zstr);
bool sv_has_suffix(String_View sv, String_View suffix);
bool sv_has_prefix(String_View sv, String_View prefix);
bool sv_eq_zstr(String_View sv, const char *zstr);
bool sv_eq_sv(String_View sv_a, String_View sv_b);

#define sv_iter(it, sv) for (const char *it = (sv).data; it < (sv).data + (sv).len; ++it)

uisz zstr_len(const char *zstr);

double parse_number(String_View sv);


#define alist_reserve(alist, count)                                \
do {                                                               \
  uisz ncap = (alist)->cap == 0 ? 256 : ((alist)->cap*2);          \
  while (ncap < count) ncap *= 2;                                  \
  uisz bytes = ncap*sizeof(*(alist)->items);                       \
  (alist)->items = platform_mem_realloc((alist)->items, bytes);    \
  (alist)->cap = ncap;                                             \
} while(0)

#define alist_append(alist, item)                                  \
do {                                                               \
  alist_reserve(alist, (alist)->len + 1);                          \
  (alist)->items[(alist)->len++] = item;                           \
} while (0)

#define define_list_reserve(T_Item, T_List)                        \
bool T_List ## _reserve(T_List *list, uisz count) {                \
  if (list->cap >= count) return true;                             \
  uisz ncap = list->cap == 0 ? 256 : (list->cap*2);                \
  while (ncap < count) ncap = ncap * 2;                            \
  uisz bytes = sizeof(T_Item)*ncap;                                \
  T_Item *ptr = (T_Item*)platform_mem_realloc(list->items, bytes); \
  if (ptr == NULL) return false;                                   \
  list->items = ptr;                                               \
  list->cap = ncap;                                                \
  return true;                                                     \
}

#define define_list_append(T_Item, T_List)                        \
bool T_List ## _append (T_List *list, T_Item item) {              \
  if (!T_List ## _reserve(list, list->len+1)) return false;       \
  list->items[list->len] = item;                                  \
  list->len++;                                                    \
  return true;                                                    \
}

#define define_list_last(T_Item, T_List)                          \
T_Item * T_List ## _last (T_List *list) {                         \
  if (list->len == 0) return NULL;                                \
  return list->items + (list->len - 1);                           \
}

#define define_list_pop(T_Item, T_List)                           \
bool T_List ## _pop (T_List *list, T_Item *out) {                 \
  if (list->len == 0) return false;                               \
  if (out) *out = list->items[--list->len];                       \
  return true;                                                    \
}

#define define_list_swap(T_Item, T_List)                          \
bool T_List ## _swap(T_List *list, uisz i, uisz j) {                \
  if (i > list->len || j > list->len) return false;               \
  T_Item t = list->items[i];                                      \
  list->items[i] = list->items[j];                                \
  list->items[j] = t;                                             \
  return true;                                                    \
}

#define define_list_clear(T_List)                                 \
void T_List ## _clear(T_List *list) {                             \
  list->len = 0;                                                  \
}

#define define_list_free(T_List)                                  \
void T_List ## _free(T_List *list) {                              \
  if (list->items != NULL) {                                      \
    platform_mem_free(list->items);                               \
  }                                                               \
  list->items = NULL;                                             \
  list->len = 0;                                                  \
  list->cap = 0;                                                  \
}

#define define_list_methods(T_Item, T_List)  \
define_list_reserve(T_Item, T_List)          \
define_list_append(T_Item, T_List)           \
define_list_pop(T_Item, T_List)              \
define_list_last(T_Item, T_List)             \
define_list_swap(T_Item, T_List)             \
define_list_clear(T_List)                    \
define_list_free(T_List)


#define list_foreach(T, it, list) for (T *it = (list)->items; it < (list)->items + (list)->len; ++it)

void *platform_mem_alloc   (uisz size);
void *platform_mem_realloc (void *ptr, uisz size);
void  platform_mem_free    (void *ptr);
void *platform_mem_copy    (void *dest, const void *src, uisz count);


#ifndef PLATFORM_TEMP_BUF_SIZE
#  define PLATFORM_TEMP_BUF_SIZE (1024*8*8)
#endif

uisz _platform_temporary_buffer_index = 0;
char _platform_temporary_buffer[PLATFORM_TEMP_BUF_SIZE];

void *platform_temp_alloc(uisz bytes_count);
uisz   platform_temp_save();
void  platform_temp_rewind(uisz save_point);

#ifdef PLATFORM_BROWSER

typedef double number_t;

#define Number_Fmt_Str    "%.4f"

extern double web_parse_number(const char *buf, uisz buf_size);
extern void   web_printf(const char *fmt, ...);
extern void   web_printfn(const char *fmt, ...);
extern void   web_temp_sprintfn(char *temp_buffer, const char *fmt, ...);
extern void   web_mfree(void *ptr);
extern void*  web_malloc(uisz bytes_count);
extern void*  web_mrealloc(void *base_ptr, uisz bytes_count);

#define platform_printf  web_printf
#define platform_printfn web_printfn
#define platform_temp_sprintf(...) web_temp_sprintfn(__VA_ARGS__)

#endif // PLATFORM_BROWSER


#ifdef PLATFORM_DESKTOP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#if defined(__SIZEOF_LONG_DOUBLE__) && LDBL_MANT_DIG >= 64
typedef long double    number_t;
#define Number_Fmt_Str "%.4Lg"
#else
#define Number_Fmt_Str "%.4f"
typedef double         number_t;
#endif


#define platform_printf printf
void platform_printfn(const char *fmt, ...);
const char *platform_temp_sprintf(char *fmt, ...);

#endif // PLATFORM_DESKTOP


#endif // _PLATFORM_H

#ifdef PLATFORM_IMPLEMENTATION

String_View sv_from_zstr(const char *zstr) {
  String_View sv = {0};
  sv.data = (char*) zstr;
  while (sv.data[sv.len] != 0) sv.len++;
  return sv;
}

bool sv_has_suffix(String_View sv, String_View suffix) {
  if (sv.len < suffix.len) return false;
  for (uisz i = 0; i < suffix.len; ++i) {
    char a = sv.data[sv.len - suffix.len + i];
    char b = suffix.data[i];
    if (a != b) return false;
  }
  return true;
}

bool sv_has_prefix(String_View sv, String_View prefix) {
  if (sv.len < prefix.len) return false;
  for (uisz i = 0; i < prefix.len; ++i) {
    char a = sv.data[i];
    char b = prefix.data[i];
    if (a != b) return false;
  }
  return true;
}

bool sv_eq_zstr(String_View sv, const char *zstr) {
  uisz zstr_sz = zstr_len(zstr);
  if (sv.len != zstr_sz) return false;
  for (uisz i = 0; i < sv.len; ++i) {
    char a = sv.data[i];
    char b = zstr[i];
    if (a != b) return false;
  }
  return true;
}

bool sv_eq_sv(String_View sv_a, String_View sv_b) {
  if (sv_a.len != sv_b.len) return false;
  for (uisz i = 0; i < sv_a.len; ++i) {
    char a = sv_a.data[i];
    char b = sv_b.data[i];
    if (a != b) return false;
  }
  return true;
}


void *platform_temp_alloc(uisz bytes_count) {
  if (_platform_temporary_buffer_index >= PLATFORM_TEMP_BUF_SIZE) {
    platform_printfn("[ERROR] Temp buffer size exceeded");
    return NULL;
  }
  void *ptr = &_platform_temporary_buffer[_platform_temporary_buffer_index];
  _platform_temporary_buffer_index += bytes_count;
  return ptr;
}

uisz platform_temp_save() {
  return _platform_temporary_buffer_index;
}
void  platform_temp_rewind(uisz save_point) {
  _platform_temporary_buffer_index = save_point;
}


#ifdef PLATFORM_DESKTOP
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void platform_printfn(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  putchar('\n');
}

double parse_number(String_View sv) {
  char buffer[sv.len+1];
  memcpy(buffer, sv.data, sv.len);
  buffer[sv.len] = 0;
  return atof((const char *)buffer);
}

void *platform_mem_alloc(uisz size) {
  return malloc(size);
}

void *platform_mem_realloc(void *ptr, uisz size) {
  return realloc(ptr, size);
}

void platform_mem_free(void * ptr) {
  free(ptr);
}

void *platform_mem_copy(void *dest, const void *src, uisz count) {
  return memcpy(dest, src, count);
}

uisz zstr_len(const char *zstr) {
  return strlen(zstr);
}

const char *platform_temp_sprintf(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n_int = vsprintf(NULL, fmt, ap);
  va_end(ap);
  if (n_int < 0) return NULL;
  uisz n = (uisz)n_int;
  char *buffer = platform_temp_alloc(n);
  if (buffer == NULL) return NULL;
  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);
  return buffer;
}

#endif // PLATFORM_DESKTOP

#ifdef PLATFORM_BROWSER

uisz zstr_len(const char *zstr) {
  uisz len = 0;
  char c;
  while ((c = zstr[len++]) != 0) { }
  len--;
  return len;
}

double parse_number(String_View sv) {
  return web_parse_number((const char *)sv.data, sv.len);
}

void *platform_mem_alloc(uisz size) {
  return web_malloc(size);
}

void *platform_mem_realloc(void *ptr, uisz size) {
  return web_mrealloc(ptr, size);
}

void platform_mem_free(void *ptr) {
  web_mfree(ptr);
}

void *platform_mem_copy(void *dest, const void *src, uisz count) {
  char *dbuf = (char*)dest;
  const char *sbuf = (const char *)src;
  for (uisz i = 0; i < count; ++i) {
    *dbuf = *sbuf;
    dbuf++;
    sbuf++;
  }
  return dest;
}

#endif // PLATFORM_BROWSER

#endif
