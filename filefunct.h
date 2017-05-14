/**********************************************************************
*           Author: Daniel Johnson & Alan Tyson
*           Last Update: 2016-03-30
*
*    FILEFUNCT.H is a catch-all header for mkdir and rm dir functions
*
**********************************************************************/

/*********************************************************************/
/*********************************************************************/
int creat_file()
{
  char parent[128];
  char child[64];
  char temp_buf[128];
  int pino;
  int dev;
  MINODE *pip = 0, *searchminode = 0;

  //guard check
  if(!pathname)
  {
    printf("%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  LOG(LOG_DBG, "in creat_file() pathname = %s\n", pathname);

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

  //set parent/child to dirname/basename respectively
  strcpy(temp_buf, pathname);
  strcpy(parent, dirname(temp_buf));
  strcpy(temp_buf, pathname);
  strcpy(child, basename(temp_buf));

  LOG(LOG_DBG, "pathname ( %s ) has been split to parent : %s , child: %s\n", pathname, parent, child);

  //Get parent ino number and minode for parent
  pino  = getino(&dev, parent);
  pip   = iget(dev, pino);

  if(pino == INVALID || pip == INVALID )
  {
    printf("%sThat parent does not exist%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  //verify parent INODE is a dir, AND child does not exist in parent directory
  if(!S_ISDIR(pip->INODE.i_mode))
  {
    printf("%sParent is not a valid directory%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }
  //child exists check
  if (search(pip, child) != INVALID)
  {
    printf("%sfile already exists.%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }
  //call mymkdir
  LOG(LOG_DBG, "Calling my_creat()\n");
  my_creat(pip, child);

  //set time in parents inode
  pip->INODE.i_ctime = pip->INODE.i_mtime = time(0L);
  //mark parent MINODE as DIRTY (giggity)
  pip->dirty = TRUE;

  LOG(LOG_DBG, "successfully created inode. callin iput() to write back\n", pathname);
  //iput back to disk

  iput(pip);
}


/*********************************************************************/
/*********************************************************************/
         //pip points at the parent minode[] of "/a/b", name is a string "c")
int my_creat(MINODE *pip, char *name)
{
  char *pp;
	int ino = 0, bno = 0, i =0;
  char buf[BLOCK_SIZE];
  MINODE *mip = 0;
  DIR newdir;
  INODE *ip = 0;

  //guard check
  if(!pip || !name)
  {
    printf("%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }


  LOG(LOG_DBG, "my_creat, child: %s... calling  on given parent\n", name);
  //printMinode(pip);

  //get inode and datablock for new dir
  ino = ialloc(pip->dev);
  bno = balloc(pip->dev);//////////////////adding one makes it so it doesnt point back to the previous entry/////
  LOG(LOG_DBG, "getting inode %d. And block %d for new file\n", ino, bno);
  mip = iget(pip->dev, ino);
  //ninja shit
  ip = &(mip->INODE);
  /*Use ip-> to setup new DIR inode*/
  LOG(LOG_DBG, "configuring new inode\n");
  ip->i_mode = FILE_MODE;		// OR 040755: DIR type and permissions
  ip->i_uid  = running->uid;	// Owner uid
  ip->i_gid  = running->gid;	// Group Id
  ip->i_size = 0;		// new files get NOTHING
  ip->i_links_count = 1;	        // Links count=1
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
  ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks
  ip->i_block[0] = 0;             // new file has no data block
  LOG(LOG_DBG, "Zero Out the rest of the data blocks %d\n", bno);
  for(i = 1; i < 15; i++)	//0 out the rest of the datablock
  {
    ip->i_block[i] = 0;
  }


  LOG(LOG_DBG, "Mark as dirty and call iput()\n");
  mip->dev = pip->dev;
  mip->dirty = 1;               // mark minode dirty
  //printMinode(mip);
  iput(mip);                    // write INODE to disk

  enter_name(pip, ino, name);

  return ino;
}


/*********************************************************************/
/*********************************************************************/
int rm_file()
{
  char parent[128];
  char child[64];
  char temp_buf[128];
  int pino, delnum;
  int dev, i = 0;
  MINODE *pip = 0, *searchminode = 0, *delnode = 0;

  //guard check
  if(!pathname)
  {
    printf("%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }


  LOG(LOG_DBG, "in rm_file() pathname = %s\n", pathname);

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

  //set parent/child to dirname/basename respectively
  strcpy(temp_buf, pathname);
  strcpy(parent, dirname(temp_buf));
  strcpy(temp_buf, pathname);
  strcpy(child, basename(temp_buf));

  LOG(LOG_DBG, "pathname ( %s ) has been split to parent : %s , child: %s\n", pathname, parent, child);

  //Get parent ino number and minode for parent
  pino    = getino(&dev, parent);
  pip     = iget(dev, pino);

  //Get the node to be deleted's minode
  delnum  = getino(&dev, pathname);
  delnode = iget(dev, delnum);

  if(delnum == INVALID || delnode == INVALID )
  {
    printf("%sThat file does not exist%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  //check owndership: super user ok, no super uid must match
  if(running->uid != 0 && delnode != INVALID)//current proc is not super user
  {
    LOG(LOG_DBG, "Non-super user detected attempting to rmdir. Checking ownership.\n");
    if(running->uid != delnode->INODE.i_uid)//allow to rmdir
    {
      printf("%sYou do not have permissions to remove that directory.%s\n", COLOR_RED, COLOR_RESET);
      iput(delnode);
      iput(pip);
      return INVALID;
    }
  }

  //verify node to be deleted INODE is a reg file
  if(!S_ISREG(delnode->INODE.i_mode))
  {
    printf("%sThat is not a REGULAR file%s\n", COLOR_RED, COLOR_RESET);
    iput(delnode);
    iput(pip);
    return INVALID;
  }
  //Verify the directory is not busy
  LOG(LOG_DBG, "printing delnode before check allowance of rm\n");
  //printMinode(delnode);
  LOG(LOG_DBG, "Made it past the printing of delnode\n");
  if(delnode->refCount > 1)//do not allow
  {
    printf("%sThat directory is busy, unable to remove.%s\n", COLOR_RED, COLOR_RESET);
    iput(delnode);
    iput(pip);
    return INVALID;
  }
  LOG(LOG_DBG, "Made it past checking allowance\n");
  LOG(LOG_DBG, "attempting to bdealloc delnode.i_blocks\n");
  //If we made it this far, time to remove the directory.
  //deallocate its block and inode
  for(i = 0; i < 12; i++){
    if(delnode->INODE.i_block[i] != 0){
      LOG(LOG_DBG, "bdealloc delnode.i_block[%d] : %d attempting bdealloc\n", i, delnode->INODE.i_block[i]);
      bdealloc(delnode->dev, delnode->INODE.i_block[i]);
    }else{
      LOG(LOG_DBG, "TRAP: bdealloc delnode.i_block[%d] : %d: Did not bdealloc\n", i, delnode->INODE.i_block[i]);
    }
  }
  LOG(LOG_DBG, "Made it past bdeallocing, attempting to idealloc\n");
  //dealloc the inode
  idealloc(delnode->dev, delnode->ino);
  LOG(LOG_DBG, "attempting to iput delnode\n");
  delnode->dirty = TRUE;
  //put it back
  iput(delnode);
  //remove child from parent's entries
  rm_child(pip, child);
  //  touch pip's atime, mtime fields;
  pip->INODE.i_atime = pip->INODE.i_mtime = time(0L);
  //  mark pip dirty;
  pip->dirty = TRUE;
  //  iput(pip);
  iput(pip);
  return 1;
}

/*********************************************************************/
/*********************************************************************/
int symlink()
{
    //symlink oldNAME newNAME <=> symlink pathname paramater

    //verify oldNAME exists
    char oldparent[128];
    char oldchild[64];
    char newparent[128];
    char newchild[64];
    char temp_buf[128];
    int opino, npino;
    int dev;
    MINODE *opip = 0, *searchminode = 0, *npip = 0;

    //guard check
    if(strlen(pathname) < 1 || strlen(parameter) < 1)
    {
      printf("%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
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

    //set (old)parent/child to dirname/basename respectively
    strcpy(temp_buf, pathname);
    strcpy(oldparent, dirname(temp_buf));
    strcpy(temp_buf, pathname);
    strcpy(oldchild, basename(temp_buf));
    //set (new)parent/child to dirname/basename respectively
    strcpy(temp_buf, parameter);
    strcpy(newparent, dirname(temp_buf));
    strcpy(temp_buf, parameter);
    strcpy(newchild, basename(temp_buf));

    opino = getino(&dev, oldparent);
    npino = getino(&dev, newparent);

    //verify oldNAME
    if(opino == INVALID)
    {
      printf("%sPath (%s) does not exist.%s\n", COLOR_RED, pathname,COLOR_RESET);
      return FALSE;
    }

    opip = iget(dev, opino);

    printf("oldchild = %s", oldchild);
    if(search(opip, oldchild) == INVALID)
    {
      printf("%sFile does not exist. Unable to create new link.%s\n", COLOR_RED, COLOR_RESET);
      iput(opip);
      return INVALID;
    }
    iput(opip);

    //creat a FILE at /parameter/ of: symlink pathname parameter
    //get the newname's parent MINODE in memory and a pointer to it
    npip = iget(dev, npino);

    if(search(npip, newchild) != INVALID)
    {
      printf("%sFile already exists. Unable to create new link.%s\n", COLOR_RED, COLOR_RESET);
      iput(npip);
      return INVALID;
    }
    //now pass that to creat to make a new FILE
    my_creat(npip, newchild);
    npip->INODE.i_ctime = npip->INODE.i_mtime = time(0L);
    //get the MINODE for the newly created file
    searchminode = iget(dev, search(npip, newchild));
    //change /x/y/z's type to LNK (0120000)=(1010.....)=0xA...
    searchminode->INODE.i_mode |= 0xA000;

    // write the string oldNAME into the i_block[ ], which has room for 60 chars.
    memcpy(&(searchminode->INODE.i_block[0]), pathname, strlen(pathname));
    //???? put a zero at the end of those iblocks so we can retreive it as a str later
    memset(&searchminode->INODE.i_block[0] + (int)strlen(pathname) + 4, 0, 1);

    //mark DIRTY (giggity)
    searchminode->dirty = TRUE;
    npip->dirty = TRUE;
    iput(searchminode);
    iput(npip);
    return;
}


/*********************************************************************/
/*********************************************************************/
int link()
{
  int olddev, dev = root->dev;
  int oldino, newpino;
  char buf[128];
  char temp_buf[128];
  MINODE *oldmp, *newmp, *newpp;
  char oldparent[128];
  char oldchild[64];
  char newparent[128];
  char newchild[64];

  if (strlen(pathname) < 1 || strlen(parameter) < 1) {
    printf("%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
    return;
  }

  LOG(LOG_DBG, "pathname: %s, parameter: %s\n", pathname, parameter);

  //set (old)parent/child to dirname/basename respectively
  strcpy(temp_buf, pathname);
  strcpy(oldparent, dirname(temp_buf));
  strcpy(temp_buf, pathname);
  strcpy(oldchild, basename(temp_buf));
  //set (new)parent/child to dirname/basename respectively
  strcpy(temp_buf, parameter);
  strcpy(newparent, dirname(temp_buf));
  strcpy(temp_buf, parameter);
  strcpy(newchild, basename(temp_buf));

  //get ino for both old and new
  oldino = getino(&dev, pathname);
  newpino = getino(&dev, newparent);
  LOG(LOG_DBG, "oldino: %d, newino: %d\n", oldino, newpino);

  //check that parent exists and new link does not
  if (oldino == INVALID || newpino == INVALID) {
    printf("%sPath does not exist%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  oldmp = iget(dev, oldino);
  if(S_ISDIR(oldmp->INODE.i_mode))
  {
    printf("%sParent is a directory. Unable to link%s\n", COLOR_RED, COLOR_RESET);
    iput(oldmp);
    return INVALID;
  }

  newpp = iget(dev, newpino);
  if(!S_ISDIR(newpp->INODE.i_mode))
  {
    printf("%sParent is not a directory. Unable to create new link.%s\n", COLOR_RED, COLOR_RESET);
    iput(newpp);
    return INVALID;
  }

  if(search(newpp, newchild) != INVALID)
  {
    printf("%sFile %s already exists. Unable to create new link.%s\n", COLOR_RED, newchild, COLOR_RESET);
    iput(newpp);
    return INVALID;
  }

  //no links across devices!
  if(oldmp->dev != newpp->dev)
  {
    printf("%scannot link across devices%s\n", COLOR_RED, COLOR_RESET);
    iput(oldmp);
    iput(newpp);
    return INVALID;
  }

  //(4). Add an entry [ino rec_len name_len z] to the data block of /x/y/
  //This creates /x/y/z, which has the SAME ino as that of /a/b/c
  enter_name(newpp, oldino, newchild);

  //(5). increment the i_links_count of INODE by 1
  oldmp->INODE.i_links_count++;

  //(6). write INODE back to disk*/
  oldmp->dirty = TRUE;
  newpp->dirty = TRUE;
  iput(oldmp);
  iput(newpp);

  return;
}

/*********************************************************************/
/*********************************************************************/
int read_link()
{
  char linkname[128];
  myreadlink(pathname, &linkname);
  LOG(LOG_DBG, "Readlink read linkname: %s\n", linkname);
  return;
}

/*********************************************************************/
/*********************************************************************/
void myreadlink(int ino, char *source)
{
  int tino;
  int dev, i = 0;
  MINODE *mip = 0;
  char localstr[128];
  char *cp;

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


  //(1) get ino number and minode for pathname
  //tino    = getino(&dev, pathname);
  mip     = iget(dev, ino);
  if(ino == INVALID || mip == INVALID )
  {
    printf("%sThat pathname does not exist%s\n", COLOR_RED, COLOR_RESET);
    iput(mip);
    return INVALID;
  }

  //(2). check INODE is a symbolic Link file.
  if(!S_ISLNK(mip->INODE.i_mode))//not a link
  {
    printf("\n%sThat is not a symlink file%s\n", COLOR_RED, COLOR_RESET);
    iput(mip);
    return INVALID;
  }

  //(3). return its string contents in INODE.i_block[ ].
    // memcpy(&localstr, &(mip->INODE.i_block[0]), 128;
    cp = &(mip->INODE.i_block[0]);
    while(*cp != 0)
    {
      localstr[i] = *cp;
      cp++;
      i++;
    }
    localstr[i] = 0;

    //copy localstr to the passed in source string
    strcpy(source, localstr);
    LOG(LOG_DBG, "localstr: %s, source: %s\n", localstr, source);
    iput(mip);
    return 1;
}


/*********************************************************************/
/*********************************************************************/
int unlink()
{
  int tino, pino;
  int dev, i = 0, newlinkcnt = 0;
  MINODE *mip = 0, *pip = 0;
  char localstr[128];
  char *cp;
  char parent[128];
  char child[64];
  char temp_buf[128];


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

  //(1) get ino number and minode for pathname
  tino    = getino(&dev, pathname);
  mip     = iget(dev, tino);
  if(tino == INVALID || mip == INVALID )
  {
    printf("%sThat pathname does not exist%s\n", COLOR_RED, COLOR_RESET);
    iput(mip);
    return INVALID;
  }

  //(2). check INODE is a directory
  // if(S_ISDIR(mip->INODE.i_mode))//not a link
  // {
  //   printf("%sThat is a directory, unable to unlink%s\n", COLOR_RED, COLOR_RESET);
  //   iput(mip);
  //   return INVALID;
  // }

  //(3). decrement INODE's i_links_count by 1;
  newlinkcnt = mip->INODE.i_links_count;
  mip->INODE.i_links_count--;
  newlinkcnt--;
  LOG(LOG_DBG, "newlinkcnt: %d, links_count: %d\n", newlinkcnt, mip->INODE.i_links_count);

  //(4). if i_links_count == 0 ==> rm pathname by deallocate its data blocks
  //we check for 1, to offset our initial value of 2 in our mk functionality
  if(mip->INODE.i_links_count == 0)
  {
    //deallocate the iblocks
    while((i < 15) && (mip->INODE.i_block[i] != 0))
    {
      bdealloc(dev, mip->INODE.i_block[i]);
      i++;
    }
    //deallocate its INODE;
    idealloc(dev, mip->ino);
  }//end of removing the file if the links_count was 0

  strcpy(temp_buf, pathname);
  strcpy(parent, dirname(temp_buf));
  strcpy(temp_buf, pathname);
  strcpy(child, basename(temp_buf));
  //grab the parent MINODE
  pino = getino(&dev, parent);
  pip = iget(dev, pino);
  rm_child(pip, child);
  pip->dirty = TRUE;
  iput(pip);
  printMinode(mip);
  mip->dirty = TRUE;
  iput(mip);
  return;
}


/*********************************************************************/
/*********************************************************************/
int touch_file() //14
{
  int dev, i, fileino;
  MINODE *mp;
  INODE *ip;
  //guard check
  if(!pathname)
  {
    printf("%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
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
    printf("%sfile does not exist.%s\n", COLOR_YELLOW, COLOR_RESET);
    creat_file();
    return TRUE;
  }
  LOG(LOG_DBG, "%sfile exists. update times%s\n", COLOR_YELLOW, COLOR_RESET);
  ip = &(mp->INODE);
  //update times to current
  ip->i_mtime = ip->i_atime = time(0L);
  //mark dirty and put it back where we found it
  mp->dirty = TRUE;
  iput(mp);
  return TRUE;
}
