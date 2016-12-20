#! /bin/sh

# Clueless user check.
if test ! -d .git -a -f configure; then
  echo "You only need to run './autogen.sh' when you checked out this project from the git repository."
  echo "Just run ./configure [--help]."
  echo "If you insist on running it, then first remove the 'configure' script."
  exit 0
fi

exec cwm4/scripts/bootstrap.sh
