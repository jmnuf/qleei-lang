
proc the_E [] -> []
    5 mem_alloc

    dup 0 + 69 rot2 mem_save_ui8
    dup 1 + 69 rot2 mem_save_ui8
    dup 2 + 69 rot2 mem_save_ui8
    dup 3 + 69 rot2 mem_save_ui8
    dup 4 +  0 rot2 mem_save_ui8

    dup print_zstr

    mem_free
end

proc square [number] -> [number]
    dup *
end

proc cube [number] -> [number]
    dup dup * *
end

the_E

2 square
print_number

3 cube
print_number

3 3 3 * * print_number
