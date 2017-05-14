/**********************************************************************
*           Author: Daniel Johnson & Alan Tyson
*           Last Update: 2016-03-30
*
*     ODDFUNCT.H is a catch-all header for nifty utility functions we
*   seem to keep using.
*
**********************************************************************/

/*********************************************************************/
/*********************************************************************/
int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit / 8;  j = bit % 8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}


/*********************************************************************/
/*********************************************************************/
int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}


/*********************************************************************/
/*********************************************************************/
int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}


/*********************************************************************/
/*********************************************************************/
void tokenize(char *source, char *dest[], char *delimiter)
{
  char str_buff[256];
  char *token;
  int i=0;

  LOG(LOG_DBG, "source: %s, delimiter: %s\n", source, delimiter);

  if(!source || !dest || !delimiter)
  {
    printf("%sInvalid input!%s\n", COLOR_RED, COLOR_RESET);
    return;
  }
  strcpy(str_buff, source);

  LOG(LOG_DBG, "Tokenizing string.\n");
  token = strtok(str_buff, delimiter);

  while(token)
  {
    dest[i] = (char *)malloc(sizeof(char) * (strlen(token)+1));
    strcpy(dest[i++], token);
    token = strtok(0, delimiter);
  }
  dest[i] = 0;
}


/*********************************************************************/
/*********************************************************************/
init()
{
  int i, j;
  PROC *p;

  for (i=0; i<NMINODES; i++)
      minode[i].refCount = 0;

  for (i=0; i<NMOUNT; i++)
      mounttab[i].busy = 0;

  for (i=0; i<NPROC; i++){
      proc[i].status = FREE;
      for (j=0; j<NFD; j++)
          proc[i].fd[j] = 0;
      proc[i].next = &proc[i+1];
  }

  for (i=0; i<NOFT; i++)
      oft[i].refCount = 0;

  LOG(LOG_DBG, "mounting root\n");
    mountroot();
  LOG(LOG_DBG, "mounted root\n");
  LOG(LOG_DBG, "creating P0, P1\n");

  p = running = &proc[0];
  p->status = BUSY;
  p->uid = 0;
  p->pid = p->ppid = p->gid = 0;
  p->parent = p->sibling = p;
  p->child = 0;
  p->cwd = root;
  p->cwd->refCount++;

  p = &proc[1];
  p->next = &proc[0];
  p->status = BUSY;
  p->uid = 2;
  p->pid = 1;
  p->ppid = p->gid = 0;
  p->cwd = root;
  p->cwd->refCount++;

  nproc = 2;
}


/*********************************************************************/
/*********************************************************************/
int quit()
{
  // write YOUR quit function here
  int k = 0;
  LOG(LOG_DBG, "writing everything back to disk\n");
  for(k = 0; k < NMINODES; k++)
  {
    if(minode[k].ino != 0){
      quit_worker(&minode[k]);
    }
  }
   exit(0);
}

//precondition: mip is already pointing to the correct minode inside the Minode array
int quit_worker(MINODE *mip)
{
  INODE *inode_temp = 0;
  char buf[BLOCK_SIZE];
  int i = 0, j = 0;

  LOG(LOG_DBG, "attempting quit_worker on mip->dev: %d mip->ino %d block: %d\n", mip->dev, mip->ino, inumtoblock(mip->ino));

  // Use minode's (dev, ino) to determine which INODE on disk
  get_block(mip->dev, inumtoblock(mip->ino), buf);
  //Read that block into a buf[ ], let INODE *ip point at the INODE in buf[ ].
  inode_temp = (INODE *)buf + inumtooffset(mip->ino);

  inode_temp->i_mode = mip->INODE.i_mode;
  inode_temp->i_uid = mip->INODE.i_uid;
  inode_temp->i_size = mip->INODE.i_size;
  inode_temp->i_atime = mip->INODE.i_atime;
  inode_temp->i_ctime = mip->INODE.i_ctime;
  inode_temp->i_mtime = mip->INODE.i_mtime;
  inode_temp->i_dtime = mip->INODE.i_dtime;
  inode_temp->i_gid = mip->INODE.i_gid;
  inode_temp->i_links_count = mip->INODE.i_links_count;

  for(j = 0; j < 15; j++)
  {
    inode_temp->i_block[j] = mip->INODE.i_block[j];
  }

  //Write the block (in buf[ ]) back to disk.
  put_block(mip->dev, inumtoblock(mip->ino), buf);
}



/*********************************************************************/
/*********************************************************************/
MOUNT *getmountp(int devID)
{
  int i = 0;

  for (i = 0; i < NMOUNT; i++) {
     if (mounttab[i].dev == devID) {
       return &mounttab[i];
     }
  }
  return 0;
}


