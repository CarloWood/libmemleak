# Installation

## Getting the source

### git

To obtain the source from github,

    git clone --recursive https://github.com/CarloWood/libmemleak.git

this will also get the needed git submodule `cwm4`.

[ or, clone without --recursive and then:
    cd libmemleak
    git submodule init
    git submodule update
]

Next run `./autogen.sh` to generate the `configure` script
and `Makefile.in` files.

### tar ball

*OR* download a tar ball from [github](https://github.com/CarloWood/libmemleak/releases)

and extract with

    tar xzf libmemleak.*.tar.gz
    cd libmemleak

## Configuration

Run

    ./configure [--prefix=/usr]

to configure, or try

    ./configure --help

for more options.

In this case you might want to configure with --enable-maintainer-mode;
then ./autogen.sh needs to be rerun after running `make maintainer-clean`.
So don't do that unless you cloned the source from git (and have autogen.sh).

## Compilation

After configuration, compile the library with:

    make

And install with

    sudo make install

See README.md for usage documentation of this library.
Or https://github.com/CarloWood/libmemleak for the
html version there of.
