0 while dup 4 rot2 - begin print_stack
    dup print_uisz
    1 +
end

while dup begin
    dup 1 'a' - + print_char
    1 rot2 -
end
