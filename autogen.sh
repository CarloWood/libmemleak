#! /bin/sh

# Clueless user check.
if test ! -d .git -a -f configure; then
  echo "You only need to run './autogen.sh' when you checked out this project from the git repository."
  echo "Just run ./configure [--help]."
  echo "If you insist on running it, then first remove the 'configure' script."
  exit 0
fi

if test ! -e cwm4/scripts/bootstrap.sh; then
  echo "*************************************************************************"
  echo "* This project has a submodule (cwm4) and should be clone-d with:"
  echo "* $ git clone --recursive $(git config --get remote.origin.url)"
  echo "* Note the '--recursive'."
  echo "*************************************************************************"
  echo "Trying to recover..."
  git submodule init
  git submodule update
fi

exec cwm4/scripts/bootstrap.sh
