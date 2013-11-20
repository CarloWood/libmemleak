This library was written to detect memory leaks in any application
by preloading it (you might also need to preload libbfd and libdl).

Usage (example):

LD_PRELOAD='/usr/local/lib/libmemleak.so /usr/lib/x86_64-linux-gnu/libdl.so /usr/lib/libbfd.so' ./a.out

After starting the application, connect to it by running 'memleak_control',
provided in the package. Type 'help' on its command prompt.

