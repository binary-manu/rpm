#define PART_FAT_C
#include "part.h"

#define PM_DATE  10345	// March-09-2000
#define PM_TIME   5320	// 2:38a
#define PM_FILE "A:\\part.exe"
//#define PM_FILE   "part2.exe"
// sets file date / time and closes the file
extern "C" int stclose(int fd, int date, int time);


/* Don't look at the code below. It isn't in the good shape yet.
   I will format it nicely later.
*/

#ifdef MT4P
#pragma pack(push) // save structure packing alignment
#pragma pack(1)    // set it to 1 byte (no alignment)
#endif

struct boot_fat16
    {
     unsigned char  jmp[3];	/* Must be 0xEB, 0x3C, 0x90		*/
     unsigned char  sys_id[8];	/* Probably:   "MSDOS5.0"		*/
     unsigned short sect_size;	/* Sector size in bytes (512)		*/
     unsigned char  clust_size;	/* Sectors per cluster (1,2,4,...,128)	*/
     unsigned short res_sects;	/* Reserved sectors at the beginning	*/
     unsigned char  num_fats;	/* Number of FAT copies (1 or 2)	*/
     unsigned short root_entr;	/* Root directory entries		*/
     unsigned short snum_sect;	/* Total sectors (if less 64k)		*/
     unsigned char  media_desc;	/* Media descriptor byte (F8h for HD)	*/
     unsigned short fat_size;	/* Sectors per fat			*/
     unsigned short num_sects;	/* Sectors per track			*/
     unsigned short num_sides;	/* Sides				*/
     unsigned long  rel_sect;	/* Special hidden sectors		*/
     unsigned long  num_sect;	/* Big total number of sectors  	*/
     unsigned short drive_num;	/* Drive number				*/
     unsigned char  ext_signat;	/* Extended Boot Record signature (29h)	*/
     unsigned long  serial_num;	/* Volume serial number			*/
     unsigned char  label[11];	/* Volume label				*/
     unsigned char  fs_id[8];	/* File system id			*/
     unsigned char  code[448];	/* Loader executable code		*/
     unsigned short magic_num;	/* Magic number (Must be 0xAA55) 	*/
    };

struct boot_fat32
    {
     /* Sector 1 */
     
     unsigned char  jmp[3];	/* Must be 0xEB, 0x58, 0x90 = jmp 5A	*/
     unsigned char  sys_id[8];	/* Probably:   "MSWIN4.1"		*/

     unsigned short sect_size;	/* Sector size in bytes (512)		*/
     unsigned char  clust_size;	/* Sectors per cluster (1,2,4,...,128)	*/
     unsigned short res_sects;	/* Reserved sectors at the beginning (33)*/
     unsigned char  fat_copies;	/* Number of FAT copies (2)		*/

     unsigned char  resrvd1[4];	/* Reserved				*/
     unsigned char  media_desc;	/* Media descriptor byte (F8h)		*/
     unsigned short sfat_size;	/* Sectors per FAT			*/

     unsigned short track_size;	/* Sectors per track			*/
     unsigned short num_sides;	/* Sides				*/

     unsigned long  rel_sect;	/* Special hidden sectors		*/
     unsigned long  num_sect;	/* Big total number of sectors  	*/
     unsigned long  fat_size;	/* Sectors per FAT (big)		*/
     unsigned char  fat_attr;	/* FAT attributes (I guess)		*/

     unsigned char  fs_ver_maj;	/* File System Version (major)		*/
     unsigned short fs_ver_min;	/* File System Version (minor)		*/

     unsigned long  root_clust;	/* First cluster in root		*/

     unsigned short fs_sect_num; /* FS Sector number (1) ???		*/
     unsigned short bs_bak_sect; /* Boot sector backup (6)		*/

     unsigned char  resrvd2[12];	/* Reserved				*/
     unsigned char  drive_num;	/* Physical drive number (80h)		*/
     unsigned char  resrvd3[1];	/* Reserved				*/

     unsigned char  ext_signat;	/* Extended Boot Record signature (29h)	*/
     unsigned long  serial_num;	/* Volume serial number			*/
     unsigned char  label[11];	/* Volume label				*/
     unsigned char  fs_id[8];	/* File system id ("FAT32   ")		*/
     unsigned char  xcode[418];	/* Boot loader code (first part)	*/
     unsigned long  magic_num;	/* Magic number (Must be 0xAA550000) 	*/
     
     /* Sector 2 */
     
     unsigned long  ext_sign2;	 /* Ext Boot Record Sign (0x41615252)	*/
     unsigned char  resrvd4[480];/* Reserved				*/
     unsigned long  ext_sign3;	 /* FS Info Signature    (0x61417272)	*/
     unsigned long  free_clust;	 /* Number of free clusters		*/
     unsigned long  next_free;	 /* Next free cluster			*/
     unsigned char  resrvd5[12]; /* Reserved				*/
     unsigned long  magic_num2;	 /* Ext Boot Record Sign (0xAA550000)	*/

     /* Sector 3 */

     unsigned char  resrvd6[508];/* Reserved				*/
     unsigned long  magic_num3;	 /* Ext Boot Record Sign (0xAA550000)	*/
    };

