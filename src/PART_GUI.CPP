#define PART_GUI_C
#include "part.h"
#include <stddef.h>

#define BORDER_COLOR  (Yellow +BakBlue)
#define TITLE_COLOR   (BrWhite+BakBlue)
#define HEADER_COLOR  (BrCyan +BakBlue)

#define UNUSED_COLOR  (BrCyan +BakBlue)
#define NORMAL_COLOR  (BrWhite+BakBlue)
#define INVALD_COLOR  (BrRed  +BakBlue)
#define EXTEND_COLOR  (BrGreen+BakBlue)

#define EDIT_COLOR    (BrWhite+BakBlack)
#define MENU_COLOR    (BrCyan +BakBlue)
#define HINT_COLOR    (Yellow +BakBlue)
#define MESG_COLOR    (BrGreen+BakBlue)
#define WARN_COLOR    (BrRed  +BakBlue)

#define GR_BORD_COLOR (Yellow+BakWhite)
#define GR_TEXT_COLOR (Black +BakWhite)

#define  BOX  21
#define  BOY  20
#define  BOW  59
#define  BOH   5

#define FmX 3
#define FmY 14
#define FmW 75
#define FmH 5

#define FsX 32
#define FsY 5
#define FsW 46
#define FsH 20

#define SDsX 32
#define SDsY 5
#define SDsW 46
#define SDsH 20

#define ErrX   35
#define ErrY    1
#define ErrW   46
#define ErrH    8

#define HelpX  10
#define HelpY   4
#define HelpW  59
#define HelpH  20

static char sv_screen[4000];
static char sv_errbuf[ErrW*ErrH*2];
static int  sv_cur_x, sv_cur_y;
static int  err_row;
static int  prev_bar;
static char prev_msg[21];

void gui_init(void)
{
 save_cursor(&sv_cur_x,&sv_cur_y);
 save_window(1,1,80,25,sv_screen);
 save_window(ErrX,ErrY,ErrW,ErrH,sv_errbuf);
 sv_cur_x=1;
 move_cursor(1,26);  /* hide it */
}/* gui_init */
 
void gui_done(void)
{
 load_window(1,1,80,25,sv_screen);
 move_cursor(sv_cur_x,sv_cur_y);
}/* gui_done */

void gui_restore(void)
{
 load_window(1,1,80,25,sv_screen);
 move_cursor(1,26);  /* hide it */
}

void show_error(char *msg)
{
 int k, x, y=12, w, h=4;
 char buf[4*80*2];

 if( cmd_line_mode )
 { fprintf(stderr,"ERROR: %s\r\n",msg); return; }


 w = strlen(msg)+6;
 if( w%2==1 ) w++;
 if( w>78 ) w=78;
 x = (80-w)/2+1;
 
 save_window(x,y,w,h,buf);
 border_window(Yellow+BakRed,x,y,w,h, MAIN_BORDER );

 write_string( BrWhite+BakRed, x+3, y+1, msg );
 write_string( Black+BakWhite, x+w/2-2, y+2," OK "  );

 while( (k=get_key())!=27 && k!=13 ); /* ESC or ENTER */

 load_window(x,y,w,h,buf);
}/* show_error */


int dialog_box(char *msg, char *ch1, ... )
{
 char buf[5*80*2], **ch;
 int k, i, a, n, x, y=12, w, h=5, w1, w2, x1, x2, x3, sv_x, sv_y;

 save_cursor(&sv_x,&sv_y);
 move_cursor(1,26);
 
 w1 = strlen(msg)+6;
 if( w1%2==1 ) w1++;
 
 ch=&ch1;
 for( w2=0, n=0 ; ch[n]!=0 ; n++ )
   w2+=strlen(ch[n]);

 if( n==0 ) ch[n++]=" OK ";
 
 w2+=(n-1)+6;
 
 if( w1>78 ) w1=78;
 if( w2>78 ) w2=78;
 w = max(w1,w2);
 x  = (80-w)/2+1;
 x1 = (80-w1)/2+4;
 x2 = (80-w2)/2+4;
 
 save_window(x,y,w,h,buf);
 border_window(Yellow+BakRed,x,y,w,h, MAIN_BORDER );

 write_string( BrWhite+BakRed, x1, y+1, msg );
 
 a=0;
 
 while(1)
    {
     for( i=0, x3=x2 ; i<n ; x3+=strlen(ch[i])+1, i++ )
        write_string( (i==a)?(Black+BakWhite):(White+BakRed), x3, y+3, ch[i] );

     k=get_key();

          if( k==27 || k==13 ) break;
     else if( k==0x4B00         ) a=(a+n-1)%n;
     else if( k==0x4D00 || k==9 ) a=(a+n+1)%n;
    }

 load_window(x,y,w,h,buf);
 move_cursor(sv_x,sv_y);

 if( k==13 ) return a+1;

 return 0;
}/* dialog_box */

void write_int(int attr, int x, int y, int w, int n)
{
 char tmp[32];
 sprintf(tmp,"%,*u",w,n);
 write_string(attr,x,y,tmp);
}/* write_int */


