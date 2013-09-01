#ifndef VTOC1_H
#define VTOC1_H

/*
 * Definitions for the on-disk structure of the VTOC table
 * Version 1.xx.xx of VTOC will have fixed size structures with some fields
 * reserved for the future self-describing data structures in version 2.xx.xx
*/

#define SECT_SIZE 512     /* Do not change! Assumed throughout the program! */

#include "types.h"

/* Next two lines are added while on MT4P */
#pragma pack(push) // save structure packing alignment
#pragma pack(1)    // set it to 1 byte (no alignment)

typedef struct MbrPartRec	/* Partition record in MBR                  */
    {
     UInt8	active;		/* This flag is set for active partition    */
     UInt8	start_head;	/* (partition from which computer boots)    */
     unsigned	start_sect:6;
     unsigned   start_cylH:2;
     UInt8	start_cylL;
     UInt8	fs_type;	/* Partition's file system type (see table) */
     UInt8	end_head;
     unsigned	end_sect:6;
     unsigned	end_cylH:2;
     UInt8	end_cylL;
     UInt32	rel_sect;	/* Number os sectors prior to partition     */
     UInt32	num_sect;	/* Number of sectors in the partition       */
    } MbrPartRec;


typedef struct ClassicalMbr	/* Located in the first sector of the disk  */
    {
     UInt8	code[0x1BE];	/* Initial Program Loader (IPL) code        */
     MbrPartRec	part_rec[4];
     UInt16	magic_num;	/* Magic number (must be 0xAA55)            */
    } ClassicalMbr;


#define MAGIC_NUM    (0xAA55)
#define MAGIC_NUM32  (0xAA550000)
#define BM_MAGIC_NUM (0x0D000100)

typedef struct Mbr		/* Located in the first sector of the disk  */
    {
     UInt8	code[0x19E];	/* Initial Program Loader (IPL) code        */

     UInt8	bm_active;	/* Boot Manager boot flag                   */
     UInt8	bm_chs[3];	/* Boot Manager CHS address on the disk     */
     UInt8	bm_options;	/* Boot Manager/IPL options (bitmap)        */
     UInt8	bm_timeout;	/* Boot Manager/IPL timeout                 */
     UInt8	bm_default;	/* Boot Manager/IPL default choice          */
     UInt8	bm_selected;	/* Boot Manager last time selected choice   */
     UInt32	bm_rel_sect;	/* Boot Manager relative sector on the disk */
     UInt32	bm_rel_sect_reserved_64;

     UInt8	reserved1;
     UInt32	bm_magic_num;	/* Magic number to identify this structure  */
     UInt8	reserved2[3];

     UInt32	vt_rel_sect;	/* Volume Table relative sector on the disk */
     UInt32	vt_rel_sect_reserved_64;

     MbrPartRec	part_rec[4];
     UInt16	magic_num;	/* Magic number (must be 0xAA55)            */

    } Mbr;


#define V1_MAX_NUM_CONFIGURED_DISKS    4
#define V1_MAX_NUM_USER_PASSWORDS     16
#define V1_MAX_NUM_PARTITION_RECORDS  31
#define V1_MAX_NUM_BOOT_MENU_CHOICES  31


typedef struct VtocPartRec	/* VTOC partition record       (32 bytes)   */
    {
     UInt16	fs_type;	/* file system type (if 0 record unused)    */
     UInt16	mbr_row;	/* in which row in MBR put this partition   */
     UInt32	part_options;	/* file system specific flags (bitmap)      */
     UInt32	reserved_0;	/* Must be zero                             */
     UInt32	reserved_1;	/* Must be zero                             */

     UInt32	rel_sect;	/* Relative sector                          */
     UInt32	reserved_2;	/* Must be zero (for future 64-bit record   */

     UInt32	num_sect;	/* Number of sectors in partition           */
     UInt32	reserved_3;	/* Must be zero (for future 64-bit record   */

    } VtocPartRec;


typedef struct DiskConfRec	/* Disk Configuration           (16 bytes)  */
    {
     UInt32	reserved_0;	/* Must be zero                             */
     UInt32	reserved_1;	/* Must be zero                             */
     UInt32	reserved_2;	/* Must be zero                             */
     UInt32	reserved_3;	/* Must be zero                             */
    } DiskConfRec;


typedef struct PartLayout	/* Partitions visible on a disk  (4 bytes)  */
    {
     UInt8	part_bitmap[4];	/* Size: ceil(max_num_partitions / 8 bits)  */
				/* exmple: V1_MAX_NUM_PARTITION_RECORDS=31  */
    } PartLayout;
    

