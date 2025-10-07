
4
while dup begin
    1 rot2 -
    dup print_number
    dup while dup begin
        dup 2 * print_number
        1 rot2 -
    end
    drop
end
