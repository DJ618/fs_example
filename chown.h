/**********************************************************************
*           Author: Daniel Johnson & Alan Tyson
*           Last Update: 2016-04-25
*
*    CHOWN.H is a header for chown/grp/mod functions
*
**********************************************************************/

/*********************************************************************/
/*********************************************************************/
int chmod_file()
{
  //be sure a file was passed int chown on
  if(strlen(parameter) > 0)
  {
    mychmod(pathname, parameter);
  }
  return 0;
}

/*********************************************************************/
/*********************************************************************/
int mychmod(char *newperm, char *file)
{
  int dev, fino = 0, mode = 0;
  MINODE *mip = 0;

  //Set device and search startpoints
  if(file[0] == '/')  //if path is absolute
  {
    //set dev to same dev as cwd Minode
    dev = root->dev;
  }
  else  //relative paths
  {
    //set dev to same dev as cwd Minode
    dev = running->cwd->dev;
  }

  //get the inode and MINODE ptr for the file to be chowned upon
  fino = getino(&dev, file);
  mip = iget(dev, fino);

  //check iiiiiiiiiiiiiiit
  if(fino == INVALID || mip == INVALID )
  {
    printf("%sThat file does not exist.%s\n", COLOR_RED, COLOR_RESET);
    iput(mip);
    return INVALID;
  }

  //shift into place, look for the next
	mode = newperm[0]-48 << 6;
  //shift into place, look for the next
	mode |= newperm[1]-48 << 3;
  //shift into place, look for the next
	mode |= newperm[2]-48;
  //zero it all out
	mip->INODE.i_mode &= 0xFF000;
  //toggle bits with bitwise or
	mip->INODE.i_mode |= mode;

  //mark dirty and put it back
  mip->dirty = TRUE;
  iput(mip);
  return 0;
}

/*********************************************************************/
/*********************************************************************/
int chown_file()
{
  int dev, fino = 0, newown = 0;
  MINODE *mip = 0;

  //Set device and search startpoints
  if(parameter[0] == '/')  //if path is absolute
  {
    //set dev to same dev as cwd Minode
    dev = root->dev;
  }
  else  //relative paths
  {
    //set dev to same dev as cwd Minode
    dev = running->cwd->dev;
  }

  //get the inode and MINODE ptr for the file to be chowned upon
  fino = getino(&dev, parameter);
  mip = iget(dev, fino);
  //get new owners id
  newown = atoi(pathname);

  //check iiiiiiiiiiiiiiit
  if(fino == INVALID || mip == INVALID )
  {
    printf("%sThat file does not exist.%s\n", COLOR_RED, COLOR_RESET);
    iput(mip);
    return INVALID;
  }

  //check owndership
  if(running->uid != mip->INODE.i_uid || running->uid != 0)
  {
    printf("%sYou do not have permissions to do that.%s\n", COLOR_RED, COLOR_RESET);
    iput(mip);
    return INVALID;
  }

  mip->INODE.i_uid = newown;
  mip->dirty = TRUE;
  iput(mip);
}


/*********************************************************************/
/*********************************************************************/
