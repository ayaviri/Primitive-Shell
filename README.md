# Primitive Shell (psh)

A primitive shell in C. Completed as a project for CS 3650, Computer Systems, at Northeastern University.

## Capabilties

This shell is capable of the following: 

- Running system commands either by alias or file path (eg. `ls` or `/bin/ls`)
- Exiting via `exit` or EOF (Ctrl + D)
- Sequencing (eg. `echo "hello world" ; ls -a`)
- Input/output redirection (eg. `echo "hello world" > example.txt`)
- Piping (eg. `sort < words.txt | nl > sorted_nl.txt`)
- Execution of the previous command via `prev`

## Built-in Commands

Execute `help` for a brief description of the the 'built-in' commands.

```
shell $ help
available commands:
 * cd [/file/path/] - changes the current working directory
 * source [file.txt] - executes a script
 * prev - prints the previous command line and executes it again. cannot be executed in a sequence
 * help - explains all of the built-in commands
```

## The Makefile

The [Makefile](Makefile) contains the following targets:

- `make shell` - compile the shell
- `make clean` - perform a minimal clean-up of the source tree