int edit_int( unsigned long lim, unsigned long *n )
{
 int k;
 unsigned long l=*n;

 k=get_key();
 if( k==8 || k==0x5300 )
   {
    l/=10;
   }
 else if( k>='0' && k<='9' )
   {
    l=l*10+(k-'0');
    if( l>lim ) l/=10;
   }
 else if( k=='+' )
   {
    if( l<lim ) l++;
   }
 else if( k=='-' )
   {
    if( l>0 ) l--;
   }
 else return k;

 *n=l;

 return -1;
}/* edit_int */


void redraw_main_screen( HardDiskInfo *di, int mode )
{
 border_window(BORDER_COLOR,1,1,80,25, MAIN_BORDER);
 border_window(BORDER_COLOR,2,3,78,1,  HORIZONTAL);
 border_window(BORDER_COLOR,1,19,80,7, BOX1_BORDER);
 border_window(BORDER_COLOR,20,19,61,7,BOX2_BORDER);
#ifdef DISK_SIM
  write_string(TITLE_COLOR,39,1," Simulation ");
#endif
 
  write_string(TITLE_COLOR, 4, 2, PROGRAM_TITLE);
  write_string(HEADER_COLOR, 4, 4, HEADER_DISK);

  write_int(NORMAL_COLOR,15,4,1, di->hd_num - 127);

  write_int(NORMAL_COLOR,18,4,6, di->total_sects/2048);
  write_string(HEADER_COLOR,4,5,(di->flags&1) ? USING_LBA : USING_CHS );
  write_int(NORMAL_COLOR,33,4,6, di->num_cyls );
  write_int(NORMAL_COLOR,53,4,3, di->num_heads );
  write_int(NORMAL_COLOR,66,4,2, di->sect_per_track );

  write_block(HEADER_COLOR, 4, 6, mode==MODE_CHS ? HEADER_CHS : HEADER_LBA );

  write_string(BORDER_COLOR, 3, 19, " MBR ");
  write_string(HEADER_COLOR, 2, 20, HEADER_MINI);
 
  write_string(HEADER_COLOR, 8, 25, HELP );
  write_string(  HINT_COLOR, 5, 25, " F1 ");

 save_window(ErrX,ErrY,ErrW,ErrH,sv_errbuf);
}/* redraw_main_screen */



void clear_error(int problems)
{
 load_window(ErrX,ErrY,ErrW,ErrH,sv_errbuf);
 if( problems )
   {
    cprintf(INVALD_COLOR, 60, 8,"%16s %c", PROBLEMS,
                                           problems==1 ? '' : '' );
   }
 err_row=0;
}

void popup_error(char *err)
{
 if( err_row==0 )
    {
     border_window(Yellow+BakWhite, ErrX, ErrY, ErrW, 1, MAIN_BORDER);
     write_string(Yellow+BakWhite, ErrX+2, ErrY, ERR_PROBLEMS);
    }
 err_row++;

 border_window(Yellow+BakWhite, ErrX, ErrY+err_row, ErrW, 2, MAIN_BORDER+3);
 write_string(Black+BakWhite, ErrX+2, ErrY+err_row, err );

}/* popup_error */



void redraw_main_prompt(int save)
{
 int mask=save?0xFF:0x77;
 
 write_string(HEADER_COLOR&mask,22,25," F2 Save ");
 write_string(  HINT_COLOR&mask,22,25," F2 ");
 write_string(HEADER_COLOR&mask,32,25," F3 Undo ");
 write_string(  HINT_COLOR&mask,32,25," F3 ");
 write_string(HEADER_COLOR,42,25," F4 Mode ");
 write_string(  HINT_COLOR,42,25," F4 ");
 write_string(HEADER_COLOR,52,25," F5 Disk ");
 write_string(  HINT_COLOR,51,25,"อ F5 ");
 write_string(HEADER_COLOR,68,25," ESC Quit ");
 write_string(  HINT_COLOR,68,25," ESC ");
}/* redraw_main_screen */

void redraw_fat_prompt(int flags)
{
 redraw_main_prompt(flags&1);
 if( flags&2 )
   {
    write_string(HEADER_COLOR,42,25," F6 Install patch ");
    write_string(  HINT_COLOR,42,25," F6 ");
   }
 else if( flags&4 )
   {
    write_string(HEADER_COLOR,42,25,"  Patch installed  ");
   }
}/* redraw_main_screen */

struct mode_desc
    {
     char pos[8];
     char len[8];
     char cur_pos[8];
    } mode_desc[2] =
      {
        { /* CHS */
         { 8, 10,    31,  38, 43,    48,  55, 60},  /* column positions */ 
         { 1, 21,     6,   4,  4,     6,   4,  4},  /* column length    */
         { 8, 10,    36,  41, 46,    53,  58, 63}   /* cursor positions */
        },
        { /* LBA */
         { 8, 10,    32,    43 },  /* column positions */ 
         { 1, 21,    10,    10 },  /* column length    */
         { 8, 10,    41,    52 }   /* cursor positions */
        }
      };

