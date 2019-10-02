#define   PART_C
#include "part.h"

#include "GUI.H"
#include "CMOS.H"
#include "MBR.H"

#ifndef DISK_SIM
void ask_and_install_to_floppy(void)
{
 if( dialog_box("Installing Partition Manager to a floppy. Insert formatted floppy into A:",
     " Copy PART.EXE and update boot sector on A: ",
     " Cancel ", 0 )==1 ) install_to_floppy(); 
}
#endif

void main(int argc, char **argv)
{
 int hd=0x80;

#ifndef DISK_SIM 
 if( get_detected_os()==5 ) // Windows NT
   {
    ask_and_install_to_floppy();
    exit(0);
   }
#endif

 if( argc==2 && argv[1][0]>='1' && argv[1][0]<='8' )
   {
    hd+=argv[1][0]-'1';
   }

 if( argc>=2 )
   {
    start_cmd(hd,argc,argv);
   }
 else 
   {
    gui_init();
    if( get_detected_os()==0 && /* bare_system */
        get_startup_hd()!=0 && /* floppy */
        get_disk_info(hd=get_startup_hd())->bm_type!=0 )
      boot_manager(hd);
    else
      partition_manager(hd);
    gui_done();
   }
 exit(0);
}/* main */


#define FORCE(x)    ((force) |=  (x))
#define RESET(x)    ((force) &= ~(x))
#define NEED_TO(x)  ((force)  &  (x))

#define GET_DISK_INFO     0x0001
#define REDRAW_SCREEN     0x0002
#define REDRAW_PART_TABLE 0x0004
#define READ_PART_TABLE   0x0008
#define SAVE_PART_TABLE   0x0010
#define SORT_PART_TABLE   0x0020
#define SORT_PART_TABLE_LATER   0x0040


