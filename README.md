# libcoro-dbg
Utilities to examine backtraces of coroutines in coredump files, that
work with modified version of [libcoro](https://github.com/semistrict/libcoro).
Currently work in progress.

### Main ideas
_libcoro_'s `coro_transfer` function is modified to save coroutine execution
state on each invocation. This state is saved into a table in process
address space, that can afterwards be read from a coredump file in 
case of crash.

_libcorostacks_ library is used to read coroutine state table 
and retrieve meaningful information about execution state. 
It uses _elfutils_ under the hood for reading coredumps and 
stack unwinding.

_print-coroutines_ is a simple program to output informaition
obtained from _libcorostacks_.

Example program and modified libcoro can be found in *demo* directory. 

### Why not use GDB?
The thing is that GDB currently has no support for green threads and can't
do unwinding outside of main stack without running process.
So plans are to implement python script for GDB that will provide a way to 
inspect coroutines in coredump via libcorostacks. \
[Link to discussion](https://lore.kernel.org/all/87ilsn784k.fsf@oldenburg.str.redhat.com/T/)

### How to build
#### Install dependencies
```sh
sudo apt install libelf-dev libdw-dev
```
#### Build using Meson
```sh
mkdir -p build
meson setup build --prefix=<where_to_install>
cd build
meson compile
meson install
```

### How to use demo
Inside libcoro-dbg/build directory:
1) `gdb demo/corodemo` & `start` - launch demo with GDB
2)  `Ctrl+c` & `gcore` after some time, to generate core file
3) `q` - exit GDB
4) `print-coroutines/print-coroutines core.<pid>` - print backtraces