#ifdef MT4P
#pragma pack(pop) // restore structure packing alignment
#endif

int fat_handler(int op, DiskInfo *di, Part *p, char **argv)
{
 int i, sys, color;
 FatInfo *fi;
 struct boot_fat16 *f16;
 struct boot_fat32 *f32;
 Sector rel, num;

 if( op==OP_FORMAT ) 
   {
    fat_format(di,p,fi);
    free_part_info(p);
    return 0;
   }

 read_boot_sect(di,p,1);

 if( !BTS_LOADED(p) )
   {
    return 0;
   }

 f16 = (struct boot_fat16 *)(p->part_boot_sect);
 f32 = (struct boot_fat32 *)(p->part_boot_sect);

 if( p->part_misc_info==0 )
   {
    if( (p->part_misc_info=malloc(sizeof(FatInfo)))==0 )
      {
       show_error(ERROR_MALLOC);
       return 0;
      }
    memset( p->part_misc_info, 0, sizeof(FatInfo) );
   }

 fi = (FatInfo*) p->part_misc_info;

 update_fat_info(di,p,fi);

 fi->exp_rel_sect = p->rel_sect - p->ext_base;
 fi->exp_num_sect = p->num_sect;
 fi->exp_drive_num = di->hd_num;
 
 if( fi->rel_sect == p->rel_sect ) fi->exp_rel_sect = p->rel_sect;

 p->flags &= ~P_ERR_BOOT_SECT;  /* Assuming that boot sector is ok */

// if( count_zeros(f16,512)==512 )  /* looks like unformatted */
//   {
//    p->flags |= P_ERR_BOOT_SECT;
//   }

 if( f16->magic_num != MAGIC_NUM ||
     fi->rel_sect != fi->exp_rel_sect ||
     //(fi->min_num_sect==0 && fi->num_sect != fi->exp_num_sect) || ???
     (fi->min_num_sect!=0 && (fi->num_sect > min(fi->exp_num_sect,fi->max_num_sect) ||
                              fi->num_sect < fi->min_num_sect ) ) )
    p->flags |= P_ERR_BOOT_SECT;

 if( op==OP_VALIDATE ) return 0;
// if( op==OP_PRINT )   print_fat_details(fi);
 if( op==OP_PREVIEW ) bgview_fat_details(di,p,fi,-1);
 if( op==OP_SETUP ) edit_fat_details(di,p,fi);

 return 0;
}/* fat_handler */


void bgview_fat_details(HardDiskInfo *di, Part *p, FatInfo *fi, int act)
{
 while(1)
    {
     redraw_fat_details(fi,act);
     if( fi->fat_sect_read >= fi->fat_size ) break;
     if( kb_hit() ) break;
     read_fat_table(di,p,fi);
    }
}