typedef struct BootMenuRec	/* Boot Menu Choice             (64 bytes)  */
    {
     UInt8	menu_name[32];  /* If name[0]==0 record is unused           */
     UInt32	boot_options;	/* boot options (bitmap)                    */
     UInt16	boot_disk;	/* boot disk (device)                       */
     UInt16	boot_part;	/* boot partition                           */
     UInt32	additional_loader_data_offset;
     UInt32	additional_boot_man_data_offset;
     PartLayout	disk_layout[ V1_MAX_NUM_CONFIGURED_DISKS ];

    } BootMenuRec;


typedef struct UserPassRec	/* User Password Record         (32 bytes)  */
    {
     UInt8	user_name[16];  /* If name[0]==0 record is unused           */
     UInt32     user_options;	/* bitmap                                   */
     UInt32	password_hash;	/* hash value to validate password          */
     UInt32	additional_user_data_offset;
     UInt8	perm_bitmap[4];	/* permissions to use boot menu choices     */

    } UserPassRec;


#define VERSION(major,minor,update) ( ((major)<<16) + ((minor)<<8) + (update) )

#define VT_MAGIC_NUM  (0x434F5456)  /* "VTOC" */
#define VT_VERSION  VERSION(1,00,00)
#define PM_VERSION  VERSION(2,38,00)

typedef struct /* Volume Table that describes partitions stored on the disk  */
    {
     UInt32		vt_magic_num;	/* VTOC                              */
     UInt32		vt_version;	/* Revision number of the structure  */
     UInt32		vt_options;	/* Volume Table options (bitmap)     */
     UInt32		vt_disk_tag;	/* Randomly selected disk tag        */
     UInt32		vt_size;	/* Size of entire VTOC in bytes      */
     UInt32		vt_stamp;	/* Must be zero (not in use yet)     */
     UInt32		vt_check_sum;	/* Must be zero (not in use yet)     */
     UInt16		part_rec_count;	/* Must be 31 in VTOC version 1.x.x  */
     UInt16		part_rec_size;	/* Must be 32 in VTOC version 1.x.x  */
//   VtocPartRec	part_rec[31];	/* Partition Table                   */

     VtocPartRec	part_rec[ V1_MAX_NUM_BOOT_MENU_CHOICES ];

     UChar		bm_title[40];	/* Title for the boot menu           */
     UInt32		bm_options;	/* Boot Manager options (bitmap)     */
     UInt32		reserved_0;

     UInt16		menu_rec_count;	/* Must be 31 in VTOC version 1.x.x  */
     UInt16		menu_rec_size;	/* Must be 64 in VTOC version 1.x.x  */

     UInt16		user_rec_count;	/* Must be 16 in VTOC version 1.x.x  */
     UInt16		user_rec_size;	/* Must be 32 in VTOC version 1.x.x  */

     UInt16		disk_rec_count;	/* Must be  4 in VTOC version 1.x.x  */
     UInt16		disk_rec_size;	/* Must be 16 in VTOC version 1.x.x  */
     
     UInt16		part_bitmap_size; /* Must be 4 in VTOC version 1.x.x */
     UInt16		perm_bitmap_size; /* Must be 4 in VTOC version 1.x.x */

//   BootMenuRec	menu_rec[31];	/* Boot menu table                   */
//   UserPassRec	user_rec[16];	/* User names and passwords table    */
//   DiskConfRec    	disk_rec[4];	/* Disks configured in the boot menu */

     BootMenuRec	menu_rec[ V1_MAX_NUM_BOOT_MENU_CHOICES ];
     UserPassRec	user_rec[ V1_MAX_NUM_USER_PASSWORDS    ];
     DiskConfRec    	disk_rec[ V1_MAX_NUM_CONFIGURED_DISKS  ];

     /* Boot Manager specific data (colors, window positions, options)       */

     UInt32		bm_data_size;
     UInt32		bm_signature;

     UInt32 gui_x;
     UInt32 gui_y;
     UInt32 bm_type;

     char slack[428]; /* MT4P - It was 512 before */
    } Vtoc;

#define VTOC_SIZE      (sizeof(Vtoc)) // 4096
#define VTOC_NUM_SECT  (8)           //(sizeof(Vtoc)/SECT_SIZE)

/* Next line was added while on MT4P */
#pragma pack(pop) // restore structure packing alignment

#endif  /* VTOC1_H */
