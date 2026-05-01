cmake_minimum_required(VERSION 3.13)

project(ncc)

file(GLOB HDRS "include/*.h")
file(GLOB SRC "src/*.c")

set(${CFLAGS} 
-D _DEBUG -ggdb3 -std=c23 -O0 -Wall -Wextra 
-Waggressive-loop-optimizations -Wmissing-declarations 
-Wcast-align -Wcast-qual -Wchar-subscripts 
-Wconditionally-supported -Wconversion -Wctor-dtor-privacy 
-Wempty-body -Wfloat-equal -Wformat-nonliteral
-Wformat-signedness -Wformat=2 -Winline -Wlogical-op 
-Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual 
-Wpacked -Wpointer-arith -Winit-self -Wredundant-decls 
-Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel 
-Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods 
-Wsuggest-final-types -Wsuggest-override -Wswitch-default
-Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast 
-Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers 
-Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector 
-fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow 
-flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=8192 
-Wstack-usage=8192 -pie -fPIE -Werror=vla -Wswitch-enum -Wformat-security 
-fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,
float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,
null,object-size,return,returns-nonnull-attribute,shift,
signed-integer-overflow,undefined,unreachable,vla-bound,vptr)

add_executable(${PROJECT_NAME} ${HDRS} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_options(${PROJECT_NAME} PRIVATE ${CFLAGS})
target_link_options(${PROJECT_NAME} PRIVATE ${CFLAGS})
