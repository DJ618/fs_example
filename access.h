/**********************************************************************
*           Author: Daniel Johnson & Alan Tyson
*           Last Update: 2016-04-26
*
*     access.h is a header for everything to do with opening, closing
*     reading, writing, copying or moving things.
*
**********************************************************************/

/*********************************************************************/
/*********************************************************************/
int open_file()
{
  int dev, fino = 0, mode = 0, i = 0, firstFreeOFT = -1, returnfd = 0;
  MINODE *mip = 0;

  //open <filename> <mode>
  //1. ask for a pathname and mode to open: You may use mode = 0|1|2|3 for R|W|RW|APPEND
  LOG(LOG_DBG, "pathname: %s  paramter: %s\n", pathname, parameter);
  LOG(LOG_DBG, "set pathname starting point\n");

  //Set device and search startpoints
  if(pathname[0] == '/')  //if path is absolute
  {
    //set dev to same dev as cwd Minode
    dev = root->dev;
  }
  else  //relative paths
  {
    //set dev to same dev as cwd Minode
    dev = running->cwd->dev;
  }

  //2. get pathname's inumber:
  //get the inode and MINODE ptr for the file to be chowned upon
  fino = getino(&dev, pathname);
  //3. get its Minode pointer
  mip = iget(dev, fino);
  LOG(LOG_DBG, "went and got ino %d for pathname\n", fino);

  //check for garbo
  if(fino == INVALID || mip == INVALID)
  {
    printf( "%sFile does not exist%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  LOG(LOG_DBG, "check if reg file\n");
  //4. check mip->INODE.i_mode to verify it's a REGULAR file and permission OK.
  if(!S_ISREG(mip->INODE.i_mode))
  {
    printf( "%sIncorrect filepath, unable to open%s\n", COLOR_RED, COLOR_RESET);
    iput(mip);
    return INVALID;
  }
  LOG(LOG_DBG, "atoi call for %s\n", parameter);
  //get the mode in int form, and check if we are attempting to write when we are unable to
  mode = atoi(parameter);
  LOG(LOG_DBG, "atoi returned %d\n", mode);

  LOG(LOG_DBG, "Permissions check\n");
  if((running->uid != mip->INODE.i_uid) && (mode == 1 || mode == 2 || mode == 3))
  {
    printf( "%sYou do not have write permissions for this file.%s\n", COLOR_RED, COLOR_RESET);
    iput(mip);
    return INVALID;
  }

  LOG(LOG_DBG, "Check whether the file is ALREADY opened with INCOMPATIBLE mode:\n");

  //Check whether the file is ALREADY opened with INCOMPATIBLE mode:
  for(i = 0; i < NOFT; i++)
  {
    //save the first open one you find for a possible later use
    if(oft[i].refCount == 0 && firstFreeOFT == -1)
    {
      firstFreeOFT = i;
      LOG(LOG_DBG, "First free oft: %d\n", firstFreeOFT);
    }
    //this oft has something in use, check it out
    if(oft[i].refCount > 0)
    {
      LOG(LOG_DBG, "file is already in use\n");
      if(mip == &(oft[i].minodeptr))//if this is the file we are attempting to open
      {
        LOG(LOG_DBG, "file in use is the same file we want.\n");
        //check which mode these ppl are using with this file
        //if its R, dont worry about it.
        if(oft[i].mode != 0)
        {
          printf( "%sSomeone is already using this file.%s\n", COLOR_RED, COLOR_RESET);
          iput(mip);
          return INVALID;
        }
      }
    }
  }

  //5. allocate a FREE OpenFileTable (OFT) and fill in values:
  //the first free OFT will be j (from earlier)
  oft[firstFreeOFT].mode = mode;
  oft[firstFreeOFT].refCount++;
  oft[firstFreeOFT].minodeptr = mip;
  //6. Depending on the open mode 0|1|2|3, set the OFT's offset accordingly:
  LOG(LOG_DBG, "switch on mode (%d)\n");
  switch(mode)
  {
   case 0 : oft[firstFreeOFT].offset = 0;     // R: offset = 0
            break;
   case 1 : truncate(mip);          // W: truncate file to 0 size
            oft[firstFreeOFT].offset = 0;
            break;
   case 2 : oft[firstFreeOFT].offset = 0;     // RW: do NOT truncate file
            break;
   case 3 : oft[firstFreeOFT].offset =  mip->INODE.i_size;  // APPEND mode
            break;
   default: printf( "%sSomeone is already using this file.%s\n", COLOR_RED, COLOR_RESET);
   return INVALID;
  }
  LOG(LOG_DBG, "configured oft[%d] with mode = %d refcount = %d offset = %d minodeptr = %x\n", firstFreeOFT, oft[firstFreeOFT].mode, oft[firstFreeOFT].refCount, oft[firstFreeOFT].offset, oft[firstFreeOFT].minodeptr);

  printMinode(oft[firstFreeOFT].minodeptr);

  //7. find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
  for(returnfd = 0; returnfd < NFD; returnfd++)// i <=> returnfd
  {
    if(running->fd[returnfd] == 0)
    {
      //Let running->fd[i] point at the OFT entry
      running->fd[returnfd] = &(oft[firstFreeOFT]);
      break;
    }
  }

  //8. update INODE's time field
  //for R: touch atime.
  mip->INODE.i_atime = time(0L);
  //for W|RW|APPEND mode : touch atime and mtime
  mip->INODE.i_mtime = mip->INODE.i_atime = time(0L);
  //mark Minode[ ] dirty
  mip->dirty = TRUE;
  iput(mip);

  //9. return i<=>returnfd as the file descriptor
  return returnfd;
}

/*********************************************************************/
/*********************************************************************/
int truncate(MINODE *listmp)
{
  int i = 0, j = 0;
  int *delme;
  char buf[BLOCK_SIZE];

  //bulletproof
  if(listmp == INVALID){return INVALID;}
  //1. release mip->INODE's data blocks;
  for (i = 0, j = 0, ind_block = 0; i < 14; i++)
  {
    LOG(LOG_DBG, "check for unallocated blocks\n");
    if (listmp->INODE.i_block[i] == 0) {
      LOG(LOG_WARN, "%sUnallocated iblock. leav truncate.%s\n", COLOR_YELLOW, COLOR_RESET);
      iput(listmp);
      return TRUE;
    }

    LOG(LOG_DBG, "get data block to buf\n");
    get_block(listmp->dev, listmp->INODE.i_block[i], buf);
    delme = &(listmp->INODE.i_block[i]);
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
        delme = ind_block;
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
        LOG(LOG_WARN, "%sUnallocated iblock. leav LS.%s\n", COLOR_YELLOW, COLOR_RESET);
        iput(listmp);
        return TRUE;
      }
    }
    //dealloc the block
    bdealloc(listmp->dev, *delme);
    //set the corresponding i_block[] back to 0
    *delme = 0;
    //get ready for the next block
    ind_block++;
    delme = ind_block;
  }//end of indirect block for loop

  //2. update INODE's time field
  listmp->INODE.i_mtime = listmp->INODE.i_atime = time(0L);
  //3. set INODE's size to 0 and mark Minode[ ] dirty
  listmp->INODE.i_size = 0;
  listmp->dirty = TRUE;
  iput(listmp);
  return 0;
}


