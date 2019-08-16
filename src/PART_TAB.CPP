#define PART_TAB_C
#include "part.h"

#define F_UNU	(P_HIGHLIGHT)
#define F_BM	(P_HIGHLIGHT + OP_VALIDATE + OP_PREVIEW + OP_SETUP)
#define F_MBR	(P_HIGHLIGHT + OP_VALIDATE + OP_PREVIEW + OP_SETUP)
#define F_HID	(P_HIGHLIGHT)

#define FLGS_EXT (P_EXTENDED + OP_VALIDATE + OP_PREVIEW)
#define FLGS_FAT (             OP_VALIDATE + OP_PREVIEW + OP_SETUP + OP_PRINT\
                               + OP_FORMAT)

#define F_EXT	(P_HIGHLIGHT + P_HIDABLE + FLGS_EXT)
#define F_FAT	(P_HIGHLIGHT + P_HIDABLE + FLGS_FAT)
#define F_XFAT	(              P_HIDABLE + FLGS_FAT)
#define F_CMPQ	(                          FLGS_FAT)
#define F_NTFS	(P_HIGHLIGHT + P_HIDABLE)

#define H_EXT	(P_HIGHLIGHT + P_UNHIDABLE + FLGS_EXT)
#define H_FAT	(P_HIGHLIGHT + P_UNHIDABLE + FLGS_FAT)
#define H_XFAT	(              P_UNHIDABLE + FLGS_FAT)
#define H_NTFS	(P_HIGHLIGHT + P_UNHIDABLE)

#define F_SYSV	(P_HIGHLIGHT)
#define F_LNX	(P_HIGHLIGHT)
#define F_SOL	(P_HIGHLIGHT)
#define F_BSD	(P_HIGHLIGHT)
#define F_BEOS	(P_HIGHLIGHT)

