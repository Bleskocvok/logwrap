# logwrap

[![test and build badge](https://github.com/Bleskocvok/logwrap/actions/workflows/build-test.yml/badge.svg)](https://github.com/Bleskocvok/logwrap/actions/workflows/build-test.yml)
[![coverage badge](https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/wiki/Bleskocvok/logwrap/coverage.md)](https://github.com/Bleskocvok/logwrap/actions/workflows/build-test.yml)

Simple logging utility to pipe output lines into separate processes.

It can be useful in instances when you want to handle the output of a program
in a separate way from the actual executable. E.g., you are unable to modify
the program or the handling of logging is in a different language.

One of the main features is the option to coalesce consecutive lines to a single
executed process using the command line option `-s seconds`.

Usage:

```
logwrap ‹app› ‹cmd_for_stdout› ‹cmd_for_stderr›
```

```
logwrap ‹app› [args...] -- ‹cmd_for_stdout› [args...] -- ‹cmd_for_stderr› [args...]
```

- `app` is the command/executable to run
- `cmd_for_stdout` is the command/executable to pipe `stdout` to
- `cmd_for_stderr` is the command/executable to pipe `stderr` to


## Example

Sample invocation:

```
logwrap ./app -- ./send-email out -- ./send-email err
```

Suppose `app` writes these lines to standard output and then terminates.
`send-email` is a supposed executable that is placed in the working folder and
let's say, for the purpose of this example, will send an e-mail to a particular
address.

```
A
B
Hello World
```

The `./send-email out` command would be run three times, with each line passed to
`stdin`. `logwrap` would wait for each invocation to complete before moving on
to the next one.

In the case that the `./send-email` executable takes a long time to perform its
task, it might be useful to invoke `logwrap` with the `-s` option. This
requires a number of seconds. When `./app` produces a line of output,
`./logwrap` will wait the given number of seconds before executing the
corresponding logging process (for `stdout` or `stderr`) and it will group any lines to
the same instance of executed `./send-email`.

In the above example, if `./logwrap` was given the option `-s 1.5` and lines
`A`, `B` and `Hello World` were sent in the span of one second, after an
additional 0.5 seconds, `./send-email out` would be run and all three lines will be
passed to its standard input.

Another useful option is `-d`, which will achieve that each output process is
run detached from the main process and `./app` will never have to be delayed to
wait for the completion of `./send-email`. This will, however, introduce
a race condition between the processes, which can result in a later line
being logged before an earlier one.

So, for this specific use case, the utility could be run with these
parameters.

```
logwrap -s 1.5 -d ./app -- ./send-email out -- ./send-email err
```

## Installation

1. Build

    These commands will clone the project and build the `logwrap` executable
    using `make` and `c99`. If you have a POSIX compatible system, the `make`
    and `c99` utilities should be already present, so no need to have installed
    anything other than `git` to build this project.

    ```
    git clone https://github.com/Bleskocvok/logwrap.git
    cd logwrap
    make
    ```

2. Install

    The resulting executable can be moved to a directory listed in `PATH`.
    Which can be for instance `~/bin`.

    ```
    cp logwrap ~/bin/
    ```

3. Run

    In any directory type to see if installed properly.

    ```
    logwrap -h
    ```
