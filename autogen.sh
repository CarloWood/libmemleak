#! /bin/sh

if test -d .git; then
  # Always update the submodule(s).
  git submodule init
  if ! git submodule update --recursive; then
    echo "autogen.sh: Failed to update submodules. Did you make uncommitted changes?"
    exit 1
  fi
  # Do more if we are the real maintainer.
  cwm4/scripts/real_maintainer.sh 15014aea5069544f695943cfe3a5348c
  RET=$?
  if test $RET -eq 2; then
    # Recursively ran autogen.sh, so we're done.
    exit 0
  elif test $RET -ne 0; then
    # An error occurred.
    exit 1
  fi
else
  # Clueless user check.
  if test -f configure; then
    echo "You only need to run './autogen.sh' when you checked out this project from the git repository."
    echo "Just run ./configure [--help]."
    if test -e cwm4/scripts/bootstrap.sh; then
      echo "If you insist on running it and know what you are doing, then first remove the 'configure' script."
    fi
    exit 0
  elif test ! -e cwm4/scripts/bootstrap.sh; then
    echo "Houston, we have a problem: the cwm4 git submodule is missing from your source tree."
    echo "I'd suggest to clone the source code of this project from github:"
    echo "git clone --recursive https://github.com/CarloWood/libmemleak.git"
    exit 1
  fi
fi

# Run the autotool commands.
exec cwm4/scripts/bootstrap.sh
