proc* range [number] -> [number]
  dup yield
  1 +
  dup yield
  2 +
  dup yield
end

3 range gen_next drop gen_next drop gen_next drop print_number