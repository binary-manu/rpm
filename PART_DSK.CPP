#define PART_DSK_C
#define PART_DSK
#include "part.h"


DiskInfo *get_disk_info(int hd)
{
 static DiskInfo *rdi[16];         /* Global Variable!!! */

 if( rdi[hd-0x80]==0 )
     rdi[hd-0x80]=new_disk_info(hd);
 return rdi[hd-0x80];
}


DiskInfo *new_disk_info(int hd)
{
 DiskInfo *di;
 
 if( (di=(DiskInfo*)malloc(sizeof(DiskInfo)))==0 ) return 0;
 
 memset(di,0,sizeof(DiskInfo));
 
 if( (di->num_disks=hard_disk_get_info(hd,di))==-1 ||
      di->num_cyls==0 || di->num_heads==0 ||
      di->sect_per_track==0 || di->sect_per_cyl==0 ||
      di->total_sects==0 )
   {
#if 0
    if( debug ) 
      {
       fprintf(stderr,"DEBUG: hard_disk_get_info() returned:\r\n"
       "  hd_num=0x%02X  flags=0x%02X\r\n"
       "  num_cyls=%,d\r\n  num_heads=%,d\r\n  sect_per_track=%,d\r\n"
       "  sect_per_cyl=%,d\r\n  total_sects=%,d  (%,dMb)\r\n",
       di->hd_num, di->flags, di->num_cyls, di->num_heads, di->sect_per_track,
       di->sect_per_cyl, di->total_sects, di->total_sects/2048 );
      }
#endif
    free(di);
    return 0;
   }

 if( read_disk_info(di)==-1 )
   {
    free(di);
    return 0;
   }

 return di; 
}/* new_disk_info */


int delete_disk_info( DiskInfo *di )
{
 int i;
 for( i=0 ; i<P_ROWS ; i++ )
   free_part_info(&di->part[i]);
 free(di);
 return 0;
}

