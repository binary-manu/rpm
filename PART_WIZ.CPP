#define PART_WIZ_C
#include "part.h"

#define WzX 6
#define WzY 6


void part_new_wiz( DiskInfo *di, int *row )
{
 Part *p=&di->part[*row];
 int i, k, act, top, col, c, x1, x2, n=6;
 unsigned long start_cyl, end_cyl, part_size;
 static char *sel[]={"FAT-16","FAT-32","Extended","BeOS","Linux","Linux swap"};
 static short  fs[]={  0x0600, 0x0B00, 0x0500, 0xEB00, 0x8300, 0x8201 };
 act=0;
 top=0;
 col=0;
 start_cyl = p->start_cyl;
 end_cyl = p->end_cyl;
 border_window(Yellow+BakWhite,WzX,WzY,61,n+5,MAIN_BORDER);
 border_window(Yellow+BakWhite,WzX,WzY,49,n+5,STEP_BORDER);
 border_window(Yellow+BakWhite,WzX,WzY,15,n+5,STEP_BORDER);
 write_block(Yellow+BakWhite,WzX+3,WzY," Step 1 ");
 write_block(Yellow+BakWhite,WzX+16,WzY," Step 2 ");
 write_block(Yellow+BakWhite,WzX+50,WzY," Step 3 ");
 write_block(Black+BakWhite,WzX+2,WzY+1,"  Select\n"
                                        "File System");
 write_block(Black+BakWhite,WzX+16,WzY+1," Select     Starting    Ending\n"
                                         "Size [KB]   Cylinder   Cylinder");
 write_block(Black+BakWhite,WzX+50,WzY+1," Format\n"
                                         "Partition");
 write_block(Black+BakWhite,WzX+16,WzY+3+n," Press Enter for the next step");

 while(1)  /* step 1 */
    {
     if( col==1 ) end_cyl = ((part_size*2+1)/di->sect_per_cyl+start_cyl-1);
             else part_size = (end_cyl-start_cyl+1)*di->sect_per_cyl/2;
     if( part_size*2<di->sect_per_cyl ) end_cyl=start_cyl;

     for( i=0 ; i<n ; i++ )
       cprintf((i==act)?(BrWhite):(BakWhite),WzX+2,WzY+4+i," %-9s ",sel[i]);

     cprintf(col==1?BrWhite:BakWhite,WzX+16,WzY+4, "%,9d", part_size);
                    cprintf(BakWhite,WzX+25,WzY+4, "    ");
     cprintf(col==2?BrWhite:BakWhite,WzX+29,WzY+4, "%,6d", start_cyl);
     cprintf(col==3?BrWhite:BakWhite,WzX+40,WzY+4, "%,6d", end_cyl);
     
     cprintf(BakWhite,WzX+16,WzY+7,"%-,6d %,24d",p->start_cyl,p->end_cyl);

     x1=(start_cyl-p->start_cyl)*31/(p->end_cyl-p->start_cyl+1);
     x2=(end_cyl-p->start_cyl)*31/(p->end_cyl-p->start_cyl+1);
     if( x1==0 && start_cyl!=p->start_cyl ) x1=1;
     if( x2>29 && end_cyl!=p->end_cyl ) x2=29;

     for( i=0 ; i<31 ; i++ )
        cprintf(BakWhite,WzX+16+i,WzY+6,(i>=x1&&i<=x2)?"Û":"°");

          if( col==0 ) { move_cursor(WzX+3,WzY+4+act);  k=get_key(); }
     else if( col==1 ) { move_cursor(WzX+24,WzY+4); k=edit_int( (p->end_cyl-p->start_cyl+1)*di->sect_per_cyl/2, &part_size ); }
     else if( col==2 ) { move_cursor(WzX+34,WzY+4); k=edit_int( end_cyl, &start_cyl ); }
     else if( col==3 ) { move_cursor(WzX+45,WzY+4); k=edit_int( p->end_cyl, &end_cyl ); }
     
     if( k!=-1 && start_cyl < p->start_cyl )
       {
        start_cyl=p->start_cyl;
        continue;
       }
     if( k!=-1 && end_cyl < start_cyl )
       {
        end_cyl=start_cyl;
        continue;
       }

     if( k==27 ) /* ESC */
        return;

     if( k==13 && col==0 )
       {
        col=1;
        continue;
       }

     if( k==13 ) /* Enter */ 
        break;

     if( up_down_navigations(k,&top,&act,n,n) )
       {
        continue;
       }
     if( k==0x4B00 && col>0 ) /* left */
       {
        col--;
       }
     if( k==0x4D00 && col<3 ) /* right */
       {
        col++;
       }
    }
 
 p->fs_type = fs[act];
 p->start_cyl = start_cyl;
 p->end_cyl = end_cyl;
 
 find_fs_num(p);
 recalculate_partition(di,p,MODE_CHS);
 part_ins_wiz(di,row);

 if( dialog_box("Do you want to save changes now?",
                " Save now "," No. I'll do it later ", 0 ) == 1 )
   {
    if( write_disk_info(di)==-1 )
      {
       show_error(ERROR_SAVE_PART_TABLE);
       return;
      }
    p=&di->part[*row];
    if( (p->flags & OP_FORMAT) &&
        dialog_box("Do you want to format partition now?",
                   " Format now "," No. I'll do it later ", 0 ) == 1 )
      {
       OP_HANDLER( OP_FORMAT, di, p, 0 );
      }
   }

 if( act==2 ) /* Extended */
   {
    sort_disk_info(di,row);
    (*row)++;
   }
 
}/* part_new_wiz */



