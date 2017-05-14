/**********************************************************************
*           Authors: KC Wang, Daniel Johnson & Alan Tyson
*           Last Update: 2016-03-30
*
*     Constants and Globals for the project. Keeping these
*   seperate to make life a little easier.
*
*   NOTE: This originally was type.h from KC
*
**********************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>   // NOTE: Ubuntu users MAY NEED "ext2_fs.h"
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

#define BLOCK_SIZE     1024
#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2

#define ROOT_INODE        2

// Default dir and regular file modes
#define DIR_MODE    0x41ED
#define FILE_MODE   0x81A4
#define SUPER_MAGIC  0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define READY             1
#define RUNNING           2
#define BUSY              1

// Table sizes
#define NMINODES        100
#define NMOUNT           10
#define NPROC            10
#define NFD              10
#define NOFT            100

//for me - Alan
#define INVALID          -1
#define NULL              0
#define TRUE              1
#define FALSE             0

// Open File Table
typedef struct Oft{
  int   mode;
  int   refCount;
  struct Minode *minodeptr;
  long  offset;
} OFT;

// PROC structure
typedef struct Proc{
  int   uid;
  int   pid;
  int   gid;
  int   ppid;
  int   status;

  struct Minode *cwd;
  OFT   *fd[NFD];

  struct Proc *next;
  struct Proc *parent;
  struct Proc *child;
  struct Proc *sibling;
} PROC;

// In-memory inodes structure
typedef struct Minode{
  INODE INODE;               // disk inode
  int   dev, ino;

  int   refCount;
  int   dirty;
  int   mounted;
  struct Mount *mountptr;
  char     name[128];           // name string of file
} MINODE;

// Mount Table structure
typedef struct Mount{
  int  ninodes;
  int  nblocks;
  int  bmap;
  int  imap;
  int  iblock;
  int  dev, busy;
  struct Minode *mounted_inode;
  char   name[256];
  char   mount_name[64];
} MOUNT;

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";
char *rootdev = "mydisk", *slash = "/", *dot = ".", *dot2 = "..";
char pathname[128], parameter[128], *name[128], cwdname[128];
char command[64], line[128];
char ind_buf[BLOCK_SIZE];

int *ind_block;
int nnames;
int iblock;
int nproc=0;
int INODEBLOCK = 0, BBITMAP = 0, IBITMAP = 0;


MINODE *root;
MINODE minode[NMINODES];
MOUNT  mounttab[NMOUNT];
PROC   proc[NPROC], *running;
OFT    oft[NOFT];

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp;

/************
these prototypes are to fix compile errors, anything
defined here should be dummy until the .h is included*/
MINODE *iget(int dev, int ino);
MOUNT *getmountp(int devID);
OFT    *falloc(){}
int findCmd(char *cmd);

//-------------- LEVEL 1 ------------------
int menu();
int make_dir();
int change_dir();
int pwd();
int list_dir();
int rmdir();
int creat_file();
int link();
int unlink();
int symlink();
int rm_file();
int chmod_file();
int chown_file();
int stat_file();
int touch_file();
int read_link();

//-------------- LEVEL 2 ------------------
int open_file();
int close_file();
int pfd();
int cat_file();
int cp_file();
int mv_file();

//------------- LEVEL 3 -------------------
int mount(){}
int umount(){}
int cs(){}
int do_fork(){}
int do_ps(){}
int sync(){}
int quit();

//oh yeah!
int (*fptr[ ])() = {(int (*)())menu,\
                              make_dir,\
                              change_dir,\
                              pwd,\
                              list_dir,\
                              rmdir,\
                              creat_file,\
                              link,\
                              unlink,\
                              symlink,\
                              rm_file,\
                              chmod_file,\
                              chown_file,\
                              stat_file,\
                              touch_file,\
                              read_link,\
                              pfd,\
                              cat_file,\
                              cp_file,\
                              mv_file,\
                              quit};