void partition_manager(int hd)
{
 char *hint, *mesg, *warn, tmp[120];
 int i, j, k, n, n_pri, mode, num_disks;
 int top, row, col, old_top, old_row, old_col, force;
 Sector sv_start_cyl, sv_last_sect;
 Part *part, part2[4], *p;
 DiskInfo *di;

 mode=MODE_CHS;

 top=0;
 row=0;
 col=1;
 warn=0;
 mesg=0;
 force=0;

 FORCE(GET_DISK_INFO);

#ifndef DISK_SIM
 if( get_detected_os() >= 3 )
  warn="Warning! Modifying partitions under Windows may cause data corruption!";
#endif

 while(1)
    {
     if( NEED_TO(GET_DISK_INFO) ) 
       {
        //redraw_main_hint(HINT,MESG_GETTING);
        if( (di=get_disk_info(hd))==0 )
          {
           show_error(ERROR_DISK_INFO);
           if( hd==0x80 ) break;
           hd=0x80;
           continue;
          }
        part=di->part;
        RESET(GET_DISK_INFO);
        FORCE(REDRAW_SCREEN);
       }

     if( NEED_TO(REDRAW_SCREEN) ) 
       {
        redraw_main_screen(di,mode);
        RESET(REDRAW_SCREEN);
        FORCE(REDRAW_PART_TABLE);
       }
     
     if( NEED_TO(READ_PART_TABLE) )
       {
        redraw_main_hint(HINT,MESG_READING);
        if( read_disk_info(di)==-1 )
          {
           show_error(ERROR_READ_PART_TABLE);
           if( hd==0x80 ) break;
           hd=0x80;
           FORCE(GET_DISK_INFO);
           continue;
          }
        RESET(READ_PART_TABLE);
        FORCE(REDRAW_PART_TABLE);
       }

     if( NEED_TO(SORT_PART_TABLE) )
       {
        sort_disk_info(di,&row);
        RESET(SORT_PART_TABLE);
        FORCE(REDRAW_PART_TABLE);
       }

     n=di->num_part;

     if( row >= n  ) row = n-1;
     if( top > row ) top = row;
     if( top+8 <= row ) top = row-7;
     if( top+8 > n   ) top = (n>8) ? (n-8) : 0;

     p = &part[row];

     if( top != old_top  ||  row != old_row  ||  col != old_col ) 
       {
        if( row != old_row )  clear_preview_box(0);
        if( col==0 && LOGICAL(p) )  col = 1; 
        old_top = top;
        old_row = row;
        old_col = col;
        FORCE(REDRAW_PART_TABLE);
       }

     if( NEED_TO(REDRAW_PART_TABLE) )
       {
        n_pri=prepare_mbr_records( part, part2 );
        redraw_part_table( top, row, col, part+top, 8, mode );
        redraw_part_table(   1,  -1,  -1, part2,    4, MODE_MINI );
        redraw_main_prompt( di->changed );

        for( i=0 ; i<n ; i++ )
         if( !VALID(&part[i]) )
           {
            if( i>=top && i<top+8 ) continue;
            clear_error( (i<top)?(-1):(+1) );
            break;
           }
        if( i==n ) clear_error(0);

        if( p->flags & P_ERR_ZERO      ) popup_error( ERR_ZERO      );
        if( p->flags & P_ERR_RANGE     ) popup_error( ERR_RANGE     );
        if( p->flags & P_ERR_OVERLAP   ) popup_error( ERR_OVERLAP   );
        if( p->flags & P_ERR_LOG_ORPH  ) popup_error( ERR_LOG_ORPH  );
        if( p->flags & P_ERR_LOG_STRU  ) popup_error( ERR_LOG_STRU  );
        if( p->flags & P_ERR_CANT_READ ) popup_error( ERR_CANT_READ );
        if( p->flags & P_ERR_BOOT_SECT ) popup_error( ERR_BOOT_SECT );


        RESET(REDRAW_PART_TABLE);
       }
     
#ifdef MT4P
     if( n_pri> P_ROWS - 1 )
       warn="You cannot have more than 31 primary partitions with this version";
#else
     if( n_pri>4 )
       warn="You cannot have more than four primary partitions with this version";
#endif

          if( warn!=0 ) redraw_main_hint( WARNING, warn );
     else if( mesg!=0 ) redraw_main_hint( MESSAGE, mesg );
     else 
       {
             if( row==0 ) hint = HINT_MBR;
        else if( col==0 ) hint = HINT_ROW;
        else if( col==1 ) hint = UNUSED(p) ? HINT_WIZ : HINT_INS;
        else
          {
           hint=tmp;
           if( mode==MODE_CHS )
              sprintf(tmp,HINT_CHS, di->num_cyls - 1,
                                    di->num_heads - 1,
                                    di->sect_per_track );
           else
              sprintf(tmp,HINT_LBA, di->total_sects - 1,
                                    di->total_sects - p->rel_sect );
          }

        if( di->bm_type!=0 ) /* boot manager */
          {
           for( i=0 ; i<n ; i++ )
            if( part[i].fs_type==FS_BM ) break;
           if( i==n ) hint="Press INS and create partition for the Boot Manager";
          }
        
        redraw_main_hint( HINT, hint );
       }

     warn=mesg=hint=0;

     OP_HANDLER( OP_PREVIEW, di, p, 0 );
     
     sv_start_cyl = p->start_cyl;
     sv_last_sect = p->last_sect;

     k = edit_part_col(p,col,mode);

     if( k==-1 ) /* numeric column was modified - we need to recalculate */
       {
        if( mode==MODE_CHS && p->start_cyl != sv_start_cyl )
          {
           if( p->start_cyl==0 && p->start_head==0 ) p->start_head=1;
           if( sv_start_cyl==0 && p->start_head==1 ) p->start_head=0;
          }
        recalculate_partition(di,p,mode);
        if( row>0 && LOGICAL(p) && EXTENDED(p-1) &&
            sv_last_sect == (p-1)->last_sect &&
            p->last_sect != (p-1)->last_sect &&
          !( p->last_sect < sv_last_sect && !LOGICAL(p-1) ) )
          {
           (p-1)->num_sect = p->last_sect + 1 - (p-1)->rel_sect;
           recalculate_partition(di,p-1,MODE_LBA);
           //if( (p-1)->end_cyl>1023 )
           //    (p-1)->fs_type=0x0F00; /* Extended LBA */
           //find_fs_num(p-1);

          }
        di->changed = 1;
        if( col==2 && (sv_start_cyl == p->start_cyl/10 ||
                       p->start_cyl == sv_start_cyl/10 ) )
          FORCE(SORT_PART_TABLE_LATER);
        else
          FORCE(SORT_PART_TABLE);
        FORCE(REDRAW_PART_TABLE);
        continue;
       }

    if( NEED_TO(SORT_PART_TABLE_LATER) )
      {
       FORCE(SORT_PART_TABLE);
       RESET(SORT_PART_TABLE_LATER);
      }

     if( k>='a' && k<='z' )
       k -= 'a'-'A';       /* converting to uppercase */
     
     if( k==0x3B00 )  /* F1 - Help */
       {
        show_help(0);
        continue;
       }

#ifdef MT4P
     if ( k == 0x4100) { /* F7 - Restore BM MBR */
        write_mbr(di);
        dialog_box("MBR written. Please reboot the system WITHOUT saving", 0);
        continue;
     }
#endif

#ifdef DISK_SIM
     if( k==0x4400 ) /* F10 - boot manager simulation */
       {
        boot_manager_simulation(di);
        FORCE(REDRAW_SCREEN);
        continue;
       }
#endif


	if( k=='S' || k=='L' )
	{
		using namespace MBR;
		using namespace GUI;
		
//		show_error("Not implemented yet. Please, run \"part -p -r > some_file\" and print it.");
		if (get_detected_os() == 0)
		{
			show_error("Can't handle files while running on bare metal");
			continue;
		}
		
		if (di->changed)
		{
			show_error("Before working with the MBR you should either save "
				"or discard changes");
			continue;
		}
		
		bool status;
		char path[256] = {0};
		size_t n = sizeof path - 1;
		int res = GUI::MBRDialog(path, &n, &status);
		
		if (res == EV_OK && status == true)
		{
			if (StoreMBR(path, di->part[0].part_boot_sect) != 0)
				show_error("Error while saving the MBR");
		}
		else if (res == EV_OK && status == false)
		{
			int userSays = dialog_box("Do you understand that this can "
				"potentially DESTROY your data\?",
				"Yes, old man", "No, err... wait!", static_cast<void*>(0));
				
			if (userSays != 1)
				continue;
			
			char mbr[MBR_BUFFER_SIZE];
			if (LoadMBR(path, mbr) != 0)
			{
				show_error("An error occurred while reading from file. This is "
					"not harmful.");
				continue;
			}
			
			if (write_boot_sect(di, &di->part[0], &mbr, 1) == -1)
				show_error("PANIC: An error occurred while writing the MBR "
					"to disk!!!");
			else
				FORCE(READ_PART_TABLE);
			continue;
		}
	}

	if (k == 'M') 
	{
		using namespace CMOS;
		using namespace GUI;
		using namespace MBR;
		
		/* CMOS save/restore */
		if (get_detected_os() == 0)
		{
			show_error("Can't handle files while running on bare metal");
			continue;
		}
		
		bool status;
		char path[256] = {0};
		size_t n = sizeof path - 1;
		RTCTimeSource timeSource;
		RTCTime theTime(RTCTime::Format_BCD, RTCTime::HourRange_24);
		int res = CMOSDialog(path, &n, &status, timeSource, &theTime);
		char cmosData[CMOS_BUFFER_SIZE] = {0};
		
		if (res == EV_OK && status == true)
		{
			ReadCMOSData(cmosData);
			if (StoreMBR(path, cmosData) != 0)
				show_error("Error while saving CMOS data");
		}
		else if (res == EV_OK && status == false)
		{
			int userSays = dialog_box("This "
				"will OVERWRITE all yout BIOS settings",
				"Yes, old man", "No, err... wait!", static_cast<void*>(0));
				
			if (userSays != 1)
				continue;
			
      if (CMOS::ReadPatchAndWriteCMOSData(path, timeSource, theTime) != 0) {
				show_error("An error occurred while reading from file. This is "
					"not harmful.");
				continue;
      }
		}
	}
       
     if( col==1 && k==13 )
       {
        if( UNUSED(p) )
          {
           part_new_wiz( di, &row );
           FORCE(REDRAW_SCREEN + SORT_PART_TABLE + REDRAW_PART_TABLE);
          }
        else if( p->flags & OP_SETUP )
          {
           OP_HANDLER( OP_SETUP, di, p, 0 );
           FORCE(SORT_PART_TABLE + REDRAW_PART_TABLE);
          }
        continue;
       }

     if( k==0x3C00 || k==27 || k==0x3F00 )/* F2, ESC or F5 */
       {
        //------- Make sure that there is active partition
        int x, n_pri_non_ext=0;
        for( x=0 ; x<n ; x++ )
           {
            if( part[x].tag!=0 ) break;
            if( !EXTENDED(&part[x]) && !LOGICAL(&part[x]) &&
                part[x].fs_type!=FS_MBR && part[x].fs_type!=0 )
              n_pri_non_ext++;
           }
        if( n_pri_non_ext>0 && x==n ) // Active partition not found
          {
           i=dialog_box("There is no active partition on this disk!!!",
                        " Proceed anyway "," No. Let me go back. ", 0 );
           if( i!=1 ) continue;
          }
       }

     if( di->changed && (k==27 || k==0x3F00) )/* ESC or F5 */
       {
        i=dialog_box("You have made changes since the last save",
                     " Save MBR "," Don't save "," Cancel ", 0 );
             if( i==0 || i==3 ) continue;
        else if( i==2 && (k!=27 || get_detected_os()==0) )
                                read_disk_info(di); /* Undo the changes */
        else if( i==1 ) FORCE(SAVE_PART_TABLE);
       }

     if( di->changed && (k=='F' || k=='R') )
       {
        i=dialog_box("You cannot do this operation until you save changes",
                     " Save MBR and proceed "," No. Let me go back. ", 0 );
        if( i==0 || i==2 ) continue;
        if( i==1 ) FORCE(SAVE_PART_TABLE);
       }

     if( NEED_TO(SAVE_PART_TABLE) || k==0x3C00 )  /* F2 - Save */
       {
        RESET(SAVE_PART_TABLE);
        
        for( i=0 ; i<n ; i++ )
         if( !VALID_LOC(&part[i]) ) break;
        
#ifdef MT4P
	if( i!=n || n_pri> P_ROWS - 1 )
#else
        if( i!=n || n_pri>4 )
#endif
          {
           i=dialog_box("There are invalid records. Do you want to save them?",
                        " Yes. Save them as they are. "," No. Let me go back. ", 0 );
           if( i==0 || i==2 ) continue;
          }

        if( write_disk_info(di)==-1 )
          {
           show_error(ERROR_SAVE_PART_TABLE);
           continue;
          }

        mesg=MESG_PART_TABLE_SAVED;
        FORCE(SORT_PART_TABLE);    // ???
        if( k==0x3C00 ) /* F2 */
        continue;
       }

     if( k==27 ) break; /* ESC */


     if( k==0x3D00 ) /* F3 - Undo */
       {
        row=0;
        FORCE(READ_PART_TABLE);
        continue;
       }

     if( k==0x3E00 ) /* F4 - Mode */
       {
        if( mode==MODE_CHS )
          {
           mode = MODE_LBA;
                if( col>=5 ) col=3;
           else if( col>=2 ) col=2;
          }
        else
          {
           mode = MODE_CHS;
           if( col==3 ) col=5;
          }
        FORCE(REDRAW_SCREEN);
        continue;
       }

     if( k==0x3F00 ) /* F5 - Next HD */
       {
        row=0;
        hd=(hd-128+1)%di->num_disks + 128;     /* go to the next disk */
        FORCE(GET_DISK_INFO);
        continue;
       }

     if( k=='V' ) /* Verify */
       {
        int x;
        unsigned long bbt[9];
        x=verify_part(di,p,bbt,9);
             if( x==EV_OK     ) mesg = MESG_VERIFY_OK;
        else if( x==EV_CANCEL ) mesg = WARN_VERIFY_CANCEL;
#ifndef DISK_SIM
        else show_bad_blocks(di,p,bbt,x);
#endif
        FORCE(REDRAW_SCREEN);
        continue;
       }

     if( k=='F' ) /* Format */
       {
        if( p->flags & OP_FORMAT )
          {
           OP_HANDLER( OP_FORMAT, di, p, 0 );
           OP_HANDLER( OP_VALIDATE, di, p, 0 );
           FORCE(REDRAW_SCREEN);
          }
        continue;
       }

#ifndef DISK_SIM
     if( k=='A' ) /* Install boot manager to floppy */
       {
        ask_and_install_to_floppy();
        FORCE(REDRAW_SCREEN);
        continue;
       }
#endif

#if 0
     if( k=='Z' ) /* Zip */
       {
        zip_example(di,p);
        FORCE(REDRAW_SCREEN);
        continue;
       }
#endif

     if( k=='E' ) /* Clear partition */
       {
        if( dialog_box("Are you sure you want to ERASE everything on this partition?",
                      " No. Cancel ", " Yes. Fill it with zeros. ", 0 )==2 )
           {
            clear_partition(di,p);
            FORCE(SORT_PART_TABLE);
            FORCE(REDRAW_SCREEN);
           }
        continue;
       }

     if( k=='C' ) /* Copy partition */
       {
        select_and_copy_part(di,p);
        FORCE(SORT_PART_TABLE);
        FORCE(REDRAW_SCREEN);
        continue;
       }

     if( k=='D' ) /* Copy disk */
       {
        select_and_copy_disk(di);
        FORCE(REDRAW_SCREEN);
        continue;
       }

     /*----------------------- Navigation ----------------------*/
     
     if( k==9 || k==13 ) /* tab or enter */
       {
        col = (mode==MODE_CHS ? "\1\2\5\5\5\1\1\1" : "\1\2\3\1" ) [col];
        continue;
       }

     if( k==0x4B00 ) /* left */
       {
        if(col>0) col--;
        continue;
       }

     if( k==0x4D00 ) /* right */
       {
        if(col<(mode==MODE_CHS?7:3) ) col++;
        continue;
       }

     if( up_down_navigations(k,&top,&row,8,n) )
       {
        continue;
       }

     /*------------------- Editing ------------------------*/

     if( k==0x5200 ) /* ins */
       {
        if( row==0 ) /* MBR */
          {
           while( row<n && (!UNUSED(p) ||    /* search unused partition */
                         p->num_sect<=63) )  /* greater than one side    */
              { row++; p++; }
           if( row==n )    /* didn't find any */
             { row=0; continue; }
          }

        i=p->fs_type;

        select_fs_type(p);

        if( p->fs_type!=i )
          {
           di->changed=1;
           free_part_info(p);
          }

        if( i==0 && p->fs_type!=0 )  /* was unused, but now some FS */
          {
           col=2;
           part_ins_wiz(di,&row);
          }

        FORCE(REDRAW_SCREEN + SORT_PART_TABLE);
        continue;
       }

     if( EMPTY(p) || row==0 ) continue;
     if( UNUSED(p) ) continue;

     if( col==1 && k==0x5300 ) /* del */
       {
        free_part_info(p);
        memset(p,0,sizeof(Part));
        di->changed=1;
        FORCE(SORT_PART_TABLE);
        continue;
       }
       
     if( k=='X' )
       {
        p->flags ^= P_LOGICAL;
        di->changed=1;
        FORCE(SORT_PART_TABLE);
        continue;
       }

     if( col==0 && (k==' ' || k==0x5300 || k>='1' && k<='4' ) )
       {
#ifdef MT4P
	int tmp_mbr_row;

	tmp_mbr_row = p->mbr_row;
	p->mbr_row = ((k==' ' || k==0x5300) ? 0 : reset_mbr_row(part, p, k-'0'));
	if (p->mbr_row == 5) {
                warn="Row in use by an active partition; make it inactive & try";
                p->mbr_row = tmp_mbr_row;
        }
#else
        p->mbr_row = ((k==' ' || k==0x5300) ? 0 : (k-'0'));
#endif
        di->changed=1;
        FORCE(REDRAW_PART_TABLE);
        continue;
       }

     if( k=='B' )
       {
        if( p->tag==0 && (EXTENDED(p) || LOGICAL(p)) )
          {
//         warn="You cannot boot from extended or logical partitions with this version";
           continue;
          }

        p->tag=p->tag?0:0x80;
#ifdef MT4P
	if( p->tag!=0 ) {
#else
        if( p->tag!=0 )
#endif
         for( i=0 ; i<n ; i++ )
          if( i!=row ) part[i].tag=0;
#ifdef MT4P
         if (p->mbr_row == 0)
                warn="Please enter a row (in MBR) for this partition [default is 1]";
        }
#endif

          
        di->changed=1;
        FORCE(REDRAW_PART_TABLE);
        continue;
       }

     if( k=='H' || k=='h' ) /* Hide/Unhide */
       {
        if( p->flags & P_HIDABLE )
          {
           p->fs_type += 0x1000;
           find_fs_num(p);
           di->changed=1;
           FORCE(REDRAW_PART_TABLE);
          }
        else if( p->flags & P_UNHIDABLE )
          {
           p->fs_type -= 0x1000;
           find_fs_num(p);
           di->changed=1;
           FORCE(REDRAW_PART_TABLE);
          }
        continue;
       }

     sprintf(tmp,"key=%04X",k);
     write_string(White,70,5,tmp);
    }

 return;
}/* partition_manager */


