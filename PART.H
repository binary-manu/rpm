#ifndef PART_H
#define PART_H

#include "ext32.h"
#include "part_vtc.h"
#include "part_msg.h"

#define MT4P /* 
              * To include all the code for supporting More Than 4 Primary 
              * (MT4P) partitions; also for including the improvements/bug fixes
              * while working on MT4P support - Muthu
              */

/* ---------- Structures for the internal use by Partition Manager ----------*/

#define P_FS_FLAGS	0x0000FFFF /* Flags that depends on file system */

#define OP_ANY_TYPE	0x000003FF /* Methods to work with file systems */ 

#define OP_VALIDATE	0x00000001
#define OP_PRINT	0x00000002
#define OP_PREVIEW	0x00000004
#define OP_SETUP	0x00000008
#define OP_CHKDSK	0x00000010
#define OP_FORMAT	0x00000020
#define OP_VERIFY	0x00000040
#define OP_DEFRAG	0x00000080
#define OP_RESIZE	0x00000100
#define OP_BOOT_FIX	0x00000200

#define P_HIGHLIGHT	0x00001000 /* Predefined file system attributes */
#define P_EXTENDED	0x00002000
#define P_HIDABLE	0x00004000
#define P_UNHIDABLE	0x00008000

#define P_DYN_FLAGS	0x00FF0000 /* Dynamic attributes of partitions */

#define P_NOT_EMPTY	0x00010000
#define P_LOGICAL	0x00020000
#define P_BTS_LOADED	0x00040000


#define P_ANY_ERROR     0xFF000000

#define P_LOC_ERROR     0x1F000000 /* Problems with partition location */
#define P_ERR_ZERO      0x01000000
#define P_ERR_RANGE     0x02000000
#define P_ERR_OVERLAP   0x04000000
#define P_ERR_LOG_ORPH  0x08000000
#define P_ERR_LOG_STRU  0x10000000

#define P_BTS_ERROR     0xE0000000 /* Problems with partition contents */
#define P_ERR_CANT_READ 0x20000000
#define P_ERR_BOOT_SECT 0x40000000


#define EMPTY(p)       (!((p)->flags & P_NOT_EMPTY))
#define VALID(p)       (!((p)->flags & P_ANY_ERROR))
#define VALID_LOC(p)   (!((p)->flags & P_LOC_ERROR))
#define VALID_BTS(p)   (!((p)->flags & P_BTS_ERROR))
#define UNUSED(p)       ((p)->fs_type == FS_UNUSED)
#define LOGICAL(p)       ((p)->flags & P_LOGICAL)
#define EXTENDED(p)      ((p)->flags & P_EXTENDED)
#define BTS_LOADED(p)    ((p)->flags & P_BTS_LOADED)


typedef struct Part		/* Internal partition record representation */
    {
     unsigned short fs_num;	/* fs number in the file system type table  */
     unsigned short fs_type;
     unsigned short mbr_row;
     unsigned short tag;
     unsigned long  flags;
     unsigned long  fs_flags;

     Sector start_cyl;
     Sector start_head;
     Sector start_sect;

     Sector end_cyl;
     Sector end_head;
     Sector end_sect;

     Sector rel_sect;
     Sector num_sect;
     Sector last_sect;
     Sector ext_base;

     Sector rel_sect_loaded;
     void   *part_boot_sect;
     void   *part_misc_info;

    } Part;



typedef struct Menu
    {
     UInt8	menu_name[32];	/* If name[0]==0 record is unused           */
     UInt32	boot_options;	/* boot options (bitmap)                    */
     UInt16	boot_disk;	/* boot disk (device)                       */
     UInt16	boot_part;	/* boot partition                           */
     UInt32	additional_loader_data_offset;
     UInt32	additional_boot_man_data_offset;
     PartLayout	layout[4];	/* Partition layout for each of the disks   */

    } Menu;


