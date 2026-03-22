proc print_range [number] -> [number]
  0 while dup swap3 dup swap3 - rot3 swap3 begin
    dup print_number // [N, i] -> [N, i, i] -> [N, i]
    1 +       // -> [N, i, 1] -> [N, i]
  end swap2 drop    // -> [i]
end

proc* @range [number] -> [number]
  0 while dup swap3 dup swap3 - rot3 swap3 begin
    dup yield // [N, i] -> [N, i, i] -> [N, i]
    // dup print_number // [N, i] -> [N, i, i] -> [N, i]
    1 +       // -> [N, i, 1] -> [N, i]
  end swap2 drop    // -> [i]
end

6 mem_alloc
  dup 0 + 'W' rot2 mem_save_ui8
  dup 1 + 'h' rot2 mem_save_ui8
  dup 2 + 'i' rot2 mem_save_ui8
  dup 3 + 'l' rot2 mem_save_ui8
  dup 4 + 'e' rot2 mem_save_ui8
  dup 5 +  0  rot2 mem_save_ui8
  dup print_zstr
mem_free

3 print_range print_number

10 mem_alloc
  dup 0 + 'G' rot2 mem_save_ui8
  dup 1 + 'e' rot2 mem_save_ui8
  dup 2 + 'n' rot2 mem_save_ui8
  dup 3 + 'e' rot2 mem_save_ui8
  dup 4 + 'r' rot2 mem_save_ui8
  dup 5 + 'a' rot2 mem_save_ui8
  dup 6 + 't' rot2 mem_save_ui8
  dup 7 + 'o' rot2 mem_save_ui8
  dup 8 + 'r' rot2 mem_save_ui8
  dup 9 +  0  rot2 mem_save_ui8
  dup print_zstr
mem_free

3 @range while gen_next 1 - begin
  print_number
end print_stack