int h_deflt( int op, DiskInfo *di, Part *p, char **argv )
{
 if( op==OP_PREVIEW   ) clear_preview_box( UNUSED(p) ? MESG_WIZARD :
                                                       MESG_UNSUPPORTED );
 if( di==0 || argv==0 ) return 0;      /* this will make compiler happy */
 return 0;
}/* h_dflt */



int ipl_handler( int op, DiskInfo *di, Part *p, char **argv )
{
 read_boot_sect(di,p,1);

 if( !BTS_LOADED(p) ) return 0;

 if( op==OP_VALIDATE || argv!=0 ) return 0;

 if( op==OP_PREVIEW ) redraw_ipl_options(di,-1);
 else /* OP_SETUP */    edit_ipl_options(di);
 
 return 0;
}/* ipl_handler */


int bm_handler( int op, DiskInfo *di, Part *p, char **argv )
{
 read_boot_sect(di,p,1);
 if( !BTS_LOADED(p) ) return 0;
 if( argv!=0 ) return 0;

 p->flags &= ~P_ERR_BOOT_SECT; /* assuming it is good */
 
 if( p->num_sect<945 )
   {
    p->flags |= P_ERR_BOOT_SECT;
    if( op==OP_PREVIEW )
       clear_preview_box("\nBoot Manager needs at least 945 sectors (500k)");
    return 0;
   }
 if( op==OP_VALIDATE ) return 0;
 if( op==OP_PREVIEW ) 
    clear_preview_box("\nPress F10 for Boot Manager simulation");

 return 0;
}/* bm_handler */