#ifdef MT4P
int read_disk_info( DiskInfo *di )
{
 Mbr *mbr;
 Part *p, *part, ptmp;
 int i, j, next, n, top, num, possible_problem = 0;
 Sector ext_base, log_base;

 part=di->part;

 for( i=0 ; i<P_ROWS ; i++ ) free_part_info(&part[i]);

 memset(part,0,P_ROWS*sizeof(Part));

 p=&part[0];
 ext_base=0;
 log_base=0;

 p->fs_type=FS_MBR;
 find_fs_num(p);
 p->rel_sect=0;
 p->num_sect=1;
 recalculate_partition(di,p,MODE_LBA);

 di->num_part=1;
 n=1;      /* n - number of records that have been read so far         */
 next=0;   /* index in part[] of (E)MBR that we are going to read next */

 if( read_boot_sect(di,p,1)==-1 ) /* Read MBR */
     return -1;

 mbr = (Mbr *) p->part_boot_sect;

 if( mbr->bm_magic_num==BM_MAGIC_NUM )
   {
    di->bm_options = mbr->bm_options;   // Vtoc also has options !!!
    di->act_choice = mbr->bm_selected;  // See import_vtoc();
    di->ipl_timeout = mbr->bm_timeout;
    di->ipl_default = mbr->bm_default;
    if( mbr->bm_active!=0 ) di->bm_type=1; /* boot manager is active */
                       else di->bm_type=0; /* IPL handles booting */
    if( mbr->vt_rel_sect!=0 )
      {
       if( hard_disk_read(di, mbr->vt_rel_sect, &di->vtoc, VTOC_NUM_SECT)!=-1 )
         {
          import_vtoc(di,&di->vtoc);
          n=di->num_part;
         }
      }
   }
 else
   {
    di->ipl_timeout = 6;                      /* Default settings */
    di->bm_options = BM_OPT_VIRUS_CHECK;
    strncpy((char*)di->vtoc.bm_title,"Boot Menu",39);
   }

 di->cur_ipl = find_ipl_type(mbr);
 di->new_ipl = di->cur_ipl;
 
//if( n>1 ) // uncomment to ignore MBR, when VTOC have been read in.

 while(1)   /* Now unpacking partition records in MBR (EMBR) */
    {
     for( i=0 ; i<4 ; i++ )
      if( mbr->part_rec[i].fs_type!=0 )   /* not unused - so we import it */
        {
         memset(&ptmp,0,sizeof(ptmp));
         unpack_part_rec( &mbr->part_rec[i], &ptmp );
         ptmp.mbr_row = i+1;
         if( EXTENDED(p) )
          ptmp.flags |= P_LOGICAL;
         import_partition(di, &ptmp, ext_base, log_base );

         for( j=0 ; j<n ; j++ ) /* Let see if we already have that partition */
          if( part[j].rel_sect==ptmp.rel_sect &&   /* Same location and size */
              part[j].num_sect==ptmp.num_sect )
            {
             if( EXTENDED(&part[j]) && EXTENDED(&ptmp) )  /* Prev listed ext */
               {
                if( next==0 ) next=j;
                break;
               }

             if( (part[j].fs_type&0xFF00)==(ptmp.fs_type&0xFF00) )
               {	 /* Same file system type is already in the table */
                part[j].tag=ptmp.tag;
                part[j].mbr_row=reset_mbr_row(part, &part[j], ptmp.mbr_row);
		if (part[j].mbr_row == 5) {
			dialog_box("Record updated; but MBR row is not retained due to an active partition having that row", 0);
			part[j].mbr_row = 0;
		}
                break;
               }
             
            if( ptmp.fs_type!=FS_HIDDEN )
              {                       /* Need to update file system type */
               part[j].fs_type = ptmp.fs_type;
               find_fs_num( &part[j] );
               break;
              }
            possible_problem++;
            break;

            }/* if same location */

         if( j==n )
	   if (n < P_ROWS )          /*
                                      * No duplicates found & not beyond
                                      * limit - insert record into the table
                                      */
           {
            if (can_insert(di, &ptmp)) {
            	if( EXTENDED(&ptmp) && next==0 ) next=n;
            	part[n]=ptmp;
		part[n].mbr_row = reset_mbr_row(part, &part[n], ptmp.mbr_row); 
		if (part[n].mbr_row == 5) {
			dialog_box("Record added; but MBR row is not retained due to an active partition having that row", 0);
			part[n].mbr_row = 0;
		}
		recalculate_partition(di, &part[n], MODE_LBA);
            	n++;
		di->num_part = n;
	    } else
		possible_problem++;
	    continue;
           } else {		/* n == P_ROWS */
		possible_problem++;
		break;
	   }
        }/* for/if import unused */

     if( next==0 || possible_problem || n == P_ROWS ) break;
     log_base=part[next].rel_sect;
     if( ext_base==0 ) ext_base=log_base;

     p = &part[next];

     if( read_boot_sect(di,p,1)==-1 )  /* Read Extended MBR */
         break;

     mbr = (Mbr *) p->part_boot_sect;
     
     next=0;
    }


 di->num_part=n;
 
 //make a guess on the real partition types
 
 i=0;
 di->changed = 0;
 sort_disk_info(di,&i);
 
 if (possible_problem)
	if (n == P_ROWS)
		dialog_box("Maximum limit of 31 partitions reached. Partition not imported", 0);
	else
		dialog_box("MBR/EMBR partition overlaps an existing partition. Partition not imported", 0);

 return 0;
}/* read_disk_info */
#else
int read_disk_info( DiskInfo *di )
{
 Mbr *mbr;
 Part *p, *part, ptmp;
 int i, j, next, n, top, num;
 Sector ext_base, log_base;

 part=di->part;

 for( i=0 ; i<P_ROWS ; i++ ) free_part_info(&part[i]);

 memset(part,0,P_ROWS*sizeof(Part));

 p=&part[0];
 ext_base=0;
 log_base=0;

 p->fs_type=FS_MBR;
 find_fs_num(p);
 p->rel_sect=0;
 p->num_sect=1;
 recalculate_partition(di,p,MODE_LBA);

 di->num_part=1;
 n=1;      /* n - number of records that have been read so far         */
 next=0;   /* index in part[] of (E)MBR that we are going to read next */

 if( read_boot_sect(di,p,1)==-1 ) /* Read MBR */
     return -1;

 mbr = (Mbr *) p->part_boot_sect;

 if( mbr->bm_magic_num==BM_MAGIC_NUM )
   {
    di->bm_options = mbr->bm_options;   // Vtoc also has options !!!
    di->act_choice = mbr->bm_selected;  // See import_vtoc();
    di->ipl_timeout = mbr->bm_timeout;
    di->ipl_default = mbr->bm_default;
    if( mbr->bm_active!=0 ) di->bm_type=1; /* boot manager is active */
                       else di->bm_type=0; /* IPL handles booting */
    if( mbr->vt_rel_sect!=0 )
      {
       if( hard_disk_read(di, mbr->vt_rel_sect, &di->vtoc, VTOC_NUM_SECT)!=-1 )
         {
          import_vtoc(di,&di->vtoc);
          //n=di->num_part;
         }
      }
   }
 else
   {
    di->ipl_timeout = 6;                      /* Default settings */
    di->bm_options = BM_OPT_VIRUS_CHECK;
    strncpy((char*)di->vtoc.bm_title,"Boot Menu",39);
   }

 di->cur_ipl = find_ipl_type(mbr);
 di->new_ipl = di->cur_ipl;

//if( n>1 ) // uncomment to ignore MBR, when VTOC have been read in.

 while(1)   /* Now unpacking partition records in MBR (EMBR) */
    {
     for( i=0 ; i<4 ; i++ )
      if( mbr->part_rec[i].fs_type!=0 )   /* not unused - so we import it */
        {
         memset(&ptmp,0,sizeof(ptmp));
         unpack_part_rec( &mbr->part_rec[i], &ptmp );
         ptmp.mbr_row = i+1;
         if( EXTENDED(p) )
          ptmp.flags |= P_LOGICAL;
         import_partition(di, &ptmp, ext_base, log_base );

         for( j=0 ; j<n ; j++ ) /* Let see if we already have that partition */
          if( part[j].rel_sect==ptmp.rel_sect &&   /* Same location and size */
              part[j].num_sect==ptmp.num_sect )
            {
             if( EXTENDED(&part[j]) && EXTENDED(&ptmp) )  /* Prev listed ext */
               {
                if( next==0 ) next=j;
                break;
               }

             if( (part[j].fs_type&0xFF00)==(ptmp.fs_type&0xFF00) )
               {         /* Same file system type is already in the table */
                part[j].tag=ptmp.tag;
                part[j].mbr_row=ptmp.mbr_row;
                break;
               }

             if( j<di->num_part )                 /* First record is in VTOC */
               {
                if( ptmp.fs_type!=FS_HIDDEN )
                  {                       /* Need to update file system type */
                   part[j].fs_type = ptmp.fs_type;
                   find_fs_num( &part[j] );
                  }
                break;
               }

            }/* if same location */

         if( j==n )   /* No duplicates found - insert record into the table */
           {
            if( EXTENDED(&ptmp) && next==0 ) next=n;
            part[n]=ptmp;
            n++;
           }

         if( n==P_ROWS ) break;
        }/* for/if import unused */

     if( n==P_ROWS ) break;
     if( next==0 ) break;
     log_base=part[next].rel_sect;
     if( ext_base==0 ) ext_base=log_base;

     p = &part[next];

     if( read_boot_sect(di,p,1)==-1 )  /* Read Extended MBR */
         break;

     mbr = (Mbr *) p->part_boot_sect;

     next=0;
    }

 di->num_part=n;

 // make a guess on the real partition types

 i=0;
 di->changed = 0;
 sort_disk_info(di,&i);

 return 0;
}/* read_disk_info */
#endif /* MT4P */


