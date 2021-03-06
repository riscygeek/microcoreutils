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

#define PROG_NAME "chgrp"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include "errprintf.h"
#include "common.h"
#include "chown.h"

static int recursive;

int main(int argc, char* argv[]) {
   if (argc < 2) {
print_usage:
      fputs("Usage: chown [-h] group file...\n", stderr);
      fputs("       chown -R [-H|-L|-P] group file...\n", stderr);
      return 1;
   }
   recursive = 0;
   int option;
   while ((option = getopt(argc, argv, ":RHLPh")) != -1) {
      switch (option) {
      case 'R':   recursive = 1; break;
      case 'h':   opt_h = 1; break;
      case 'H':   opt_upper = 'H'; break;
      case 'L':   opt_upper = 'L'; break;
      case 'P':   opt_upper = 'P'; break;
      default:    goto print_usage;
      }
   }
   if ((argc - optind) < 2) goto print_usage;

   const char* group = argv[optind++];
   unsigned tmp;
   gid_t gid;

   if (isdigit(group[0])) {
      if (!parse_number(group, &tmp)) {
         fprintf(stderr, "chgrp: invalid group '%s'\n", group);
         return 1;
      } else gid = tmp;
   } else {
      struct group* grp = getgrnam(group);
      if (!grp) {
         errprintf("failed to get group info for '%s'", group);
         return 1;
      }
      gid = grp->gr_gid;
   }

   int rv = 0;
   for (; optind < argc; ++optind) {
      const char* path = argv[optind];
      initial = 1;
      if (!do_chown(path, -1, gid)) rv = 1;
   }

   return rv;
}
