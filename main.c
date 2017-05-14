/**********************************************************************
*           Authors: Alan Tyson & Daniel Johnson
*           Last Update: 2016-03-30
*
*     EXT2 Filesystem Manageer Final Project Thingy
*   project for CptS 360 more info at the following link:
*   http://www.eecs.wsu.edu/~cs360/proj10.html
*
**********************************************************************/

#include "includes.h"


int main(int argc, char const *argv[]) {
  /* DEBUGGING CONFIGURATION*/
  dbgstream = stdout;
  debug_level = 2;
  /* DEBUGGING CONFIGURATION*/

  int i,cmd;
  LOG(LOG_DBG, "Before running init\n");
  init();
  LOG(LOG_DBG, "After running init.\n");
  while(1){
      printf( "(P%d running) ", running->pid);
      cmd = -1;

      /* set the strings to 0 */
      memset(pathname, 0, 64);
      memset(parameter,0, 64);
      memset(command, 0, 64);
      memset(line, 0, 128);

      printf( "command: ");
      gets(line);
      LOG(LOG_DBG, "Check for null input\n");
      if (line[0]==0) continue;
      LOG(LOG_DBG, "Split line ( %s ) to command, pathname, and parameter\n", line);
      sscanf(line, "%s %s %64c", command, pathname, parameter);

      cmd = findCmd(command);
      LOG(LOG_DBG, "findCmd returns %d\n", cmd);
      if(cmd == -1)
      {
        printf( "Invalid command\n");
        continue;
      }
      fptr[cmd]();
    }

    return 0;
}