int write_disk_info( DiskInfo *di )
{
 int i, j, curr, next;
 Sector ext_base, log_base;
 Part part2[4], *part=di->part;
 char tmp[512];
 BmBootSect bm;
 Mbr *mbr = (Mbr*)tmp;

 memmove(mbr,part[0].part_boot_sect,0x1BE);  /* Copy current IPL code */

 if( di->new_ipl==IPL_TYPE_STD )	 /* Do we have to replace it? */
   {
    di->cur_ipl=IPL_TYPE_STD;
    memmove(mbr,STD_IPL,STD_IPL_SIZE);
   }
 else if( di->new_ipl==IPL_TYPE_BM )
   {
    di->cur_ipl=IPL_TYPE_BM;
    memset(mbr,0,512);
    memmove(mbr,BM_IPL,BM_IPL_SIZE);
    for( i=0 ; i < di->num_part ; i++ )
     if( VALID(&part[i]) && part[i].fs_type==FS_BM )
       {
        if( di->bm_type!=0 ) mbr->bm_active=0x80;
        mbr->bm_chs[0] = part[i].start_head;
        if( part[i].start_cyl<1023 )
          {
           mbr->bm_chs[1]=((part[i].start_cyl>>2)&0xC0) + part[i].start_sect;
           mbr->bm_chs[2]=((part[i].start_cyl&0xFF));
          }
        else
          {
           mbr->bm_chs[1]=0xC1;
           mbr->bm_chs[2]=0xFF;
          }
        mbr->bm_rel_sect = part[i].rel_sect;
        memmove(&bm,BM_BOOT,512);
        bm.rel_sect = part[i].rel_sect;
        bm.num_sect = part[i].num_sect;
        bm.loader_rel_sect = 1;
        bm.loader_num_sect = 127;
        write_part(di,&part[i],0,&bm,1);
        write_part(di,&part[i],1,own_code,127);
        //if( di->use_vtoc )
          {
           export_vtoc(di,&di->vtoc);
           mbr->vt_rel_sect=part[i].rel_sect+128;
           write_part(di,&part[i],128,&di->vtoc,VTOC_NUM_SECT);
          }
        break;
       }
    mbr->bm_options=di->bm_options;
    mbr->bm_timeout=di->ipl_timeout;
    mbr->bm_default=di->ipl_default;
    mbr->bm_selected=di->bm_selected;
    mbr->bm_magic_num=BM_MAGIC_NUM;
    mbr->reserved2[0]=(di->quiet?13:10);
   }

 curr=0;       /* current MBR or EMBR that is being saved */
 next=0;       /* EMBR that will be processed next        */
 log_base=0;
 ext_base=0;

 prepare_mbr_records(part,part2);

 for( i=0 ; i<P_ROWS ; i++ )
  if( EXTENDED(&part[i]) )
    {
     next=i;
     break;
    }

 while(1)
    {
     for( i=0 ; i<4 ; i++ )
        {
         export_partition(di,&part2[i],ext_base,log_base);
         pack_part_rec(&part2[i],&mbr->part_rec[i]);
        }

     mbr->magic_num=MAGIC_NUM;

     if( write_boot_sect(di,&part[curr],mbr,1)==-1 )
       {
        return -1;
       }

     if( next==0 ) break;
                                             /* preparing logical partitions */
     curr=next;
    
     log_base = part[curr].rel_sect;

     memset(mbr,0,512);
     memset(part2,0,4*sizeof(Part)); 

     if( !LOGICAL(&part[curr]) )
       {
        ext_base = log_base;
        memmove( mbr, EMP_IPL, EMP_IPL_SIZE );
        strcpy( ((char*)mbr) + EMP_IPL_SIZE, EXT_NOT_BOOTABLE );
       }
     
     j=0;
     for( next=curr+1 ; next<P_ROWS ; next++ )
      if( !UNUSED(&part[next]) && (LOGICAL(&part[next]) ||
                                  EXTENDED(&part[next]) ) )
        {
         if( j<3 || j==3 && EXTENDED(&part[next]) )
           {
            part2[j]=part[next];
            j++;
           }
         if( EXTENDED(&part[next]) ) break;
        }
 
     if( next==P_ROWS ) /* no more extended partitions */
       next=0;
    }/* while */

 di->changed = 0;
 
 return 0;
}/* write_disk_info */



