#include "part.h"

void import_vtoc(DiskInfo *di, Vtoc *vt)
{
 int i, j, n;

 if( vt->vt_magic_num != VT_MAGIC_NUM ) return; 
 
 // Check version, stamp, checksum here
 
 di->vt_options  =  vt->vt_options;
 di->bm_options  =  vt->bm_options;

 /* #if 0   - commented while on MT4P */
 memmove(di->bm_title,di->vtoc.bm_title,40);

 for( i=0, n=1 ; i<vt->part_rec_count ; i++ )
  if( vt->part_rec[i].fs_type!=0 )
    {
     //memset(&di->part[n],0,sizeof(Part));
     unpack_vtoc_part_rec(&vt->part_rec[i],&di->part[n]);
     recalculate_partition(di,&di->part[n],MODE_LBA);

     n++;
     if( n==P_ROWS ) break; /* MT4P - "break;" was "do something" before */
    }
 di->num_part=n;

 for( i=0, n=0 ; i<vt->menu_rec_count ; i++ )
  if( vt->menu_rec[i].menu_name[0]!=0 )
    {
     memmove(&di->menu[n].menu_name,&vt->menu_rec[i].menu_name,sizeof(BootMenuRec));
     n++;
     if( n==M_ROWS ) break; /* MT4P - "break;" was "do something" before */
    }
  di->num_menus=n;
/* #endif - commented while on MT4P */
 
  // BM specific
  
  di->gui_x   = vt->gui_x;
  di->gui_y   = vt->gui_y;
  di->bm_type = vt->bm_type;

}/* import_vtoc */


void export_vtoc(DiskInfo *di, Vtoc *vt)
{
 int i, n;
 
 memset(vt,0,sizeof(Vtoc));
 
 vt->vt_magic_num = VT_MAGIC_NUM;
 vt->vt_version   = VT_VERSION;
 vt->vt_size      = sizeof(Vtoc);
 /* #if 0   - commented while on MT4P */
 
 vt->part_rec_count = 31;
 vt->part_rec_size  = 32;

 vt->disk_rec_count =  4;
 vt->disk_rec_size  = 16;

 vt->user_rec_count = 16;
 vt->user_rec_size  = 32;

 vt->menu_rec_count = 31;
 vt->menu_rec_size  = 64;

 vt->part_bitmap_size = 4;
 vt->perm_bitmap_size = 4;
 
 vt->vt_options  =  di->vt_options;
 vt->bm_options  =  di->bm_options;

 memmove(di->vtoc.bm_title,di->bm_title,40);
 for( i=1, n=0 ; i< di->num_part ; i++ )
  if( di->part[i].fs_type!=0 )
    {
     pack_vtoc_part_rec(&di->part[i],&vt->part_rec[n]);
     n++;
     if( n==vt->part_rec_count ) break; /*
                                         * MT4P - it was like this before: 
                                         * // if( n==Limit ) do something
                                         */
    }
 
 for( i=0, n=0 ; i<di->num_menus ; i++ )
  if( di->menu[i].menu_name[0]!=0 )
    {
     memmove(&vt->menu_rec[i].menu_name,&di->menu[n].menu_name,sizeof(BootMenuRec));
     n++;
     if( n==vt->menu_rec_count ) break; /*
                                         * MT4P - it was like this before:
                                         * // if( n==Limit ) do something
                                         */

    }
/* #endif - commented while on MT4P */
  vt->bm_data_size = 32;
  vt->gui_x   = di->gui_x;
  vt->gui_y   = di->gui_y;
  vt->bm_type = di->bm_type;

}/* export_vtoc */