#ifdef  CMD_LINE
#define h_deflt 0
#define fat_handler 0
#define ext_handler 0
#define bm_handler 0
#define ipl_handler 0
#endif

 struct fs_desc fs_desc[] = {

/*   fs_id   flags         long name              short name    handler  */
/*                   "123456789 123456789 1"     "1234567890"            */

 {   0x0000, F_UNU,  "Unused",				h_deflt     },
 {   FS_BM,  F_BM,   "Boot Manager"         "\0" "BootMan",	bm_handler  },
 {   0x0100, F_XFAT, "DOS FAT-12 "          "\0" "FAT-12",	fat_handler },
//x {   0x0200, 0,      "XENIX root",				h_deflt     },
//x {   0x0300, 0,      "XENIX /usr",				h_deflt     },
 {   0x0400, F_XFAT, "DOS FAT-16 (<32M)"    "\0" "FAT-16",	fat_handler },
 {   0x0500, F_EXT,  "Extended",				ext_handler },
 {   0x0600, F_FAT,  "DOS FAT-16 "          "\0" "FAT-16",	fat_handler },
 {   0x0700, F_NTFS, "Windows NT NTFS"      "\0" "NTFS",	h_deflt     },
//x {   0x0701, 0,      "QNX 2.x",					h_deflt     },
//x {   0x0702, 0,      "OS/2 HPFS",				h_deflt     },
//x {   0x0703, 0,      "Advanced Unix"        "\0" "Adv Unix",	h_deflt     },
//x {   0x0800, 0,      "OS/2 (v1.0-1.3 only)" "\0" "OS/2",	h_deflt     },
//x {   0x0801, 0,      "AIX bootable part."   "\0" "AIX",		h_deflt     },
//x {   0x0802, 0,      "Commodore DOS"        "\0" "Commodore",	h_deflt     },
//x {   0x0803, 0,      "DELL multi-drive part""\0" "DELL",	h_deflt     },
//x {   0x0900, 0,      "AIX data partition"   "\0" "AIX",		h_deflt     },
//x {   0x0901, 0,      "Coherent filesystem"  "\0" "Coherent",	h_deflt     },
//x {   0x0A00, 0,      "OS/2 Boot Manager"    "\0" "OS/2 BM",	h_deflt     },
//x {   0x0A01, 0,      "OPUS",					h_deflt     },
//x {   0x0A02, 0,      "Coherent swap part."  "\0" "Coherent",	h_deflt     },
 {   0x0B00, F_FAT,  "Windows FAT-32"       "\0" "FAT-32",	fat_handler },
 {   0x0C00, F_FAT,  "Windows FAT-32 LBA"   "\0" "FAT-32",	fat_handler },
 {   0x0E00, F_FAT,  "FAT-16 LBA"      "\0" "FAT-16",	fat_handler },
 {   0x0F00, F_EXT,  "Extended LBA"    "\0" "Extended",	ext_handler },
//x {   0x1000, 0,      "OPUS",					h_deflt     },
 {   0x1100, H_XFAT, "Hidden FAT-12"        "\0" "Hid FAT-12",	fat_handler },
 {   0x1200, F_CMPQ, "Compaq Diagnostics"   "\0" "Compaq",	fat_handler },
//x {   0x1300, 0,      "Reliable Systems FTFS""\0" "FTFS",	h_deflt     },
 {   0x1400, H_XFAT, "Hidden FAT-16 (<32M)" "\0" "Hid FAT-16",	fat_handler },
 {   0x1500, H_EXT,  "Hidden Extended"      "\0" "Hid Extend",	ext_handler },
 {   0x1600, H_FAT,  "Hidden FAT-16"        "\0" "Hid FAT-16",	fat_handler },
 {   0x1700, H_NTFS, "Hidden NTFS"          "\0" "Hid NTFS",	h_deflt     },
//x {   0x1800, 0,      "AST Windows swap file""\0" "AST swap",	h_deflt     },
//x {   0x1900, 0,      "Willowtech Photon coS""\0" "Photon",	h_deflt     },
 {   0x1B00, H_FAT,  "Hidden FAT-32"        "\0" "Hid FAT-32",	fat_handler },
 {   0x1C00, H_FAT,  "Hidden FAT-32 LBA"    "\0" "Hid FAT-32",	fat_handler },
 {   0x1E00, H_FAT,  "Hidden FAT-16 LBA"    "\0" "Hid FAT-16",	fat_handler },
 {   0x1F00, H_EXT,  "Hidden Extended LBA"  "\0" "Hid Extend",	ext_handler },
//x {   0x2000, 0,      "Willowsoft OFS1"      "\0" "OFS1",	h_deflt     },
//x {   0x2100, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x2300, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x2400, 0,      "NEC MS-DOS 3.x"       "\0" "NEC MS-DOS",	h_deflt     },
//x {   0x2600, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x3100, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x3300, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x3400, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x3600, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x3800, 0,      "Theos",					h_deflt     },
//x {   0x3C00, 0,      "Part. Magic recovery" "\0" "PQM recov.",	h_deflt     },
//x {   0x4000, 0,      "VENIX 80286"          "\0" "VENIX",	h_deflt     },
//x {   0x4100, 0,      "Personal RISC Boot"   "\0" "RISC",	h_deflt     },
//x {   0x4200, 0,      "SFS by Peter Gutmann" "\0" "SFS",		h_deflt     },
//x {   0x5000, 0,      "OnTrack Disk Mgr, R/O""\0" "OnTrck R/O",	h_deflt     },
//x {   0x5100, 0,      "OnTrack Disk Mgr, R/W""\0" "OnTrck R/W",	h_deflt     },
//x {   0x5101, 0,      "NOVEL",					h_deflt     },
//x {   0x5200, 0,      "CP/M",					h_deflt     },
//x {   0x5201, 0,      "Microport Sys V/386"  "\0" "V/386",	h_deflt     },
//x {   0x5300, 0,      "OnTrack Disk Mgr, W/O""\0" "OnTrck W/O",	h_deflt     },
//x {   0x5400, 0,      "OnTrack Disk Mgr, DDO""\0" "OnTrck DDO",	h_deflt     },
//x {   0x5500, 0,      "EZ-Drive",				h_deflt     },
//x {   0x5600, 0,      "GoldenBow VFeature"   "\0" "GoldenBow",	h_deflt     },
//x {   0x6100, 0,      "SpeedStor",				h_deflt     },
 {   0x6300, F_SYSV, "Unix SysV/386, 386/ix""\0" "SysV/386",	h_deflt     },
//x {   0x6301, 0,      "Mach, MtXinu BSD 4.3" "\0" "Mach/BSD",	h_deflt     },
//x {   0x6302, 0,      "GNU HURD",				h_deflt     },
//x {   0x6400, 0,      "Novell NetWare 286"   "\0" "NetWare 2",	h_deflt     },
 {   0x6500, 0,      "Novell NetWare (3.11)""\0" "NetWare 3+",	h_deflt     },
//x {   0x6700, 0,      "Novell",					h_deflt     },
//x {   0x6800, 0,      "Novell",					h_deflt     },
//x {   0x6900, 0,      "Novell",					h_deflt     },
//x {   0x7000, 0,      "DiskSecure Multi-Boot""\0" "DiskSecure",	h_deflt     },
//x {   0x7100, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x7300, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x7400, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0x7500, 0,      "PC/IX",					h_deflt     },
//x {   0x7600, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
 {   0x7700, 0,      "QNX 4.x",					h_deflt     },
 {   0x7800, 0,      "QNY 4.x",					h_deflt     },
 {   0x7900, 0,      "QNZ 4.x",					h_deflt     },
//x {   0x8000, 0,      "Minix v1.1 - 1.4a"    "\0" "Minix",	h_deflt     },
 {   0x8100, F_LNX,  "Linux",					h_deflt     },
//x {   0x8101, 0,      "Minix v1.4b+"         "\0" "Minix",	h_deflt     },
//x {   0x8102, 0,      "Mitac Adv Disk Mgr"   "\0" "Mitac DM",	h_deflt     },
 {   0x8200, F_LNX,  "Linux swap",				h_deflt     },
 {   0x8201, F_SOL,  "Solaris x86"          "\0" "Solaris",	h_deflt     },
//x {   0x8202, 0,      "Prime",					h_deflt     },
 {   0x8300, F_LNX,  "Linux ext2fs"         "\0" "Linux ext2",	h_deflt     },
//x {   0x8400, 0,      "OS/2-renumbered FAT16""\0" "ren FAT-16",	h_deflt     },
 {   0x8500, F_LNX,  "Linux Extended"       "\0" "Linux ext",	h_deflt     },
 {   0x8600, 0,      "FAT-16 volume/stripe" "\0" "FAT-16",	h_deflt     },
 {   0x8700, 0,      "NTFS volume/stripe"   "\0" "NTFS",	h_deflt     },
//x {   0x8701, 0,      "HPFS F-T mirrored prt""\0" "HPFS mirr.",	h_deflt     },
//x {   0x9300, 0,      "Amoeba file system"   "\0" "Amoeba",	h_deflt     },
//x {   0x9400, 0,      "Amoeba bad block tbl.""\0" "Amoeba BBT",	h_deflt     },
 {   0xA000, 0,      "Phoenix Power Mngmnt" "\0" "Phoenix PM",	h_deflt     },
//x {   0xA100, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xA300, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xA400, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
 {   0xA500, F_BSD,  "FreeBSD, BSD/386"     "\0" "FreeBSD",	h_deflt     },
//x {   0xA600, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xB100, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xB300, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xB400, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xB600, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xB700, 0,      "BSDI secondarily swap""\0" "BSDI swap",	h_deflt     },
//x {   0xB800, 0,      "BSDI swap partition"  "\0" "BSDI swap",	h_deflt     },
//x {   0xC100, 0,      "DR DOS secured FAT-12""\0" "DR FAT-12",	h_deflt     },
//x {   0xC400, 0,      "DR DOS secured FAT-16""\0" "DR FAT-16",	h_deflt     },
//x {   0xC600, 0,      "DR DOS secured FAT-16""\0" "DR FAT-16",	h_deflt     },
//x {   0xC601, 0,      "WinNT corrupted FAT16""\0" "Cor FAT-16",	h_deflt     },
//x {   0xC700, 0,      "Syrinx Boot"          "\0" "Syrinx",	h_deflt     },
//x {   0xC701, 0,      "WinNT corrupted NTFS" "\0" "Corr NTFS",	h_deflt     },
//x {   0xD800, 0,      "CP/M-86",					h_deflt     },
//x {   0xDB00, 0,      "CP/M, Concurrent DOS" "\0" "Concur DOS",	h_deflt     },
//x {   0xDB01, 0,      "CTOS",					h_deflt     },
//x {   0xE100, 0,      "SpeedStor ext FAT-12" "\0" "SpeedStor",	h_deflt     },
//x {   0xE300, 0,      "DOS read-only"        "\0" "DOS R/O",	h_deflt     },
//x {   0xE301, 0,      "Storage Dimensions"   "\0" "Stor. Dim.",	h_deflt     },
//x {   0xE400, 0,      "SpeedStor ext FAT-16" "\0" "SpeedStor",	h_deflt     },
//x {   0xE500, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xE600, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
 {   0xEB00, F_BEOS, "BeOS",					h_deflt     },
//x {   0xF100, 0,      "Storage Dimensions"   "\0" "Stor. Dim.",	h_deflt     },
//x {   0xF200, 0,      "DOS 3.3+ secondary"   "\0" "DOS 3.3+",	h_deflt     },
//x {   0xF300, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xF400, 0,      "SpeedStor",				h_deflt     },
//x {   0xF401, 0,      "Storage Dimensions"   "\0" "Stor. Dim.",	h_deflt     },
//x {   0xF600, 0,      "-- reserved --"       "\0" "-resrvd-",	h_deflt     },
//x {   0xFE00, 0,      "LANstep",					h_deflt     },
//x {   0xFE01, 0,      "IBM PS/2 IML"         "\0" "IBM PS/2",	h_deflt     },
 {   0xFF00, 0,      "Xenix bad block table""\0" "Xenix BBT",	h_deflt     },
 {FS_HIDDEN, F_HID,  "Hidden",				h_deflt     },
 {   FS_MBR, F_MBR,  "Master Boot Record"   "\0" "MBR",	   	ipl_handler },
 {FS_UNKNOWN,0,      "Unknown",				h_deflt     }
};