void update_fat_info(HardDiskInfo *di, Part *p, FatInfo *fi)
{
 unsigned long x;
 struct boot_fat16 *f16;
 struct boot_fat32 *f32;

 if( fi->rel_sect_loaded == p->rel_sect_loaded ) return;
 
 fi->rel_sect_loaded = p->rel_sect_loaded;

 f16 = (struct boot_fat16 *)(p->part_boot_sect);
 f32 = (struct boot_fat32 *)(p->part_boot_sect);
 
      if( memcmp(f16->fs_id,"FAT12   ",8)==0 ) fi->sys=12;
 else if( memcmp(f16->fs_id,"FAT16   ",8)==0 ) fi->sys=16;
 else if( memcmp(f32->fs_id,"FAT32   ",8)==0 ) fi->sys=32;
 else if( p->fs_type==0x0B00 || p->fs_type==0x0C00 ||
          p->fs_type==0x1B00 || p->fs_type==0x1C00 ) fi->sys=32;
                                            else fi->sys=16;
 if( fi->sys==32 )
   {
    fi->fat_size = f32->fat_size;
    fi->rel_sect = f32->rel_sect;
    fi->num_sect = f32->num_sect;
    fi->drive_num = f32->drive_num;
   }
 else
   {
    fi->fat_size =  f16->fat_size;
    fi->rel_sect =  f16->rel_sect;
    fi->num_sect = (f16->snum_sect ? f16->snum_sect : f16->num_sect);
    fi->drive_num = f16->drive_num;
   }

 fi->clust_size = f16->clust_size;
 fi->fat_rel_sect = f16->res_sects;


 strncpy( fi->sysid, f16->sys_id, 8 );
 strncpy( fi->fs_id, fi->sys==32 ? f32->fs_id : f16->fs_id, 8 );
 strncpy( fi->label, fi->sys==32 ? f32->label : f16->label, 11 );

 fi->boot_fix = 0;
 if( fi->sys!=32 && memcmp(f16, BOOT_FAT_1X, 3)==0 &&
                    memcmp(f16->code, BOOT_FAT_1X+0x3E, 402)==0 )
   {
    int i;
    for( i=1 ; i<=3 ; i++ )
      if( memcmp(((char*)f16)+(512-48),BOOT_FAT_1X_NAMES[i-1],48)==0 )
         fi->boot_fix = i;
   }

 fi->fat_sect_read = 0;
 fi->num_data_sect = 0;
 fi->first_data_sect = fi->fat_rel_sect +
                    ( (fi->sys==32) ? (f32->fat_copies):(f16->num_fats) ) * fi->fat_size +
                    ( (fi->sys==32) ? (0)              :(f16->root_entr/16) );
 fi->min_num_sect = fi->first_data_sect + fi->clust_size;


 // if( fi->fat_size<0 ) fi->fat_size=0; // fat_size is unsigned 
 if( fi->fat_size<0 ) fi->fat_size=0;
 x=fi->fat_size;
 if( fi->sys==16 && x>256 ) /* Fat size cannot be more than 128k (256 sect) */
   x=256;
 if( fi->sys==12 && x>12 ) /* For FAT-12 the limit is 6144 bytes (12 sect) */ 
   x=12;

 fi->max_num_sect = fi->first_data_sect + 
                (x*512*8/fi->sys - 2) * fi->clust_size;

 /* if FAT is greater than 100M for FAT-32 and
              greater than 200k for FAT-16
    there must be garbage in the boot sector */

  if( (long)fi->fat_size > (fi->sys==32 ? 200000L : 400) ) /* Num sectors */
            fi->fat_sect_read = fi->fat_size;    /* Don't read fat tables */

}/* update_fat_info */



void read_fat_table(HardDiskInfo *di, Part *p, FatInfo *fi)
{
 int i, l, found;
 char tmp[512];
 unsigned short *fat16 = (unsigned short *)tmp;
 unsigned long  *fat32 = (unsigned long *)tmp;

 if( fi->sys==12 ) /* we dont deal with FAT12 yet */
   {
    fi->fat_sect_read = fi->fat_size;
    fi->num_data_sect = fi->num_sect;
    fi->min_num_sect  = fi->num_sect;
    return;
   }

 if( read_part(di, p, fi->fat_rel_sect + fi->fat_sect_read, tmp, 1)==-1 )
   {
    fi->fat_sect_read = fi->fat_size;
    fi->num_data_sect = -1;
    fi->min_num_sect  = -1;
    return;
   }

 if( fi->sys==16 )
   {
    for( found=0, l=0, i=((fi->fat_sect_read==0)?2:0) ; i<256 ; i++ )
     if( fat16[i]!=0 )
       {
        found=1;
        l=i;
        fi->num_data_sect += fi->clust_size;
       }
    if( found ) fi->min_num_sect = fi->first_data_sect +
                                  (fi->fat_sect_read*256+l-2)*fi->clust_size;
   }
 else /* must be fat32 */
   {
    for( found=0, l=0, i=((fi->fat_sect_read==0)?2:0) ; i<128 ; i++ )
     if( fat32[i]!=0 )
       {
        found=1;
        l=i;
        fi->num_data_sect += fi->clust_size;
       }
    if( found ) fi->min_num_sect = fi->first_data_sect +
                                  (fi->fat_sect_read*128+l-2)*fi->clust_size;
   }

 fi->fat_sect_read++;

}/* read_fat_table */

