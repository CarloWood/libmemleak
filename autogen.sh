#! /bin/sh

if test -d .git; then
  # Just always bring submodules up to date.
  git submodule init
  git submodule update
else
  # Clueless user check.
  if -f configure; then
    echo "You only need to run './autogen.sh' when you checked out this project from the git repository."
    echo "Just run ./configure [--help]."
    if test -e cwm4/scripts/bootstrap.sh; then
      echo "If you insist on running it and know what you are doing, then first remove the 'configure' script."
    fi
    exit 0
  elif test ! -e cwm4/scripts/bootstrap.sh
    echo "Houston, we have a problem: the cwm4 git submodule is missing from your source tree."
    echo "I'd suggest to clone the source code of this project from github:"
    echo "git clone --recursive https://github.com/CarloWood/libmemleak.git"
    exit 1
  fi
fi

exec cwm4/scripts/bootstrap.sh
