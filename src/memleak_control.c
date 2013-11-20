// libmemleak -- Detect leaking memory by allocation backtrace
//
//! @file memleak.c Keep track of all allocations, sorted by backtrace.
//
// Copyright (C) 2010, by
// 
// Aleric Inglewood <aleric.inglewood@gmail.com>
// Aleric on freenode.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <readline/readline.h>

void error(char*);

// A static variable for holding the line.
static char* line_read = NULL;

// Read a string, and return a pointer to it. Returns NULL on EOF.
char* rl_gets()
{
  // If the buffer has already been allocated, return the memory to the free pool.
  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  // Get a line from the user.
  line_read = readline("libmemleak> ");

  // If the line has any text in it, save it on the history.
  if (line_read && *line_read)
    add_history(line_read);

  return line_read;
}

void main(int argc, char* argv[])
{
  int sockfd, servlen, n;
  struct sockaddr_un serv_addr;
  char buffer[82];

  memset((char*)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  char const* sockname = getenv("LIBMEMLEAK_SOCKNAME");
  if (!sockname)
    sockname = "memleak_sock";
  strcpy(serv_addr.sun_path, sockname);
  servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    error("Creating socket");
  if (connect(sockfd, (struct sockaddr*)&serv_addr, servlen) < 0)
  {
    if (errno == ENOENT)
    {
      fprintf(stderr, "%s: connect: %s: No such file or directory\n", argv[0], sockname);
      fprintf(stderr, "%s: Set the environment variable LIBMEMLEAK_SOCKNAME to open a different socket.\n", argv[0]);
      exit(0);
    }
    error("Connecting");
  }
  for(;;)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    select(sockfd + 1, &rfds, NULL, NULL, NULL);
    if (FD_ISSET(sockfd, &rfds))
    {
      size_t len = read(sockfd, buffer, 80);
      if (len == 0)
      {
	printf("Application terminated.\n");
	exit(0);
      }
      int do_prompt = 0, quit = 0;
      if (len >= 7 && strncmp(buffer + len - 7, "PROMPT\n", 7) == 0)
      {
	do_prompt = 1;
	len -= 7;
      }
      else if (len >= 5 && strncmp(buffer + len - 5, "QUIT\n", 5) == 0)
      {
	len -= 5;
	quit = 1;
      }
      if (len > 0)
	write(1, buffer, len);
      if (quit)
      {
	printf("Application terminated.\n");
	exit(0);
      }
      if (do_prompt)
      {
	char* line = NULL;
	while (!line || !*line)
	  line = rl_gets();
	write(sockfd, line, strlen(line));
      }
    }
  }
}

void error(char* msg)
{
  perror(msg);
  exit(0);
}
