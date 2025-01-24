# Dungeons and Diagrams

This is a solver and puzzle generator for the zacktronics game "Dungeons and Diagrams" which is mini-game in "Last Call BBS".

A summary of the rules can be found [here](https://trashworldnews.com/files/advanced_dungeons_and_diagrams.pdf)

It implements a puzzle solver and a valid puzzle generator. They are both depth first backtracking search variants that iterate through valid partial solutions (or puzzles) until they find a full one.
Currently the solver will find all solutions, instead of stopping at the first one. The generator will only generate as many puzzles as you ask for since there are a TON of valid puzzles. I haven't tried generating them all and I don't know how long it would take to run.

## Building and Running
```
CC -o dandd ./dandd.c
./dandd
```

It should run with any modern c compiler under any modern c version (>= c99). You can look at the github actions workflow file for examples of how to build it on various platforms. The github actions workflow tests that it works on these os/compiler/c-versions. (It also runs it under address sanitizer where applicable.)
* windows 
  * mingw-64 (c99, c11, c17, c2x)
  * clang (c99, c11, c17, c2x)
  * msvc (c11, c17)
* macos
  * gcc (c99, c11, c17, c2x)
  * clang (c99, c11, c17, c2x)
* linux
  * gcc (c99, c11, c17, c2x)
  * clang (c99, c11, c17, c2x)