#define BM_OPT_VIRUS_CHECK   0x0001
#define BM_OPT_INSTALL_PM    0x0002
#define BM_OPT_USE_PART      0x0004
#define BM_OPT_USE_MENU      0x0008
#define BM_OPT_USE_DEFAULT   0x0010
#define BM_OPT_USE_PASSWORD  0x0020

#ifdef MT4P
#define P_ROWS 32     /* Maximum number of partition records in this version */
#define M_ROWS 32     /* Maximum number of menu records in this version      */
#else
#define P_ROWS 64     /* Maximum number of partition records in this version */
#define M_ROWS 30     /* Maximum number of menu records in this version      */
#endif

typedef struct disk_info : HardDiskInfo  /* DiskInfo                         */
    {
     unsigned long  vt_options;    /* Volume Table options (bitmap)          */
     unsigned long  bm_options;    /* Boot Manager options (bitmap)          */

     char           changed;
     char           quiet;
     char           use_vtoc;
     char           num_disks;
     char           cur_ipl;       /* Current IPL type (detected in MBR)     */
     char           new_ipl;       /* Intended IPL type (should be in MBR)   */
     char           bm_type;
     char           ipl_default;   /* Default choice  (not last time booted) */
     unsigned long  ipl_timeout;   /* Boot Manager prompt timeout            */
     char           bm_selected;
     
     int            gui_x;
     int            gui_y;
     
     char bm_title[40];

     int  num_part;              /* number of partition records in the array */
     int  num_menus;
     int  act_choice;
     
     Vtoc vtoc;
     
     unsigned long BmRelSect;
     unsigned long VtocRelSect;

     Part part[P_ROWS];
     Menu menu[M_ROWS];

     Part part_exp[P_ROWS];
     
     int  num_part_exp;

    } DiskInfo;

/*
 Methods to work with DiskInfo:
*/

DiskInfo *new_disk_info(int hd); /* reads all tables from disk (0 if errors) */
DiskInfo *get_disk_info(int hd); /* returns DiskInfo from preallocated array */
int   read_disk_info(DiskInfo*); /* rereads all tables        (-1 if errors) */
int  write_disk_info(DiskInfo*); /* saves all tables to disk  (-1 if errors) */
int delete_disk_info(DiskInfo*); /* deallocates all memory                   */
int save_disk_info(DiskInfo*, int fd); /* saves data to file  (-1 if errors) */
int load_disk_info(DiskInfo*, int fd); /* loads data to file  (-1 if errors) */
int sort_disk_info(DiskInfo*, int *row); /* sort partition table             */
#ifdef MT4P
int write_mbr(DiskInfo *); /* Write only the MBR for RPM - helps in restoring */
#endif

#define FS_UNUSED    0x0000
#define FS_BM        0xF000
#define FS_DOSEXT    0x0500
#define FS_HIDDEN    0xFF80
#define FS_MBR       0xFF81
#define FS_UNKNOWN   0xFFFF

struct fs_desc
    {
     unsigned short fs_id;
     unsigned short flags;
     char           *name;
     int (*handler)(int op, DiskInfo *, Part *, char **argv);
    };

#define OP_HANDLER(op,di,p,v) (fs_desc[(p)->fs_num].handler((op),(di),(p),(v)))

#define IPL_TYPE_BM    0
#define IPL_TYPE_STD   1
#define IPL_TYPE_XBM   2  /* Obsolete boot manager */
#define IPL_TYPE_UNKN  3

#define BM_IPL_SIZE	0x19E
#define STD_IPL_SIZE	0x0DB
#define EMP_IPL_SIZE	0x023
//#define FAT16_BOOT_SIZE 0x1C0

extern char BM_IPL[];			/* external assembly code */
extern char BM_BOOT[];
extern char STD_IPL[];
extern char EMP_IPL[];
//extern char FAT16_BOOT[];
extern char BOOT_FAT_1X[];
extern char BOOT_FAT_1X_NAMES[][48];

