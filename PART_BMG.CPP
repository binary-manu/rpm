#define PART_BMG_C
#include "part.h"
//#include "graph.h"
//#include "window.h"

#define BG_COLOR 0
#define DG_COLOR 12


#define BmX 36
#define BmY  1

void make_boot_menu(DiskInfo *sdi);
void simulate_compact_bm(DiskInfo *di);
//int simulate_gui_thing(DiskInfo *di);

int bootable(Part *p)
{
 return ( !UNUSED(p) && !EXTENDED(p) &&
          !(p->fs_type==FS_BM) &&          /* boot manager partition */
       // !(p->fs_type==0x8201) &&           /* linux swap partition */
          !(p->flags & P_UNHIDABLE) &&       /* hidden FAT partition */
          !((p->flags & P_HIDABLE) && LOGICAL(p) ) ); /* logical FAT */
}


void boot_manager( int hd )
{
 char tmp[512];
 int i, n, r, act, boot_disk, boot_part;
 DiskInfo *boot_di, *di = get_disk_info(hd);

 make_boot_menu(di);

 while(1)
    {
     gui_restore();
 cprintf(White,1,25," Press 0 to run Setup... ");
     r=boot_manager_menu(di);
     act=di->act_choice;
     if( r==2 )
       {
        partition_manager(hd);
        make_boot_menu(di);
        continue;
       }
     boot_disk = di->menu[act].boot_disk;
     boot_part = di->menu[act].boot_part;
     boot_di   = get_disk_info(boot_disk);
     if( r==0 ) /* boot floppy */
       {
        if( floppy_disk_read(0,0,tmp,1)==-1 )
          {
           show_error("Error reading boot sector from floppy");
           continue;
          }
        break;
       }
     if( read_part( boot_di, &boot_di->part[boot_part], 0, tmp, 1 )==-1 )
       {
        show_error("Error reading boot sector");
        continue;
       }
     break;
    }

 // prepare_boot_choice(act); 
 if( boot_part!=0 && !LOGICAL(&boot_di->part[boot_part]) )
   {
    for( i=0 ; i< boot_di->num_part ; i++ )
      boot_di->part[i].tag=0;
    boot_di->part[boot_part].tag=0x80;
   }

 write_disk_info(di);
 write_disk_info(boot_di);

 copy_to_zero_sel( 0x7C00, tmp, 512 );
 set_on_exit_jump( 0x7C00, r==0?0:boot_disk );
#ifdef MT4P
// MT4P: Muthu changed jump(0x07C00000) to jump(0x7C00)
// the old jump() was clearly a bug. No need to keep it anymore.
#endif

}/* boot_manager */



void make_boot_menu(DiskInfo *sdi)
{
 char tmp[96];
 int hd, i, m;
 DiskInfo *di;
 Part *p;
 Menu *menu=sdi->menu;

 m=0;
 memset(menu,0,M_ROWS*sizeof(Menu));
 
 strncpy(sdi->bm_title,"Boot Menu",39);

 sdi->act_choice = 0;
 
 for( hd=0x80 ; hd < sdi->num_disks+0x80 ; hd++ )  /* scanning all disks */
    {
     di=get_disk_info(hd);
     for( i=1 ; i < di->num_part ; i++ )     /* scanning all partitions  */
        {
         if( i==0 && hd == sdi->hd_num ) continue;    /* don't boot self */
         p=&di->part[i];
         if( !bootable(p) ) continue;
         if( sdi->act_choice == 0 && p->tag!=0 ) sdi->act_choice = m+1;
         sprintf(menu[m].menu_name,"Disk %d / %s", hd-127, fs_desc[p->fs_num].name);
         menu[m].boot_disk=hd;
         menu[m].boot_part=i;
         m++;
#ifdef MT4P
	 if (m == M_ROWS) break;
#endif
        }
#ifdef MT4P
     if (m == M_ROWS) break;
#endif
    }

 sdi->num_menus = m;
 sdi->act_choice--;
 
}/* make_boot_menu */