/* This function is called when user changes FS type of unused record */

void part_ins_wiz( DiskInfo *di, int *row )
{
 int i;
 Part *p=&di->part[*row];

 if( p->end_cyl>1023 && (p->num_sect>0||cmd_line_mode) )
   {
    unsigned x = p->fs_type | 0x1000;
    if( x==0x1600 ) /* FAT-16 */
      {
       p->fs_type += 0x0E00-0x0600; /* FAT-16 LBA */
      }
    if( x==0x1B00 ) /* FAT-32 */
      {
       p->fs_type += 0x0C00-0x0B00; /* FAT-32 LBA */
      }
    if( x==0x1500 && !LOGICAL(p)  ) /* Extended */
      {
       p->fs_type += 0x0F00-0x0500; /* Extended LBA */
      }

    find_fs_num(p);
   }

 if( p->rel_sect  < di->sect_per_track &&
     p->last_sect > di->sect_per_track )
   {
    p->start_head = 1;
    p->start_sect = 1;
    recalculate_partition(di,p,MODE_CHS);
   }

 if( p->start_cyl < di->num_cyls && 
     p->end_cyl  == di->num_cyls )
   {
    p->end_cyl  = di->num_cyls-1;
    p->end_head = di->num_heads-1;
    p->end_sect = di->sect_per_track;
    recalculate_partition(di,p,MODE_CHS);
   }

 if( LOGICAL(p) && !EXTENDED(p) )
   {
    if( !EXTENDED(p-1) && di->num_part<P_ROWS )
      {
       for( i=di->num_part ; i>(*row) ; i-- ) /* inserting ext partition */
         di->part[i]=di->part[i-1];
       p->fs_type=FS_DOSEXT;
       //if( p->end_cyl>1023 )
         // p->fs_type=0x0F00; /* Extended LBA */
       find_fs_num(p);
       di->num_part++;
       (*row)++;
       p++;
       p->rel_sect += di->sect_per_track;
       p->num_sect -= di->sect_per_track;
       recalculate_partition(di,p,MODE_LBA);
      }
    else if( EXTENDED(p-1) && LOGICAL(p-1) )
      {
        (p-1)->num_sect = p->num_sect + di->sect_per_track;
       recalculate_partition(di,p-1,MODE_LBA);
      }
   }
}/* part_ins_wiz */