/*********************************************************************/
/********************************************************************
oftp = running->fd[fd];
     running->fd[fd] = 0;
     oftp->refCount--;
     if (oftp->refCount > 0) return 0;

     // last user of this OFT entry ==> dispose of the Minode[]
     mip = oftp->inodeptr;
     iput(mip);

     return 0;
*/
int close_file(int fd)
{
  OFT *oftp = running->fd[fd];
  //1. verify fd is within range.
  if(fd < 0 || fd > NFD)
  {
    printf( "%sInvalid fd, failed to close file.%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  //2.running->fd[fd] is pointing at a OFT entry
  running->fd[fd] = 0;
  oftp->refCount--;

  if(oftp->minodeptr->refCount > 0)
  {
    LOG(LOG_WARN, "%sFile is busy, failed to close file.%s\n", COLOR_YELLOW, COLOR_RESET);
    return 0;
  }

  //put it back
  oftp->minodeptr->dirty = TRUE;
  iput(oftp->minodeptr);
  return 0;
}

/*********************************************************************/
/*********************************************************************/
int pfd()
{
  int i = 0;
  char modestr[16];

  printf( "  fd      mode      offset      INODE\n");
  printf( "------  --------   ---------    -------\n");

  //7. find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
  for(i = 0; i < NOFT; i++)// i <=> returnfd
  {
    if(oft[i].refCount > 0)
    {
      ret_mode(oft[i].mode, modestr);
      printf("  %d       %s          %d          %d,%d     \n", i, modestr, oft[i].offset, oft[i].minodeptr->dev, oft[i].minodeptr->ino);
      //reset the mode string
      memset(modestr, 0, 16);
    }
  }
  return;
}

/*********************************************************************/
/*********************************************************************/
int ret_mode(int fd, char *mode)
{
  switch(fd){
    case 0: strcpy(mode, "Read");        break;
    case 1: strcpy(mode, "Write");      break;
    case 2: strcpy(mode, "R/W");          break;
    case 3: strcpy(mode, "Append");    break;
  }
  return 0;
}







/*********************************************************************/
/*********************************************************************/
int mylseek(int fd, int position)
{
  OFT *of = &(oft[fd]);
  int originalPosition;
  int i = 0;

  //verify inputs are good
  if(fd == INVALID || position == INVALID)
  {
    printf( "%sInvalid Parameters%s", COLOR_RED, COLOR_RESET);
    return INVALID;
  }


  //verify the oft entry is good
  if(of == 0)
  {
    printf( "%sOFT entry does not exist%s", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  //change OFT entry's offset to position but make sure NOT to over run either end of the file.
  if(of->offset < 0 || of->offset > of->minodeptr->INODE.i_size)
  {
    printf( "%sInvalid Offset%s", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  originalPosition = of->offset;
  //set position
  of->offset = position;
  //return originalPosition
  return originalPosition;

}


/*********************************************************************/
/*********************************************************************/
int myread(int fd, char *buf, int nbytes)
{
  int count = 0, avil = 0, remain = 0;
  int start = 0, lblk = 0, blk;
  OFT *of = &(oft[fd]);
  char readbuf[BLOCK_SIZE];
  char *cp, *cq = buf;
  int i = 0;

  LOG(LOG_DBG, "myread from fd: %d for %d bytes\n", fd, nbytes);
  LOG(LOG_DBG, "check OFT for fd\n", fd, nbytes);
  //verify the oft entry is good
  if(of == 0)
  {
    printf( "%sOFT entry does not exist%s", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  //OFT entry in of
  //avil = distance to end of file
  avil = of->minodeptr->INODE.i_size - of->offset;

  LOG(LOG_DBG, "looping for nbytes: %d, until file is out at avil: %d\n", nbytes, avil);
  while (nbytes && avil){
    //logical block = block # in inode to load
    lblk = of->offset / BLOCK_SIZE;
    //offset into lblk to start from
    start = of->offset % BLOCK_SIZE;
    LOG(LOG_DBG, "nbytes: %d, avil: %d lblk: %d start = %d\n", nbytes, avil, lblk, start);
    //get blocknumber to read
    //direct blocks
    if (lblk < 12)
    {
      LOG(LOG_DBG, "direct block for block %d\n", lblk);
        blk = of->minodeptr->INODE.i_block[lblk]; // map LOGICAL lbk to PHYSICAL blk
    }
    //indirect block
    else if (lblk >= 12 && lblk < 256 + 12) {
      LOG(LOG_DBG, "indirect block for block %d\n", lblk);
      get_block(of->minodeptr->dev, of->minodeptr->INODE.i_block[12], ind_buf);
      ind_block = (int *)ind_buf;

      ind_block += (lblk - 12);
      blk = (int) *cp;

    }
    //double indirect
    else{
      LOG(LOG_DBG, "double-indirect block for block %d\n", lblk);
      //not working yet
      get_block(of->minodeptr->dev, of->minodeptr->INODE.i_block[13], ind_buf);
      ind_block = (int *)ind_buf;

      ind_block += (lblk - 12);
      blk = (int) *cp;
    }
    LOG(LOG_DBG, "getblock number %d from %d\n", blk, of->minodeptr->dev);
    //get the data block into readbuf[BLKSIZE]/
    get_block(of->minodeptr->dev, blk, readbuf);

    //copy from startByte to buf[ ], at most remain bytes in this block
    char *cp = readbuf + start;
    LOG(LOG_DBG, "set cp to readbuf  + start \n");
    remain = BLOCK_SIZE - start;   // number of bytes remain in readbuf[]
    LOG(LOG_DBG, "bytes remaining in readbuf = %d\n", remain);

    while (remain > 0){
         (*cq) = *cp;             // copy byte from readbuf[] into buf[]
         cq++, cp++;
         LOG(LOG_DBG, "copy one byte from readbuf to buf\n");
          of->offset++;           // advance offset
          LOG(LOG_DBG, "advance offset\n");
          count++;                  // inc count as number of bytes read
          LOG(LOG_DBG, "increment count\n");
          avil--;  nbytes--;  remain--;
          LOG(LOG_DBG, "decrement avil nbytes remain\n");
          if (nbytes <= 0 || avil <= 0)
          {
            LOG(LOG_DBG, "stop reading from this block\n");
            break;
          }
    }
    // if one data block is not enough, loop back to OUTER while for more
    LOG(LOG_DBG, "we must read more\n");
  }
  LOG(LOG_WARN, "%smyread: read %d char from file descriptor %d%s\n", COLOR_YELLOW, count, fd, COLOR_RESET);
  return count;   // count is the actual number of bytes read
}


/*********************************************************************/
/*********************************************************************/
int mywrite(int fd, char buf[ ], int nbytes)
{
  int count = 0, avil = 0, remain = 0;
  int start = 0, lblk = 0, blk;
  OFT *of = &(oft[fd]);
  char writebuf[BLOCK_SIZE];
  char *cp, *cq = buf;
  int i = 0;

  LOG(LOG_DBG, "write_file to fd: %d for %d bytes\n", fd, nbytes);
  LOG(LOG_DBG, "check OFT for fd\n", fd, nbytes);
  //verify the oft entry is good
  if(of == 0)
  {
    printf( "%sOFT entry does not exist%s", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  //OFT entry in of
  //avil = distance to end of file
  avil = of->minodeptr->INODE.i_size - of->offset;

  LOG(LOG_DBG, "looping for nbytes: %d, until file is out at avil: %d\n", nbytes, avil);
  while (nbytes > 0){
    //logical block = block # in inode to load
    lblk = of->offset / BLOCK_SIZE;
    //offset into lblk to start from
    start = of->offset % BLOCK_SIZE;
    LOG(LOG_DBG, "nbytes: %d  lblk: %d start: %d\n", nbytes, avil, lblk, start);
    //get blocknumber to read
    //direct blocks
    if (lblk < 12)
    {
      if(of->minodeptr->INODE.i_block[lblk] == 0)
      {
        of->minodeptr->INODE.i_block[lblk] = balloc(of->minodeptr->dev);
        get_block(of->minodeptr->dev, of->minodeptr->INODE.i_block[lblk], buf);
        memset(buf, 0, BLOCK_SIZE);
        put_block(of->minodeptr->dev, of->minodeptr->INODE.i_block[lblk], buf);
      }
      blk = of->minodeptr->INODE.i_block[lblk];
    }
    //indirect block
    else if (lblk >= 12 && lblk < 256 + 12) {
      //LOG(LOG_DBG, "indirect block for block %d\n", lblk);
      // get_block(of->minodeptr->dev, of->minodeptr->INODE.i_block[12], ind_buf);
      // ind_block = (int *)ind_buf;
      //
      // ind_block += (lblk - 12);
      // blk = (int) *cp;

    }
    //double indirect
    else{
      //LOG(LOG_DBG, "double-indirect block for block %d\n", lblk);
      // //not working yet
      // get_block(of->minodeptr->dev, of->minodeptr->INODE.i_block[13], ind_buf);
      // ind_block = (int *)ind_buf;
      //
      // ind_block += (lblk - 12);
      // blk = (int) *cp;
    }
    LOG(LOG_DBG, "getblock number %d from %d\n", blk, of->minodeptr->dev);
    //get the data block into readbuf[BLKSIZE]/
    get_block(of->minodeptr->dev, blk, writebuf);

    //copy from startByte to buf[ ], at most remain bytes in this block
    char *cp = writebuf + start;
    LOG(LOG_DBG, "set cp to readbuf  + start \n");
    remain = BLOCK_SIZE - start;   // number of bytes remain in readbuf[]
    LOG(LOG_DBG, "bytes remaining in readbuf = %d\n", remain);

    while (remain > 0){
          (*cp) = *cq;             // copy byte from readbuf[] into buf[]
          cq++, cp++;
          LOG(LOG_DBG, "copy one byte from buf to writebuf\n");
          of->offset++;           // advance offset
          LOG(LOG_DBG, "advance offset\n");
          nbytes--;  remain--;
          LOG(LOG_DBG, "decrement nbytes remain\n");
          if(of->offset > of->minodeptr->INODE.i_size)
          {
            of->minodeptr->INODE.i_size++;
          }
          if (nbytes <= 0 )
          {
            LOG(LOG_DBG, "stop writing to this block\n");
            break;
          }
    }
    put_block(of->minodeptr->dev, blk, writebuf);

    // if one data block is not enough, loop back to OUTER while for more
    LOG(LOG_DBG, "we must read more\n");
  }
  of->minodeptr->dirty = TRUE;
  printf("wrote %d char into file descriptor fd=%d\n", nbytes, fd);
}


/*********************************************************************/
/*********************************************************************/
int cp_file()
{

}

/*********************************************************************/
/*********************************************************************/
int mv_file()
{

}