#ifdef DISK_SIM
void boot_manager_simulation(DiskInfo *di)
{
 char tmp[4000];
 save_window(1,1,80,25,tmp);
 clear_window(White,1,1,80,25); /* clear screen */
 border_window(BrWhite,1,1,27,3,MAIN_BORDER);
 write_string(Yellow,2,2," Boot Manager Simulation ");
 if( di->bm_type==0 )
   {
    simulate_compact_bm(di);
   }
 else if( di->bm_type==2 ) /* GUI */
   {
    int x=di->act_choice;
    make_boot_menu(di);
    //simulate_gui_thing(di);
    show_error("Sorry. Graphic inteface is disabled.");
    di->act_choice=x;
   }
 else
   {
    int x=di->act_choice;
    move_cursor(1,26);
    make_boot_menu(di);
    boot_manager_menu(di);
    while( kb_hit() ) get_key();
    di->act_choice=x;
   }
 load_window(1,1,80,25,tmp);
}
#endif

int boot_manager_menu(DiskInfo *di)
{
 char tmp[96], flag;
 int i, n, n_scr, k, top, act;
 Menu *menu = di->menu;
#ifdef MT4P
 int j;
#endif
// if( di->bm_type==2 ) return simulate_gui_thing(di);
 
 top=0;
 act=di->act_choice;
 n = di->num_menus;
 n_scr = min(n,16);

#ifdef MT4P
 if( n==0 ) return 2; /* let them run partition manager
                          to create some bootable partitions */
#else
 if( n==0 ) return -1; /* let them run partition manager
                          to create some bootable partitions */
#endif

 border_window(BrGreen,BmX,BmY,45,n_scr+6,MAIN_BORDER);
 cprintf(Yellow,BmX+3,BmY+1, di->bm_title[0]!=0 ? di->bm_title : "(no title)");
 flag=0; /* enable timeout */
#ifdef MT4P
 if( di->ipl_default==-1 ) flag=2; /* default action is to prompt user */
#else
 if( di->ipl_default==' ' ) flag=2; /* default action is to prompt user */
#endif
 
#ifndef MT4P
 if( act>=n ) act=0;
#endif

 while(1)
    {
#ifdef MT4P
     if( act>=n ) act=n-1;
     if(top > act) top = act;
     if(top + n_scr <= act) top = act - n_scr + 1;
     if(top + n_scr > n) top = (n > 16) ? (n-16) : 0;

     j = top;
     for( i=0 ; i < n_scr ; i++, j++ )
        {
         sprintf(tmp, " %2d  %-35s ", j+1, menu[j].menu_name);
         write_string((j==act ? BakCyan : BrWhite),BmX+2,BmY+3+i,tmp);
        }
#else
     for( i=top ; i < top+n_scr ; i++ )
        {
         sprintf(tmp, " %2d  %-35s ", i+1, menu[i].menu_name);
         write_string((i==act ? BakCyan : BrWhite),BmX+2,BmY+3+i,tmp);
        }
#endif  
     //preview_mbrs(di,act);
     
     if( flag==1 ) 
       {
        get_key();
        flag=2;
       }

     if( flag==0 ) 
       {
        cprintf(Yellow,BmX+2,BmY+n_scr+4,"ESC");
        cprintf(BrCyan,BmX+5,BmY+n_scr+4," - Boot ");

        for( i=0 ; i<30 ; i++ )
          cprintf(BrWhite,BmX+13+i,BmY+n_scr+4,"");

        for( i=0 ; i<30 ; i++ )
           {
            cprintf(BrWhite,BmX+13+i,BmY+n_scr+4,"ú");
            for( k=0 ; k<10 ; k++ )
               {
                if( kb_hit() )break;
                delay(di->ipl_timeout*100/30);
               }
            if( kb_hit() )break;
           }
        k=kb_hit();
        if( k==0 ) break; /* timeout */
        flag=1;
       }
     else
       {
        cprintf(BrCyan,BmX+2,BmY+n_scr+4,"Press Enter to boot highlighted menu item");
        cprintf(Yellow,BmX+8,BmY+n_scr+4,"Enter");
        k=get_key();
       }

     di->act_choice=act;

     if( k==27 || k==13 )
       {
        if( flag==1 ) get_key();
        break;
       }

     if( k=='0' ) /* run Partition Manager */
       {
        if( flag==1 ) get_key();
        return 2;
       }

     if( k=='A' || k=='a' ) /* boot from floppy */
       {
        if( flag==1 ) get_key();
        return 0;
       }

     if( k>='1' && k<='9' )
       {
        if( flag==1 ) get_key();
        if( k<='0'+n )
          {
           top=0;
           act=k-'1';
           flag=0;
          }
        else if( flag==1 ) flag=0;
        continue;
       }

     if( k==' ' || up_down_navigations(k,&top,&act,n_scr,n) )
       {
        continue;
       }
     
     break; /* unknown key - will pass it to OS */
    }
 
 return 1;
}/* boot_manager_menu */

