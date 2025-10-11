
proc ascii_null [] -> [number] 0 end
proc ascii_equal_sign [] -> [number] 61 end

proc alloc_divisor [] -> [ptr]
    61 mem_alloc
    0
    // 59
    while dup 59 - begin
        // [n, ptr] -> [ptr, n] -> [ptr, ptr, n] -> [n, ptr, ptr] -> [n, n, ptr, ptr] -> [ptr, n, n, ptr] -> [ptr_off, n, ptr]
        rot2 dup swap3 dup swap3 +

        // [ptr_off, n, ptr] -> ['=', ptr_off, n, ptr] -> [ptr_off, '=', n, ptr] -> [n, ptr]
        ascii_equal_sign rot2 mem_save_si8

        1 +
    end drop
    // !
    dup 60 +
    ascii_null rot2 mem_save_si8
end

alloc_divisor // !
dup print_ptr
dup print_zstr

4 while dup begin         // [i, ptr] -> [i, i, ptr] -> [i, ptr]
    1 rot2 -              // [i, ptr] -> [1, i, ptr] -> [i, 1, ptr] -> [i, ptr]
    dup print_uisz
    rot2 dup print_zstr
    rot2

    // 10 repeat begin
    //     69 print_char
    // end

    dup while dup begin      // [i, ptr] -> [j, i, ptr] -> [j, j, i, ptr] -> [j, i, ptr]
        dup 2 * print_number // [j, i, ptr] -> [n, j, i, ptr] -> [2, n, j, i, ptr] -> [n, j, i, ptr] -> [j, i, ptr]
        1 rot2 -             // [j, i, ptr] -> [1, j, i, ptr] -> [j, 1, i, ptr] -> [j, i, ptr]
    end drop                 // [j, i, ptr] -> [i, ptr]
end
