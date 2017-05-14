/**********************************************************************
*           Author: KC Wang Daniel Johnson & Alan Tyson
*           Last Update: 2016-03-30
*
*     INODEFUNCT.H should contain all the functions related to inode
*   address resolution or I/O.
*
*     Lots of code came from KC Wang here http://www.eecs.wsu.edu/~cs360/util.html
**********************************************************************/


/*********************************************************************/
/*********************************************************************/
int firstAvailableMinode()
{
 int i = 0;

  for(i = 0; i < NMINODES; i++){
    if(minode[i].refCount == 0){
      memset(&(minode[i]), 0 , sizeof(MINODE));
     	return i;
    }
  }
	return INVALID;
}


/*********************************************************************/
/*********************************************************************/
int decFreeInodes(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}


/*********************************************************************/
/*********************************************************************/
int incFreeInodes(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}


/*********************************************************************/
/*********************************************************************/
int inumtoblock(int inumber)
{
  // Solution:  block# = (i_number - 1) / INODES_PER_BLOCK + inode_table;
  //               inode# = (i_number - 1) % INODES_PER_BLOCK;
  return ((inumber - 1) / INODES_PER_BLOCK) + INODEBLOCK;

}


/*********************************************************************/
/*********************************************************************/
int inumtooffset(int inumber)
{
  // Solution:  block# = (i_number - 1) / INODES_PER_BLOCK + inode_table;
  //               inode# = (i_number - 1) % INODES_PER_BLOCK;
  return (inumber - 1) % INODES_PER_BLOCK;
}


/*********************************************************************/
/*********************************************************************/
int ialloc(int dev)
{
  int  i;
  char buf[BLOCK_SIZE];
  MOUNT *mp = &(mounttab[0]);

  // read inode_bitmap block
  get_block(dev, mp->imap, buf);

  for (i=0; i < BLOCK_SIZE; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, IBITMAP, buf);
       //inodes start at 1
       return i+1;
    }
  }
  //printf("No available inodes\n");
  return INVALID;
}


/*********************************************************************/
/*********************************************************************/
int idealloc(dev, ino)
{
  int  i;
  char buf[BLOCK_SIZE];

  // read inode_bitmap block
  get_block(dev, IBITMAP, buf);

  clr_bit(buf, ino);
  incFreeInodes(dev);

}