void redraw_part_table( int first_row,
                        int row, int col, Part *p, int n, int mode )
{
 char tmp[120];
 int i, x, y, color;
 struct mode_desc *m = &mode_desc[mode];
 
      if( mode==MODE_CHS ) { x=3;  y=9; }
 else if( mode==MODE_LBA ) { x=3;  y=9; }
 else if( mode==MODE_EXT ) { x=22; y=21; }
 else if( mode==MODE_MINI ) { x=2; y=21; }
 else return;

 for( i=0 ; i<n ; i++ )
    {
     color = ( UNUSED(p+i)   ? UNUSED_COLOR :
             ( !VALID(p+i)   ? INVALD_COLOR :
             ( EXTENDED(p+i) ? EXTEND_COLOR : NORMAL_COLOR )));

     sprintf_partition( tmp, first_row+i, &p[i], mode );

     write_string(color,x,y+i,tmp);
     
     if( first_row+i == row )	 /* this is highlighted row */
       {
        tmp[ m->pos[col] + m->len[col] ]=0;

        write_string(EDIT_COLOR,
                       x + m->pos[col],
                       y + i,
                     tmp + m->pos[col] );
        move_cursor( x + m->cur_pos[col], y + i );
       }
    }

}/* redraw_part_table */



void redraw_main_hint(int h_type, char *hint)
{
 clear_window(BORDER_COLOR,9,18,71,1);

 write_string( h_type==MESSAGE ? MESG_COLOR : 
              (h_type==WARNING ? WARN_COLOR : HINT_COLOR), 9, 18, hint );

}/* redraw_main_hint */



void clear_preview_box(char *hint)
{
 clear_window(BORDER_COLOR,BOX,BOY,BOW,BOH);
 if( hint!=0  ) write_block(HINT_COLOR,BOX+8,BOY+1,hint);
}


void redraw_ipl_options( DiskInfo *di, int act )
{
 int mask;
 char tmp[30];
 
 static char *ipl_name[] = { "Boot Manager",
                             "Standard IPL",
                             "Old BootMan ",
                             "Unknown IPL "};
 static char *bm_name[] = {  "Compact     ",
                             "Text 25x80  ",
                             "GUI 640x480 "};
                              
 if( act!=-1 )
   {
    write_block(HINT_COLOR, BOX+37, BOY+0, IPL_HINT_SPACE );
    border_window(BORDER_COLOR, 22,25,56,1,"อออ");
    move_cursor(BOX+(act==0?22:(act==3?24:22)) ,BOY+act);
   }

 write_string( HEADER_COLOR, BOX+1, BOY+0, IPL_OPTION1 );
     
 mask=(di->new_ipl==IPL_TYPE_BM?0xFF:0x77);

 write_block( HEADER_COLOR & mask, BOX+1, BOY+1, IPL_OPTIONS );

 write_string( act==0 ? EDIT_COLOR : NORMAL_COLOR, BOX+22, BOY,
                                              ipl_name[ di->new_ipl ] );

 write_string( act==1 ? EDIT_COLOR : NORMAL_COLOR & mask, BOX+22, BOY+1,
                                               bm_name[ di->bm_type ] );

 write_string( act==2 ? EDIT_COLOR : (NORMAL_COLOR) & mask, BOX+22, BOY+2,
                     (di->bm_options & BM_OPT_VIRUS_CHECK ? "Yes" : "No ") );

 write_int( act==3 ? EDIT_COLOR : NORMAL_COLOR & mask, BOX+22, BOY+3,
                                          act==3 ? 3 : -3, di->ipl_timeout );

 sprintf(tmp,"Partition %c", di->ipl_default);
 write_string( act==4 ? EDIT_COLOR : NORMAL_COLOR & mask, BOX+22, BOY+4,
                         (di->ipl_default== 0 ? "Not set    ":
                         (di->ipl_default==-1 ? "Prompt user": tmp )) );

}/* redraw_ipl_options */


void edit_ipl_options(DiskInfo *di)
{
 int act, k;

 static char bm_next[4]={1,0,0,0};
 
 bm_next[1] = (di->cur_ipl>1) ? di->cur_ipl : 0 ;

 act=0;

 while(1)
    {
     redraw_ipl_options(di,act);

     if( act==3 )
       {
        k=edit_int(255,&di->ipl_timeout);
        if( k==-1 ) /* timeout was modified */
          {
           di->changed=1;
           continue;
          }
       }
     else k=get_key();

     if( k==27 || k==13 ) break;

#ifdef DISK_SIM
     if( k==0x4400 ) /* F10 */
       {
        boot_manager_simulation(di);
        continue;
       }
#endif

     if( act==0 && k==' ' )
       {
        di->new_ipl = bm_next[ di->new_ipl ];
        di->changed=1;
        continue;
       }

     if( di->new_ipl!=IPL_TYPE_BM ) continue;

     if( act==1 && k==' ' ) 
       {
        di->bm_type = (di->bm_type+1)%2; // Was 3, now 2 to disable GUI
        di->changed = 1;
        continue;
       }

     if( act==2 && k==' ' ) 
       {
        di->bm_options^=BM_OPT_VIRUS_CHECK;
        di->changed=1;
        continue;
       }

     if( act==4 && k==' ' ) 
       {
             if( di->ipl_default==0   ) di->ipl_default=-1;
        else if( di->ipl_default==-1  ) di->ipl_default='1';
        else if( di->ipl_default=='1' ) di->ipl_default='2';
        else if( di->ipl_default=='2' ) di->ipl_default='3';
        else if( di->ipl_default=='3' ) di->ipl_default='4';
        else if( di->ipl_default=='4' ) di->ipl_default=0;
        else                            di->ipl_default=0;
        di->changed=1;
        continue;
       }

     if( k==0x4800 ) /* up */
       {
        if( act>0 ) act--;
        continue;
       }

     if( k==0x5000 ) /* down */
       {
        if( act<4 ) act++;
        continue;
       }
    }
}/* edit_ipl_options */