#ifdef DISK_SIM
void simulate_compact_bm(DiskInfo *di)
{
 int i, k, y=4;
 char act='1', flag;
 
 write_block(White,43,1,"Keys: Space - pause booting and wait\n"
                        "        ESC - boot now (don't wait)\n"
                        "         A  - boot from the floppy\n"
                        "        1-4 - switch to partition 1-4\n"
                        "        TAB - switch to the next disk\n"
                        "  All other keys will be passed to OS");

 if( di->ipl_default!=0 ) act=di->ipl_default;

 cprintf(White,1,y,"Booting HD%d/",di->hd_num-0x80+1);

 while(1)
    {
     if( y==25 ) break;
     cprintf(White,13,y,"%c ..............................", act );

     for( i=0 ; i<30 ; i++ )
        {
         cprintf(White,15+i,y," ");
         move_cursor(16+i,y);
         if( !kb_hit() ) delay(di->ipl_timeout*1000/30);
        }
     y++;

     if( !kb_hit() ) break;
     
     flag=0; /* first time in the loop */
     
     while(1)
        {
         if( flag==0 )
           {
            cprintf(White,1,y,"Booting HD%d/",di->hd_num-0x80+1);
            move_cursor(13,y);
           }
         k=get_key();
         if( k=='a' ) k='A';
         if( k>='0' && k<='4' || k=='A' || k==9 ) /* Tab */
           {
            act=k;
            if( k==9 ) act=26;
            flag=1;
            break;
           }
         if( k!=' ' && flag==0 ) break;
         flag=1;
        }
     if( flag==0 || act=='0' ) break;
    }
 if( act!='0' && act!=26 )
   {
    cprintf(White,1,y,"Starting MS-DOS...");
    move_cursor(19,y);
   }
 y=min(y+2,25);
 cprintf(White,1,y,"Press any key to continue ... ");
 move_cursor(30,y);
 get_key();
}/* simulate_compact_bm */
#endif


int prepare_partitions( DiskInfo *di, PartLayout *pl )
{
 return pl->part_bitmap[0]+di->flags;
}/* prepare_partitions */

#if 0


class MenuWindow : public Window
   {
    public:
      int timer;
      int top;
      int act;
      int n_scr;
      Menu *menu;
    MenuWindow(CScreen *s, int x, int y, int nn_scr, Menu *m, char *t);
    void ReDraw(int active);
    void ReDrawMenu(void);
    void ReDrawTimer(void);
   };

MenuWindow::MenuWindow(CScreen *s, int x, int y, int nn_scr, Menu *m, char *t):
                         Window(s,x,y,240,36+nn_scr*16+30,t)
{
 menu=m;
 n_scr=nn_scr;
 timer=0;
}

void MenuWindow::ReDraw(int active)
{
 int i;
 char tmp[64];
 screen->hold();
 Window::ReDraw(active);
 screen->draw_3d_border(DOWN_3D,x+4,y+26,w-8,h-30-30);
 ReDrawMenu();
// ReDrawTimer();
 screen->draw_3d_border(GRAY_3D,x+ 6,y+h-30,44,24);
 screen->draw_3d_border(GRAY_3D,x+56,y+h-30,50,24);
 screen->draw_string(BLACK,x+12,y+h-25,ibm8x16,1,"Boot");
 screen->draw_string(BLACK,x+61,y+h-25,ibm8x16,1,"Setup");
 screen->release();
}