typedef struct
    {
     unsigned char   jmp[3];
     unsigned char   fs_magic[7];  /* "?????64" */
     unsigned short  sector_size;  /* 512 */
     unsigned long   fs_options;   /*  File System features (bitmap) */

     unsigned long   rel_sect;     /* Number of sectors prior to partition */
     unsigned long   rel_sect64;
     unsigned long   num_sect;     /* Number of sectors in the partition   */
     unsigned long   num_sect64;

     unsigned long   fat1_rel_sect;
     unsigned long   fat1_rel_sect64;
     unsigned long   fat2_rel_sect;
     unsigned long   fat2_rel_sect64;

     unsigned long   loader_rel_sect;
     unsigned long   loader_rel_sect64;
     unsigned long   loader_num_sect;
     unsigned long   loader_check_sum;

     unsigned char   reserved[64];

     unsigned char   xcode[384];

    } BmBootSect;


#define max(x,y)   ((x)>(y)?(x):(y))
#define min(x,y)   ((x)<(y)?(x):(y))


#define MODE_CHS     0    /* modes for displaying partition table */
#define MODE_LBA     1
#define MODE_EXT     2
#define MODE_MINI    3
#define MODE_MISC    4
#define MODE_EXACT   5
#define MODE_TITLE   6
#define MODE_BM      7


typedef struct /* FatInfo */
    {
     Sector rel_sect_loaded;
     int  sys;
     int  boot_fix;
     char sysid[12];
     char fs_id[12];
     char label[12];
     unsigned fat_size;
     unsigned clust_size;
     
     Sector rel_sect;
     Sector exp_rel_sect;
     Sector num_sect;
     Sector exp_num_sect;
     Sector max_num_sect;
     Sector drive_num;
     Sector exp_drive_num;

     Sector fat_rel_sect;
     Sector fat_sect_read;
     Sector num_data_sect;
     Sector min_num_sect;    // Determined by the location of the last data clust
     Sector first_data_sect;

    } FatInfo;


/* part.c */

void partition_manager( int hd );

int edit_part_col( Part *p, int col, int mode );

int h_deflt    ( int op, DiskInfo *, Part *, char **argv );
int bm_handler ( int op, DiskInfo *, Part *, char **argv );
int ipl_handler( int op, DiskInfo *, Part *, char **argv );
int ext_handler( int op, DiskInfo *, Part *, char **argv );
int fat_handler( int op, DiskInfo *, Part *, char **argv );

/* part_bmg.c */

void boot_manager( int hd );
int boot_manager_menu( DiskInfo * );
void boot_manager_simulation( DiskInfo * );

/* part_aux.c */

int prepare_partitions( DiskInfo *, PartLayout * );
int  prepare_mbr_records( Part *src, Part *dst );

void   pack_part_rec( Part *, MbrPartRec * );
void unpack_part_rec( MbrPartRec *, Part * );

void   pack_vtoc_part_rec( Part *, VtocPartRec * );
void unpack_vtoc_part_rec( VtocPartRec *, Part * );

void recalculate_partition( HardDiskInfo *, Part *, int mode);
void import_partition( HardDiskInfo *, Part *, Sector ext_base, Sector log_base);
void export_partition( HardDiskInfo *, Part *, Sector ext_base, Sector log_base);

#ifdef MT4P
int can_insert(DiskInfo *, Part *);
int non_overlap_check(Part *, Part *);
int reset_mbr_row(Part *, Part *, int);
#endif

void sprintf_partition(char *tmp, int num, Part *p, int mode);

int  find_ipl_type( Mbr * );
void find_fs_num( Part *p );
void free_part_info( Part *p );
int count_zeros( void *buf, int len );
unsigned long calc_hash( void *p, int n );

/* part_cmd.c */

void start_cmd( int hd, int argc, char **argv );
void print_part_table( DiskInfo *, int details );
void print_fat_details( FatInfo * );

/* part_dsk.c */

