# QLeii

QLeei is a simple interpreted stack based language written for experimental purposes.
This is not meant to be a serious language in any way.

This language tries to do as little "parsing" as possible so it runs the code as it sees it.

Try it out at the [github page](https://jmnuf.github.com/qleei-lang/web/)

## Language Constructs

### Types

In QLeei as of now there are only 2 types:
- number:   Represented as a 64 bit floating point number
- pointer:  Some address to the heap

Inputs towards called procedures will be type checked at runtime! Outputs don't get type checked as of now.

### Intrinsics

We have a set of intrinsic procedures defined for the sake of operating on the stack, a bit of memory usage, and basic printing to stdout.

Syntax used to list the intrinsics follows this format: `<name> :: [<inputs>] -> [<outputs>]` inputs/outputs will be refered as existing types or generics if not an existing type. Inputs and outputs will be read as top being left most value.

Printing:
- print_number :: [number]  -> []
  - Consumes a number and prints it to stdout with a newline
- print_char   :: [number]  -> []
  - Consumes a number, casts it to an C char (8 bit integer) and prints it to stdout with a newline
- print_zstr   :: [pointer] -> []
  - Consumes a pointer expecting it to be a null terminated string and prints it to stdout with a newline
- print_stack  :: [] -> []
  - Prints the current stack to stdout read as top to bottom from left to right, can be used for debugging a program

General Stack Operations:
- dup          :: [a] -> [a, a]
  - Duplicates the element at the top of the stack
- drop          :: [a] -> []
  - Drops the first element of the stack
- rot2         :: [a, b] -> [b, a]
  - Swaps the top and second to top elements of the stack
- rot3         :: [a, b, c] -> [b, c, a]
  - Rotates the top 3 elements of the stack, so the first (top) goes to third, second goes to first and third goes to second

Memory Management:
- mem_alloc   :: [number] -> [pointer]
  - Consumes a number which is casted to an unsigned integer used for saying how many bytes to allocate. Program crashes if allocation fails.
- mem_free    :: [pointer] -> []
  - Consumes a pointer and frees the memory related to this pointer.
- mem\_load\_ui8    :: [pointer] -> [number]
  - Consumes a pointer to read its value as an unsigned integer of 8 bits
- mem\_save\_ui8    :: [pointer, number] -> []
  - Consumes a pointer to write to it the value as an unsigned integer of 8 bits
- mem\_load\_ui32    :: [pointer] -> [number]
  - Consumes a pointer to read its value as an unsigned integer of 32 bits
- mem\_save\_ui32    :: [pointer, number] -> []
  - Consumes a pointer to write to it the value as an unsigned integer of 32 bits

## User Procedures

You can define your own procedures in QLeei by using the `proc` keyword. 
Fun fact, currently this is the only thing that requires "parsing" in the language!

When defining a procedure you must define an expected input types and supposed output types. The input is type checked at RUNTIME!
Output types are not type checked at all, they are there to make you feel better about yourself.

Syntax is like:
```qleei
proc <name-of-proc> [<..inputs>] -> [<..outputs>] <body> end
```

Examples:
```qleei
// Receives no input and outputs a single number
proc ascii_E [] -> [number] 69 end

proc alloc_hello_world_zstr [] -> [pointer]
    14 mem_alloc // Allocates 14 bytes of memory, 13 for for characters and one more for the null byte
    dup  0 +  72 rot2 mem_save_ui8 // 'H'
    dup  1 + 101 rot2 mem_save_ui8 // 'e'
    dup  2 + 108 rot2 mem_save_ui8 // 'l'
    dup  3 + 108 rot2 mem_save_ui8 // 'l'
    dup  4 + 111 rot2 mem_save_ui8 // 'o'
    dup  5 +  44 rot2 mem_save_ui8 // ','
    dup  6 +  32 rot2 mem_save_ui8 // ' '
    dup  7 +  87 rot2 mem_save_ui8 // 'W'
    dup  8 + 111 rot2 mem_save_ui8 // 'o'
    dup  9 + 114 rot2 mem_save_ui8 // 'r'
    dup 10 + 108 rot2 mem_save_ui8 // 'l'
    dup 11 + 100 rot2 mem_save_ui8 // 'd'
    dup 12 +  33 rot2 mem_save_ui8 // '!'
    dup 13 +   0 rot2 mem_save_ui8 // '\0'
end

// Input is the desired array length in this case
proc alloc_u32_array [number] -> [pointer]
    dup 32 * mem_alloc
	rot2 1 -
	// We actually don't have loops yet so this is just a theoretical syntax that doesn't work KEKW
	while dup 0 < begin // Zero out array
		dup rot3 dup rot3 rot2
		32 * + 0
		rot2 mem_save_ui32
		rot2 1 -
	end
	drop
end
```

