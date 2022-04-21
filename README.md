# Assign1-CLE

# ex1

## Compiling

```c
gcc -Wall -o3 -o ex1/src/main ex1/src/main.c ex1/src/shared_region.c  -lpthread -lm
```

## Usage examples

```c
./main numberWorkers [files]

./main 1 text0.txt
./main 4 text0.txt
./main 4 text0.txt text1.txt text2.txt text3.txt text4.txt
```


# ex2

## Compiling

```c
gcc -Wall -o3 -o ex2/src/main ex2/src/main.c ex2/src/shared_region.c  -lpthread -lm
```

## Usage examples

```c
./main numberWorkers [files]

./main 1 ex2/matrices/mat128_32.bin
./main 4 ex2/matrices/mat128_32.bin
./main 4 ex2/matrices/mat128_32.bin ex2/matrices/mat128_256.bin
```