void redraw_ext_details(Part *part2)
{
 write_string( HEADER_COLOR, BOX+1, BOY+0, HEADER_EXT );
 redraw_part_table( 1, -1, -1, part2, 4, MODE_EXT );
}/* redraw_ext_details */



#define fd_color(cond,row)  ( (cond)?( ((act)==(row)) ? (BrWhite) : (BrWhite+BakBlue)  ) : \
                                     ( ((act)==(row)) ? (BrRed)   : (BrRed+BakBlue)    )  )

void redraw_fat_details(FatInfo *fi, int act)
{
 cprintf(HEADER_COLOR, BOX+1, BOY+0, 
 "Volume label:\n"
 "   System id:\n"
 " File system:\n"
 "Cluster Size:\n"
 "    FAT Size:");

 write_string( NORMAL_COLOR, BOX+15, BOY+0, fi->label);
 write_string( NORMAL_COLOR, BOX+15, BOY+1, fi->sysid);
 write_string( NORMAL_COLOR, BOX+15, BOY+2, fi->fs_id);
 cprintf( NORMAL_COLOR, BOX+15, BOY+3,"%,dk", fi->clust_size/2 );
 cprintf( NORMAL_COLOR, BOX+15, BOY+4,"%,dk", fi->fat_size/2 );

 write_string(HEADER_COLOR, BOX+29, BOY+0, "Starting:");
 write_block(HEADER_COLOR, BOX+23, BOY+1,
 "     Drive num:\n"
 "  Minimum size:\n"
 "Partition size:");

 cprintf( fd_color( (fi->rel_sect == fi->exp_rel_sect), 0 ),
                        BOX+38, BOY+0, "%,11u", fi->rel_sect );
 cprintf( fd_color( (fi->drive_num == fi->exp_drive_num), 1 ),
                        BOX+46, BOY+1, "%,3u", fi->drive_num );
 cprintf( HEADER_COLOR, BOX+38, BOY+2, "%,11u", fi->min_num_sect );
 cprintf( fd_color( (fi->num_sect >= fi->min_num_sect &&
                     fi->num_sect <= min(fi->exp_num_sect,fi->max_num_sect)), 2 ),
                        BOX+38, BOY+3, "%,11u", fi->num_sect );

 cprintf(   MESG_COLOR, BOX+50, BOY+1, "%,6uM", fi->num_data_sect/2048 );
 cprintf( HEADER_COLOR, BOX+50, BOY+2, "%,6uM", fi->min_num_sect/2048 );
 cprintf( NORMAL_COLOR, BOX+50, BOY+3, "%,6uM", fi->num_sect/2048 );
 cprintf( HEADER_COLOR, BOX+25, BOY+4, "Maximum size:%,11u %,6uM", min(fi->exp_num_sect,fi->max_num_sect),
                                                                       min(fi->exp_num_sect,fi->max_num_sect)/2048 );

 if( fi->fat_sect_read != fi->fat_size && fi->fat_size!=0 )
   cprintf( HINT_COLOR, BOX+52, BOY+0, " %3d%%", 100*fi->fat_sect_read/fi->fat_size );
 else
   write_string(MESG_COLOR, BOX+52, BOY+0,"Used ");

 if( act!=-1 ) move_cursor( BOX+48, BOY+(act==0?0:(act==1?1:3)) );
}/* redraw_fat_details */