/*********************************************************************/
/*********************************************************************/
/*
====================== USAGE of getino() ===================================
Given a pathname, if pathname begins with / ==> dev = root->dev;
                  else                          dev = running->cwd->dev;
With mounting (in level-3), dev may change when crossing mounting point(s).
Whenever dev changes, record it in  int *dev  => dev is the FINAL dev reached.

int ino = getino(&dev, pathname) essentially returns (dev,ino) of a pathname
*/
int getino(int *dev, char *pathname)
{
  int inumber = INVALID, i = 0;
  char buf[BLOCK_SIZE];
  char *path[64];
  MINODE *searchminode = 0;
  INODE *tempInode = 0;


  LOG(LOG_DBG, "in get_ino() pathname = %s\n", pathname);

  //validate input
  if(!pathname || !(*dev))
  {
    printf("%sInvalid parameters!%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  //Set device and search startpoints
  if(pathname[0] == '/')  //if path is absolute
  {
    LOG(LOG_DBG, "pathname is absolute! Setting searchminode to root\n");
    //set dev to root device
    dev = root->dev;
    //set searchminode to root minode
    searchminode = root;
  }
  else  //relative paths
  {
    LOG(LOG_DBG, "pathname is relative! Setting searchminode to cwd\n");
    //set dev to same dev as cwd Minode
    dev = running->cwd->dev;
    //set searchminode to cwd
    searchminode = running->cwd;
  }

  LOG(LOG_DBG, "tokenize pathname to path\n");
  //LETS SEARCH THIS BIAAATCH
  tokenize(pathname, path, "/");

  while (path[i]) { //while we still have tokens to kind
    //get inumber of next directory
    LOG(LOG_DBG, "search searchminode ( %s ) for path[%d]: %s\n", searchminode->name, i, path[i]);
    inumber = search(searchminode, path[i]);
    LOG(LOG_DBG, "search returned inumber: %d\n", inumber);    //check that inumber is valid
    if (inumber > 0) {
      //found the inode
      searchminode = iget(dev, inumber);
      searchminode->refCount--;
      LOG(LOG_DBG, "loaded minode by inumber: %d\n", inumber);    //check that inumber is valid
      //printminode(searchminode);
    }
    else //path does not exist
    {
      printf("%sPath does not exist%s\n", COLOR_RED, COLOR_RESET);
      return INVALID;
    }
    i++;
  }
  LOG(LOG_DBG, "search complete returning inumber: %d\n", searchminode->ino);
  return searchminode->ino;
}


/*********************************************************************/
/*********************************************************************/
/*
This function searches the data blocks of a DIR inode (inside an Minode[])
   for name. You may assume a DIR has only 12 DIRECT data blocks.
*/
int search(MINODE *minodePtr, char *name)
{
  char *cp;
  char buf[BLOCK_SIZE];
  int i;
  DIR *dp;
  INODE *ip;

  if (!minodePtr || !name || !strcmp(name, ""))
  {
    printf("%sInvalid parameters!%s\n", COLOR_RED, COLOR_RESET);
      return INVALID;
  }

  //grab the inode from the MINODE
  ip = &(minodePtr->INODE);
  //iterate over direct blocks
  for (i=0; i<12; i++){
    LOG(LOG_DBG, "have inode pointer begin checking datablock at i = %d\n", i);
      //check for valid block
      if (ip->i_block[i] == 0)
      {
          LOG(LOG_WARN, "%sInvalid block%s\n", COLOR_YELLOW, COLOR_RESET);
          return INVALID;
      }
      //get inode data block to buf and set the pointers
      get_block(minodePtr->dev, ip->i_block[i], buf);
      dp = (DIR *)buf;
      cp = buf;
      LOG(LOG_DBG, "getblock and pointer assignment set, check buf for dir\n");
      //Let it begin!
      while (cp < (buf + BLOCK_SIZE)){ //iterate pointer until we hit end of block
        LOG(LOG_DBG, "cp = %8x, buf = %8x, buf+BLOCK_SIZE = %8x\n", cp, buf, (buf + BLOCK_SIZE));
         //search for name and return the inumber
         LOG(LOG_DBG, "comparing dp->name: %s to name: %s for %d chars\n", dp->name, name, strlen(name));
         if (strncmp(dp->name, name, strlen(name)) == 0) {
           return dp->inode;
         }
         else if(!strcmp(dp->name, ""))
         {
           printf("%sInvalid name!%s\n", COLOR_RED, COLOR_RESET);
           return INVALID;
         }
         LOG(LOG_DBG, "search failed move to next dir (dp->rec_len = %d)\n", dp->rec_len);
         //increment cp and dp to the next DIR entry
         cp = cp + dp->rec_len;
         dp = (DIR *)cp;
      }
  }

  //TODO: finish double indirect, and triple indirect blocks.

  return INVALID;
}

/*********************************************************************/
/*********************************************************************/
MINODE *iget(int dev, int ino){

  int i = 0, j = 0;
  INODE *inode_temp = 0;
  char buf[BLOCK_SIZE];

  if(ino == INVALID)
  {
    printf("%sInvalid parameters!%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  LOG(LOG_DBG, "looking for ino: %d in Minodes\n", ino);
  //search already grabbed inodes
  for(i = 0; i < NMINODES; i++)
  {
    if(minode[i].refCount > 0 && minode[i].ino == ino && minode[i].dev == dev)//the inode already exists in our array of 'memory'
    {
      LOG(LOG_DBG, "found inode in minode array at index %d\n", i);
      //increment refcount and return
      minode[i].refCount++;
      LOG(LOG_DBG, "inode found!\n", i);
      LOG(LOG_DBG, "mode=%4x \n", minode[i].INODE.i_mode);
      LOG(LOG_DBG, "uid=%d  gid=%d\n", minode[i].INODE.i_uid, minode[i].INODE.i_gid);
      LOG(LOG_DBG, "size=%d\n", minode[i].INODE.i_size);
      LOG(LOG_DBG, "link=%d\n", minode[i].INODE.i_links_count);
      //add to array and update refcount
      return &minode[i];
    }
  }
  LOG(LOG_DBG, "get the block %d for inode %d\n", inumtoblock(ino), ino);
  //not already in array, get block from disk containing inode
  get_block(dev, inumtoblock(ino), buf);
  //offset to particular inode
  LOG(LOG_DBG, "offset into the block by %d\n", inumtooffset(ino));

  inode_temp = (INODE *)buf + inumtooffset(ino);

  LOG(LOG_DBG, "inode found in disk, pulling into memory array!\n", i);
  LOG(LOG_DBG, "mode=%4x \n", inode_temp->i_mode);
  LOG(LOG_DBG, "uid=%d  gid=%d\n", inode_temp->i_uid, inode_temp->i_gid);
  LOG(LOG_DBG, "size=%d\n", inode_temp->i_size);
  LOG(LOG_DBG, "link=%d\n", inode_temp->i_links_count);

  LOG(LOG_DBG, "putting found disk INODE into minode array\n", i);
  //get first open index in array
  i =  firstAvailableMinode();
  LOG(LOG_DBG, "firstAvailableMinode returned: %d\n", i);
  //if none available return null

  if (i < 0)
  {
    printf("%sNo available Minodes%s\n", COLOR_RED, COLOR_RESET);
    return INVALID;
  }

  LOG(LOG_DBG, "configure minode\n", i);
  //add to array and update refcount

  // minode[i].INODE = *inode_temp;
  minode[i].INODE.i_mode = inode_temp->i_mode;
  minode[i].INODE.i_uid = inode_temp->i_uid;
  minode[i].INODE.i_size = inode_temp->i_size;
  minode[i].INODE.i_atime = inode_temp->i_atime;
  minode[i].INODE.i_ctime = inode_temp->i_ctime;
  minode[i].INODE.i_mtime = inode_temp->i_mtime;
  minode[i].INODE.i_dtime = inode_temp->i_dtime;
  minode[i].INODE.i_gid = inode_temp->i_gid;
  minode[i].INODE.i_links_count = inode_temp->i_links_count;

  for(j = 0; j < 15; j++)
  {
    minode[i].INODE.i_block[j] = inode_temp->i_block[j];
  }

  minode[i].mountptr = getmountp(dev);
  minode[i].dev = dev;
  minode[i].refCount++;
  minode[i].ino = ino;
  minode[i].dirty = minode[i].mounted = 0;

  //printminode(&minode[i]);

  return &minode[i];
}


/*********************************************************************/
/*********************************************************************/
//precondition: mip is already pointing to the correct minode inside the Minode array
int iput(MINODE *mip)
{
  INODE *inode_temp = 0;
  char buf[BLOCK_SIZE];
  int i = 0, j = 0;

  LOG(LOG_DBG, "attempting iput on mip->dev: %d mip->ino %d block: %d\n", mip->dev, mip->ino, inumtoblock(mip->ino));

  mip->refCount--;
  //if still in use, leave it alone
  if(mip->refCount > 0){
    LOG(LOG_DBG, "MINODE Still in use. refcount = %d\n", mip->refCount);
    return;
  }
  //if it's not dirty, leave it alone
  if(mip->dirty == 0){
    LOG(LOG_DBG, "MINODE not dirty. dirty = %d\n", mip->dirty);
    return;
  }

  //otherwise, write back to disk...
  // Use minode's (dev, ino) to determine which INODE on disk
  get_block(mip->dev, inumtoblock(mip->ino), buf);
  LOG(LOG_DBG, "get block %d to write inode back to.\n", inumtoblock(mip->ino));
  //Read that block into a buf[ ], let INODE *ip point at the INODE in buf[ ].
  inode_temp = (INODE *)buf + inumtooffset(mip->ino);
  LOG(LOG_DBG, "offset by %d into buf\n", inumtooffset(mip->ino));

  //Copy mip->INODE into *ip in buf[ ]
  LOG(LOG_DBG, "Copying Inode to buf. ## WARNING: MAY NOT BE GOOD#\n", inumtooffset(mip->ino));

  // *inode_temp = mip->INODE;
  inode_temp->i_mode = mip->INODE.i_mode;
  inode_temp->i_uid = mip->INODE.i_uid;
  inode_temp->i_size = mip->INODE.i_size;
  inode_temp->i_atime = mip->INODE.i_atime;
  inode_temp->i_ctime = mip->INODE.i_ctime;
  inode_temp->i_mtime = mip->INODE.i_mtime;
  inode_temp->i_dtime = mip->INODE.i_dtime;
  inode_temp->i_gid = mip->INODE.i_gid;
  inode_temp->i_links_count = mip->INODE.i_links_count;

  LOG(LOG_DBG, "copying over iblocks\n");

  for(j = 0; j < 15; j++)
  {
    inode_temp->i_block[j] = mip->INODE.i_block[j];
  }

  //Write the block (in buf[ ]) back to disk.
  LOG(LOG_DBG, "now sending to put_block on blk: %d\n", inumtoblock(mip->ino) );
  put_block(mip->dev, inumtoblock(mip->ino), buf);
}


/*********************************************************************/
/*********************************************************************/
int findino(MINODE *minodeptr, int *myino, int *parentino)
{
  char buf[BLOCK_SIZE], *cp;
  DIR *dp;


  if (!minodeptr)
  {
    printf("%sInvalid Parameters%s\n", COLOR_RED, COLOR_RESET);
      return INVALID;
  }
  LOG(LOG_DBG, "getting block %d to buf\n", minodeptr->INODE.i_block[0]);
  //get first datablock of dir
  get_block(minodeptr->dev, minodeptr->INODE.i_block[0], buf);

  LOG(LOG_DBG, "set pointers\n");
  //set pointers
  dp = (DIR *)buf;
  cp = buf;

  LOG(LOG_DBG, "our ino: %d\n", dp->inode);
  //dot = us
  *myino = dp->inode;

  //move pointers
  cp += dp->rec_len;
  dp = (DIR *)cp;

  LOG(LOG_DBG, "parent ino: %d\n", dp->inode);
  //dotdot = parent
  *parentino = dp->inode;

}


/*********************************************************************/
/*********************************************************************/
int findmyname(MINODE *parent, int myino, char *myname)
{

  char *cp;
  char buf[BLOCK_SIZE];
  int i;
  DIR *dp;
  INODE *ip;

  if (!parent || myino == INVALID)
  {
      printf("%sInvalid parameters!%s\n", COLOR_RED, COLOR_RESET);
      return INVALID;
  }

  //grab the inode from the MINODE
  ip = &(parent->INODE);
  //iterate over direct blocks
  for (i=0; i<12; i++){
    LOG(LOG_DBG, "have inode pointer begin checking datablock at i = %d\n", i);
      //check for valid block
      if (ip->i_block[i] == 0)
      {
        LOG(LOG_WARN, "%sInvalid block%s\n", COLOR_YELLOW, COLOR_RESET);
          return INVALID;
      }
      //get inode data block to buf and set the pointers
      get_block(parent->dev, ip->i_block[i], buf);
      dp = (DIR *)buf;
      cp = buf;
      //Let it begin!
      while (cp < (buf + BLOCK_SIZE)){ //iterate pointer until we hit end of block
         //search for name and return the inumber
         LOG(LOG_DBG, "comparing dp->ino: %d to myino: %d\n", dp->inode, myino);
         if (dp->inode == myino) {
           strcpy(myname, dp->name);
           return TRUE;
         }
         else if(!strcmp(dp->name, ""))
         {
           printf("%sInvalid name!%s\n", COLOR_RED, COLOR_RESET);
           return INVALID;
         }
         LOG(LOG_DBG, "search failed, move to next dir (dp->rec_len = %d)\n", dp->rec_len);
         //increment cp and dp to the next DIR entry
         cp = cp + dp->rec_len;
         dp = (DIR *)cp;
      }
    }
}