int read_boot_sect( HardDiskInfo *di, Part *p, unsigned n )
{
 if(  p->rel_sect == p->rel_sect_loaded )
   {
    if( BTS_LOADED(p) ) return 0;
    if( p->flags & P_ERR_CANT_READ ) return -1;
   }
 
 p->flags &= ~(P_ERR_CANT_READ + P_BTS_LOADED); /* clearing flags */
 
 if( p->part_boot_sect==0 )
   {
    if( (p->part_boot_sect=malloc(512*n))==0 )
      {
       p->flags |= P_ERR_CANT_READ;
       return -1;
      }
   }

 p->rel_sect_loaded = p->rel_sect;

 if( hard_disk_read(di,p->rel_sect,p->part_boot_sect,n)==-1 )
   {
    p->flags |= P_ERR_CANT_READ;
    return -1;
   }

 p->flags |= P_BTS_LOADED;
 return 0;
}/* read_boot_sect */



int write_boot_sect( HardDiskInfo *di, Part *p, void *buf, unsigned n)
{
 if( buf==0 ) buf = p->part_boot_sect;
 if( buf==0 ) return -1;
 if( p->num_sect<n ) return -1;

 if( buf != p->part_boot_sect &&
     BTS_LOADED(p) &&
     memcmp(buf, p->part_boot_sect, n*512)==0 )
   {
    return 0;
   }
     
 if( hard_disk_write(di,p->rel_sect,buf,n)==-1 )
   {
    return -1;
   }

 if( p->part_boot_sect!=0 && p->part_boot_sect!=buf )
   {
    memmove(p->part_boot_sect,buf,n*512);
    p->rel_sect_loaded = p->rel_sect;
    p->flags |= P_BTS_LOADED;
    p->flags &= ~P_ERR_CANT_READ;
   }

 return 0;
}/* write_boot_sect */


