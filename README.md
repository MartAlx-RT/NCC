This c-like language has weak typing.
To declare a variable or a pointer just write a name and initialize them:

a = 5;
b = &a;

To declare a function, type 'func' before it's name:

func useless() { pass; }

To use a variable via pointer, use asm-like syntax:

[b] = [b] * 2;

Standard library doesn't have an allocator. This simple excercise for the curious reader.
If you are shure that your index won't out from memory range, you can try to use arrays:

arr = 100; /* <--that's enough, I'm shure...(probably) */
i = 0; while (i < 10) { [arr + i] = i; i = i+1; }

Ok, now you are ready for programming on my incredible language. Good luck!

Report bugs to mav07@list.ru
If I don't have any problems with MATAN and FIZOS, I'll fix them.
