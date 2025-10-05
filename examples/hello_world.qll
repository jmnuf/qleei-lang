14 mem_alloc // Allocate 14 bytes of memory, 13 for for characters and one more for the null byte
  dup  0 +  72 rot2 mem_save_ui8
  dup  1 + 101 rot2 mem_save_ui8
  dup  2 + 108 rot2 mem_save_ui8
  dup  3 + 108 rot2 mem_save_ui8
  dup  4 + 111 rot2 mem_save_ui8
  dup  5 +  44 rot2 mem_save_ui8
  dup  6 +  32 rot2 mem_save_ui8
  dup  7 +  87 rot2 mem_save_ui8
  dup  8 + 111 rot2 mem_save_ui8
  dup  9 + 114 rot2 mem_save_ui8
  dup 10 + 108 rot2 mem_save_ui8
  dup 11 + 100 rot2 mem_save_ui8
  dup 12 +  33 rot2 mem_save_ui8
  dup 13 +   0 rot2 mem_save_ui8
  dup print_zstr
mem_free