void MenuWindow::ReDrawMenu(void)
{
 int i;
 char tmp[64];
 screen->hold();
 for( i=top ; i < top+n_scr ; i++ )
    {
     screen->fill_rect(15,x+8,y+29+i*16,w-16,16);
     sprintf(tmp, "%2d %-35s ", i+1, menu[i].menu_name);
     screen->draw_string((i==act ? 0 : 9),x+8,y+30+i*16,ibm8x16,0,tmp);
     if(i==act) screen->draw_rect(8,x+8,y+29+i*16,w-16,16);
    }
 screen->release();
}


#if 0
void MenuWindow::ReDrawTimer(void)
{
 screen->hold();
 screen->draw_digit(DG_COLOR,x+w-20,y+h-12,timer);
 screen->release();
}
#endif

int simulate_gui_thing(DiskInfo *di)
{
// int active=1, px, py;
 CScreen s;

/* --------------------------- */
 char tmp[96], flag;
 int i, n, n_scr, k;
 Menu *menu = di->menu;

 n = di->num_menus;
 n_scr = min(n,16);

 MenuWindow w(&s,di->gui_x,di->gui_y,n_scr,menu,
 		 di->bm_title[0]!=0 ? di->bm_title : "(no title)" );
 w.top=0;
 w.act=di->act_choice;
 if( w.act>=n ) w.act=0;

 s.hold();
 s.clear(BG_COLOR);
 w.ReDraw(1);
 s.release();

 if( n==0 ) return -1; /* let them run partition manager
                          to create some bootable partitions */

 flag=0; /* enable timeout */
 if( di->ipl_default==' ' ) flag=2; /* default action is to prompt user */
 
/* --------------------------- */

 while(1)
    {
    /* --------------------------- */
//    s.hold();
//    w.ReDraw(1);
//    s.release();
#if 0
     //preview_mbrs(di,act);
     
     if( flag==1 ) 
       {
        get_key();
        flag=2;
       }

     if( flag==0 ) 
       {
        cprintf(Yellow,BmX+2,BmY+n_scr+4,"ESC");
        cprintf(BrCyan,BmX+5,BmY+n_scr+4," - Boot ");

        for( i=0 ; i<30 ; i++ )
          cprintf(BrWhite,BmX+13+i,BmY+n_scr+4,"");

        for( i=0 ; i<30 ; i++ )
           {
            cprintf(BrWhite,BmX+13+i,BmY+n_scr+4,"ú");
            for( k=0 ; k<10 ; k++ )
               {
                if( kb_hit() )break;
                delay(di->ipl_timeout*100/30);
               }
            if( kb_hit() )break;
           }
        k=kb_hit();
        if( k==0 ) break; /* timeout */
        flag=1;
       }
     else
       {
        cprintf(BrCyan,BmX+2,BmY+n_scr+4,"Press Enter to boot highlighted menu item");
        cprintf(Yellow,BmX+8,BmY+n_scr+4,"Enter");
        k=get_key();
       }
#endif
     di->act_choice=w.act;

     s.get_event(EV_KEY|EV_MOUSE);

     if( s.ev_type==EV_KEY )
       {
        if( s.m_key==27 || s.m_key==13 || s.m_key==0x4400 ) /* ESC/ENRTER/F10 */
          {
           //if( flag==1 ) get_key();
           break;
          }

        if( s.m_key=='0' ) /* run Partition Manager */
          {
           //if( flag==1 ) get_key();
           return 2;
          }

        if( s.m_key=='A' || s.m_key=='a' ) /* boot from floppy */
          {
           //if( flag==1 ) get_key();
           return 0;
          }
#if 0
        if( k>='1' && k<='9' )
          {
           if( flag==1 ) get_key();
           if( k<='0'+n )
             {
              top=0;
              act=k-'1';
              flag=0;
             }
           else if( flag==1 ) flag=0;
           continue;
          }
#endif
        if( s.m_key==' ' )
          {
           continue;
          }
        
        if( up_down_navigations(s.m_key,&(w.top),&(w.act),n_scr,n) )
          {
           w.ReDrawMenu();
           continue;
          }
     
        //break; /* unknown key - will pass it to OS */
    /* --------------------------- */
       } /* EV_KEY */
     else if( s.ev_type==EV_MOUSE )
       {
        if( s.m_x >= w.x+w.w-22 &&     /* Mouse on 'X' button */
            s.m_x <  w.x+w.w-6  &&
            s.m_y >= w.y+6 &&
            s.m_y <  w.y+22 )
          {
           if( s.m_left==1 ) /* click */
             {
              w.x_depressed=1;
              w.ReDraw(1);
              while(1)
                 {
                  s.get_event(EV_MOUSE);
                  if( s.m_left==0 ) break;
                  if( s.m_x >= w.x+w.w-22 &&     /* Mouse still on 'X' button */
                      s.m_x <  w.x+w.w-6  &&
                      s.m_y >= w.y+6 &&
                      s.m_y <  w.y+22 )
                     {
                      if( w.x_depressed==0 )
                        {
                         w.x_depressed=1;
                         w.ReDraw(1);
                        }
                     }
                   else
                     {
                      if( w.x_depressed==1 )
                        {
                         w.x_depressed=0;
                         w.ReDraw(1);
                        }
                     }
                 }
              if( s.m_x >= w.x+w.w-22 &&     /* Mouse still on 'X' button */
                  s.m_x <  w.x+w.w-6  &&
                  s.m_y >= w.y+6 &&
                  s.m_y <  w.y+22 )
                 {
                  break;
                 }
               else
                 {
                  w.x_depressed=0;
                  w.ReDraw(1);
                 }
             }/* click */
           }/* mouse on 'X' button */
         else if( s.m_x >= w.x+4 &&          /* Mouse on window title */
                  s.m_x <  w.x+w.w-4 &&
                  s.m_y >= w.y+4 &&
                  s.m_y <  w.y+24 )
           {
            if( s.m_left==1 ) /* click */
              {
               int mx=s.m_x, my=s.m_y;
                  
               while(1)
                  {
                   s.get_event(EV_MOUSE);
                   if( s.m_left==0 ) break;
                   s.hold();
                   s.clear(BG_COLOR);
                   w.ReDraw(1);
                   s.draw_rect(7,w.x+(s.m_x-mx),w.y+(s.m_y-my),w.w,w.h);
                   s.release();
                  }
               w.x = w.x+(s.m_x-mx);
               w.y = w.y+(s.m_y-my);
               di->gui_x = w.x;
               di->gui_y = w.y;
               di->changed=1;
               s.hold();
               s.clear(BG_COLOR);
               w.ReDraw(1);
               s.release();
              }/* click */
           }/* mouse on title */
         else if( s.m_x >= w.x+6 &&          /* Mouse on "Boot" button */
                  s.m_x <  w.x+6+44 &&
                  s.m_y >= w.y+w.h-30 &&
                  s.m_y <  w.y+w.h-30+24 )
           {
            if( s.m_left==1 ) /* click */
              {
               di->act_choice=w.act;
               return 1;
              }
           }
         else if( s.m_x >= w.x+56 &&          /* Mouse on "Setup" button */
                  s.m_x <  w.x+56+50 &&
                  s.m_y >= w.y+w.h-30 &&
                  s.m_y <  w.y+w.h-30+24 )
           {
            if( s.m_left==1 ) /* click */
              {
               //s.draw_string(BLACK,300,460,ibm8x16,0,"I wish we could run setup");
               return 2;
              }
           }
         else
           {
            for( i=0 ; i<n_scr ; i++ )
             if( s.m_x >= w.x+8 &&          /* Mouse on i-th item */
                 s.m_x <  w.x+w.w-16 &&
                 s.m_y >= w.y+29+i*16 &&
                 s.m_y <  w.y+29+i*16+16 )
               {
                w.act=w.top+i;
                if( s.m_left==1 )
                  {
                   di->act_choice=w.act;
                   return 1;
                  }
                w.ReDrawMenu();
               }
            }
        }/* EV_MOUSE */
    }/* while(1) */
 
 return 1;
}
#endif