void edit_fat_details(HardDiskInfo *di, Part *p, FatInfo *fi)
{
 int act, x, k, changed=0;

 act=0;

 while(1)
    {
     bgview_fat_details(di,p,fi,act);
     redraw_fat_prompt(changed + (fi->sys==32?0:(fi->boot_fix?4:2)) );

     if( act==0 ) k=edit_int(  -1, &fi->rel_sect );
     if( act==1 ) k=edit_int( 255, &fi->drive_num );
     if( act==2 ) k=edit_int(  -1, &fi->num_sect );
     if( k==-1 )
       {
        changed=1;
        continue;
       }

     x=0;
     if( changed && (k==27||k==13) )
       {
        x=dialog_box("Some boot sector settings have been changed",
                     " Save ", " Discard ", " Continue editing ", 0 );
        if( x==0 || x==3 ) continue;
       }
     
     if( x==1 || k==0x3C00 ) /* F2 - Save */
       {
        save_fat_boot_sect(di,p,fi);
        x=2;
       }

     if( x==2 || k==0x3D00 ) /* F3 - Undo */
       {
        fi->rel_sect_loaded=0;
        update_fat_info(di,p,fi);
        changed=0;
        if( k==0x3C00 || k==0x3D00 ) continue;
       }

     if( k==27 || k==13 )
       {
        break;
       }
     
     if( k=='x' || k=='X' )
       {
        Sector X=min(fi->exp_num_sect,fi->max_num_sect);
        if( fi->rel_sect  != p->rel_sect ||
            fi->drive_num != fi->exp_drive_num ||
            fi->num_sect  != X )
          changed=1;
        fi->rel_sect = p->rel_sect;
        fi->drive_num = fi->exp_drive_num;
        fi->num_sect = X;
        continue;
       }

     if( k==0x4000 && fi->sys!=32 ) /* F6 */
       {
        int i;
        i=dialog_box("What OS do you wand to boot from this partition?",
                " Win9x or MS-DOS "," PC-DOS or DR-DOS "," Windows NT "," Cancel ", 0);
        if( i>0 && i<4 )
          {
           if( fi->boot_fix!=i )
              changed=1;
           fi->boot_fix=i;
          }
        continue;
       }

     if( k==0x4800 ) /* up */
       {
        if( act>0 ) act--;
        continue;
       }

     if( k==0x5000 ) /* down */
       {
        if( act<2 ) act++;
        continue;
       }
    }

}/* edit_fat_options */


int floppy_os_dialog_box(void)
{
 int i=dialog_box("Which OS would you like to boot from floppy in additioin to PM?",
                     " Win9x or MS-DOS ", " PC-DOS or DR-DOS ", " Windows NT ", 0 );
 if( i==0 ) i++;
 return i+=2;
}


void show_help(int topic)
{
 int sv_x, sv_y;
 char *sv_buf=(char*)malloc(4000);
 
 if( sv_buf==0 ) 
   {
    show_error(ERROR_MALLOC);
    return;
   }

 while( topic==0 ) topic++; /* making compiler happy */
 
 save_cursor(&sv_x,&sv_y);
 move_cursor(1,26);
 save_window(1,1,80,25,sv_buf);

 border_window(GR_BORD_COLOR, HelpX, HelpY, HelpW, HelpH, MAIN_BORDER);

 write_block(GR_TEXT_COLOR, HelpX+2, HelpY+1, GUI_HELP_TEXT);

 get_key();
 
 load_window(1,1,80,25,sv_buf);
 move_cursor(sv_x,sv_y);
 
 free(sv_buf);
 return;   
}/* show_help */


void select_fs_type(Part *p)
{
 char tmp[96];
 int i, j, k, top, act, color, N;

 border_window( Yellow+BakWhite, FsX, FsY, FsW, FsH, MAIN_BORDER );
 write_string( Black+BakWhite, FsX+2, FsY+1, HEADER_SYSTYPE );
 border_window( Yellow+BakWhite, FsX+1, FsY+2, FsW-2, 1, HORIZONTAL );
 write_string( Yellow+BakWhite, FsX+3, FsY+FsH-1, HINT_INSFSID );

 for( N=0 ; fs_desc[N].fs_id<=FS_HIDDEN /* valid ids */ ; N++ );
 
 /* N - number of records in table  */
 /* n - number of records on screen */
 
 #define n  (FsH-4)

 top=0;
 act = ( fs_desc[p->fs_num].fs_id==FS_UNKNOWN ) ? 0 : p->fs_num;
 
 if( act>=n ) top=act;
 
 i=0;
      if( p->fs_type>0x0B00 && p->fs_type<0x1000 ) i=0x0B00;  /* FAT-32 area */
 else if( p->fs_type>0x1100 && p->fs_type<0x2000 ) i=0x1100;  /* Hidden FATs */
 else if( p->fs_type>0x8100 && p->fs_type<0x8600 ) i=0x8100;  /* Linux  area */
 
 if( i!=0 )
  for( top=0 ; top<N && fs_desc[top].fs_id!=i ; top++);

 while(1)
    {
     if( top+n>N ) top=N-n;
     if( act<top ) top=act;
     if( act>=top+n ) top=act-n+1;
     
     for( i=0 ; i<n ; i++ )
        {
         sprintf(tmp," 0x%02X   %-21s      %s  ",
                fs_desc[top+i].fs_id>>8,
                fs_desc[top+i].name,
               (fs_desc[top+i].flags & OP_ANY_TYPE)?"Yes":" - ");
         
         if( fs_desc[top+i].flags & P_HIGHLIGHT ) 
           {
            color = ( (top+i==act)?(BrWhite+BakBlack):(Black+BakWhite) );
           }
         else
           {
            color = ( (top+i==act)?(White+BakBlack):(BrBlue+BakWhite) );
           }

         write_string( color, FsX+2, FsY+3+i, tmp );
        }

     move_cursor(1,26);

     k=get_key();
     
     if( k==27 ) /* ESC */
       {
        break;
       }
     
     if( k==13 ) /* Enter */
       {
        p->fs_type=fs_desc[act].fs_id;
        find_fs_num(p);
        break;
       }
     else if( k==0x5200 ) 	/* Insert */
       {
        int x2, w2;
        w2=strlen(PROMPT_FSID)+6;
        x2=FsX+2+w2-4;
        border_window( Yellow+BakWhite, FsX+2, FsY+FsH-4, w2, 3, MAIN_BORDER );
        write_string( Black+BakWhite, FsX+4, FsY+FsH-3, PROMPT_FSID );
        move_cursor( x2+1, FsY+FsH-3 );

	i=0;
        while(1)
           {
            sprintf(tmp,"%-2X",i);
            write_string(BrWhite+BakBlack, x2, FsY+FsH-3, tmp );
            k=get_key();
            if( k==27 ) break;
            if( k==8 || k==0x5300 ) i/=16;
            else if( k==13 )
              {
               p->fs_type=i<<8;
               find_fs_num(p);
               return;
              }
            else if( i<16 )
              {
               if( k>='0' && k<='9' ) i=i*16+(k-'0');
               if( k>='a' && k<='f' ) i=i*16+(k-'a'+10);
               if( k>='A' && k<='F' ) i=i*16+(k-'A'+10);
              }
           }
       }
     else if( up_down_navigations(k,&top,&act,n,N) )
       {
        /* One of {Up, Down, Home, End, PgUp, PgDown} keys */
       }
     else if( k>='A' && k<='Z' || k>='a' && k<='z' )
       {
        char ch = k | 0x20; /* Make it lower case */
        long h = (fs_desc[act].flags & P_HIGHLIGHT);

        for( i=act+1 ; i<N ; i++ )
         if( (fs_desc[i].flags & P_HIGHLIGHT) == h &&
             (fs_desc[i].name[0] | 0x20)==ch ) break;

        if( h!=0 && i==N )  /* if didn't find among highlighted look through */
          for( i=0 ; i<N ; i++ )              /* the list of not highlighted */
           if( (fs_desc[i].flags & P_HIGHLIGHT) == 0 &&
               (fs_desc[i].name[0] | 0x20)==ch ) break;

        if( i!=N ) /* found something */
          {
           if( top+n<i ) top=min(i,N-n);
           act=i;
          }
        
       }
    }/* while(1) */

 #undef n
}/* select_fs_type */