int ext_handler( int op, DiskInfo *di, Part *p, char **argv )
{
 int i;
 Mbr *mbr;
 Part part2[4];

 if( argv!=0 ) return 0;

 read_boot_sect(di,p,1);

 if( !BTS_LOADED(p) )
   {
    return 0;
   }
 
 mbr = (Mbr *)(p->part_boot_sect);

 p->flags &= ~P_ERR_BOOT_SECT; /* assuming it is good */
 
 if( mbr->magic_num!=MAGIC_NUM )  p->flags |= P_ERR_BOOT_SECT;

 /* heuristic rule - EMBR usually is filled with zeros */
 //if( count_zeros(mbr,512)<256 ) p->flags |= P_ERR_BOOT_SECT;
 
 memset(part2,0,4*sizeof(Part));

 for( i=0 ; i<4 ; i++ )
    {
     unpack_part_rec(&mbr->part_rec[i],&part2[i]);
     if( !UNUSED(&part2[i]) )
       {
        import_partition( di, &part2[i], p->ext_base, p->rel_sect );
        if( part2[i].start_sect!=1 )
          {
           part2[i].flags |= P_ANY_ERROR;
           p->flags |= P_ERR_BOOT_SECT;
          }
       }
    }

 if( op==OP_PREVIEW ) redraw_ext_details(part2);

 return 0;
}/* ext_handler */


int edit_part_col( Part *p, int col, int mode )
{
 int k;
 
#ifdef MT4P 		/* Support upto 1.99 TiB (terabytes) - UNTESTED */
 static unsigned long chs_lim[] = { 262143, 255, 63, 262143, 255, 63 };
 static unsigned long lba_lim[] = { 4294967295, 4294967295 };
 /* 
  * 1.99 TiB support is by utilising the full 32 bits of "long". Support
  * beyond this limit may involve some code changes.
  */ 
#else
 static long chs_lim[] = { 65535, 255, 63, 65535, 255, 63 };
 static long lba_lim[] = { 268435455, 268435455 };  /* IDE limit: 128G */
#endif

 if( col>=2 &&  /* numeric fields */
    !UNUSED(p) && p->fs_type!=FS_MBR )
   {
    if( mode==MODE_CHS )
      k=edit_int( chs_lim[col-2], &(p->start_cyl) + (col-2) );
    else
      k=edit_int( lba_lim[col-2],  &(p->rel_sect) + (col-2) );
   }
 else k=get_key();

 return k;
}/* edit_part_col */



