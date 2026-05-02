# NANO COMPILER COLLECTION

*Нано — это такое, маленькое    ©Анатолий Чубайс*

## Репозиторий

### Ветки
Репозиторий состоит из 2 веток: `master` и `nasm_backend`.
На ветке `master` эмулятор, на ветке `nasm_backend` — компилятор
в ассемблер `nasm`.

### Структура проекта
.\
├── `CMakeLists.txt`\
├── `include`\
│   ├── `colors.h`        (цветной вывод ошибок)\
│   ├── `def\_macro.h`    (\#define макросов)\
│   ├── `ncc.h`           (осовной заголовок)\
│   └── `undef\_macro.h`  (\#undef макросов)\
├── `readme.md`\
└── `src`\
    ├── `ast.c`           (функции работы с ast)\
    ├── `ast\_dump.c`     (генератор дампа ast для `graphviz`)\
    ├── `backend.c`       (генерация asm-кода)\
    ├── `grammar.txt`     (грамматика парсера)\
    ├── `input.c`         (чтение входных файлов)\
    ├── `main.c`          (cli)\
    ├── `parser.c`        (парсер)\
    └── `tokenizer.c`     (токенизатор)\

## Синтаксис
```
/* This is a comment */

func main()
{
    x = 0; y = 0;            /* without types */

    swap(&x, &y);            /* has pointers */

    return x;

    asm("; asm comment");   /* inline asm supported */
}

func swap(px, py)
{
    x = [px];               /* asm-style dereference */
    [px] = [py];
    [py] = x;

    return;                 /* return always needed */
}

func loop_factorial(x)
{
    i = 1;
    ans = 1;

    while(i < x or i == x)  /* C/C++ like loops */
    {
        ans *= i;   i = i+1;
    }

    return ans;
}

func recursive_factorial(x)
{
    if(x > 1)   return x*recursive_factorial(x);

    return 1;
}
```

## Компиляция

### В исполняемый файл
```bash
$ ncc <file>.cmm --asm <output_asm>.nasm  # компиляция в nasm
$ nasm <output_nasm>.nasm -f elf64        # компиляция nasm
$ ld <output_nasm>.o -o <executable>      # линковка
$ ./<executable>
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
-   Генерация бинарного файла без зависимостей
-   Добавление типизации
-   Строки
-   Стандартная библиотека