int atoi(char *s)
{
 int i=0;

 while( (*s)==' ' || (*s)=='\t' ) s++;

 while( (*s)>='0' && (*s)<='9' )
    {
     i = i*10 + (*s)-'0';
     s++;
    }

 return i;
}


void progress_bar(char *bar, int percent)
{
 int i;
 
 if( percent<0 ) percent=0;
 if( percent>100 ) percent=100;

 for( i=0 ; i<percent/2 ; i++ )
        bar[i]=0xB2;
 while( i!=50 ) bar[i++]=0xB0;
 bar[i]=0;
}

int progress_cmd(char *msg)
{
 int k;
 char tmp[96];
 static int  prev_bar=0;
 static char prev_msg[21];

 if( quiet )  return EV_OK;
 if( msg==0 ) return EV_OK;

 if( *msg=='^' )
   {
    printf("%s\r\n",msg+1);
    prev_bar=0;
   }
 else if( *msg=='%' )
   {
    if( verbose && (prev_bar==0 || strcmp(prev_msg,msg+1)!=0) )
      {
       //progress_bar( tmp, atoi(msg+1) );
       printf("\r%s ",msg+1);
       if( strncmp(msg,"% 100%",6)==0 )
         {
          printf("\r\n");
         }
       prev_bar=1;
       strncpy(prev_msg,msg+1,20);
      }
   }
 else
   {
    printf("%s\r\n",msg);
    prev_bar=0;
   }

 #if 0
 while( kb_hit() )
    {
     k=get_key();
     if( k==27 ) /* ESC */
       return EV_CANCEL;
    }
 #endif
 return EV_OK;
}/* progress */


int confirm(void)
{
 int key;
 while(1)
    {
     fprintf(stderr,"\rAre you sure (y/n)?   \b\b\b ");
     key=get_key();
     if( key=='Y' || key=='y' )
       {
        fprintf(stderr,"%c\r\n",key);
        return 1;
       }
     if( key=='N' || key=='n' )
       {
        fprintf(stderr,"%c\r\n",key);
        return 0;
       }
    }
}



int progress(char *msg)
{
 int k;
 char tmp[96];
 
 if( cmd_line_mode ) return progress_cmd(msg);

 if( msg==0 ) return EV_OK;

 move_cursor(1,26);
 if( *msg=='^' )
   {
    border_window( Yellow+BakWhite, FmX, FmY, FmW, FmH, MAIN_BORDER );
    write_string( Black+BakWhite, FmX+2, FmY+1, msg+1);
    prev_bar=0;
   }
 else if( *msg=='%' )
   {
    if( prev_bar==0 || strcmp(prev_msg,msg+1)!=0 )
      {
       progress_bar( tmp, atoi(msg+1) );
       write_string( Black+BakWhite, FmX+2,    FmY+3, tmp  );
       clear_window( Black+BakWhite, FmX+2+50, FmY+3, 20, 1);
       write_string( Black+BakWhite, FmX+2+50, FmY+3, msg+1);
       prev_bar=1;
       strncpy(prev_msg,msg+1,20);
      }
   }
 else
   {
    clear_window( Black+BakWhite, FmX+2, FmY+3, 70,1);
    write_string( Black+BakWhite, FmX+2, FmY+3, msg );
    prev_bar=0;
   }

 
 while( kb_hit() )
    {
     k=get_key();
     if( k==27 ) /* ESC */
       return EV_CANCEL;
    }

 return EV_OK;
}/* progress */



