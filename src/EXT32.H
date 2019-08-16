#ifndef _EXT32_H
#define _EXT32_H

#include "disk_sim.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SECTOR_SIZE 512		// Never change this value!!!

/*--------------  Keyboard and Screen IO  --------------*/

int get_detected_os(void);
/*
	0 - no OS
	1 - MS-DOS <5.00
	2 - MS-DOS  5.xx-7.xx
	3 - Windows 2.xx-3.xx
	4 - Windows 95/98/...
	5 - Windows NT
*/
int get_startup_hd(void);    /* when running without OS */

void delay(int);  /* takes time in 1/1000 of the second */

int kb_hit(void);
int get_key(void);
int put_key(int scan_code); /* put key into keyb buffer */

void move_cursor( int  x, int  y );
void save_cursor( int *x, int *y );

void write_string( int attr, int x, int y, char *str );
void write_block( int attr, int x, int y, char *str );
void save_window( int x, int y, int w, int h, char *buf );
void load_window( int x, int y, int w, int h, char *buf );
void clear_window( int attr, int x, int y, int w, int h );
void border_window( int attr, int x, int y, int w, int h, char *border );

/*---------------------  File IO  ---------------------*/

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2

#define O_CREAT  0x0100
#define O_TRUNC  0x0200

#define O_TEXT   0x4000   /* Not implemented */
#define O_BINARY 0x8000   /* Not implemented */

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define stdin  0
#define stdout 1
#define stderr 2

#define fprintf fdprintf
/*
 open            returns file descriptor             or -1 on errors
 close           returns 0                           or -1 on errors
 lseek           returns position in file            or -1 on errors
 read and write  return number of bytes read/written or -1 on errors
*/


int open(char *path, int mode);
int close(int fd);

int lseek(int fd, int offset, int fromwhere);

int read(int fd, void *buf, int len);
int write(int fd, void *buf, int len);

/*------------------------  std C staff  ------------------------*/

void exit(int status);


/*  print functions return number of bytes written */

int printf(char *, ... );
int sprintf(char *, char *, ... );
int fdprintf(int fd, char *, ... );
int vsprintf(char *, char *, void *);
int vfdprintf(int fd, char *, void *);

int cprintf(int attr, int x, int y, char *, ... );
int vcprintf(int attr, int x, int y, char *, void *);

int  memcmp(void *s1, void *s2, int n); 
void *memset(void *dest, int c, int n);       /* returns dest */
void *memmove(void *dest, void *src, int n ); /* returns dest */

int strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strcmpi(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
int strncmpi(const char *s1, const char *s2, int n);
//char *strcat(char *dest, const char *src);         /* returns dest */
char *strcpy(char *dest, const char *src);         /* returns dest */
char *strncpy(char *dest, const char *src, int n); /* returns dest */

/*------------------------  Colors  ------------------------*/

#if 1
#define  Black      0x00               /* Foreground colors */
#define  Blue       0x01
#define  Green      0x02
#define  Cyan       0x03
#define  Red        0x04
#define  Magenta    0x05
#define  Brown      0x06
#define  White      0x07
            
#define  Gray       0x08
#define  BrBlue     0x09
#define  BrGreen    0x0A
#define  BrCyan     0x0B
#define  BrRed      0x0C
#define  BrMagenta  0x0D
#define  Yellow     0x0E
#define  BrWhite    0x0F


#define  BakBlack      0x00            /* Background colors */
#define  BakBlue       0x10
#define  BakGreen      0x20
#define  BakCyan       0x30
#define  BakRed        0x40
#define  BakMagenta    0x50
#define  BakBrown      0x60
#define  BakWhite      0x70

#define  Blink         0x80
#endif
/*--------------------------------------------------------------*/


void set_text_mode(int);
void set_graph_mode(int);
void put_pixel(int,int,int);
void get_mouse(int*,int*,int*);
void set_mouse_resolution(int,int);

/*-------------------  Hard Disk IO  -------------------*/

typedef unsigned long  Sector;

typedef struct hard_disk_info
    {
     long   flags;
     long   hd_num;
     unsigned long num_cyls;
     unsigned long num_heads;
     unsigned long sect_per_track;
     unsigned long sect_per_cyl;
     Sector        total_sects;
    } HardDiskInfo;

/*
  get_info        returns num_disks, or -1 on errors
  read and write  return 0 on success and -1 on errors
  verify          returns number of sectors successfully verified
*/

#ifndef DISK_SIM
int hard_disk_get_info(int hd, HardDiskInfo *);
int hard_disk_read(HardDiskInfo *, Sector rel_sect, void *buf, Sector n_sect);
int hard_disk_write(HardDiskInfo *, Sector rel_sect, void *buf, Sector n_sect);
int hard_disk_clear(HardDiskInfo *, Sector rel_sect, Sector n_sect);
int hard_disk_verify(HardDiskInfo *, Sector rel_sect, Sector n_sect);
#else
 #define hard_disk_get_info	sim_hard_disk_get_info
 #define hard_disk_read		sim_hard_disk_read
 #define hard_disk_write	sim_hard_disk_write
 #define hard_disk_clear	sim_hard_disk_clear
 #define hard_disk_verify	sim_hard_disk_verify

 int sim_hard_disk_get_info(int hd, HardDiskInfo *);
 int sim_hard_disk_read(HardDiskInfo *, Sector rel_sect, void *buf, Sector n_sect);
 int sim_hard_disk_write(HardDiskInfo *, Sector rel_sect, void *buf, Sector n_sect);
 int sim_hard_disk_clear(HardDiskInfo *, Sector rel_sect, Sector n_sect);
 int sim_hard_disk_verify(HardDiskInfo *, Sector rel_sect, Sector n_sect);
 #ifdef PART_DSK
  #include "disk_sim.inc"
 #endif
#endif

int floppy_disk_read(  int fd_num, int rel_sect, void *buf, int num_sect );
int floppy_disk_write( int fd_num, int rel_sect, void *buf, int num_sect );

/*--- Special functions to support partition manager ---*/


extern char own_code[];  /* array of 127*512 bytes with program's own code */

void set_on_exit_jump(unsigned long jump_addr, int dl); 
void copy_to_zero_sel(int mem_addr, void *buf, int len);



#ifdef __cplusplus
}
#endif

void *malloc(int len);
void free(void *);
int mem_left(void);
int mem_largest(void);


#endif