/*********************************************************************/
/*********************************************************************/
int findCmd(char *cmd)
{
  if(strcmp(cmd, "menu") == FALSE)
  {
    return 0;
  }
  else if(strcmp(cmd, "mkdir") == FALSE)
  {
    return 1;
  }
  else if(strcmp(cmd, "cd") == FALSE)
  {
    return 2;
  }
  else if(strcmp(cmd, "pwd") == FALSE)
  {
    return 3;
  }
  else if(strcmp(cmd, "ls") == FALSE)
  {
    return 4;
  }
  else if(strcmp(cmd, "rmdir") == FALSE)
  {
    return 5;
  }
  else if(strcmp(cmd, "creat") == FALSE)
  {
    return 6;
  }
  else if(strcmp(cmd, "link") == FALSE)
  {
    return 7;
  }
  else if(strcmp(cmd, "unlink") == FALSE)
  {
    return 8;
  }
  else if(strcmp(cmd, "symlink") == FALSE)
  {
    return 9;
  }
  else if(strcmp(cmd, "rm") == FALSE)
  {
    return 10;
  }
  else if(strcmp(cmd, "chmod") == FALSE)
  {
    return 11;
  }
  else if(strcmp(cmd, "chown") == FALSE)
  {
    return 12;
  }
  else if(strcmp(cmd, "stat") == FALSE)
  {
    return 13;
  }
  else if(strcmp(cmd, "touch") == FALSE)
  {
    return 14;
  }
  else if(strcmp(cmd, "readlink") == FALSE)
  {
    return 15;
  }
  else if(strcmp(cmd, "pfd") == FALSE)
  {
    return 16;
  }
  else if(strcmp(cmd, "cat") == FALSE)
  {
    return 17;
  }
  else if(strcmp(cmd, "cp") == FALSE)
  {
    return 18;
  }
  else if(strcmp(cmd, "mv") == FALSE)
  {
    return 19;
  }
  else if(strcmp(cmd, "quit") == FALSE || strcmp(cmd, "exit") == FALSE)
  {
    return 20;
  }
  else
  {
    return -1;
  }
}


/*********************************************************************/
/*********************************************************************/
void printDir(DIR *dir)
{
  MINODE *mp;
  LOG(LOG_DBG, "inode: %d\n", dir->inode);
  LOG(LOG_DBG, "rec_len: %d\n", dir->rec_len);
  LOG(LOG_DBG, "name_len: %d\n", dir->name_len);
  LOG(LOG_DBG, "name: %s\n", dir->name);
}

/*********************************************************************/
/*********************************************************************/
void printInode(INODE *ip)
{
  int i = 0;
  LOG(LOG_DBG, "mode=%4x \n", ip->i_mode);
  LOG(LOG_DBG, "uid=%d  gid=%d\n", ip->i_uid, ip->i_gid);
  LOG(LOG_DBG, "size=%d\n", ip->i_size);
  LOG(LOG_DBG, "links_count=%d\n", ip->i_links_count);
  LOG(LOG_DBG, "i_atime=%d\n", ip->i_atime);

  for (i = 0; i < 12; i++) {
    LOG(LOG_DBG, "i_block[%d]=%d\n", i, ip->i_block[i]);
  }
}


/*********************************************************************/
/*********************************************************************/
void printMinode(MINODE *mp)
{
  LOG(LOG_DBG, "name=%s \n", mp->name);
  LOG(LOG_DBG, "dev=%d \n", mp->dev);
  LOG(LOG_DBG, "ino=%d \n", mp->ino);
  LOG(LOG_DBG, "refCount=%d \n", mp->refCount);
  LOG(LOG_DBG, "dirty=%d \n", mp->dirty);
  LOG(LOG_DBG, "mounted=%d \n", mp->mounted);
  LOG(LOG_DBG, "mountptr=%d \n", mp->mountptr);

  printInode(&(mp->INODE));
}

/*********************************************************************/
/*********************************************************************/
void printMinodeArray()
{
  int k = 0;
  for(k = 0; k < NMINODES; k++)
  {
    if(minode[k].refCount > 0){
      printMinode(&minode[k]);
    }
  }
}


/*********************************************************************/
/*********************************************************************/
int menu()
{
  printf("\n");
  printf("%s************************************************%s\n", COLOR_GREEN, COLOR_RESET);
  printf("*%s***********Dan & Alan ext2 simulator**********%s*\n", COLOR_RED, COLOR_RESET);
  printf("* %s-------  LEVEL 1 ------------%s                *\n", COLOR_MAGENTA, COLOR_RESET);
  // printf("* %smount_root%s                                   *\n", COLOR_YELLOW, COLOR_RESET);
  printf("* %smkdir, rmdir, ls, cd, pwd%s                    *\n", COLOR_YELLOW, COLOR_RESET);
  printf("* %screat, link,  unlink, symlink%s                *\n", COLOR_YELLOW, COLOR_RESET);
  printf("* %sstat,  chmod, chown, touch%s                          *\n", COLOR_YELLOW, COLOR_RESET);
  printf("*                                              *\n");
  printf("* %s-------  LEVEl 2 -------------%s             *\n", COLOR_MAGENTA, COLOR_RESET);
  printf("* %sopen,  close,  read,  write%s                *\n", COLOR_YELLOW, COLOR_RESET);
  printf("* %slseek  cat,    cp,    mv%s                   *\n", COLOR_YELLOW, COLOR_RESET);
  printf("*                                              *\n");
  //printf("* %s-------  LEVEl 3 ------------%s              *\n", COLOR_MAGENTA, COLOR_RESET);
  //printf("* %smount, umount%s                              *\n", COLOR_YELLOW, COLOR_RESET);
  //printf("* %sFile permission checking%s                   *\n", COLOR_YELLOW, COLOR_RESET);
  printf("* %s-----------------------------%s                *\n", COLOR_MAGENTA, COLOR_RESET);
  printf("%s************************************************%s\n", COLOR_GREEN, COLOR_RESET);
  return 0;
}