#if 0
void fix_fat16_boot_sector( HardDiskInfo *di, Part *p, struct boot_fat16 *f16 )
{
}

void fix_fat32_boot_sector( HardDiskInfo *di, Part *p, struct boot_fat32 *f32 )
{
}
#endif

void save_fat_boot_sect(HardDiskInfo *di, Part *p, FatInfo *fi)
{
 struct boot_fat16 *f16;
 struct boot_fat32 *f32;

 f16 = (struct boot_fat16 *)(p->part_boot_sect);
 f32 = (struct boot_fat32 *)(p->part_boot_sect);

 f16->num_sects = di->sect_per_track;
 f16->num_sides = di->num_heads;

 if( fi->sys == 32 )
   {
    f32->rel_sect = fi->rel_sect;
    f32->num_sect = fi->num_sect;
    f32->drive_num = fi->drive_num;
   }
 else
   {
    if( fi->boot_fix>0 )
      {
       memmove(f16,BOOT_FAT_1X,3);
       memmove(f16->code,BOOT_FAT_1X+0x3E,402);
       memmove(((char*)f16)+(512-48),BOOT_FAT_1X_NAMES[fi->boot_fix-1],48);
      }
    f16->rel_sect = fi->rel_sect;
    f16->snum_sect = 0;
    f16->num_sect = fi->num_sect;
    f16->drive_num = fi->drive_num;
   }

 write_boot_sect(di,p,0,1);
}/* save_fat_boot_sect */




#define BBT_SIZE 16000

#define F_NORM  0
#define F_QUICK 1
#define F_DESTR 2

#define ROOT_ENTR    (512)
#define ROOT_SIZE    (ROOT_ENTR/16)
#define MAX_CLUST12  (4084)    /* Maximum number of clusters in FAT12 system */
#define MAX_CLUST16  (65524)   /* Maximum number of clusters in FAT16 system */

/*   0x01, "DOS FAT-12"			*/
/*   0x04, "DOS FAT-16 (<=32Mb)"	*/
/*   0x06, "BIGDOS FAT-16 (>=32Mb)"	*/


void fat_format(HardDiskInfo *di, Part *p, FatInfo *fi)
{
 int i, x=0;
 Sector *bbt=0;
 
 i=dialog_box("Warning!!! Formatting will destroy all data on the partition!!!",
            " Format ", " Quick format without surface test ", " Cancel ", 0 );
 if( i==0 || i==3 ) return;

 if( i==1 )
   {
    bbt = (Sector*) malloc(BBT_SIZE*sizeof(Sector));
    if( bbt==0 )
      {
       show_error(ERROR_MALLOC);
       return;
      }
    x=verify_part(di,p,bbt,BBT_SIZE);
#ifndef DISK_SIM
    if( x==BBT_SIZE )
      {
       show_error("Too many bad sectors. Move partition somewhere else.");
       show_bad_blocks(di,p,bbt,x);
       free(bbt);
       return;
      }
#endif
    if( x==EV_CANCEL )
      {
       free(bbt);
       return;
      }
   }
 
 if( p->fs_type==0x0B00 || p->fs_type==0x0C00 ||
     p->fs_type==0x1B00 || p->fs_type==0x1C00 )  fat32_format(di,p,bbt,x,0);
                                            else fat16_format(di,p,bbt,x,0);

 if( bbt!=0 ) free(bbt);
}/* fat_format */