int read_part( HardDiskInfo *di, Part *p, Sector rel, void *buf, int num_sect)
{
 return hard_disk_read(di, p->rel_sect + rel, buf, num_sect);
}


int write_part( HardDiskInfo *di, Part *p, Sector rel, void *buf, int num_sect)
{
 return hard_disk_write(di, p->rel_sect + rel, buf, num_sect);
}


int verify_part( HardDiskInfo *di, Part *p, Sector *bbt, unsigned  bbt_size)
{
#ifdef DISK_SIM
 return 0;
#else
 char tmp[30];
 Sector base_sect, x, num_sect, num_bad, num_done;

 num_bad = 0;
 base_sect = 0;

 // Win95 prevents writing/verifying its drives to protect long file names
 // disk_lock(hd); it seems that we don't have to lock drives from 32-bit
 // DPMI application
 
 progress(MESG_VERIFYING);
 progress("%   0%");
 
 if( p->num_sect==0 ) return EV_OK;
 
 while( base_sect < p->num_sect )
    {
     num_sect = min( p->num_sect-base_sect, di->sect_per_cyl );

     num_done = hard_disk_verify(di, p->rel_sect+base_sect, num_sect);

     base_sect += num_done;
     if( base_sect<40000000 )
        x = base_sect*100/p->num_sect;
     else
        x = base_sect/(p->num_sect/100);

     if( num_done!=num_sect )
       {
        if( num_bad==bbt_size ) /* BBT is full */
          {
           break;
          }
        bbt[num_bad] = base_sect;
        num_bad++;
        base_sect++; /* skipping bad sector */
        sprintf(tmp,"%% %3d%% Bad sector:%3d", x, num_bad );
       }
     else
       {
        sprintf(tmp,"%% %3d%% Cylinder:%,6d", x,
                           (p->rel_sect+base_sect)/di->sect_per_cyl );
       }

     if( progress(tmp)==EV_CANCEL )
       {
        //disk_unlock(hd);
        return EV_CANCEL;
       }
    }
    
 //disk_unlock(hd);
 return num_bad;
#endif
}/* verify_part */




int copy_disk_area( HardDiskInfo *src, Sector src_start, 
                    HardDiskInfo *dst, Sector dst_start, Sector total_count )
{
 char tmp[30], cycle;
 char *buf;
 Sector total_done, buf_total, buf_done, n, buf_sect=65536;
 
 if( src!=dst ) buf_sect=1024;

 while( (buf=(char*)malloc(buf_sect*512))==0 )
    {
     if( buf_sect>2048 )
         buf_sect-=1024;
     else
         buf_sect/=2;
     if( buf_sect==0 ) return -1;
    }

 
 progress(MESG_COPYING);
 progress("%   0%");

 total_done=0;
 buf_total=0;
 cycle=2;
 while(1)
    {
     if( cycle==2 ) // set buf_total
       {
        total_done += buf_total;
        if( total_done==total_count ) break;
        buf_total = min( buf_sect, total_count-total_done);
        buf_done = 0;
        cycle=0;
       }

     n = min( 1024, buf_total-buf_done );

     if( cycle==0 ) // read
       {
        if( hard_disk_read(src,src_start,buf+buf_done*512,n)==-1 ) { free(buf); return -1; }
        src_start+=n;
       }
     else // write
       {
        if( hard_disk_write(dst,dst_start,buf+buf_done*512,n)==-1 ) { free(buf); return -1; }
        dst_start+=n;
       }

     buf_done+=n;
     Sector x = (total_done+buf_done/2);
     if( cycle==1 ) x+=buf_total/2;
     if( x<40000000 ) x=x*100/total_count;
                 else x=x/(total_count/100);
     if( cycle==1 && total_done+buf_done==total_count ) x=100;
     sprintf(tmp,"%% %3d%% %s%,9d ", x, cycle==0?"Read: ":"Write:", (total_done+buf_done)/2 );
     if( progress(tmp)==EV_CANCEL )
       {
        if( dialog_box("Do you really want to cancel copying?",
            " Yes. Abort it ", " No. Continue copying ", 0 ) != 1 ) continue;
        free(buf);
        return EV_CANCEL;
       }
     if( buf_done==buf_total )
       {
        buf_done=0;
        cycle++;
       }
    }

 free(buf);
 return 0;
}


