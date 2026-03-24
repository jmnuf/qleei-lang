proc* n123 [] -> []
    1 yield
    2 yield
    3 yield
end

n123 while gen_next print_stack 1 - begin
    print_number
end