#if 0
    struct fat12
        {
         unsigned c0:12;
         unsigned c1:12;
        } *fat12 = (struct fat12*)fat;
    
    memset(fat,0,6144);

    fat12[0].c0=0xFF8;
    fat12[0].c1=0xFFF;

    next_bad = 0;
    base_sect = 1 + 2*fat_size + ROOT_SIZE;

    while( next_bad!=num_bad )
       {
        j=(bbt[next_bad++]-base_sect)/clust_size+2;
        
        if( j%2==0 ) fat12[j/2].c0=0xFF7;
                else fat12[j/2].c1=0xFF7;
       }

    for( k=0 ; k<2 ; k++ )
      for( i=0 ; i<fat_size ; i++ )
       if( disk_write_rel(p,wr_sect++,fat+i*256,1)==-1 )
         {
          progress("Error writing FAT");
          goto failed;
         }
   }/* fat12 */
#endif


void fat16_format(HardDiskInfo *di, Part *p, Sector *bbt, int bbt_size, char *label)
{
 int i;
 char tmp[30];
 char *data_pool;
 struct boot_fat16 *b;
 unsigned short int *fat;
 Sector clust_size, num_clust;
 
 if( bbt_size!=0 && bbt[0] < 1+2*256+ROOT_SIZE )
   {
    show_error("Beginning of the partition is unusable. Move it forward.");
    return;
   }

 if( p->num_sect < 1+2*256+ROOT_SIZE+8 ||
     p->num_sect > 1+2*256+ROOT_SIZE+128*MAX_CLUST16 )
   {
    show_error("FAT-16 must be between 300k and 4G. Use FAT-12 or FAT-32 instead.");
    return;
   }

 if( (data_pool=(char*)malloc(512+512*256))==0 )
   {
    show_error(ERROR_MALLOC);
    return;
   }

 b   = (struct boot_fat16*) data_pool;
 fat = (unsigned short*) (data_pool+512);

 memmove(b,BOOT_FAT_1X,SECT_SIZE);
 memmove(b->sys_id,"MSDOS5.0",8);
 memmove(b->fs_id,"FAT16   ",8);
 memmove(b->label,"NO NAME    ",11);
 if( label!=0 && label[0]!=0 )
   {
    for( i=0 ; i<11 && label[i]!=0 ; i++ )
       {
        b->label[i]=label[i];
       }
    while( i<11 ) { b->label[i]=' '; i++; }
   }
 b->sect_size=SECT_SIZE;
 b->res_sects=1;
 b->num_fats=2;
 b->root_entr=ROOT_ENTR;
 b->media_desc=0xF8;
 b->ext_signat=0x29;
 b->magic_num=MAGIC_NUM;

 b->fat_size = 256;

 for( clust_size=2 ; clust_size<128 ; clust_size*=2 )
    {
     num_clust = (p->num_sect - 1+2*256+ROOT_SIZE)/clust_size;
     if( num_clust < MAX_CLUST16 ) break;
    }
	
 b->clust_size = clust_size;
 
 b->drive_num=di->hd_num;
 b->num_sects=di->sect_per_track;
 b->num_sides=di->num_heads;

 b->rel_sect = p->rel_sect;
 b->snum_sect = 0;
 b->num_sect = p->num_sect;
 
 b->serial_num=((p->rel_sect<<16)+(p->num_sect*((long)b%451)) )+
               ((di->total_sects%12345L)^(di->total_sects*67891L))+
               ((di->hd_num*123L)^(di->num_heads%7));
 
// flush_caches();

 progress("^Writing boot sector ...");
 
 if( write_part(di,p,0,b,1)==-1 )	/*  Writing boot sector  */
   {
    show_error("Error writing boot sector.");
    free(data_pool);
    return;
   }
 
 progress("^Writing FAT tables ...");

 memset(fat,0,256*512);
 fat[0]=0xFFF8;
 fat[1]=0xFFFF;
 
 for( i=0 ; i<bbt_size ; i++ )
    {
     fat[ (bbt[i]-(1+2*256+ROOT_SIZE))/clust_size+2 ] = 0xFFF7;
    }

 progress("%   0%");
 if( write_part(di,p,  1,fat,256)==-1 || /* first FAT */
     progress("%  50%")==333333333L || /* always false */
     write_part(di,p,257,fat,256)==-1 )  /* second FAT */
   {
    show_error("Error writing FAT");
    free(data_pool);
    return;
   }
 progress("% 100%");

 memset(fat,0,ROOT_SIZE*512);
 if( label!=0 && label[0]!=0 )
   {
    // set vol label
    for( i=0 ; i<11 ; i++ )
       {
        ((char*)fat)[i]=b->label[i];
       }
    ((char*)fat)[0x0B]=0x28; // volume label+archive attributes
    ((char*)fat)[0x18]=0x21; // / Jan-01-2000
    ((char*)fat)[0x19]=0x28; // \ Jan-01-2000
   }
 progress("^Writing root directory ...");
 progress("%   0%");
 if( write_part(di,p,513,fat,ROOT_SIZE)==-1 )
   {
    show_error("Error writing root directory");
    free(data_pool);
    return;
   }
 progress("% 100%");

 return;
}/* format_fat */



