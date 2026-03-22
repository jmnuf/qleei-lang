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

3 @range while gen_next 1 - begin
  print_number
end print_stack