#ifndef DISK_SIM
void show_bad_blocks(HardDiskInfo *di, Part *p, Sector *bbt, int bbt_size)
{
 int i;
 unsigned long n;
 
 border_window(GR_BORD_COLOR,21,5,35,12,MAIN_BORDER);
 write_string(GR_BORD_COLOR,23, 5," First 9 bad sectors ");
 write_string(GR_TEXT_COLOR,23,6,"#  Sector num     Cyl Head Sect");
 for( i=0 ; i < min(bbt_size,9) ; i++ )
    {
     n = p->rel_sect + bbt[i];
     
     cprintf(GR_TEXT_COLOR,23,7+i,"%d %,11u  %,6u %,4u %,4u", i+1, n,
                         n / di->sect_per_cyl,
                         n % di->sect_per_cyl / di->sect_per_track,
                         n % di->sect_per_cyl % di->sect_per_track + 1 );
   }

 get_key(); 
}/* show_bad_blocks */
#endif


int up_down_navigations(int k, int *top, int *act,
					 int n, /* records on screen*/
					 int N /* displayable records */ )
{
 if( k==0x5000 ) /* down */
   {
    if( (*act)<N-1 ) (*act)++;
    return 1;
   }

 if( k==0x4800 ) /* up */
   {
    if( (*act)>0 ) (*act)--;
    return 1;
   }

 if( k==0x4700 ) /* Home */
   {
    *act=0;
    return 1;
   }

 if( k==0x4F00 ) /* End */
   {
    *act=N-1;
    return 1;
   }

 if( k==0x5100 ) /* PgDn */
   {
    if( (*act)==(*top)+n-1 )
      {
       (*top)+=n;
       if( (*top)>N-n ) (*top)=N-n;
      }
    (*act)=(*top)+n-1;
#ifdef MT4P
    return 1;
#endif
   }

 if( k==0x4900 ) /* PgUp */
   {
    if( (*act)==(*top) )
      {
       (*top)-=n;
       if( (*top)<0 ) (*top)=0;
      }
     (*act)=(*top);
#ifdef MT4P
     return 1;
#endif
   }

 return 0;  /* some other key */
 
}/* up_down_navigations */



void select_and_copy_disk(DiskInfo *src_disk)
{
 char tmp[96];
 int hd, i, m;
 DiskInfo *di, *dst_disk;
 Part *p;
 int j, k, top, act, color, N;

 N=src_disk->num_disks;
 if( N==1 ) 
   {
    show_error("You have only one disk.");
    return;
   }

 border_window( Yellow+BakWhite, SDsX, SDsY, SDsW, SDsH, MAIN_BORDER );
 write_string( Black+BakWhite, SDsX+2, SDsY+1, "Select destination disk" );
 border_window( Yellow+BakWhite, SDsX+1, SDsY+2, SDsW-2, 1, HORIZONTAL );

 
 /* N - number of records in table  */
 /* n - number of records on screen */
 
 #define n  (SDsH-4)

 top=0;
 act = 0;
 
 while(1)
    {
     if( top+n>N && n<=N ) top=N-n;
     if( act<top ) top=act;
     if( act>=top+n ) top=act-n+1;
     
     for( i=0 ; i<min(n,N) ; i++ )
        {
         di = get_disk_info(top+i+128);
         sprintf(tmp," Disk%2d %,7dM ", i+1, di->total_sects/2048 );
         
         if( top+i==act )
           {
            color = BrWhite+BakBlack;
            dst_disk=di;
           }
         else
           {
            color=Black+BakWhite;
            if( di==src_disk ) color=BrBlue+BakWhite;
           }

         write_string( color, SDsX+2, SDsY+3+i, tmp );
        }

     move_cursor(1,26);

     k=get_key();
     
     if( k==27 ) /* ESC */
       {
        break;
       }
     
     if( k==13 && dst_disk!=src_disk ) /* Enter */
       {
        i=dialog_box("Warning!!! This will destroy all data on the target disk!!!",
                        " Cancel!!! I don't want to do that!!! ", " Ok. Copy it.", 0 );
        if( i<2 ) continue;
        int x=copy_disk(src_disk,dst_disk);
        if( x==0 ) show_error("Copying have been completed successfully.");
	else if( x!=EV_CANCEL ) 
	    show_error("Error has occured during the copying!!!");
	read_disk_info(dst_disk);
        break;
       }
     else if( up_down_navigations(k,&top,&act,n,N) )
       {
        /* One of {Up, Down, Home, End, PgUp, PgDown} keys */
       }
    }/* while(1) */

 #undef n
}/* select_and_copy_disk */