void fat32_format(HardDiskInfo *di, Part *p, Sector *bbt, int bbt_size, char *label)
{
 char tmp[30];
 char *data_pool;
 struct boot_fat32 *b;
 unsigned long int *fat;
 int i, j, k, wr_sect, fat_size, next_bad;
 Sector l, num_clust, base_sect, base_clust;
 char label2[12];
 label2[0]=0;

 unsigned int clust_size = 8;

 if( p->num_sect < 33+2*512+8*MAX_CLUST16 )
   {
    show_error("FAT-32 must be at least 512M. Use FAT-16 instead.");
    return;
   }

 if( p->num_sect > 16777216 ) /* 8G */
             clust_size = 16; /* 8k */
 if( p->num_sect > 33554432 ) /* 16G */
             clust_size = 32; /* 16k */

 fat_size =  (p->num_sect)/(clust_size*128) + 1;
 num_clust = (p->num_sect-33-2*fat_size)/(clust_size);
     
 if( bbt_size!=0 && bbt[0] < 33+2*fat_size+2*clust_size )
   {
    show_error("Beginning of the partition is unusable. Move it forward.");
    return;
   }

 if( (data_pool=(char*)malloc(SECT_SIZE*3))==0 )
   {
    show_error(ERROR_MALLOC);
    return;
   }

 b   = (struct boot_fat32*)(data_pool);
 fat = (unsigned long int*)(data_pool);

 memset(b,0,SECT_SIZE*3);

 memmove(b->jmp,"\xEB\x58\x90",3);
 memmove(b->sys_id,"MSWIN4.1",8);
 b->sect_size=SECT_SIZE;
 b->res_sects=33;
 b->fat_copies=2;
 b->fs_sect_num=1;
 b->bs_bak_sect=6;
 b->media_desc=0xF8;
 b->ext_signat=0x29;
 memmove(b->label,"NO NAME    ",11);
 if( label!=0 && label[0]!=0 )
   {
    for( i=0 ; i<11 && label[i]!=0 ; i++ )
       {
        label2[i]=b->label[i]=label[i];
       }
    while( i<11 ) { label2[i]=b->label[i]=' '; i++; }
   }
 memmove(b->fs_id,"FAT32   ",8);
 memmove(b->xcode, EMP_IPL, EMP_IPL_SIZE );
 strncpy(b->xcode + EMP_IPL_SIZE, MESG_NON_SYSTEM, sizeof(b->xcode)-EMP_IPL_SIZE);

 b->ext_sign2 = 0x41615252L;
 b->ext_sign3 = 0x61417272L;

 b->magic_num = 0xAA550000L;
 b->magic_num2= 0xAA550000L;
 b->magic_num3= 0xAA550000L;

 b->fat_size=fat_size;
 b->clust_size=clust_size;

 b->root_clust=2;
 b->next_free=3;
 b->free_clust=num_clust-1;

 b->drive_num=di->hd_num;
 b->track_size=di->sect_per_track;
 b->num_sides=di->num_heads;

 b->rel_sect = p->rel_sect;
 b->num_sect = p->num_sect;
 
 b->serial_num=((p->rel_sect<<16)+(p->num_sect*((long)b%451)) )+
               ((di->total_sects%12345L)^(di->total_sects*67891L))+
               ((di->hd_num*123L)^(di->num_heads%7));
 
// flush_caches();

 progress("^Writing boot sector ...");
 
 if( write_part(di,p,0,b,3)==-1 || /*  Writing boot sector */
     write_part(di,p,6,b,3)==-1 )  /*  Writing boot sector (backup) */
   {
    show_error("Error writing boot sector.");
    free(data_pool);
    return;
   }
 
 progress("^Writing FAT tables ...");
 wr_sect = 33;
 progress("%   0%");
#ifndef DISK_SIM
 if( bbt_size==0 )
   {
    for( k=0 ; k<2 ; k++ )		 /* Writing two copies of FAT */
       {
        Sector base_sect=33+k*fat_size;
        memset(fat,0,512);
        fat[0]=0x0FFFFFF8;
        fat[1]=0x0FFFFFFF;
        fat[2]=0x0FFFFFFF; /* root directory */

        if( write_part(di,p,base_sect,fat,1)==-1 )
          {
           show_error("Error writing FAT");
           free(data_pool);
           return;
          }
        i=1;
        while( i<fat_size )
           {
            long n=min(fat_size-i,1024);
            if( hard_disk_clear( di, p->rel_sect+base_sect+i, n)==-1 )
              {
               show_error("Error writing to hard disk");
               break;
              }
            i+=n;
            j = (i+k*fat_size)*100/(2*fat_size);
            sprintf(tmp,"%% %3d%%",j);
            progress(tmp);
           }
       }
   }
 else
#endif
 for( k=0 ; k<2 ; k++ )		 /* Writing two copies of FAT */
    {
     next_bad = 0;
     base_sect = 33 + 2*fat_size - 2*clust_size; /* first 2 entries reserved */

     for( i=0 ; i<fat_size ; i++ )
        {
         memset(fat,0,512);
         if( i==0 )
           {
            fat[0]=0x0FFFFFF8;
            fat[1]=0x0FFFFFFF;
            fat[2]=0x0FFFFFFF; /* root directory */
           }

         while(next_bad!=bbt_size && bbt[next_bad]<base_sect+clust_size*128)
           fat[ (bbt[next_bad++]-base_sect)/clust_size ]=0x0FFFFFF7;
         base_sect+=clust_size*128;

         if( write_part(di,p,wr_sect++,fat,1)==-1 )
           {
            show_error("Error writing FAT");
            free(data_pool);
            return;
           }
         j = (i+k*fat_size)*100/(2*fat_size);
         sprintf(tmp,"%% %3d%%",j);
         progress(tmp);
        }
    }
 progress("% 100%");


 wr_sect=33+2*fat_size;
 progress("^Writing root directory ...");
 progress("%   0%");
 for( i=0 ; i<(int)clust_size ; i++ )
    {
     if( i==0 || i==1 )
       {
        memset(fat,0,512);
       }
     if( i==0 && label!=0 && label[0]!=0 )
       {
        // set vol label
        for( j=0 ; j<11 ; j++ )
           {
            ((char*)fat)[j]=label2[j];
           }
        ((char*)fat)[0x0B]=0x28; // volume label+archive attributes
        ((char*)fat)[0x18]=0x21; // / Jan-01-2000
        ((char*)fat)[0x19]=0x28; // \ Jan-01-2000
       }
     if( write_part(di,p,wr_sect++,fat,1)==-1 )
       {
        show_error("Error writing root directory");
        free(data_pool);
        return;
       }
    }
 progress("% 100%");

 free(data_pool);
 return;
}/* format_fat32 */



