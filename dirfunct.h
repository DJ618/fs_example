/**********************************************************************
*           Author: Daniel Johnson & Alan Tyson
*           Last Update: 2016-04-23
*
*    DIRFUNCT.H is a catch-all header for mkdir and rm dir functions
*
**********************************************************************/


/*********************************************************************/
/*********************************************************************/
int make_dir()
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

  //set parent/child to dirname/basename respectively
  strcpy(temp_buf, pathname);
  strcpy(parent, dirname(temp_buf));
  strcpy(temp_buf, pathname);
  strcpy(child, basename(temp_buf));

  LOG(LOG_DBG, "pathname ( %s ) has been split to parent : %s , child: %s\n", pathname, parent, child);

  //Get parent ino number and minode for parent
  pino  = getino(&dev, parent);
  if(pino == INVALID)
  {
    printf( "%sPath (%s) does not exist.%s\n", COLOR_RED, pathname, COLOR_RESET);
    return INVALID;
  }
  pip   = iget(dev, pino);

  //verify parent INODE is a dir, AND child does not exist in parent directory
  if(!S_ISDIR(pip->INODE.i_mode))
  {
    printf( "%sParent is not a valid directory.%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }
  LOG(LOG_DBG, "passed dir check\n");
  //child exists check
  if (search(pip, child) != INVALID)
  {
    printf( "%sDirectory already exists.%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }
  LOG(LOG_DBG, "passed child dup check\n");
  //call mymkdir
  LOG(LOG_DBG, "Calling mymkdir()\n");
  mymkdir(pip, child);

  //inc i_links_count, touch atime, and mark it dirty
  pip->INODE.i_links_count++;
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
int mymkdir(MINODE *pip, char *name)
{
  char *pp;
	int ino = 0, bno = 0, i =0;
  char buf[BLOCK_SIZE];
  MINODE *mip = 0;
  DIR newdir;
  INODE *ip = 0;

  if(!pip || !name)
  {
    printf( "%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  LOG(LOG_DBG, "Mymakedir, child: %s... calling ////printminode on given parent\n", name);
  //printminode(pip);

  //get inode and datablock for new dir
  ino = ialloc(pip->dev);
  if(ino == INVALID)
  {
    printf( "%sino is invalid%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  bno = balloc(pip->dev);//////////////////adding one makes it so it doesnt point back to the previous entry/////
  if(bno == INVALID)
  {
    printf( "%sbno is invalid%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  LOG(LOG_DBG, "getting inode %d. And block %d for new item\n", ino, bno);
  mip = iget(pip->dev, ino);
  if(mip == INVALID)
  {
    printf( "%smip is invalid%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }
  //ninja shit
  ip = &(mip->INODE);
  /*Use ip-> to setup new DIR inode*/
  LOG(LOG_DBG, "configuring new inode\n");
  ip->i_mode = 0x41ED;		// OR 040755: DIR type and permissions
  ip->i_uid  = running->uid;	// Owner uid
  ip->i_gid  = running->gid;	// Group Id
  ip->i_size = BLOCK_SIZE;		// Size in bytes
  ip->i_links_count = 2;	        // Links count=2 because of . and ..
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
  ip->i_blocks = 2;                	// LINUX: Blocks count in 512-byte chunks
  LOG(LOG_DBG, "set first data block in new inode to %d\n", 0);
  ip->i_block[0] = bno;             // new DIR has one data block
  LOG(LOG_DBG, "Zero Out the rest of the data blocks\n");
  for(i = 1; i < 15; i++)	//0 out the rest of the datablock
  {
    ip->i_block[i] = 0;
  }


  LOG(LOG_DBG, "Mark as dirty and call iput()\n");
  mip->dirty = 1;               // mark minode dirty
  //printminode(mip);
  iput(mip);                    // write INODE to disk

  LOG(LOG_DBG, "get datablock for Dir entry\n");
  //get i_bock[0] for the new inode
  get_block(pip->dev, bno, buf);

  /*struct ext2_dir_entry_2 {
      u32  inode;        // Inode number; count from 1, NOT from 0
      u16  rec_len;      // This entry length in bytes
      u8   name_len;     // Name length in bytes
      u8   file_type;    // for future use
      char name[EXT2_NAME_LEN];  // File name: 1-255 chars, no NULL byte
	};*/
  //USE dp to write dir entries for "." and ".."
  // GLOBALS : char *dot = "." , 2dot = ".."
  pp = buf;
  dp = (DIR *)pp;
  LOG(LOG_DBG, "create . and .. entries: . with %d, and .. with %d\n", ino, pip->ino);
  // "." entry
  dp->inode = ino;
  dp->name_len = strlen(dot);
  dp->rec_len = 4 * ( (8 + dp->name_len + 3)/4 );
  dp->file_type = 0;
  strcpy(dp->name, dot);

  pp += dp->rec_len;
  dp = (DIR *)pp;
  //".." entry
  dp->inode = pip->ino;
  dp->name_len = strlen(dot2);
  dp->rec_len = 1012;
  dp->file_type = 0;
  strcpy(dp->name, dot2);

  LOG(LOG_DBG, "write datablock back\n");
  put_block(pip->dev, bno, buf);

  enter_name(pip, ino, name);

  LOG(LOG_DBG, "leaving mymkdir\n");
  return ino;
}


/*********************************************************************/
/*********************************************************************/
int enter_name(MINODE *pip, int myino, char *myname)
{
	int i = 0, remain = 0, newblock = 0, flag = 0;
  INODE *ip, *pipnode = &pip->INODE;
  char buf[BLOCK_SIZE];
  char *cp;

  //guard check
  if(!pip || myino == INVALID || !myname)
  {
    printf( "%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }


  LOG(LOG_DBG, "Enter_name on parent, myino: %d,  myname: %s\n", myino, myname);

  //NOTE: need to implement indirect blocks
  for(i = 0; i < 12; i++) //iterate over data blocks in parent inode
  {

    //if data block is zero (unallocated) exit for loop and maintain i value
    if(pipnode->i_block[i] == 0){
      LOG(LOG_DBG, "pipnode->i_block[%d] = 0, set flag to true and exit for loop\n", i);
      flag = TRUE;
      break;
    }
    //otherwise, go get that data block
    LOG(LOG_DBG, "Getting parent->iblock[%d]'s block]\n", i);
    get_block(pip->dev, pipnode->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    LOG(LOG_DBG, "got block from disk, now read directory entries\n");
    //Step to the last entry in a data block
    while (cp + dp->rec_len < buf + BLOCK_SIZE)
    {
      printDir(dp);
      cp += dp->rec_len;
    	dp = (DIR *)cp;
    }

    LOG(LOG_DBG, "printing one more entry for curiousity\n");
    printDir(dp);

    //dp now points to last entry in block, now check the space after its name WRT its ideal length
    remain = dp->rec_len - (4 * ( (8 + dp->name_len + 3)/4 ));
    LOG(LOG_DBG, "Calculate remaining space in block as %d\n", remain);
    LOG(LOG_DBG, "compare to ideal length of new dir %d\n", (4 * ( (8 + strlen(myname) + 3)/4 )));

    if(remain >= (4 * ( (8 + strlen(myname) + 3)/4 )))//we have space remaining in this datablock for the new DIR
    {
     	//enter the new entry as the LAST entry and trim the previous entry to its IDEAL_LENGTH;
      LOG(LOG_DBG, "new directory entry should fit in current block\n");
      //trim previous length
      dp->rec_len = 4 * ( (8 + strlen(dp->name) + 3)/4 );
      //move cp to end of the previous (start of new last entry)
      cp += dp->rec_len;
      //update dir pointer
    	dp = (DIR *)cp;

      LOG(LOG_DBG, "setting newblock to pipnode->i_block[%d]: %d\n", i, pipnode->i_block[i]);
      newblock = pipnode->i_block[i];

      //now this is where the new dir will go
      LOG(LOG_DBG, "cp and dp are ready at location, for the insertion of new dir info\n");
      LOG(LOG_DBG, "Hitting break statement here:\n");

      break;//Kc's goto
    }
  }//end for block

  if (flag == TRUE)//if flag == true do the code below(GOTO hack)
  {
    LOG(LOG_DBG, "we need to allocate a new block for this directory entry\n");
    //Allocate a new data block; INC parent's i_size by 1024;
    newblock = balloc(pip->dev);
    //set block num in parent datablock
    pipnode->i_block[i] = newblock;
    pipnode->i_size += BLOCK_SIZE;

    //Enter new entry as the first entry in the new data block with rec_len=BLKSIZE
    get_block(pip->dev, newblock, buf);

    //prep dp for the write to disk
    cp = buf;
    dp = (DIR *)cp;
  }
  LOG(LOG_DBG, "dp and cp should point to where the new dir will be written. configure DIR\n");

  //insert dir at dp, consume the remaining block space for entry.
  dp->inode = myino;
  dp->name_len = strlen(myname);
  //not sure this is working as intended
  dp->rec_len = (int)(buf + BLOCK_SIZE) - (int)dp;
  dp->file_type = 2;//////////     0  or 2 ?         ///////
  strcpy(dp->name, myname);

  //testing final results before iputting to memeory/disk
  dp = (DIR *)buf;
  cp = buf;
  while (cp + dp->rec_len < buf + BLOCK_SIZE)
  {
    printDir(dp);
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

  LOG(LOG_DBG, "printing one more to see the newest added dir\n");
  //go one more time to view the newest added
  //printDir(dp);

  //write back to fs
  LOG(LOG_DBG, "put_block to disk, block #: %d\n", newblock);
  put_block(pip->dev, newblock, buf);

  // LOG(LOG_INFO, "Trying iput on the passed in pointer to the parent too\n");
//////////////////////////////////////////////
  //iput(pip);
///////////////////////////////////////////////
}



/*********************************************************************/
/*********************************************************************/
int rmdir()
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

  //set parent/child to dirname/basename respectively
  strcpy(temp_buf, pathname);
  strcpy(parent, dirname(temp_buf));
  strcpy(temp_buf, pathname);
  strcpy(child, basename(temp_buf));

  LOG(LOG_DBG, "pathname ( %s ) has been split to parent : %s , child: %s\n", pathname, parent, child);

  //Get parent ino number and minode for parent
  pino    = getino(&dev, parent);
  pip     = iget(dev, pino);

  if(pino == INVALID || pip == INVALID )
  {
    printf( "%sThat parent does not exist%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  //Get the node to be deleted's minode
  delnum  = getino(&dev, pathname);
  delnode = iget(dev, delnum);

  if(delnum == INVALID || delnode == INVALID )
  {
    printf( "%sThat directory does not exist%s\n", COLOR_RED, COLOR_RESET);
    iput(pip);
    return INVALID;
  }

  //check owndership: super user ok, no super uid must match
  if(running->uid != 0 && delnode != INVALID)//current proc is not super user
  {
    LOG(LOG_DBG, "Non-super user detected attempting to rmdir. Checking ownership.\n");
    if(running->uid != delnode->INODE.i_uid)//allow to rmdir
    {
      printf( "%sYou do not have permissions to remove that directory%s\n", COLOR_RED, COLOR_RESET);
      iput(delnode);
      iput(pip);
      return INVALID;
    }
  }

  if(delnode->INODE.i_links_count > 2)
  {
    printf( "%sThat directory is not empty%s\n", COLOR_RED, COLOR_RESET);
    iput(delnode);
    iput(pip);
    return INVALID;
  }

  //verify node to be deleted INODE is a dir
  if(!S_ISDIR(delnode->INODE.i_mode))
  {
    printf( "%sThat is not a valid directory.%s\n", COLOR_RED, COLOR_RESET);
    iput(delnode);
    iput(pip);
    return INVALID;
  }
  //Verify the directory is not busy
  LOG(LOG_DBG, "printing delnode before check allowance of rmdir\n");
  //printminode(delnode);
  LOG(LOG_DBG, "Made it past the printing of delnode\n");
  if(delnode->refCount > 1)//do not allow
  {
    LOG(LOG_DBG, "Attempting to remove busy directory, failure.\n");
    printf("That directory is busy, unable to remove.\n");
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
  // decrement pip's link_count by 1;
  pip->INODE.i_links_count--;
  //  touch pip's atime, mtime fields;

  //  mark pip dirty;
  pip->dirty = TRUE;
  //  iput(pip);
  iput(pip);
  return 1;
}

/*********************************************************************/
/*********************************************************************/
int rm_child(MINODE *pip, char *name)
{
  int i = 0, remain = 0, newblock = 0, flag = 0, limit = 0, j = 0;
  INODE *ip, *pipnode = &pip->INODE;
  char buf[BLOCK_SIZE];
  char *cp, *placeHolder;
  int firstEntryFlag = TRUE, foundFlag = FALSE;

  //guard check
  if(!pip || !name)
  {
    printf( "%sInvalid parameter%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }


  LOG(LOG_DBG, "Rm_child: %s , on parent ino: %d\n", name, pip->ino);
  for(i = 0; i < 12; i++) //iterate over data blocks in parent inode
  {
    if(foundFlag == TRUE)
    {
      i--;
      break;
    }
    if(pipnode->i_block[i] == 0)//reached the end of datablocks
    {
      LOG(LOG_DBG, "pipnode->i_block[%d] = 0, ending i_block iteration\n", i);
      break;
    }

    //otherwise, go get that data block
    LOG(LOG_DBG, "Getting parent->iblock[%d]'s block]\n", i);
    get_block(pip->dev, pipnode->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    LOG(LOG_DBG, "got block from disk, now read directory entries and find name to removed\n");
    //Step to the last entry in a data block
    while (cp + dp->rec_len <= buf + BLOCK_SIZE)
    {
      printDir(dp);
      LOG(LOG_DBG, "comparing dir->name: %s with %s\n", dp->name, name);
      if(strncmp(dp->name, name, dp->name_len) == 0)
      {
        LOG(LOG_DBG, "match found! i = %d\n", i);
        foundFlag = TRUE;
        break;
      }
      LOG(LOG_DBG, "no match, iterate cp/dp\n");
      firstEntryFlag = FALSE;//need to keep track if first entry
      cp += dp->rec_len;
    	dp = (DIR *)cp;
    }
  }//end of for

  //check if we didnt find it
  if(!foundFlag){
    printf( "%sUnable to locate rmdir in parent node%s\n", COLOR_YELLOW, COLOR_RESET);
    return INVALID;
  }else{//did find the directory

    //get limit for shifty shifts
    limit = buf + BLOCK_SIZE;

    if(dp->rec_len == BLOCK_SIZE)//first AND only entry in block -> deallocate this block
    {
      LOG(LOG_DBG, "first and only entry in block\n");
    	//handle only entry case;
      //dealloc block
      bdealloc(pip->dev, inumtoblock(dp->inode));

      //adjust all the iblocks up one. (NOTE: THIS WILL HAVE TO BE ADJUSTED FOR INDIRECT BLOCKS)
      LOG(LOG_DBG, "setting j = i: j=%d i= %d\n", j, i);
      j = i;
      while((j+1) < 12){
        LOG(LOG_DBG, "setting pipnode->i_block[%d] = pipnode->i_block[%d]", j, (j+1));
        pipnode->i_block[j] = pipnode->i_block[j+1];
      }
      //reduce parents size
      pipnode->i_size -= BLOCK_SIZE;
      put_block(pip->dev, pipnode->i_block[i], buf);
      return;
    }
    else if(cp + dp->rec_len == buf + BLOCK_SIZE) //last item in block // cp + dp->rec_len < buf + BLOCK_SIZE
    {
      LOG(LOG_DBG, "last entry in block\n");
      //handle last entry case
      placeHolder = cp;//hold this place for maff
      //restart at the beginning and go thru the directories again
      dp = (DIR *)buf;
      cp = buf;

      //go to the entry right before the one to be deleted
      while(cp + dp->rec_len != placeHolder){
        cp += dp->rec_len;
      	dp = (DIR *)cp;
      }

      //set the rec length to the end of the BLOCK_SIZE
      dp->rec_len = (dp->rec_len) + ((DIR *)placeHolder)->rec_len;
      //zero out the deleted directories memory
      memset(placeHolder, 0, (buf + BLOCK_SIZE - placeHolder));
      //gtfo
      put_block(pip->dev, pipnode->i_block[i], buf);
      return;
    }

    LOG(LOG_DBG, "removing middle entry\n");
    //Remove middle entry case
    placeHolder = cp + dp->rec_len;
    //keep a copy of how much memory we are writing over (use previous variable, j)
    j = dp->rec_len;
    LOG(LOG_DBG, "attempting memcpy trick\n");
    /*

    |  QQ  |  RR  |  deleteMe  |  SS  | TT |
    ^buf          ^cp/dp       |           ^buf+BLOCK_SIZE
                  |<--- j ---->|
                               ^placeholder
                               |-----------|
                            V---right to the end of block---V
    */
    memcpy(cp, placeHolder, (buf + BLOCK_SIZE) - placeHolder);

    //get back to the last entry
    cp = buf;
    dp = (DIR *)buf;
    LOG(LOG_DBG, "attempting to get to the previous dir entry w/ a while loop\n");
    LOG(LOG_DBG, "dp at first, before the first iteration of while loop\n");
    printDir(dp);
    while(cp + dp->rec_len != (buf + BLOCK_SIZE -j))//where we were before
    {
      cp += dp->rec_len;
      dp = (DIR *)cp;
      LOG(LOG_DBG, "dp in while loop\n");
      //printDir(dp);
    }//now we are in position to do some magic
    //add the amount of deleted memory to the rec lec
    dp->rec_len += j;
    LOG(LOG_DBG, "attempting put_block after removing middle entry\n");
    put_block(pip->dev, pipnode->i_block[i], buf);
    return;
  }//end of found else
}
