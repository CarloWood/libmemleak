# A preloaded memory leak detection library

This library was written to detect memory leaks in any application
by preloading it (you might also need to preload libbfd and libdl).

## Usage example

`LD_PRELOAD='/usr/local/lib/libmemleak.so' ./a.out`

Possibly you'll need to preload libbfd and libdl too, so use `LD_PRELOAD='/usr/local/lib/libmemleak.so /usr/lib/x86_64-linux-gnu/libdl.so /usr/lib/x86_64-linux-gnu/libbfd.so'`.

After starting the application, connect to it by running `memleak_control`,
provided in the package. Type 'help' on its command prompt.

<pre>
```
$ memleak_control
libmemleak> help
help     : Print this help.
start    : Erase all intervals and start recording the first interval.
stop     : Stop recording.
restart  : Start a new interval. Keep, and possibly combine, previous intervals.
delete   : Delete the oldest interval.
stats    : Print overview of backtrace with highest leak probability.
stats N  : Automatically print stats every N seconds (use 0 to turn off).
restart M: Automatically restart every N * M stats.
list N   : When printing stats, print only the first N backtraces.
dump N   : Print backtrace number N.
libmemleak> start
Auto restart interval is 6 * 10 seconds.
</pre>
```

## The memleak_control executable

The library does not print any memory leak stats by default: one MUST connect to it
with `memleak_control` and give the command `start` to begin recording an interval.

To print an overview of backtraces with highest leak probability, give the command `stats`.
To stop recording give the command `stop`. The number of backtraces printed (starting with
the one with the highest leak probability) can be controlled with the command `list N`
where `N` is the number of backtraces to print each stats.

Backtraces are reported with an integer number (N). To dump the actual stacktrace use
the command `dump N`. Backtraces are also written to a file `memleak_backtraces` in
the current directory.

Printing stats can be automated, as if the command `stats` is given every N seconds, with
the command `stats N`.

Restarting an interval can be automated, as if the command `restart` is given even N * M seconds,
with the command `restart M`.

## Environment variables

The following environment variables can be set to configure
the library:

* `LIBMEMLEAK_SOCKNAME` : Path to the filename used for the UNIX socket that is used for communication between `libmemleak.so` and `memleak_control`. The default is `"./memleak_sock"`.
* `LIBMEMLEAK_STATS_INTERVAL` : The (initial) time in seconds between printing memory leak stats. The default is 1 second. This value can be changed on the fly through `memleak_control` with the command `stats N` where `N` is a decimal value in seconds (or 0 to turn off printing of stats).
* `LIBMEMLEAK_RESTART_MULTIPLIER` : The (initial) restart multiplier. The default is 5. This value can be changed on the fly through `memleak_control` with the command `restart M` where `M` is a decimal value. The restart multiplier must be at least 2. It causes a new interval to be automatically started every N * M seconds, where N is the stats print interval (see `LIBMEMLEAK_STATS_INTERVAL`).

