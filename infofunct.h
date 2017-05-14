

/*********************************************************************/
/*********************************************************************/
int change_dir()
{
  MINODE *newcwd = 0;
  int newino;

  //guard check
  if(!pathname)
  {
    printf( "%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }
  //Set device and search startpoints
  if(pathname[0] == '/')  //if path is absolute
  {
    LOG(LOG_WARN, "%spathname is absolute!%s\n", COLOR_YELLOW, COLOR_RESET);
    //set dev to root device
    newcwd = root;
  }
  else  //relative paths
  {
    LOG(LOG_WARN, "%spathname is relative!%s\n", COLOR_YELLOW, COLOR_RESET);
    //set searchminode to cwd
    newcwd = running->cwd;
  }

  LOG(LOG_DBG, "cwd before changing dir\n");
  //printminode(running->cwd);

  //ready the sacrifices, here be black magic!
  newino = getino(newcwd->dev, pathname);
  newcwd = iget(newcwd->dev, newino);

  if (newcwd != INVALID) {
    running->cwd = newcwd;
    LOG(LOG_DBG, "cwd after changing dir\n");
    //printminode(newcwd);
    return TRUE;
  }

  LOG(LOG_DBG, "newcwd doesn't exist\n");
  return FALSE;

}


/*********************************************************************/
/*********************************************************************/
int pwd()
{

  MINODE *mp = running->cwd;

  LOG(LOG_DBG, "entering recursive pwd\n");
  r_pwd(mp);
  //tee hee
  printf("\n");
  LOG(LOG_DBG, "returned from recursive pwd\n");
}

int r_pwd(MINODE *mip)
{
  int myino, parent;
  char name[128];
  MINODE *pminode;

  LOG(LOG_DBG, "findino for self and parent\n");
  findino(mip, &myino, &parent);
  LOG(LOG_DBG, "comparint myino: %d to parent: %d\n", myino, parent);
  if(myino == parent)
  {
    printf("/");
    return;
  }
  LOG(LOG_DBG, "get inode of parent: %d\n", parent);
  pminode = iget(mip->dev, parent);

  r_pwd(pminode);

  LOG(LOG_DBG, "call findmyname to print name of inode %d from parent %d\n", myino, parent);
  findmyname(pminode, myino, name);
  printf("%s/", name);
}


/*********************************************************************/
/*********************************************************************/
int list_dir()
{
  MINODE *listmp = 0, *dirminode = 0;
  int newino, dev, i, j;
  char buf[BLOCK_SIZE], *cp;
  DIR *dp;

  //check if pathname exists, if not or empty use cwd
  if(pathname)
  {
      if(pathname[0] == '/')  //if path is absolute
      {
        LOG(LOG_DBG, "pathname is absolute!\n");
        //set dev to root device
        listmp = root;
        dev = root->dev;
      }
      else  //relative paths
      {
        LOG(LOG_DBG, "pathname is relative!\n");
        //set searchminode to cwd
        listmp = running->cwd;
        dev = root->dev;
      }
  }
  else
  {
    LOG(LOG_DBG, "pathname is relative!\n");
    //set searchminode to cwd
    listmp = running->cwd;
    dev = root->dev;
  }

  //get ino of inode to ls
  newino = getino(dev, pathname);

  //get the actual inode
  listmp = iget(dev, newino);

  //guard agains garbage
  if(listmp == INVALID || newino == INVALID || !S_ISDIR(listmp->INODE.i_mode))
  {
    printf( "%sPath (%s) is invalid%s\n", COLOR_RED, pathname, COLOR_RESET);
    return INVALID;
  }


  for (i = 0, j=0, ind_block = 0; i < 14; i++)
  {
    LOG(LOG_DBG, "check for unallocated blocks\n");
    if (listmp->INODE.i_block[i] == 0) {
      LOG(LOG_WARN, "%sunallocated iblock. leav LS.%s\n", COLOR_YELLOW, COLOR_RESET);
      iput(listmp);
      return TRUE;
    }

    LOG(LOG_DBG, "get data block to buf\n");
    get_block(listmp->dev, listmp->INODE.i_block[i], buf);

    //firsttime we've hit this indirect block get it to buffer
    if(i > 11)
    {
      //first time we've hit this i_block[i]
      if(ind_block == 0)
      {
        if(i == 13)
        {
          get_block(listmp->dev, buf + j, ind_buf);
        }
        else
        {
          get_block(listmp->dev, listmp->INODE.i_block[i], ind_buf);
        }
        ind_block = (int *)ind_buf;
      }

      //reached the end of an indirect block reset and loop
      if(ind_block == ind_buf + BLOCK_SIZE)
      {
        LOG(LOG_DBG, "out of indirect blocks\n");
        j++;
        ind_block = 0;
        if(i == 13)
        {
          i--;
        }
        //loop again
        continue;
      }

      //check for unallocated blcok
      if(*ind_block == 0)
      {
        LOG(LOG_WARN, "%sunallocated iblock. leav LS.%s\n", COLOR_YELLOW, COLOR_RESET);
        iput(listmp);
        return TRUE;
      }
      //i--;
      //get block based on ind_block location
      get_block(listmp->dev, *ind_block, buf);
      ind_block++;
    }

    cp = buf;
    dp = (DIR *) cp;

    while (cp < buf + BLOCK_SIZE)
    {
      dirminode = iget(dev, dp->inode);

      if(dirminode == INVALID)
      {
        printf( "%sinvalid dir entry!%s\n", COLOR_RED, COLOR_RESET);
        iput(listmp);
        return;
      }

      LOG(LOG_DBG, "copy name to minode\n");
      strncpy(dirminode->name, dp->name, dp->name_len);
      dirminode->name[dp->name_len] = 0;

      LOG(LOG_DBG, "pass minode to myls\n");
      myls(dirminode);
      iput(dirminode);
      cp += dp->rec_len;
      dp = (DIR *) cp;
    }
  }
  iput(listmp);
  return TRUE;
}



/*********************************************************************/
/*********************************************************************/
int myls(MINODE *currentdir)
{
  INODE *ip = &(currentdir->INODE);
  int r, i;
  char ftime[64], linkname[64];
  char *color = COLOR_RESET;

  if (currentdir == INVALID){
     return INVALID;
  }

  if ((ip->i_mode & 0xF000) == 0x8000)
  {
     printf("%c",'-');
     color = COLOR_RESET;
  }
  if ((ip->i_mode & 0xF000) == 0x4000)
  {
     printf("%c",'d');
     color = COLOR_BLUE;
  }
  if ((ip->i_mode & 0xF000) == 0xA000)
  {
     printf("%c",'l');
     color = COLOR_CYAN;
  }

  for (i=8; i >= 0; i--){
    if (ip->i_mode & (1 << i))
	printf("%c", t1[i]);
    else
	printf("%c", t2[i]);
  }

  printf("%4d ",ip->i_links_count);
  printf("%4d ",ip->i_gid);
  printf("%4d ",ip->i_uid);
  printf("%8d ",ip->i_size);

  // print time
  strcpy(ftime, ctime(&ip->i_ctime));
  ftime[strlen(ftime)-1] = 0;
  printf("%s  ",ftime);

  // print time
  strcpy(ftime, ctime(&ip->i_atime));
  ftime[strlen(ftime)-1] = 0;
  printf("%s  ",ftime);

  // print name
  printf("%s%s%s", color, basename(currentdir->name), COLOR_RESET);

  // print -> linkname if it's a symbolic file
  if ((ip->i_mode & 0xF000) == 0xA000){ // YOU FINISH THIS PART
     // use readlink() SYSCALL to read the linkname
     myreadlink(currentdir->ino, linkname);
     //linkname[ip->i_size] = 0;
     printf("%s -> %s%s", color, linkname, COLOR_RESET);
  }
  printf("\n");
}


/*********************************************************************/
/*********************************************************************/
int stat_file()
{
  int dev, i, fileino;
  char ftime[64];
  MINODE *mp;
  INODE *ip;

  if(strlen(pathname) < 1)
  {
    printf( "%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  LOG(LOG_DBG, "in make_dir() pathname = %s\n", pathname);

  //Set device and search startpoints
  if(pathname[0] == '/')  //if path is absolute
  {
    LOG(LOG_WARN, "%spathname is absolute!%s\n", COLOR_YELLOW, COLOR_RESET);
    //set dev to root device
    dev = root->dev;
  }
  else  //relative paths
  {
    LOG(LOG_WARN, "%spathname is relative!%s\n", COLOR_YELLOW, COLOR_RESET);
    //set dev to same dev as cwd Minode
    dev = running->cwd->dev;
  }

  //get ino of file
  fileino = getino(dev, pathname);
  mp = iget(dev, fileino);

  //file does not exist
  if (fileino == INVALID || mp == INVALID) {
    printf( "%sfile does not exist.%s\n", COLOR_RED, COLOR_RESET);
    iput(mp);
    return INVALID;
  }

  /* STAT OUTPUT FROM LINUX MINT
  File: ‘debug_print.h’
  Size: 1985      	Blocks: 8          IO Block: 4096   regular file
  Device: 801h/2049d	Inode: 3278111     Links: 1
  Access: (0644/-rw-r--r--)  Uid: ( 1000/    alan)   Gid: ( 1000/    alan)
  Access: 2016-04-25 17:39:44.463270615 -0700
  Modify: 2016-04-25 17:39:44.447270616 -0700
  Change: 2016-04-25 17:39:44.447270616 -0700
  Birth: -
  */
  //grab inode to make life easier
  ip = &(mp->INODE);
  //print file info
  printf("File: \'%s\'\n", basename(pathname));

  printf("size: %d\tBlocks: %d\tType: ", ip->i_size, ip->i_blocks);
  if ((ip->i_mode & 0xF000) == 0x8000)
  {
     printf("Regular");
  }
  if ((ip->i_mode & 0xF000) == 0x4000)
  {
    printf("Directory");
  }
  if ((ip->i_mode & 0xF000) == 0xA000)
  {
    printf("Link");
  }

  printf("Device: %x\tInode: %d\tLinks: %d\n", mp->dev, mp->ino, ip->i_links_count);

  printf("Permissions: ");
  for (i=8; i >= 0; i--){
    if (ip->i_mode & (1 << i))
  printf("%c", t1[i]);
    else
  printf("%c", t2[i]);
  }
  printf("\tUID: %d\tGID: %d\n", ip->i_uid, ip->i_gid);

  printf("Access: ");
  // print time
  strcpy(ftime, ctime(&ip->i_atime));
  ftime[strlen(ftime)-1] = 0;
  printf("%s\n",ftime);

  printf("Modify: ");
  // print time
  strcpy(ftime, ctime(&ip->i_atime));
  ftime[strlen(ftime)-1] = 0;
  printf("%s\n",ftime);

  printf("Change: ");
  // print time
  strcpy(ftime, ctime(&ip->i_atime));
  ftime[strlen(ftime)-1] = 0;
  printf("%s\n",ftime);


}







int cat_file()
{

  int fd, i, n;
  char buf[1024];

  //set parameter to read only
  strcpy(parameter, "0");
  //open file to fd
  fd = open_file(pathname, O_RDONLY);

  if(fd < 0)
  {
    printf("%sCould not open file!%s", COLOR_RED, COLOR_RESET);
    return 0;
  }

  while (n = myread(fd, buf, 1024)){
      for (i=0; i < n; i++)
      {
        printf("%c", (buf[i] == '\n') ? '\n' : buf[i]);
      }
  }


}
