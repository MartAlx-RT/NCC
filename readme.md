# NANO COMPILER COLLECTION

*Нано — это такое, маленькое    ©Анатолий Чубайс*

## Репозиторий

### Ветки
Репозиторий состоит из 4 веток: `master`, `nasm`,
`elf`, `elf-nasm`.

-   `master` — эмулятор
-   `nasm` — компилятор в ассемблер **nasm**
-   `elf` — компилятор в исполняемый **elf**
-   `elf-nasm` — то же, что и `elf`, но с дампом в **nasm** (*бетта-версия*)

### Структура проекта

Ветка `nasm`
.\
├── `CMakeLists.txt`\
├── `include`\
│   ├── `colors.h`        (цветной вывод ошибок)\
│   ├── `def_macro.h`    (\#define макросов)\
│   ├── `ncc.h`           (осовной заголовок)\
│   └── `undef_macro.h`  (\#undef макросов)\
├── `readme.md`\
└── `src`\
    ├── `ast.c`           (функции работы с ast)\
    ├── `ast_dump.c`     (генератор дампа ast для `graphviz`)\
    ├── `backend.c`       (генерация asm-кода)\
    ├── `grammar.txt`     (грамматика парсера)\
    ├── `input.c`         (чтение входных файлов)\
    ├── `main.c`          (cli)\
    ├── `parser.c`        (парсер)\
    └── `tokenizer.c`     (токенизатор)

Ветка `elf`
.\
├── `CMakeLists.txt`\
├── `include`\
│   ├── `def_emitters.h`    (\#define макросов для эмиттеров)\
│   ├── `def_grammar.h`     (\#define макросов для грамматики)\
│   ├── `def_perror.h`      (\#define макросов для вывода ошибок)\
│   ├── `emitter.h`         (эмиттеры)\
│   ├── `ncc.h`\
│   └── `undef_macros.h`    (\#undef макросов)\
└── `src`\
    ├── `ast.c`\
    ├── `ast_dump.c`\
    ├── `backend.c`\
    ├── `emitter.c`\
    ├── `grammar.txt`\
    ├── `main.c`\
    ├── `parser.c`\
    └── `tokenizer.c`

## Синтаксис
Приведем пример функции `printf`, написанной на языке `cmm`.
```
/* this is a comment */

/* function declaration */
func printf(fmt /*, ...*/)
{
    /* assembly insertion `nasm` will run for code in nasm("") */
	nasm("lea r8, [rbp+3*8]");	/* first va_arg addr */

    /* variable declaration */
	arg = 0;			        /* variable for args */

    /* `while` and `if` using C-like syntax */
    /* `[var]` is the `var` dereferencing
	while([fmt])
	{
		if([fmt] == '%')
		{
			fmt = fmt+1;

			if([fmt] == '%')	putc('%');
			else
			{
				nasm("mov rax, [r8]\n mov [rbp-8], rax\n add r8, 8");	/* get arg */

				if([fmt] == 'd')	    putn_base(arg,  10);    /* function call */
				else if([fmt] == 'x')	putn_base(arg,  16);
				else if([fmt] == 'b')	putn_base(arg,  2);
				else if([fmt] == 'o')	putn_base(arg,  8);
				else if([fmt] == 'c')	putc(arg);
				else if([fmt] == 's')	puts(arg);
				else if([fmt] == '*')	pass;                   /* `pass` means to do nothing
				else
				{
					puts("Runtime error: unexpected modifier\n");
					return;
				}
			}
		}
		else	putc([fmt]);

		fmt = fmt+1;
	}

    /* all functions need `return;` or `return value;` at the end
	return;
}
```

## Компиляция

### В исполняемый файл
```bash
$ ncc <file>.cmm -o <executable>    # компиляция в nasm
$ ./<executable>                    # запуск
```

### Дамп дерева абстрактного синтаксиса
```bash
$ ncc <file>.cmm --dump # дамп ast
$ xdg-open dump.csv
```

## Вывод ошибок

### Токенизатор
```bash
$ cat wrong_file.cmm
func main()
{
        x = 3; y = 4;

        if(x != y)      x = y;

        swap(&x, &y);

        return x;
}

$ ncc wrong_file.cmm --asm output.nasm

Tokenize: syntax error
-->!= y)        x = y;

        swap(...

Compile: tokenize failed
```

### Парсер
```bash
$ cat wrong_file.cmm
func main()
{
        x = 3; y = 4;

        swap(&x, &y)

        return x;
}

$ ncc wrong_file.cmm --asm output.nasm

error: simple.cmm:7:
missing ';'

Compile: parse failed
```

## Дальнейшее развитие проекта
-   Динамическая и статическая линковки
-   Добавление поддержки va_args компилятором
-   Добавление поддержки архитектуры arm
-   Добавление типизации

## Команда разработчиков
-   Мартынов Александр, школа ФРКТ МФТИ

## Благодарности
-   Илья Дединский (или просто дед)
-   Продва РТ-2025
-   Владимир Абубакиров, Данила Жебряков, и др. менторы
-   Александр Белокопытов

