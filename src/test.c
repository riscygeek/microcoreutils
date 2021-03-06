//  Copyright (C) 2021 Benjamin Stürz
//  
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

#define PROG_NAME "test"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "errprintf.h"
#include "common.h"

#define streq(s1, s2) (strcmp(s1, s2) == 0)

static const char* progname;

static int parse_expr_xsi(int*, int, char*[]);
static int parse_expr(int* const i, int argc, char* argv[]) {
   if (*i == argc) return 1;
   else if (!*argv[*i]) return ++*i, 1;
   else if (streq(argv[*i], "!")) return ++*i, !parse_expr_xsi(i, argc, argv);
   else if (streq(argv[*i], "(")) {
      ++*i;
      const int expr = parse_expr_xsi(i, argc, argv);
      if (!streq(argv[*i], ")")) {
         fprintf(stderr, "%s: expected ')'\n", progname);
         exit(2);
      }
      ++*i;
      return expr;
   } else if (*argv[*i] == '-') {
      const char op = argv[*i][1];
      ++*i;
      if ((argc - *i) == 0) return 0;

      struct stat st;
      switch (op) {
      case 'b':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         else return (st.st_mode & S_IFMT) != S_IFBLK;
      case 'c':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         else return (st.st_mode & S_IFMT) != S_IFCHR;
      case 'd':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         else return (st.st_mode & S_IFMT) != S_IFDIR;
      case 'e': return !!stat(argv[(*i)++], &st);
      case 'f':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         else return (st.st_mode & S_IFMT) != S_IFREG;
      case 'g':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         else return !(st.st_mode & S_ISGID);
      case 'h':
      case 'L':
         if (lstat(argv[(*i)++], &st) != 0) return 1;
         else return (st.st_mode & S_IFMT) != S_IFLNK;
      case 'n': return !*argv[(*i)++];
      case 'p':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         else return (st.st_mode & S_IFMT) != S_IFIFO;
      case 'r':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         if (geteuid() == st.st_uid) return !(st.st_mode & S_IRUSR);
         else if (getegid() == st.st_gid) return !(st.st_mode & S_IRGRP);
         else return !(st.st_mode & S_IROTH);
      case 'S':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         else return (st.st_mode & S_IFMT) != S_IFSOCK;
      case 's':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         else return st.st_size == 0;
      case 't': {
         char* endp;
         const int tty = strtol(argv[(*i)++], &endp, 10);
         return *endp ? 1 : !isatty(tty);
      }
      case 'u':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         else return !(st.st_mode & S_ISUID);
      case 'w':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         if (geteuid() == st.st_uid) return !(st.st_mode & S_IWUSR);
         else if (getegid() == st.st_gid) return !(st.st_mode & S_IWGRP);
         else return !(st.st_mode & S_IWOTH);
      case 'x':
         if (stat(argv[(*i)++], &st) != 0) return 1;
         if (geteuid() == st.st_uid) return !(st.st_mode & S_IXUSR);
         else if (getegid() == st.st_gid) return !(st.st_mode & S_IXGRP);
         else return !(st.st_mode & S_IXOTH);
      case 'z': return !!*argv[(*i)++];
      default:  return 0;
      }
   }
   else if ((argc - *i) >= 3) {
      if (streq(argv[*i + 1], "=")) {
         const int val = strcmp(argv[*i], argv[*i + 2]) != 0;
         *i += 3;
         return val;
      }
      else if (streq(argv[*i + 1], "!=")) {
         const int val = strcmp(argv[*i], argv[*i + 2]) == 0;
         *i += 3;
         return val;
      }
      else if (streq(argv[*i + 1], "-o") || streq(argv[*i + 1], "-a")) return !*argv[(*i)++];
      else if (argv[*i + 1][0] == '-') {
         int val;
         char* endp;
         const long a = strtol(argv[*i], &endp, 10);
         if (*endp) {
            fprintf(stderr, "%s: '%s': integer expression expected\n", progname, argv[*i]);
            exit(2);
         }
         const long b = strtol(argv[*i + 2], &endp, 10);
         if (*endp) {
            fprintf(stderr, "%s: '%s': integer expression expected\n", progname, argv[*i + 2]);
            exit(2);
         }

         if (streq(argv[*i + 1], "-eq")) val = a == b;
         else if (streq(argv[*i + 1], "-ne")) val = a != b;
         else if (streq(argv[*i + 1], "-gt")) val = a >  b;
         else if (streq(argv[*i + 1], "-lt")) val = a <  b;
         else if (streq(argv[*i + 1], "-ge")) val = a >= b;
         else if (streq(argv[*i + 1], "-le")) val = a <= b;
         else {
            fprintf(stderr, "%s: '%s': expected binary operator\n", progname, argv[*i + 1]);
            exit(2);
         }

         *i += 3;
         return !val;
      }
   }
   ++*i;
   return 0;
}

static int parse_expr_and(int* const i, int argc, char* argv[]) {
   int left = parse_expr(i, argc, argv);
   while ((argc - *i) > 0) {
      if (streq(argv[*i], "-a")) {
         ++*i;
         const int right = parse_expr(i, argc, argv);
         left = !(!left && !right);
      }
      else break;
   }
   return left;
}
static int parse_expr_xsi(int* const i, int argc, char* argv[]) {
   int left = parse_expr_and(i, argc, argv);
   while ((argc - *i) > 0) {
      if (streq(argv[*i], "-a")) {
         ++*i;
         const int right = parse_expr_and(i, argc, argv);
         left = !(!left && !right);
      }
      else break;
   }
   return left;
}

int main(int argc, char* argv[]) {
   int i = 1;

   progname = basename(argv[0]);

   // check if the program name is '['
   if (strcmp(progname, "[") == 0) {
      if (strcmp(argv[argc - 1], "]") != 0) {
         fputs("[: expecting ']'\n", stderr);
         return 2;
      }
      --argc;
   }

   if (argc == 1) return 1;
   else return parse_expr_xsi(&i, argc, argv); 
}
