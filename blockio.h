/**********************************************************************
*           Author: Daniel Johnson & Alan Tyson
*           Last Update: 2016-03-30
*
*     BLOCKIO.H is a catch-all header for nifty utility functions we
*   seem to keep using.
*
**********************************************************************/


/*********************************************************************/
/*********************************************************************/
int decFreeBlocks(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}


/*********************************************************************/
/*********************************************************************/
int incFreeBlocks(int dev)
{
  char buf[BLOCK_SIZE];

  // dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
}


/*********************************************************************/
/*********************************************************************/
int get_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLOCK_SIZE, 0);
  read(fd, buf, BLOCK_SIZE);
}


/*********************************************************************/
/*********************************************************************/
int put_block(int fd, int blk, char buf[ ])
{
  lseek(fd, (long)blk*BLOCK_SIZE, 0);
  write(fd, buf, BLOCK_SIZE);
}


/*********************************************************************/
/*********************************************************************/
int balloc(int dev)
{
  int  i;
  char buf[BLOCK_SIZE];

  // read inode_bitmap block
  get_block(dev, BBITMAP, buf);

  for (i=0; i < BLOCK_SIZE; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       decFreeBlocks(dev);

       put_block(dev, BBITMAP, buf);
       //blocks start at 0
       return i;
    }
  }
  printf( "%sNo available blocks%s\n", COLOR_RED, COLOR_RESET);
  return INVALID;
}


/*********************************************************************/
/*********************************************************************/
int bdealloc(dev, bno)
{
  int  i;
  char buf[BLOCK_SIZE];

  // read inode_bitmap block
  get_block(dev, BBITMAP, buf);

  clr_bit(buf, bno);
  incFreeBlocks(dev);

}


/*********************************************************************/
/*********************************************************************/
mountroot()   /* mount root file system */
{
  int i, ino, fd, dev;
  MOUNT *mp;
  SUPER *sp;
  MINODE *ip;

  char line[64], buf[BLOCK_SIZE], *rootdev;
  int ninodes, nblocks, ifree, bfree;

  printf("Enter Rootdev Name (RETURN for mydisk) : ");
  gets(line);

  rootdev = "mydisk";

  if (line[0] != 0)
     rootdev = line;

  dev = open(rootdev, O_RDWR);
  if (dev < 0){
     printf( "%sCannot open root device, Program wil exit%s\n", COLOR_RED, COLOR_RESET);
     exit(1);
  }

  /* get super block of rootdev */
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* check magic number */
  printf( "%sSUPER magic=0x%x%s\n", COLOR_GREEN, sp->s_magic, COLOR_RESET);
  if (sp->s_magic != SUPER_MAGIC){
     printf( "%sSuper Magic=%x : %s is not a valid Ext2 filesystem%s\n", COLOR_RED, sp->s_magic, rootdev, COLOR_RESET);
     exit(0);
  }

  mp = &mounttab[0];      /* use mounttab[0] */

  /* copy super block info to mounttab[0] */
  ninodes = mp->ninodes = sp->s_inodes_count;
  nblocks = mp->nblocks = sp->s_blocks_count;
  bfree = sp->s_free_blocks_count;
  ifree = sp->s_free_inodes_count;

  LOG(LOG_DBG, "get block 2 to buf\n");
  get_block(dev, 2, buf);
  LOG(LOG_DBG, "cast buf as ext2_group_desc\n");
  gp = (GD *)buf;

  mp->dev = dev;
  mp->busy = BUSY;

  mp->bmap = gp->bg_block_bitmap;
  BBITMAP = mp->bmap;
  mp->imap = gp->bg_inode_bitmap;
  IBITMAP = mp->imap;
  mp->iblock = gp->bg_inode_table;
  INODEBLOCK = mp->iblock;

  strcpy(mp->name, rootdev);
  strcpy(mp->mount_name, "/");


  printf( "%s bmap=%d  ",   COLOR_GREEN, gp->bg_block_bitmap);
  printf( "imap=%d  ",   gp->bg_inode_bitmap);
  printf( "iblock=%d%s\n", gp->bg_inode_table, COLOR_RESET);


  /***** call iget(), which inc the Minode's refCount ****/
  LOG(LOG_DBG, "calling iget for root inode (2)\n");
  root = iget(dev, ROOT_INODE);          /* get root inode */
  strcpy(root->name, "/");
  LOG(LOG_DBG, "inode found!\n", i);
  LOG(LOG_DBG, "mode=%04x ", root->INODE.i_mode);
  LOG(LOG_DBG, "uid=%d  gid=%d\n", root->INODE.i_uid, root->INODE.i_gid);
  LOG(LOG_DBG, "size=%d\n", root->INODE.i_size);
  LOG(LOG_DBG, "link=%d\n", root->INODE.i_links_count);
  mp->mounted_inode = root;
  root->mountptr = mp;

  printf("%smount : %s  mounted on / %s\n", COLOR_GREEN, rootdev, COLOR_RESET);
  return;
}