int  read_boot_sect( HardDiskInfo *, Part *, unsigned num_sect );
int write_boot_sect( HardDiskInfo *, Part *, void *buf, unsigned num_sect );

int read_part( HardDiskInfo *, Part *, Sector rel, void *buf, int num_sect );
int write_part( HardDiskInfo *, Part *, Sector rel, void *buf, int num_sect );
int verify_part( HardDiskInfo *, Part *, Sector *bbt, unsigned bbt_size );

void fat16_format(HardDiskInfo *di, Part *p, Sector *bbt, int bbt_size,char *label);
void fat32_format(HardDiskInfo *di, Part *p, Sector *bbt, int bbt_size,char *label);

int install_to_floppy(void);
int copy_disk( HardDiskInfo *, HardDiskInfo * );
int copy_part( HardDiskInfo *src, Part *src_part, HardDiskInfo *dst, Part *dst_part );
void clear_partition( HardDiskInfo *di, Part *p );

/* part_gui.c */


#define EV_CANCEL -2
#define EV_FAILED -1
#define EV_OK      0

void gui_init(void);
void gui_done(void);
void gui_restore(void);

#define HINT    0
#define MESSAGE 1
#define WARNING 2

void redraw_main_screen( HardDiskInfo *, int mode );
void redraw_main_prompt( int save );
void redraw_main_hint( int h_type, char *hint );
void redraw_part_table( int first_row, int row, int col, Part *, int n_rows, int mode );

void clear_error( int p );
void popup_error( char *err );

void show_help( int topic );
void show_error( char *msg );
int dialog_box( char *msg, char *ch1, ... );

int   edit_int( unsigned long lim, unsigned long *n );
void write_int( int attr, int x, int y, int w, int n );

int progress( char *msg );
void select_fs_type( Part * );

void clear_preview_box( char *);

void   edit_ipl_options( DiskInfo * );
void redraw_ipl_options( DiskInfo *, int act );

void redraw_ext_details( Part * );

void redraw_fat_details( FatInfo *, int act );
void redraw_fat_prompt( int changed );

void update_fat_info( HardDiskInfo *, Part *, FatInfo * );
void read_fat_table( HardDiskInfo *, Part *, FatInfo * );
void bgview_fat_details( HardDiskInfo *, Part *, FatInfo *, int act );
void edit_fat_details( HardDiskInfo *, Part *, FatInfo * );
void save_fat_boot_sect( HardDiskInfo *, Part *, FatInfo * );
void fat_format( HardDiskInfo *, Part *, FatInfo * );

void show_bad_blocks( HardDiskInfo *, Part *, Sector *bbt, int bbt_size );

int up_down_navigations(int k, int *top, int *act,
					 int n, /* records on screen*/
					 int N /* displayable records */ );
int floppy_os_dialog_box(void);

void select_and_copy_disk(DiskInfo *src_disk);
void select_and_copy_part(DiskInfo *src_disk, Part *src_part);

/* part_wiz.c */

void part_ins_wiz( DiskInfo *, int *row );
void part_new_wiz( DiskInfo *, int *row );

/* part_zip.c */

void zip_example( HardDiskInfo *, Part * );

/* vtoc.cpp */

void import_vtoc(DiskInfo *, Vtoc *);
void export_vtoc(DiskInfo *, Vtoc *);

char *get_partition_label(DiskInfo *di, Part*p);

#ifdef PART_CMD_CPP
 int cmd_line_mode=0;
 int debug=0;
 int verbose=0;
 int quiet=0;
 int sure=0;
 int scan=0;
 char *batch=0;
 int gdisk_num=0x80;
 int gdisk_spec=0;
#else
 extern int cmd_line_mode;
 extern int debug;
 extern int verbose;
 extern int quiet;
 extern int scan;
 extern char *batch;
 extern int gdisk_num;
 extern int gdisk_spec;
#endif

#ifndef PART_TAB_C
  extern struct fs_desc fs_desc[];
#endif


#endif