int copy_disk( HardDiskInfo *src, HardDiskInfo *dst )
{
 return copy_disk_area( src, 0,
                        dst, 0,
                        min(src->total_sects,dst->total_sects) );
}

int copy_part( HardDiskInfo *src, Part *src_part,
               HardDiskInfo *dst, Part *dst_part )
{
 return copy_disk_area( src, src_part->rel_sect,
                        dst, dst_part->rel_sect,
                        min(src_part->num_sect,dst_part->num_sect) );
}



void clear_partition( HardDiskInfo *di, Part *p )
{
 char tmp[30];
 Sector base_sect, x, num_sect;

 base_sect = 0;

 progress(MESG_CLEANING);
 progress("%   0%");
 
 if( p->num_sect==0 ) return;
 
 while( base_sect < p->num_sect )
    {
     num_sect = min( p->num_sect-base_sect, 1024 );

     if( hard_disk_clear( di, p->rel_sect+base_sect, num_sect)==-1 )
       {
        show_error("Error writing to hard disk");
        break;
       }

     base_sect += num_sect;
     if( base_sect<40000000 )
        x = base_sect*100/p->num_sect;
     else
        x = base_sect/(p->num_sect/100);

     sprintf(tmp,"%% %3d%%    %,12d", x, base_sect/2 );

     if( progress(tmp)==EV_CANCEL )
       {
        break;
       }
    }
}

#ifdef MT4P
int write_mbr( DiskInfo *di )
{
 int i;
 Sector ext_base, log_base;
 Part part2[4], *part=di->part;
 char tmp[512];
 Mbr *mbr = (Mbr*)tmp;

 memset(mbr,0,512);
 memmove(mbr,BM_IPL,BM_IPL_SIZE);
 for( i=0 ; i < di->num_part ; i++ )
  if( VALID(&part[i]) && part[i].fs_type==FS_BM )
    {
     mbr->bm_active=0x80;
     mbr->bm_chs[0] = part[i].start_head;
     if( part[i].start_cyl<1023 )
       {
        mbr->bm_chs[1]=((part[i].start_cyl>>2)&0xC0) + part[i].start_sect;
        mbr->bm_chs[2]=((part[i].start_cyl&0xFF));
       }
     else
       {
        mbr->bm_chs[1]=0xC1;
        mbr->bm_chs[2]=0xFF;
       }
     mbr->bm_rel_sect = part[i].rel_sect;
     mbr->vt_rel_sect=part[i].rel_sect+128;
     break;
    }
 mbr->bm_options=di->bm_options;
 mbr->bm_timeout=di->ipl_timeout;
 mbr->bm_default=di->ipl_default;
 mbr->bm_selected=di->bm_selected;
 mbr->bm_magic_num=BM_MAGIC_NUM;
 mbr->reserved2[0]=(di->quiet?13:10);

 log_base=0;
 ext_base=0;

 prepare_mbr_records(part,part2);

 for( i=0 ; i<4 ; i++ )
    {
     export_partition(di,&part2[i],ext_base,log_base);
     pack_part_rec(&part2[i],&mbr->part_rec[i]);
    }

 mbr->magic_num=MAGIC_NUM;

 if( write_boot_sect(di,&part[0],mbr,1)==-1 )
   {
    return -1;
   }

 di->changed = 0;
 
 return 0;
} /* write_mbr */
#endif /* MT4P */