#if 0
int install_to_floppy(void)
{
 int i, j;
 char tmp[512];
 BmBootSect bm;
 Mbr *mbr = (Mbr*)tmp;

 memmove(&bm,BM_BOOT,512);
 bm.rel_sect = 0;
 bm.num_sect = 1000;
 bm.loader_rel_sect = 1;
 bm.loader_num_sect = 127;
 if( floppy_disk_write(0,0,&bm,1)==-1 ) return -1;
 if( floppy_disk_write(0,1,own_code,127)==-1 ) return -1;

 return 0;
}
#endif

#ifndef DISK_SIM

struct exe_header
    {
     char sign[2];
     unsigned short n_bytes;  /* in the last 512-byte page */
     unsigned short n_pages;  /* number of 512-byte pages */
     unsigned short n_reloc_entries;
     unsigned short header_size;
     unsigned short min_add_par;
     unsigned short max_add_par;
     unsigned short init_ss;
     unsigned short init_sp;
     unsigned short check_sum;
     unsigned short init_ip;
     unsigned short init_cs;
     unsigned short reocation_table_offset_in_header;
     unsigned short overlay_number;
    } part_exe_header =
  { {'M','Z'}, 512, 128, // 128 sectors, 512 bytes in the last one 
    0, 0x20,	// reloc entries, header size in paragraphs
    0x0040, 0xFFFF, // min / max paragraphs
    0x0FE0, 0x0400,	// initial SS, SP
    0, // checksum
    0, 0, // initial CS:IP
    0x1C, // relocation table offset
    0 }; // ovelay_number (0=main)
    

