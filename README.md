# QLeii

QLeei is a simple interpreted stack based language written for experimental purposes.
This is not meant to be a serious language in any way.

The language is handled in a a single pass, so it attempts to have no AST and execute things as soon as it sees them.

Try it out at the [online playground](https://jmnuf.github.io/qleei-lang/playground/)

## Language Constructs

### Types

In QLeei as of now there are 3 types:
- number:   Represented as a 64 bit floating point number
- bool:     Whatever is the bool type in "stdbool.h" which we copy pasted from the header in my machine
- pointer:  Some address to the heap

Inputs towards called procedures will be type checked at runtime! Outputs don't get type checked as of now.

### Intrinsics

We have a set of intrinsic procedures defined for the sake of operating on the stack, a bit of memory usage, and basic printing to stdout.

Syntax used to list the intrinsics follows this format: `<name> :: [<inputs>] -> [<outputs>]` inputs/outputs will be refered as existing types or generics if not an existing type. Inputs and outputs will be read as top being left most value.

Printing:
- print_number :: [number]  -> []
  - Consumes a number and prints it to stdout with a newline
- print_ptr    :: [pointer]  -> []
  - Consumes a pointer and prints it to stdout with a newline
- print_bool :: [bool]  -> []
  - Consumes a boolean and prints it to stdout with a newline
- print_char   :: [number]  -> []
  - Consumes a number, casts it to an C char (8 bit integer) and prints it to stdout with a newline
- print_zstr   :: [pointer] -> []
  - Consumes a pointer expecting it to be a null terminated string and prints it to stdout with a newline
- print_stack  :: [] -> []
  - Prints the current stack to stdout read as top to bottom from left to right, can be used for debugging a program

General Stack Operations:
- dup          :: [a] -> [a, a]
  - Duplicates the element at the top of the stack.
- drop         :: [a] -> []
  - Drops the first element of the stack.
- rot2         :: [a, b] -> [b, a]
  - Swaps the top and second to top elements of the stack.
- swap2        :: [a, b] -> [b, a]
  - Alias of `rot2`. Swaps the top and second to top elements of the stack.
- rot3         :: [a, b, c] -> [b, c, a]
  - Rotates the top 3 elements of the stack, so the first (top) goes to third, second goes to first and third goes to second.
- swap3        :: [a, b, c] -> [c, b, a]
  - Swaps the 1st element with the 3rd element of the stack. It's the same as doing `rot3 rot2`
- over         :: [a, b] -> [b, a, b]
  - Duplicates the 2nd element of the stack at the top of the stack. It's the same as doing `rot2 dup rot3`

Memory Management:
- mem_alloc          :: [number] -> [pointer]
  - Consumes a number which is casted to an unsigned integer used for saying how many bytes to allocate. Program crashes if allocation fails.
- mem_free           :: [pointer] -> []
  - Consumes a pointer and frees the memory related to this pointer.
- mem\_load\_ui8     :: [pointer] -> [number]
  - Consumes a pointer to read its value as an unsigned integer of 8 bits
- mem\_save\_ui8     :: [pointer, number] -> []
  - Consumes a pointer to write to it the value as an unsigned integer of 8 bits
- mem\_load\_ui32    :: [pointer] -> [number]
  - Consumes a pointer to read its value as an unsigned integer of 32 bits
- mem\_save\_ui32    :: [pointer, number] -> []
  - Consumes a pointer to write to it the value as an unsigned integer of 32 bits

## Loops

QLeei only supports while loops and if you don't know how while loops work I think you are in the wrong place.
```qleei
while <condition> begin
    <body>
end
```

Example:
```qleei
// Printing numbers 1 to 69
1 while dup 70 - begin
    dup print_number
    1 +
end drop
```

## User Procedures

You can define your own procedures in QLeei by using the `proc` keyword.

When defining a procedure you must define an expected input types and supposed output types. The input is type checked at RUNTIME!
Output types are not type checked at all, they are there to make you feel better about yourself.
```qleei
proc <name> [<..inputs>] -> [<..outputs>] <body> end
```

Examples:
```qleei
// Receives no input and outputs a single number
proc ascii_E [] -> [number] 69 end

// Receives 2 numbers as input and outputs nothing
proc print_sum [number, number] -> [] + print_number end
```

## Generators

Generators are procedures that can suspend their execution and resume later. They are useful for creating iterators and lazy sequences.

Define a generator using the `proc*` keyword:
```qleei
proc* <name> [<..inputs>] -> [<..outputs>] <body> end
```

Example:
```qleei
proc* counter [] -> [] 1 yield 2 yield end
```

- yield :: [value] -> []
  - The `yield` keyword suspends the generator and returns a value to the caller. Takes the value on top of the stack, saves the generator's current state (stack and position), and suspends execution returning control to the caller.

- gen_next :: [generator] -> [value, generator, is_done]
  - The `gen_next` keyword resumes a suspended generator. Requires a generator on top of the stack, resumes from where it last yielded, and pushes the yielded value (if any), generator, and done flag.

Example:
```qleei
proc* counter [] -> [] 1 yield 2 yield end

counter              // Creates generator: [generator]
gen_next             // Resumes, yields 1: [1, generator, false]
rot3 swap3 drop swap2 print_number  // Prints 1
gen_next             // Resumes, yields 2: [2, generator, false]
rot3 swap3 drop swap2 print_number  // Prints 2
gen_next             // Generator ends: [generator, true]
swap2 drop print_bool  // Prints true
```

