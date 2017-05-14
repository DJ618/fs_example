/**********************************************************************
*           Author: Alan Tyson & Daniel Johnson
*           Last Update: 2016-03-30
*
*     DEBUG_PRINT.H sets up print statements that write log messages
*   to the output stream (dbgstream). Statements will be prepended
*   with filename, and line number, followed by the message defined.
*
*     NOTE: code blatently stolen and modified from the following SO answers
*   http://stackoverflow.com/questions/327836/multi-file-c-program-how-best-to-implement-optional-logging/
*   http://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c
*
**********************************************************************/

#include <stdio.h>

extern FILE *dbgstream = 0;
extern int debug_level = 0;

#define LOG_FATAL    (1)
#define LOG_ERR      (2)
#define LOG_WARN     (3)
#define LOG_DBG      (4)
#define LOG_INFO     (5)


#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

char *colors[] = {
  COLOR_RED,\      //FATAL
  COLOR_RED,\      //ERR
  COLOR_YELLOW,\   //WARN
  COLOR_CYAN,\     //DBG
  COLOR_GREEN\     //INFO
}

#define LOG(level, ...) do {  \
                            if (level <= debug_level) { \
                              fprintf(dbgstream, "%s%s:%d:%s() ", colors[##level - 1], __FILE__, __LINE__, __func__); \
                              fprintf(dbgstream, __VA_ARGS__); \
                              fprintf(dbgstream,"%s", COLOR_RESET); \
                            } \
                        } while (0)

//Example Debugging, as debug level -> 1 more detail is shown
//LOG(LOG_DBG,  "DEBUG 5\n");
//LOG(LOG_INFO, "DEBUG 4\n");
//LOG(LOG_WARN, "DEBUG 3\n");
//LOG(LOG_ERR,  "DEBUG 2\n");
//LOG(LOG_FATAL,"DEBUG 1\n");
// printf("%sThis text is RED!%s\n", COLOR_RED, COLOR_RESET);
// printf("%sThis text is GREEN!%s\n", COLOR_GREEN, COLOR_RESET);
// printf("%sThis text is YELLOW!%s\n", COLOR_YELLOW, COLOR_RESET);
// printf("%sThis text is BLUE!%s\n", COLOR_BLUE, COLOR_RESET);
// printf("%sThis text is MAGENTA!%s\n", COLOR_MAGENTA, COLOR_RESET);
// printf("%sThis text is CYAN!%s\n", COLOR_CYAN, COLOR_RESET);
