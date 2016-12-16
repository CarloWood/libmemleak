# A preloaded memory leak detection library

This library was written to detect memory leaks in any application
by preloading it (you might also need to preload libbfd and libdl).

## Usage example

`LD_PRELOAD='/usr/local/lib/libmemleak.so' ./a.out`

Possibly you'll need to preload libbfd and libdl too, so use `LD_PRELOAD='/usr/local/lib/libmemleak.so /usr/lib/x86_64-linux-gnu/libdl.so /usr/lib/x86_64-linux-gnu/libbfd.so'`.

After starting the application, connect to it by running `memleak_control`,
provided in the package. Type 'help' on its command prompt.

<pre>
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

When using this on the executable `./hello` provided in the package,
the output, after a couple of minutes, will look something like this:

<pre>
hello: Now: 287;        Backtraces: 77;         allocations: 650036;    total memory: 83,709,180 bytes.
backtrace 50 (value_n: 104636.00); [ 178, 238>(  60): 25957 allocations (1375222 total,  1.9%), size 3311982; 432.62 allocations/s, 55199 bytes/s
backtrace 50 (value_n: 104636.00); [  55, 178>( 123): 52722 allocations (2793918 total,  1.9%), size 6734135; 428.63 allocations/s, 54749 bytes/s
backtrace 49 (value_n: 58296.00); [ 178, 238>(  60): 14520 allocations (1382814 total,  1.1%), size 1860716; 242.00 allocations/s, 31011 bytes/s
backtrace 49 (value_n: 58296.00); [  55, 178>( 123): 29256 allocations (2794155 total,  1.0%), size 3744938; 237.85 allocations/s, 30446 bytes/s
</pre>

Showing two intervals here: from 55 seconds after start till 178 seconds after start,
and a second interval between 178 and 238 seconds. The size of each interval is
shown between brackets for convenience (123 and 60 seconds respectively).

The number of not-freed allocations is printed next with the total number
of allocations done in that interval and the percentage of non-freed
allocations between brackets. Finally the total amount of leaked memory
in bytes, and the number of leaks in allocations and bytes per second
is given.

As you can see in this example, backtrace 50 leaks about twice as much as backtrace 49.
In fact, backtrace 49 doesn't really leak at all (it just naturally causes the heap
to grow in the beginning), but backtrace 50 does (deliberately) have a bug that causes
leaking on top of that.

We can print both backtraces from `memleak_control` with the commands,

<pre>
libmemleak> dump 49
 #0  00007f84b862d33b  in malloc at /home/carlo/projects/libmemleak/libmemleak-objdir/src/../../libmemleak/src/memleak.c:1008
 #1  00000000004014da  in do_work(int)
 #2  000000000040101c  in thread_entry0(void*)
 #3  00007f84b7e7070a  in start_thread
 #4  00007f84b7b9f82d  in ?? at /build/glibc-Qz8a69/glibc-2.23/misc/../sysdeps/unix/sysv/linux/x86_64/clone.S:111
libmemleak> dump 50
 #0  00007f84b862d33b  in malloc at /home/carlo/projects/libmemleak/libmemleak-objdir/src/../../libmemleak/src/memleak.c:1008
 #1  00000000004014da  in do_work(int)
 #2  0000000000401035  in thread_entry1(void*)
 #3  00007f84b7e7070a  in start_thread
 #4  00007f84b7b9f82d  in ?? at /build/glibc-Qz8a69/glibc-2.23/misc/../sysdeps/unix/sysv/linux/x86_64/clone.S:111
</pre>

So apparently the leak is caused by a call from `thread_entry1()`.

## The memleak_control executable

The library does not print any memory leak stats by default: one MUST connect to it
with `memleak_control` and give the command `start` to begin recording an interval.

To print an overview of backtraces with highest leak probability, give the command `stats`.
To stop recording give the command `stop`. The number of backtraces printed (starting with
the one with the highest leak probability) can be controlled with the command `list N`
where `N` is the number of backtraces to print each stats.

Backtraces are reported with an integer number (N). To dump the actual stacktrace use
the command `dump N`. Backtraces are also written to a file `memleak_backtraces` in
the current directory every time a `stats` command is executed, so all backtraces
are available at all times, even if the program crashes or halts.

Printing stats can be automated, as if the command `stats` is given every N seconds, with
the command `stats N`.

Restarting an interval can be automated, as if the command `restart` is given even N * M seconds,
with the command `restart M`. Note that when a restart is performed, a new interval is started
but the old intervals are kept. Older intervals however are combined (made into a larger interval),
growing exponentially in size the older they are. In virtually all cases, when you
run an application with libmemleak and started recording, then all that is needed is to
do whatever causes the leak for a while, with an `N * M` seconds (see below) being a bit smaller
than the period over which the leak should be detectable (that is, not too large so that
pre- and/or initialization allocations might "confuse" libmemleak, and also not too short
so that the amount leaked disappears in the noise of normal allocations), and the memory leak
backtrace will be the one that is reported at the top.

Since libmemleak pushes older intervals to the bottom (you can delete them with the
command `delete`) and combines intervals to make larger and larger ones, intervals
being too short or too long should never be a problem really when the leak is continuous:
at some point in time the leak should become clearly visible in a slowly increasing number
of printed intervals.

Hence, you can just sit back and watch until you see the leak pop-up on the top.

In the case of the `hello` test program, the leak is detected immediately.
Using `stats 1` and `restart 2` will show the correct backtrace after 2 seconds.

## Environment variables

The following environment variables can be set to configure
the library:

* `LIBMEMLEAK_SOCKNAME` : Path to the filename used for the UNIX socket that is used for communication between `libmemleak.so` and `memleak_control`. The default is `"./memleak_sock"`.
* `LIBMEMLEAK_STATS_INTERVAL` : The (initial) time in seconds between printing memory leak stats. The default is 1 second. This value can be changed on the fly through `memleak_control` with the command `stats N` where `N` is a decimal value in seconds (or 0 to turn off printing of stats).
* `LIBMEMLEAK_RESTART_MULTIPLIER` : The (initial) restart multiplier. The default is 5. This value can be changed on the fly through `memleak_control` with the command `restart M` where `M` is a decimal value. The restart multiplier must be at least 2. It causes a new interval to be automatically started every N * M seconds, where N is the stats print interval (see `LIBMEMLEAK_STATS_INTERVAL`).