void select_and_copy_part(DiskInfo *src_disk, Part *src_part)
{
 char tmp[96];
 int hd, i, m;
 DiskInfo *di, *dst_disk;
 Part *p, *dst_part;
 int j, k, top, act, color, N, z;

 border_window( Yellow+BakWhite, SDsX, SDsY, SDsW, SDsH, MAIN_BORDER );
 write_string( Black+BakWhite, SDsX+2, SDsY+1, "Select destination partition" );
 border_window( Yellow+BakWhite, SDsX+1, SDsY+2, SDsW-2, 1, HORIZONTAL );
// write_string( Yellow+BakWhite, SDsX+3, SDsY+SDsH-1, HINT_INSFSID );

 N=0;
 for( i=0 ; i<src_disk->num_disks; i++ )
   {
    di=get_disk_info(i+128);
    for( j=0 ; j<di->num_part ; j++ )
       {
        p=&(di->part[j]);
        if( EXTENDED(p) && LOGICAL(p) ||
            p->fs_type==FS_MBR ||
            p->num_sect<=63 ) continue;
        N++;
       }
   }
 
 /* N - number of records in table  */
 /* n - number of records on screen */
 
 #define n  (SDsH-4)

 top=0;
 act = 0;
 
 while(1)
    {
     if( top+n>N && n<=N ) top=N-n;
     if( act<top ) top=act;
     if( act>=top+n ) top=act-n+1;
     
     k=0;
     for( i=0 ; i<src_disk->num_disks; i++ )
       {
        di=get_disk_info(i+128);
        for( j=0 ; j<di->num_part ; j++ )
           {
            p=&(di->part[j]);
            if( EXTENDED(p) && LOGICAL(p) ||
                p->fs_type==FS_MBR ||
                p->num_sect<=63  ) continue;
            if( k>=top && k<top+n )
              {
               if( k==act ) { dst_part=p; dst_disk=di; z=j; }
               sprintf(tmp," Disk%2d / %-21s %,7dM ", i+1, fs_desc[p->fs_num].name, p->num_sect/2048 );
               color = ( (k==act)?(BrWhite+BakBlack):(Black+BakWhite) );
               if( p==src_part && k!=act ) color = (BrBlue+BakWhite);
               write_string( color, SDsX+2, SDsY+3+k-top, tmp );
              }
            k++;
           }
       }

     move_cursor(1,26);

     k=get_key();
     
     if( k==27 ) /* ESC */
       {
        break;
       }

     if( k==13 && dst_part!=src_part) /* Enter */
       {
        i=dialog_box("Warning!!! This will destroy all data on the target partition!!!",
                        " Cancel!!! I don't want to do that!!! ", " Ok. Copy it.", 0 );
        if( i<2 ) continue;
        if( dst_part->fs_type==0 ) // Unused
          {
           dst_part->fs_type=src_part->fs_type;
           find_fs_num(dst_part);
           Sector delta = (dst_disk->sect_per_track - 
                           dst_part->rel_sect%dst_disk->sect_per_track)%dst_disk->sect_per_track;
           if( dst_part->num_sect>delta )
             {
              dst_part->rel_sect+=delta;
              dst_part->num_sect-=delta;
             }
           dst_part->num_sect=min(dst_part->num_sect,src_part->num_sect);
           recalculate_partition(dst_disk,dst_part,MODE_LBA);
           if( dst_disk==src_disk ) dst_disk->changed=1;
           else 
             {
              sort_disk_info(dst_disk,&z);
              dst_part=&(dst_disk->part[z]);
              if( write_disk_info(dst_disk)== -1 )
                {
                 show_error(ERROR_SAVE_PART_TABLE);
                 continue;
                }
             }
          }

        if( src_part->num_sect != dst_part->num_sect )
          {
           i=dialog_box("Warning!!! Destination partition size differs from source!!!",
           	     " Proceed anyway ", " Cancel ", 0 );
           if( i!=1 ) continue;
          }
	free_part_info(dst_part);

        int x=copy_part(src_disk,src_part,dst_disk,dst_part);
        if( x==0 ) show_error("Copying have been completed successfully.");
	else if( x!=EV_CANCEL ) 
	    show_error("Error has occured during the copying!!!");

	if( dst_part->flags & OP_VALIDATE )
	  {
            OP_HANDLER( OP_VALIDATE, dst_disk, dst_part, 0 );
            if( fs_desc[(dst_part)->fs_num].handler==fat_handler )
              {
               if( BTS_LOADED(dst_part) && dst_part->part_misc_info!=0 )
                 {
                  FatInfo *fi=(FatInfo *)dst_part->part_misc_info;
                  fi->rel_sect = dst_part->rel_sect;
                  fi->num_sect = dst_part->num_sect;
                  fi->drive_num = dst_disk -> hd_num;
                  save_fat_boot_sect(dst_disk,dst_part,fi);
                  OP_HANDLER( OP_VALIDATE, dst_disk, dst_part, 0 );
                 }
              }
          }
        break;
       }
     else if( up_down_navigations(k,&top,&act,n,N) )
       {
        /* One of {Up, Down, Home, End, PgUp, PgDown} keys */
       }
    }/* while(1) */

 #undef n
}/* select_and_copy_part */
