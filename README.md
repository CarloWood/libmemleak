# A preloaded memory leak detection library

This library was written to detect memory leaks in any application
by preloading it (you might also need to preload libbfd and libdl).

Usage (example):

`LD_PRELOAD='/usr/local/lib/libmemleak.so /usr/lib/x86_64-linux-gnu/libdl.so /usr/lib/x86_64-linux-gnu/libbfd.so' ./a.out`

After starting the application, connect to it by running `memleak_control`,
provided in the package. Type 'help' on its command prompt.

## Environment variables

The following environment variables can be set to configure
the library:

* `LIBMEMLEAK_SOCKNAME` : Path to the filename used for the UNIX socket that is used for communication between `libmemleak.so` and `memleak_control`. The default is `"./memleak_sock"`.
* `LIBMEMLEAK_STATS_INTERVAL` : The (initial) time in seconds between printing memory leak stats. The default is 1 second. This value can be changed on the fly through `memleak_control` with the command `stats N` where `N` is a decimal value in seconds (or 0 to turn off printing of stats).
* `LIBMEMLEAK_RESTART_MULTIPLIER` : The (initial) restart multiplier. The default is 5. This value can be changed on the fly through `memleak_control` with the command `restart M` where `M` is a decimal value. The restart multiplier must be at least 2. It causes the memory leaks stats to be reset (restarted) every N * M seconds, where N is the stats print interval (see `LIBMEMLEAK_STATS_INTERVAL`).

## `memleak_control`