int install_to_floppy(void)
{
 int i, j, copy_flag=1;
 char tmp[512];
 char tmp2[512];
 struct boot_fat16 *bs=(struct boot_fat16*)tmp;
 struct boot_fat16 *bs2=(struct boot_fat16*)tmp2;

 int name_num=floppy_os_dialog_box();
 int ctrl_flag = dialog_box("Boot Partition Manager from floppy only when",
    " Ctrl key is pressed "," None of the shift keys is pressed ", 0 );
 
 memset(tmp,0,512);
 memmove(tmp,&part_exe_header,sizeof(part_exe_header));

 if( get_detected_os()!=0 ) 
   {
    progress("^Writing PART.EXE to A: ... ");
    progress("%   0%");
    int fd=open(PM_FILE, O_RDONLY);
    if( fd!=-1 )
      {
       close(fd);
       i=dialog_box("File " PM_FILE " already exists.",
                    " Overwrite ", " Do not overwrite ", 0);
       if( i==1 ) fd=-1;
       else if( i==2 ) copy_flag=0;
      }
    if( fd==-1 )
      {
       fd=open(PM_FILE, O_WRONLY | O_CREAT | O_TRUNC);
       if( fd==-1 || write(fd,tmp,512)==-1 )
         {
          show_error("Error writing to " PM_FILE " !!!");
          return -1;
         }
       progress("%   5%");
       
       for( i=0 ; i<127 ; i++ )
          {
           sprintf(tmp,"%%  %2d%%",i*2/3+5);
           progress(tmp);
           if( write(fd,((char*)own_code)+i*512,(unsigned)512)<512 )
             {
              show_error("Error writing to " PM_FILE " !!!");
              return -1;
             }
         }
       if( i==127 ) copy_flag=0;



       if( fd!=-1 ) stclose(fd, PM_DATE, PM_TIME);
       progress("%  90%");
      }
   }

 progress("^Updating boot sector...");
 if( floppy_disk_read(0,0,bs,1)==-1 ) return -1;
 progress("%  95%");

 memmove(bs2,BOOT_FAT_1X,512);
 memmove(tmp2+3,tmp+3,(bs->ext_signat==0x29)?(0x3E-3):(0x26-3) );
 memmove(tmp2+(512-48),BOOT_FAT_1X_NAMES[name_num],48);
 if( ctrl_flag==1 ) tmp2[512-48+14]=4;
 if( floppy_disk_write(0,0,tmp2,1)==-1 )
   {
    show_error("Error writing boot sector.");
    return -1;
   }
 progress("% 100%");

 show_error( copy_flag ? "Please copy PART.EXE to the floppy." :
             "You may now use A: disk to boot Partition Manager.");

 return 0;
}/* write_disk_info */

#endif

#if 0
void print_fat_details( FatInfo *fi )
{
 printf("      Volume Label: %s\r\n", fi->label );
 printf("         System id: %s\r\n", fi->sysid );
 printf("       File system: %s\r\n", fi->fs_id );
 printf("      Cluster size: %,dk (%ds)\r\n", fi->clust_size/2, fi->clust_size );
 printf("          FAT size: %,dk\r\n", fi->fat_size/2 );
 printf("      Drive number: %d   Exp: %d\r\n", fi->drive_num, 
                                                fi->exp_drive_num );
 printf("   Starting sector: %,u\r\n", fi->rel_sect );
 printf("    Expected value: %,u\r\n", fi->exp_rel_sect );
 printf(" Number of sectors: %,u\r\n", fi->num_sect );
 printf("    Expected value: %,u\r\n", fi->exp_num_sect );
}/* print_fat_details */
#endif