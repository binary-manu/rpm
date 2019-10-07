#define PART_CMD_CPP
#include "part.h"
#include "MBR.H"
#include "CMOS.H"

//#define SCAN_OPT

#define MAX_FAT16_SIZE   (1+2*256+32+ 64*65524) // in sectors, cluster_size=32k
#define MAX_NTFAT16_SIZE (1+2*256+32+128*65524) // in sectors, cluster_size=64k
#define MIN_FAT32_SIZE   (33+2*512+8*65524)

void usage(void);
void cmd_line(int argc, char **argv);
int process_line(int line_num,int argc, char**argv);

int  get_partition_index(DiskInfo*,int part_type, int num);
void sprintf_partition_header(char *header, DiskInfo*,int index);
void sprintf_partition(char *buf, DiskInfo*,int index, int mode);

void print_disk_info_header(void);
void print_disk_info_header_ext(void);
void print_disk_info(DiskInfo*);
void print_partitions(DiskInfo* di,int details);

int confirm(void);

DiskInfo* get_disk_info_or_print_error(int hd);

void update_mbr_ipl_code(DiskInfo* di);
void delete_all_partitions(DiskInfo* di);
void delete_all_partitions_except(DiskInfo* di, int no_del_count, int *no_del_type);
void delete_partition(DiskInfo *di,int index);
void format_fat_partition(DiskInfo* di,int index,char *label);
void create_partition(DiskInfo* di, int part_type, unsigned long size, 
                       int fat_type, int format, char *label);
void activate_partition(DiskInfo *di,int index);

#define ATOI(var,p) \
 for( (var)=0 ; *(p)>='0' && *(p)<='9' ; (var)=10*(var)+*(p++)-'0' );

int ATOIX(char *p)
{
 int var=0;
 while(1)
    {
     int dig;
          if( *p>='0' && *p<='9' ) dig=*p-'0';
     else if( *p>='a' && *p<='f' ) dig=*p-'a'+10;
     else if( *p>='A' && *p<='F' ) dig=*p-'A'+10;
     else break;
     p++;
     var = var*16+dig;
    }
 return var;
}

void start_cmd(int hd, int argc, char **argv)
{
 (void)hd;

 cmd_line_mode=1;
 if( argc>=2 && argv[1][0]!=0 )
   {
    cmd_line(argc-1,argv+1);
   }
 else usage();
#if 0
   {
    int i, hd=0x80;
    DiskInfo *di=get_disk_info(hd);
    int n_disks = ((di==0)?0:di->num_disks);
    printf("To list command line options run \"part_cmd -help\"\r\n\r\n");
    print_disk_info_header();
    for( i=0 ; i<n_disks ; i++ )
       {
        hd=0x80+i;
        di=get_disk_info_or_print_error(hd);
        if( di!=0 )
          {
           print_disk_info(di);
          }
       }
    exit(0);
   }
#endif
 exit(0);
}

void usage(void)
{
 printf("%s\r\n"
 "part.exe [disk] [/<command>|/BATCH:file [/QUIET|/VERBOSE] [/DEBUG] [/SURE]]\r\n\r\n"
#ifndef DISK_SIM
 "disk={1..8}\r\n"
 "/Print or /Extra - for disk and partitions info\r\n"
 "/MBR - Write standard IPL\r\n"
 "/ACT /P:partition-no |/PRI[:nth]\r\n"
 "/DEL /P:partition-no |/PRI[:nth] |/LOG[:nth] |/EXT |/ALL\r\n"
 "/FOR /P:partition-no |/PRI[:nth] |/LOG[:nth] [/V:label]\r\n"
 "/CRE [/PRI |/EXT |/LOG] [/SZ:mb|pcnt%%] [/FOR [/V:label]] [/NTFAT16|/FAT16|/FAT32|/NTFS]\r\n"
 "/MBS /F:FILENAME - Save MBR to FILENAME\r\n"
 "/MBL /F:FILENAME - Load MBR from FILENAME\r\n"
 "/CMS /F:FILENAME - Save CMOS to FILENAME\r\n"
 "/CML /F:FILENAME [/T:YY-MM-DD-HH-MM-SS|/T:] - Load MBR from FILENAME\r\n"
#endif
#if 0
 "Commands: disk-no - hard disk number, valid values are 1..8\r\n"
 "                  - if no command is specified GUI will run\r\n"
 "          /Print  - Print disk and partitions information\r\n"
 "          /Extra  - Print detailed disk and partitions information\r\n"
#ifdef SCAN_OPT
 "          /SCAN   - Print tab delimitered partitions information\r\n"
#endif
 "          /MBR    - Write standard Initial Program Loader into MBR\r\n"
 "          /ACT /P:partition-no |/PRI[:nth]\r\n"
 "          /DEL /P:partition-no |/PRI[:nth] |/LOG[:nth] |/EXT |/ALL\r\n"
 "          /FOR /P:partition-no |/PRI[:nth] |/LOG[:nth] [/V:label]\r\n"
 "          /CRE [/PRI |/EXT |/LOG] [/SZ:mb|pcnt%%] [/FOR [/V:label]]\r\n"
 "          Additional /CREATE options: /NTFAT16 /FAT16 /FAT32 /NTFS\r\n\r\n"
 "Examples: part_cmd /print    - print information about the first disk\r\n"
 "          part_cmd /all      - print information about all fixed disks\r\n"
 "          part_cmd 2 /extra  - print detailed information about disk 2\r\n"
 "          part_cmd /del /ext - delete extended partition and all logical drives\r\n"
 "          part_cmd /cre /pri /sz:4000 /for /NTFAT16 - create 4G FAT16 partition\r\n"
 "          part_cmd /for /log:2 /v:BACKUP - format existing second logical drive\r\n"
 "          part_cmd /del /all /nodel:NTFS,82,83 - delete all but 0x7, 0x82, 0x83\r\n\r\n"
 "Note: disk-no and /sure from the command line are applied to all batch commands\r\n"
#endif 
 "",PROGRAM_TITLE );
}

#define MAX_CMD_CHARS 100
#define MAX_CMD_ARGS 20

void cmd_line(int argc, char **argv) // argc>0 is true
{
 DiskInfo *di;
 int largc, num_lines=0;
 char line[MAX_CMD_CHARS];
 char *largv[MAX_CMD_ARGS];

 int cmd_line_sure;

 batch=0;
 debug=0;
 verbose=0;
 quiet=0;
 sure=0;
 gdisk_num=0x80; // first hard disk is default
 gdisk_spec=0;
 int console=0;

 process_line(0,argc,argv);

 cmd_line_sure=sure;
 
 di=get_disk_info_or_print_error(gdisk_num);
 if( di==0 )
   {
    exit(1);
   }

 if( batch ) 
   {
    if( debug ) printf("DEBUG: Opening batch file \"%s\"\r\n",batch);
    
    if( di->num_disks > 1 && !quiet )
      {
       printf("Disk %d will be used as a default for all batch commands.\r\n",
       gdisk_num-0x80+1);
      }

    if( *batch==0 ) batch="CON";
    if( strcmpi(batch,"CON")==0 ) console=1;
    
    int batch_fd=open(batch,O_RDONLY);
    if( batch_fd==-1 )
      {
       printf("ERROR: Cannot open batch file: \"%s\"\r\n",batch);
       exit(1);
      }
    
    while(1) // processing lines from batch file
       {
        int c=0, n=0, EOF_flag=0;
        largc=0;
        num_lines++;
        if( console )
            fprintf(stderr,"Disk %d> ",gdisk_num-0x80+1);
        while(1) // read chars in line
           {
            if( read(batch_fd,&c,1)<1 ) // error or EOF
              {
               EOF_flag=1;
               break;
              }
            if( c=='\n' )
              {
               break;
              }
            if( c=='\r' ) continue; // ignore '\r'
            if( n>0 && line[0]=='#' ) continue; // ignore comments
            if( c=='\t' || c==' ' )
              { 
               if( n==0 || line[n-1]==0 ) continue;
               line[n]=0;
               n++;
               continue;
              }
            // any other character
            if( n>=MAX_CMD_CHARS-1 ||
                n>0 && line[n-1]==0 &&
                largc>=MAX_CMD_ARGS-1 )
              {
               printf("ERROR: Cannot parse line (%d) - line is too long!\r\n",num_lines); 
               exit(1);
              }
            line[n]=c;
            if( n==0 && c=='#' )
              {
               n++;
               continue;
              }
            if( n==0 || line[n-1]==0 )
              {
               largv[largc]=line+n;
               largc++;
              }
            n++;
           }
        line[n]=0;
        if( largc>0 ) // ignore comment line
          {
           sure=cmd_line_sure;
           process_line(num_lines,largc,largv);
          }
        if( EOF_flag ) break;
       }
    close(batch_fd);
   }
}

#define CMD_NONE       0
#define CMD_CREATE     1
#define CMD_DELETE     2 
#define CMD_FORMAT     3
#define CMD_ACTIVATE   4
#define CMD_UPDATE_MBR 5
#define CMD_MBR_STORE  6
#define CMD_MBR_LOAD   7
#define CMD_CMOS_STORE 8
#define CMD_CMOS_LOAD  9

#define FAT_TYPE_ANY      0
#define FAT_TYPE_FAT16    1
#define FAT_TYPE_NTFAT16  2
#define FAT_TYPE_FAT32    3
#define FAT_TYPE_NTFS     4

#define PART_TYPE_NONE    0
#define PART_TYPE_PRI     1
#define PART_TYPE_EXT     2
#define PART_TYPE_LOG     3
#define PART_TYPE_PART_NO 4

#define MAX_TYPE_COUNT 16

int process_line(int line_num,int argc, char**argv)
{
 int i, x;
 DiskInfo *di;
 int command=CMD_NONE;
 int format=0;
 int all_flag=0;
 int part_type=PART_TYPE_NONE;
 int part_num=0;
 int part_index=0;
 int print=0;
 int extra=0;
 char label[12];
 unsigned long part_size=0;
 int size_type_pcent=0;
 int fat_type=FAT_TYPE_ANY;
 int disk_spec=0;
 const char *filename = 0;
 const char *dateTime = 0;

 int no_del_count=0, no_del_type[MAX_TYPE_COUNT];
 
 label[0]=0; 
 
 scan=0;

 if( line_num==0 && argc>0 && strcmpi(argv[0],"-debug")==0 )
   {
    debug=1;
    printf("MODE: Debug mode is on.\r\n");
   }

 if( debug )
   {
    printf("\r\nDEBUG: Processing command line (%d)\r\n",line_num);
    for( i=0 ; i<argc ; i++ )
       {
        printf("DEBUG: argv[%d]=\"%s\"\r\n",i,argv[i]);
       }
   }

 while( argc>0 )
    {
     char *arg=argv[0];
     if( arg[0]=='-' || arg[0]=='/' )
       {
        arg++;
       }
       
     if( arg[0]==0 )
       {
        // do nothing - skip it
       }
     else if( *arg>='1' && *arg<='8' &&  *(arg+1)==0 )
       {
        gdisk_num=*arg-'1'+0x80;
        disk_spec=1;
        if( line_num==0 ) gdisk_spec=1;
       }
     else if( strncmpi(arg,"Debug",5)==0 )
       {
        if( !debug ) printf("MODE: %s mode is on.\r\n","Debug");
        debug=1;
       }
     else if( strncmpi(arg,"Verbose",7)==0 )
       {
        verbose=1;
        printf("MODE: %s mode is on.\r\n","Verbose");
       }
     else if( strncmpi(arg,"Quiet",5)==0 )
       {
        quiet=1;
       }
     else if( strncmpi(arg,"Sure",4)==0 )
       {
        sure=1;
        if( !quiet ) printf("MODE: %s mode is on.\r\n","Sure");
       }
     else if( strncmpi(arg,"help",4)==0 || *arg=='?' )
       {
        usage();
        return 1;
       }
     else if( strncmpi(arg,"batch:",6)==0 )
       {
        batch=arg+6;
       }
     else if( strncmpi(arg,"v:",2)==0 )
       {
        for( i=0 ; i<11 && arg[2+i]!=0 ; i++ )
           {
            int ch=arg[2+i];
            if( ch>='a' && ch<='z' ) ch-='a'-'A';
            label[i]=ch;
           }
        label[i]=0;
       }
     else if( strcmpi(arg,"pri")==0 )
       {
        part_type=PART_TYPE_PRI;
        part_num=1;
       }
     else if( strcmpi(arg,"ext")==0 )
       {
        part_type=PART_TYPE_EXT;
        part_num=1;
       }
     else if( strcmpi(arg,"log")==0 )
       {
        part_type=PART_TYPE_LOG;
        part_num=1;
       }
     else if( strncmpi(arg,"p:",2)==0 )
       {
        char *p=arg+2;
        part_type=PART_TYPE_PART_NO;
        ATOI(part_num,p);
       }
     else if( strncmpi(arg,"log:",4)==0 )
       {
        char *p=arg+4;
        part_type=PART_TYPE_LOG;
        ATOI(part_num,p);
       }
     else if( strncmpi(arg,"pri:",4)==0 )
       {
        char *p=arg+4;
        part_type=PART_TYPE_PRI;
        ATOI(part_num,p);
       }
     else if( strncmpi(arg,"sz:",3)==0 )
       {
        char *p=arg+3;
        ATOI(part_size,p);
        if( part_size >  1048575L ) // *2048 will overflow
          {
           printf("\r\nERROR: Specified size is too large: %,d\r\n",part_size);
           exit(1);
          }
        part_size*=2048;    // converting /sz: in megabytes to # of sectors
        if( *p=='.' )
          {
           int mult=2048;
           p++;
           while( *p>='0' && *p<='9' )
              {
               mult/=10;
               part_size+=(*p-'0')*mult;
               p++;
              }
          }
        if( *p=='%' )
          {
           size_type_pcent=1;
           if( part_size > 100*2048 || part_size<100 )
             {
              printf("\r\nERROR: Specified percentage is invalid: %s\r\n"
                         "       It must be within 0.05%% to 100%%.\r\n",arg+3);
              exit(1);
             }
          }
       }
     else if( strncmpi(arg,"mbr",3)==0 )
       {
        command=CMD_UPDATE_MBR;
       }
     else if( strncmpi(arg,"cre",3)==0 )
       {
        command=CMD_CREATE;
       }
     else if( strncmpi(arg,"for",3)==0 )
       {
        format=1;
        if( command==CMD_NONE )
            command=CMD_FORMAT;
       }
     else if( strncmpi(arg,"del",3)==0 )
       {
        command=CMD_DELETE;
       }
     else if( strncmpi(arg,"act",3)==0 )
       {
        command=CMD_ACTIVATE;
       }
     else if( strncmpi(arg,"all",3)==0 )
       {
        all_flag=1;
       }
     else if( strncmpi(arg,"fat16",5)==0 )
       {
        fat_type=FAT_TYPE_FAT16;
       }
     else if( strncmpi(arg,"ntfat16",7)==0 )
       {
        fat_type=FAT_TYPE_NTFAT16;
       }
     else if( strncmpi(arg,"fat32",5)==0 )
       {
        fat_type=FAT_TYPE_FAT32;
       }
     else if( strncmpi(arg,"ntfs",4)==0 )
       {
        fat_type=FAT_TYPE_NTFS;
       }
     else if( strncmpi(arg,"print",5)==0 )
       {
        print=1;
       }
#ifdef SCAN_OPT
     else if( strncmpi(arg,"scan",4)==0 )
       {
        print=1;
        scan=1;
       }
#endif
     else if( strncmpi(arg,"extra",5)==0 )
       {
        print=1;
        extra=1;
       }
     else if( strncmpi(arg,"nodel:",6)==0 )
       {
        int x, x2, x3, n2;
        char *p=arg+6;
        while( *p!=0 )
           {
            if( *p==',' )
              { p++; continue; }
            x2=0;
            x3=0;
                 if( strncmpi(p,"FAT16",5)==0 ) { x=0x4; x2=0x6; x3=0xE; }
            else if( strncmpi(p,"FAT32",5)==0 ) { x=0xB; x2=0xC; }
            else if( strncmpi(p,"NTFS",4)==0 )  { x=0x7; }
            else x=ATOIX(p);
            while(*p!=',' && *p!=0 ) p++;
            if( x==0 ) continue;
            n2=0;
            if( x2!=0 ) n2++;
            if( x3!=0 ) n2++;
            if( no_del_count+n2>=MAX_TYPE_COUNT )
              {
               printf("\r\nERROR: Option contains too many arguments: \"%s\"\r\n",arg);
               exit(1);
              }
            no_del_type[no_del_count]=x;
            no_del_count++;
            if( x2!=0 )
              {
               no_del_type[no_del_count]=x2;
               no_del_count++;
              }
            if( x3!=0 )
              {
               no_del_type[no_del_count]=x3;
               no_del_count++;
              }
           }
       }
     else if( strncmpi(arg,"mbs",3)==0 )
       {
        command=CMD_MBR_STORE;
       }
     else if( strncmpi(arg,"mbl",3)==0 )
       {
        command=CMD_MBR_LOAD;
       }
     else if( strncmpi(arg,"cms",3)==0 )
       {
        command=CMD_CMOS_STORE;
       }
     else if( strncmpi(arg,"cml",3)==0 )
       {
        command=CMD_CMOS_LOAD;
       }
     else if( strncmpi(arg,"f:",2)==0 )
       {
        filename = arg+2;
       }
     else if( strncmpi(arg,"t:",2)==0 )
       {
        dateTime = arg+2;
       }
     else
       {
        printf("\r\nERROR: Unknown command in line (%d): \"%s\"\r\n",line_num,argv[0]);
        exit(1);
       }
     argc--;
     argv++;
    }// while(argc>0)

 if( line_num==0 && batch )
   {
    return 0;
   }

 di=get_disk_info_or_print_error(gdisk_num);
 if( di==0 )
   {
    exit(1);
   }

 if( di->num_disks > 1 &&
     command!=CMD_NONE &&
     !(disk_spec || gdisk_spec) )
   {
    printf("ERROR: You MUST specify disk number on the systems with multiple disks!\r\n");
    exit(1);
   }
  
 if( size_type_pcent )
   {
    // we need: part_size = (di->total_sects/100) * (part_size/2048);
    // better:  part_size = (di->total_sects*part_size) / (100*2048);
    // but this could easy overflow, so:
    if( part_size==100*2048 )  // 100%
      {
       part_size=di->total_sects - di->sect_per_track;
      }
    else if( di->total_sects > 1048576L ) // *2048 could overflow
      {
       part_size = (di->total_sects/2048) * (part_size/100);
      }
    else
      {
       part_size = (di->total_sects) * (part_size/100) / 2048;
      }
   }

 if( command!=CMD_CREATE && part_num!=0 )
   {
    part_index=get_partition_index(di,part_type,part_num);
    if( part_index==0 )
      {
       printf("ERROR: Cannot find referenced partition!\r\n");
       exit(1);
      }
   }
 
 if( command==CMD_NONE || print ) {
	if( !all_flag ) {
		print_disk_info_header_ext();
		print_partitions(di,extra);
    } else {
		int n_disks=di->num_disks;
		print_disk_info_header_ext();
		for( i=0 ; i<n_disks ; i++ ) {
           di=get_disk_info_or_print_error(0x80+i);
           print_partitions(di,extra);
        }
	}
 }
 else if( command==CMD_UPDATE_MBR )
   {
    update_mbr_ipl_code(di);
   }
 else if( command==CMD_FORMAT )
   {
    if( part_index==0 )
      {
       printf("ERROR: \"/%s\" requires a reference to the partition!\r\n","FOR");
       exit(1);
      }
    format_fat_partition(di,part_index,label);
   }
 else if( command==CMD_ACTIVATE )
   {
    if( part_index==0 || 
        LOGICAL(&(di->part[part_index])) ||
        EXTENDED(&(di->part[part_index])) )
      {
       printf("ERROR: \"/%s\" requires a reference to the PRIMARY partition!\r\n","ACT");
       exit(1);
      }
    activate_partition(di,part_index);
   }
 else if( command==CMD_DELETE )
   {
    if( all_flag )
      {
       if( no_del_count==0 )
          delete_all_partitions(di);
       else
          delete_all_partitions_except(di,no_del_count,no_del_type);
      }
    else
      {
       if( part_index==0 )
         {
          printf("ERROR: \"/%s\" requires a reference to the partition!\r\n","DEL");
          exit(1);
         }
       delete_partition(di,part_index);
      }
   }
 else if( command==CMD_CREATE )
   {
    if( part_type==PART_TYPE_NONE || 
        part_type==PART_TYPE_PART_NO )
      {
       printf("ERROR: \"/CRE\" requires partition type: /PRI, /EXT, or /LOG\r\n");
       exit(1);
      }
    if( label[0]!=0 && !format )
      {
       printf("ERROR: Label has no meaning unless \"/FOR\" is requested.\r\n");
       exit(1);
      }
    create_partition(di, part_type, part_size, fat_type, format, label);
   }
 else if( command==CMD_MBR_STORE )
   {
    if(!filename)
      {
       printf("ERROR: \"/MBS\" requires a file name: /F:filename\n");
       exit(1);
      }
    if (MBR::StoreMBR(filename, di->part[0].part_boot_sect) != 0) {
      printf("ERROR: unable to save the MBR to '%s'\n", filename);
      exit(1);
    }
   }
 else if( command==CMD_MBR_LOAD )
   {
      if(!filename) {
        printf("ERROR: \"/MBL\" requires a file name: /F:filename\n");
        exit(1);
      }
			char mbr[MBR::MBR_BUFFER_SIZE];
			if (MBR::LoadMBR(filename, mbr) != 0) {
        printf("ERROR: unable to load the MBR from '%s'\n", filename);
        exit(1);
			}
			
			if (write_boot_sect(di, &di->part[0], &mbr, 1) == -1) {
        printf("ERROR: unable to write the MBR to disk\n");
        exit(1);
      }
   }
 else if( command==CMD_CMOS_STORE )
   {
    if(!filename)
      {
       printf("ERROR: \"/CMS\" requires a file name: /F:filename\n");
       exit(1);
      }
    char cmosData[CMOS::CMOS_BUFFER_SIZE] = {0};
    CMOS::ReadCMOSData(cmosData);
    if (MBR::StoreMBR(filename, cmosData) != 0) {
      printf("ERROR: unable to save CMOS data to '%s'\n", filename);
      exit(1);
    }
   }
 else if( command==CMD_CMOS_LOAD )
   {
    if(!filename)
      {
       printf("ERROR: \"/CML\" requires a file name: /F:filename\n");
       exit(1);
      }

    CMOS::RTCTimeSource timeSource = CMOS::RTC_USE_USERDATA;
    CMOS::RTCTime time;
    if (!dateTime) {
      timeSource = CMOS::RTC_KEEP_CMOS;
    } else if (*dateTime == '\0') {
      timeSource = CMOS::RTC_USE_SAVED;
    } else {
			if (strlen(dateTime) != 2 * 6 + 5) {
				printf("ERROR: wrong date/time '%s'\n", dateTime);
				exit(1);
			}

			char val;
			int ok = 1;
      time.SetFormat(CMOS::RTCTime::Format_Binary);
      time.SetRange(CMOS::RTCTime::HourRange_24);

			ok &= CMOS::ToTimeItem(dateTime, 2, &val);
			dateTime += 3;
      time.SetYear(val);

			ok &= CMOS::ToTimeItem(dateTime, 2, &val);
			dateTime += 3;
      time.SetMonth(val);

			ok &= CMOS::ToTimeItem(dateTime, 2, &val);
			dateTime += 3;
      time.SetDay(val);

			ok &= CMOS::ToTimeItem(dateTime, 2, &val);
			dateTime += 3;
      time.SetHour(val);

			ok &= CMOS::ToTimeItem(dateTime, 2, &val);
			dateTime += 3;
      time.SetMinutes(val);

			ok &= CMOS::ToTimeItem(dateTime, 2, &val);
      time.SetSeconds(val);

			if (!ok) {
				printf("ERROR: wrong date/time '%s'\n", dateTime);
				exit(1);
			}
    }

    if (CMOS::ReadPatchAndWriteCMOSData(filename, timeSource, time) != 0) {
      printf("ERROR: unable to load CMOS data from '%s'\n", filename);
      exit(1);
    }
   }

 if( debug )
     printf("\r\nDEBUG: Finished processing line (%d)\r\n\r\n",line_num);

 return 0;
}


void print_disk_info_header(void)
{
 if( !scan )
 printf("Disk #  Int13  Partitions  Cylinders Heads Sectors  Mbytes   Used\r\n");
}

void print_disk_info_header_ext() 
{
#ifdef SCAN_OPT
if ( scan ) {
printf("DiskNumber\tTranslation\tPrimaryPartitionCount\tLogicalPartitionCount\t\
TotalPartitionCount\tCylinders\tHeads\t\Sectors\tTotalMB\tPercentOfDisk\t\
PartitionNumber\tPartitionType\tSeqPartNumber\tActivePartition\tFileSystem\t\
VolumeLabel\tPartStartCyl\tPartEndCyl\tSizeMB\tUsePercentage\r\n");	
} else
#endif
  printf("Disk #  Int13  Partitions  Cylinders Heads Sectors  Mbytes   Used\r\n");


}

void print_disk_info(DiskInfo* di)
{
 int i, n_parts=0, n_pri=0, n_log=0;
 Part *p, *part=di->part;
 Sector used=0;
 for( p=part+1, i=1 ; i<P_ROWS ; i++, p++ )
  if( !UNUSED(p) && !(LOGICAL(p) && EXTENDED(p)) )
    {
     n_parts++;
     if( LOGICAL(p) ) n_log++;
      else n_pri++;
     if( !EXTENDED(p) ) used+=p->num_sect;
    }

#ifdef SCAN_OPT
  if (scan) {
		printf("%d\t(%s)\t%d\t%d\t%d\t%d\t%d\t%d\t%d.%d\t%d.%d%%",
					di->hd_num-128, (di->flags&1)?"LBA":"CHS",n_pri, n_log, n_parts,
					di->num_cyls, di->num_heads, di->sect_per_track, di->total_sects/2048, 
					(di->total_sects%2048)*10/2048, (used/di->sect_per_track*100)/((di->total_sects-di->sect_per_track)/di->sect_per_track),
					(used/di->sect_per_track*100)%((di->total_sects-di->sect_per_track)/di->sect_per_track)*10/(di->total_sects-di->sect_per_track) );
  } else
#endif
   printf("%s %d  (%s)  %d +%2d = %2d  [%,7d x %3d x %d ] %,7d.%d  %3d.%d%%\r\n",
	"Disk", di->hd_num-127, (di->flags&1)?"LBA":"CHS", n_pri, n_log, n_parts,
	di->num_cyls, di->num_heads, di->sect_per_track, di->total_sects/2048, 
	(di->total_sects%2048)*10/2048, (used/di->sect_per_track*100)/((di->total_sects-di->sect_per_track)/di->sect_per_track),
	(used/di->sect_per_track*100)%((di->total_sects-di->sect_per_track)/di->sect_per_track)*10/(di->total_sects-di->sect_per_track) );
}


#define PART_INFO_MODE_LABEL   0
#define PART_INFO_MODE_SHORT   1
#define PART_INFO_MODE_SCAN    2
#define PART_INFO_MODE_DETAILS 3

void sprintf_partition(char *buf,DiskInfo* di, int index, int mode)
{
 int i;
 Part *p=&di->part[index];
 char tmp[24];
 char tmp2[24];
 char tmp3[24];
 char tmp_lbl[12], *s;
 char *fs_name, *fs_short_name;

 sprintf_partition_header(tmp,di,index);

 s=get_partition_label(di,p);

 for( i=0 ; i<sizeof(tmp_lbl)-1 && s[i] ; i++ )
    {
     tmp_lbl[i]=s[i];
     if( s[i]=='\t' ) tmp_lbl[i]=' ';
    }
 tmp_lbl[i]=0;
 
 fs_name = fs_desc[p->fs_num].name;
 fs_short_name  = fs_name;
 int len = strlen(fs_name);
 if( len>10 ) fs_short_name += len+1;


 if( fs_desc[p->fs_num].fs_id == FS_HIDDEN ||
     fs_desc[p->fs_num].fs_id == FS_UNKNOWN )
   {
    sprintf(tmp2,"%s (0x%02X)", fs_name, p->fs_type>>8 );
    fs_name=tmp2;
   }


 if( mode==PART_INFO_MODE_LABEL )
   {
    sprintf(buf,"%s  %-21s  %-11s %,7d.%d  %3d.%d%c",
            tmp,
            fs_name, tmp_lbl,
            p->num_sect/2048, (p->num_sect%2048)*10/2048,
            p->num_sect*100/(di->total_sects-di->sect_per_track),
            (p->num_sect*100%(di->total_sects-di->sect_per_track))*10/(di->total_sects-di->sect_per_track),
            EXTENDED(p) ? '*' : '%' );
   }
 else if( mode==PART_INFO_MODE_SCAN )
   {
    sprintf(buf,"%s\t%s\t%s\t%d\t%d\t%d.%d\t%d.%d%%",
            tmp,
            fs_short_name,
            tmp_lbl,
            p->start_cyl,
            p->end_cyl, 
            p->num_sect/2048, (p->num_sect%2048)*10/2048,
            p->num_sect*100/(di->total_sects-di->sect_per_track),
            (p->num_sect*100%(di->total_sects-di->sect_per_track))*10/(di->total_sects-di->sect_per_track) );
   }
 else if( mode==PART_INFO_MODE_SHORT )
   {
    sprintf(buf,"%s %s  %s  %,d.%d Mb",
            tmp,
            fs_short_name, tmp_lbl,
            p->num_sect/2048, (p->num_sect%2048)*10/2048 );
   }
 else if( mode==PART_INFO_MODE_DETAILS )
   {
    sprintf(buf,"%s %02Xh%,6d %3d %2d %,6d %3d %2d %,11d %,11d %,11d",
            tmp,
            p->fs_type>>8,
            p->start_cyl,  p->start_head, p->start_sect,
            p->end_cyl,    p->end_head,   p->end_sect,
            p->rel_sect,   p->num_sect,   p->last_sect );

    if( index==0 ) // MBR
      {
       buf[11]=buf[12]=buf[13]=' ';
      }
   }
}


void sprintf_partition_header(char *header, DiskInfo *di,int index)
{
 int i, n, x=0;
 Part *p, *part;
 int log=0;
 int pri=0;
 char tmp1[8];
 char tmp2[8];
 
 if( index==0 )
   {
    sprintf(header,scan?"%s\tMBR\t\t":"   MBR    ");
    return;
   }

 sprintf(tmp1,"  ");
 sprintf(tmp2,"  ");

 part=di->part;
 n=di->num_part;
 
 for( p=part+1, i=1 ; i<n ; i++, p++ )
    {
     if( LOGICAL(p) && EXTENDED(p) ) continue;
     if( UNUSED(p) && p->num_sect<=63 ) continue;
     if( !UNUSED(p) )
       {
        if( LOGICAL(p) ) x=++log;
                    else x=++pri;
       }
     if( i==index )
       {
        if( !UNUSED(p) )
          {
           sprintf(tmp1,scan?"%d":"%-2d",pri+log);
           sprintf(tmp2,scan?"%d":":%d",x);
          }
        break;
       }
     x=0;
    }

 p=&part[index];

 if( LOGICAL(p) ) {
	if( EXTENDED(p) ) {
      sprintf(header,scan?"\t%s\tExt\t\t":"%s �Ext   ",tmp1);
	} else {
      sprintf(header,scan?"\t%s\tLog\t%s\t":"%s � Log%s",tmp1, tmp2+((x>9)?1:0));
	}
 } else { 
#ifdef SCAN_OPT
	if (scan) {
		if (UNUSED(p)) {
			sprintf(header,"\t\tPri\t\t");
		} else {
			if (p->tag) {
				sprintf(header,"\t%s\tPri\t%s\t%c",tmp1, tmp2, 'A');
			} else {
				sprintf(header,"\t%s\tPri\t%s\t",tmp1, tmp2);
			}
		}
	} else
#endif	
		sprintf(header,"%s Pri%s %c",tmp1, tmp2,p->tag?'A':' ');
 }	
	 
}


int get_partition_index(DiskInfo* di, int part_type, int num)
{
 int i, n;
 Part *p, *part;

 part=di->part;
 n=di->num_part;
 
 int log=0;
 int pri=0;
 
 for( p=part+1, i=1 ; i<n ; i++, p++ )
    {
     if( EXTENDED(p) && part_type==PART_TYPE_EXT ) break;
     if( LOGICAL(p) && EXTENDED(p) ) continue;
     if( UNUSED(p) && p->num_sect<=63 ) continue;
     if( !UNUSED(p) )
       {
        if( LOGICAL(p) )
          {
           log++;
           if( part_type==PART_TYPE_LOG && num==log ) break;
          }
        else
          {
           pri++;
           if( part_type==PART_TYPE_PRI && num==pri ) break;
          }
        if( part_type==PART_TYPE_PART_NO )
          {
           if( num==pri+log ) break;
          }
       }
    }
 return (i<n) ? i : 0;
}


void print_partitions(DiskInfo *di,int details)
{
 int i, j, problems, n;
 Mbr *mbr;
 Part *p, ptmp, *part;
 char tmp[120];
#ifdef SCAN_OPT
 int firstPart = 1;
#endif

 part=di->part;
 print_disk_info(di);
 
 n=di->num_part;

 problems=0;
 for( p=part, i=0 ; i<n ; i++, p++ )
  if( !VALID(p) )
    {
     problems=1;
     break;
    }

if( !scan )
printf(
//"%s   Active                  Volume     Starting  Ending  Partition  %% of\r\n"
//"%c # Type:#  File System Type      Label      Cylinder Cylinder Size [Mb]  Disk\r\n\r\n",
"%s   Active                    Volume      Partition   %% of\r\n"
"%c # Type:#   File System Type       Label       Size [Mb]   disk\r\n\r\n",
 problems ? "Problems" : "        ", problems ? '' : ' ');
 for( p=part+1, i=1 ; i<n ; i++, p++ ) {
     if( LOGICAL(p) && EXTENDED(p) ) continue;
     if( UNUSED(p) && p->num_sect<=63 ) continue;
     sprintf_partition(tmp,di,i,scan?PART_INFO_MODE_SCAN:PART_INFO_MODE_LABEL);
#ifdef SCAN_OPT
     if( scan ) {
		if (firstPart != 1) {
			// Tab over to align the partition information
			printf("\t\t\t\t\t\t\t\t\t");
		}
		printf("%s", tmp);
		if (!VALID(p)) {
			printf("\t%s", "<-Error(s)");
		}
		printf("\r\n");
		firstPart = 0;
     } else
#endif
         printf("%c %s\r\n", !VALID(p)?'!':' ', tmp );
     
 }

#ifdef SCAN_OPT
 if( scan )
     return;
#endif

 if( !details )
   {
    if( problems )
      printf("There are problems with one or more partitions.\r\n"
             "Please, refer to the detailed disk view for the error descriptions.\r\n");
    return;
   }

 if( di->cur_ipl!=IPL_TYPE_STD )
     printf("Master Boot Record DOES NOT HAVE standard IPL code.\r\n\r\n");

printf("Partition table details:\r\n\r\n"
"%s   Active    Starting       Ending     Starting   Number of      Ending\r\n"
"%c # Type:#  FS    Cyl Head Sct  Cyl Head Sct    sector     sectors      sector\r\n\r\n",
 problems ? "Problems" : "        ", problems ? '' : ' ');

 for( p=part, i=0 ; i<n ; i++, p++ )
    {
     sprintf_partition(tmp,di,i,PART_INFO_MODE_DETAILS);
     printf("%c %s\r\n", !VALID(p)?'!':' ', tmp );
    }

 printf("\r\n");

 for( p=part, i=0 ; i<n ; i++, p++ )
  if( !VALID(p) )
    {
     sprintf_partition(tmp,di,i,PART_INFO_MODE_SHORT);
     printf("Problems with partition %s\r\n\r\n",tmp);
     if( p->flags & P_ERR_ZERO      ) printf("   %s\r\n",ERR_ZERO);
     if( p->flags & P_ERR_RANGE     ) printf("   %s\r\n",ERR_RANGE);
     if( p->flags & P_ERR_OVERLAP   ) printf("   %s\r\n",ERR_OVERLAP);
     if( p->flags & P_ERR_LOG_ORPH  ) printf("   %s\r\n",ERR_LOG_ORPH);
     if( p->flags & P_ERR_LOG_STRU  ) printf("   %s\r\n",ERR_LOG_STRU);
     if( p->flags & P_ERR_CANT_READ ) printf("   %s\r\n",ERR_CANT_READ);
     if( p->flags & P_ERR_BOOT_SECT ) printf("   %s\r\n",ERR_BOOT_SECT);
     printf("\r\n");
    }

// print detailed IPL info here

printf("Partition records exactly as they appear in MBR (EMBR):\r\n\r\n"
"                   Starting          Ending        Starting   Number of\r\n"
"    #  HD  FS    Cyl Head Sect    Cyl Head Sect      sector     sectors\r\n");

 for( p=part, i=0 ; i<n ; i++, p++ )
  if( i==0 || EXTENDED(p) )
    {
     printf("(%d,%d,%d):\r\n", p->start_cyl, p->start_head, p->start_sect );
     read_boot_sect(di,p,1);
     if( !BTS_LOADED(p) )
       {
        printf("%s\r\n",ERR_CANT_READ);
        continue;
       }
     mbr = (Mbr*) p->part_boot_sect;
     if( mbr->magic_num!=MAGIC_NUM )
        printf("Magic number is not (0xAA55)!!!\r\n");
       
     for( j=0 ; j<4 ; j++ )
        {
         unpack_part_rec(&mbr->part_rec[j],&ptmp);
         sprintf_partition(tmp,j+1,&ptmp,MODE_EXACT);
         printf("   %s\r\n",tmp);
        }
    }
}/* print_partitions */


DiskInfo* get_disk_info_or_print_error(int hd)
{
 DiskInfo *di=get_disk_info(hd);

 if( di==0 )
   {
    printf("ERROR: Cannot get hard disk information from hd=0x%02X\r\n",hd);
    return 0;
   }
 return di;
}


void update_mbr_ipl_code(DiskInfo* di)
{
 if( !sure )
   {
    fprintf(stderr,"You are about to update MBR code on disk %d\r\n",di->hd_num-0x80+1);
    if( !confirm() )
      {
       printf("\"/%s\" command is canceled by user\r\n","MBR");
       exit(1);
      }
   }

 if( debug || verbose ) 
   printf("Updating MBR code on disk %d ...\r\n",di->hd_num-0x80+1);

 di->new_ipl=IPL_TYPE_STD;
 if( write_disk_info(di)==-1 )
   {
    printf("ERROR: There was an error writing partition information to disk %d\r\n",di->hd_num-0x80+1);
    exit(1);
   }

 if( !quiet )
   printf("MBR code on disk %d updated.\r\n",di->hd_num-0x80+1);
}


void delete_all_partitions(DiskInfo* di)
{
 int i;

 if( debug || verbose )
	print_disk_info_header_ext();		
	print_partitions(di,0);

 if( !sure )
   {
    fprintf(stderr,"You are about to delete all partitions on disk %d\r\n",di->hd_num-0x80+1);
    if( !confirm() )
      {
       printf("\"/%s\" command is canceled by user\r\n","DEL /ALL");
       exit(1);
      }
   }

 if( debug || verbose )
   printf("Deleting all partitions on disk %d ...\r\n",di->hd_num-0x80+1);

 for( i=1 ; i<P_ROWS ; i++ )
    {
     free_part_info( &(di->part[i]) );
     memset( &(di->part[i]), 0, sizeof(Part) );
    }
 
 i=0;
 sort_disk_info(di,&i);

 if( write_disk_info(di)==-1 )
   {
    printf("ERROR: There was an error writing partition information to disk %d\r\n",di->hd_num-0x80+1);
    exit(1);
   }

 if( !quiet ) 
   printf("All partitions on disk %d are deleted.\r\n",di->hd_num-0x80+1);
}



void delete_all_partitions_except(DiskInfo* di, int no_del_count, int *no_del_type)
{
 int i;
 int part_del_flag[P_ROWS];
 Part *p;
 char tmp[80];
 int n_to_del=0;

 if( debug || verbose )
   {
    print_disk_info_header_ext();
    //print_disk_info(di);
    print_partitions(di,0);
    printf("\r\nUser requested to delete all partitions, except: ");
    for( i=0 ; i<no_del_count ; i++ )
      printf("%s0x%x",(i==0)?"":", ",no_del_type[i]);
    printf("\r\n");
   }

 for( i=1 ; i<P_ROWS ; i++ )
    {
     p = & di->part[i];
     int part_type = (p->fs_type >> 8);
     int j;
     part_del_flag[i]=1; // delete all partitions by default
     for( j=0 ; j<no_del_count ; j++ )
       if( part_type==no_del_type[j] ) 
         {
          part_del_flag[i]=0;
          break;
         }
    }

 for( i=1 ; i<P_ROWS ; i++ )
  if( part_del_flag[i] && di->part[i].fs_type!=0 )
    {
     n_to_del++;
     if( !sure || !quiet || verbose || debug )
       {
        sprintf_partition(tmp,di,i,PART_INFO_MODE_SHORT);
        fprintf(stderr,"Deleting partition (%s) from disk %d\r\n",
                        tmp, di->hd_num-0x80+1);
       }
    }

  if( n_to_del==0 )
    {
     if( !quiet ) 
       printf("No partitions were deleted on disk %d.\r\n",di->hd_num-0x80+1);
     return;
    }


  if( !sure )
    {
     if( !confirm() )
       {
        printf("\"/%s\" command is canceled by user\r\n","DEL");
        exit(1);
       }
    }

 int x; 
 for( x=1 ; x<P_ROWS ; x++ )
  if( part_del_flag[x] )
    {
     p = & di->part[x];

 if( EXTENDED(p) && !LOGICAL(p) ) // Primary extended - deleting everything inside
   {
    for( i=1 ; i<P_ROWS ; i++ )
     if( LOGICAL(&(di->part[i])) || EXTENDED(&(di->part[i])) )
       {
        free_part_info( &(di->part[i]) );
        memset( &(di->part[i]), 0, sizeof(Part) );
       }
   }
 else if( LOGICAL(p) )
   {
    if( EXTENDED(p) ) // one of the internal extended partitions
      {
       for( i=1 ; i<P_ROWS ; i++ )
        if( di->part[i].ext_base==p->rel_sect )
          {
           free_part_info( &(di->part[i]) );
           memset( &(di->part[i]), 0, sizeof(Part) );
          }
       free_part_info(p);
       memset( p, 0, sizeof(Part) );
      }
    else // logical disk
      {
       for( i=1 ; i<P_ROWS ; i++ )
        if( di->part[i].rel_sect==p->ext_base &&
            LOGICAL(&(di->part[i])) )
          {
           free_part_info( &(di->part[i]) );
           memset( &(di->part[i]), 0, sizeof(Part) );
          }
       free_part_info(p);
       memset( p, 0, sizeof(Part) );
      }
   }
 else  // primary
   {
    free_part_info(p);
    memset( p, 0, sizeof(Part) );
   }
 }
 i=0;
 sort_disk_info(di,&i);

 if( write_disk_info(di)==-1 )
   {
    printf("ERROR: There was an error writing partition information to disk %d\r\n",di->hd_num-0x80+1);
    exit(1);
   }

 if( !quiet ) 
   printf("Partitions on disk %d are deleted.\r\n",di->hd_num-0x80+1);
}





void delete_partition(DiskInfo *di,int index)
{
 int i;
 Part *p = & di->part[index];
 char tmp[80];

 sprintf_partition(tmp,di,index,PART_INFO_MODE_SHORT);

 if( !sure )
   {
   
    fprintf(stderr,"Deleting partition (%s) from disk %d\r\n",
                    tmp, di->hd_num-0x80+1);
    if( !confirm() )
      {
       printf("\"/%s\" command is canceled by user\r\n","DEL");
       exit(1);
      }
   }

 if( debug || verbose ) 
   printf("Deleting partition (%s) from disk %d ...\r\n",tmp,di->hd_num-0x80+1);

 if( EXTENDED(p) && !LOGICAL(p) ) // Primary extended - deleting everything inside
   {
    for( i=1 ; i<P_ROWS ; i++ )
     if( LOGICAL(&(di->part[i])) || EXTENDED(&(di->part[i])) )
       {
        free_part_info( &(di->part[i]) );
        memset( &(di->part[i]), 0, sizeof(Part) );
       }
   }
 else if( LOGICAL(p) )
   {
    if( EXTENDED(p) ) // one of the internal extended partitions
      {
       for( i=1 ; i<P_ROWS ; i++ )
        if( di->part[i].ext_base==p->rel_sect )
          {
           free_part_info( &(di->part[i]) );
           memset( &(di->part[i]), 0, sizeof(Part) );
          }
       free_part_info(p);
       memset( p, 0, sizeof(Part) );
      }
    else // logical disk
      {
       for( i=1 ; i<P_ROWS ; i++ )
        if( di->part[i].rel_sect==p->ext_base &&
            LOGICAL(&(di->part[i])) )
          {
           free_part_info( &(di->part[i]) );
           memset( &(di->part[i]), 0, sizeof(Part) );
          }
       free_part_info(p);
       memset( p, 0, sizeof(Part) );
      }
   }
 else  // primary
   {
    free_part_info(p);
    memset( p, 0, sizeof(Part) );
   }
 
 i=0;
 sort_disk_info(di,&i);

 if( write_disk_info(di)==-1 )
   {
    printf("ERROR: There was an error writing partition information to disk %d\r\n",di->hd_num-0x80+1);
    exit(1);
   }

 if( !quiet ) 
   printf("Partition  (%s) deleted from disk %d.\r\n",tmp,di->hd_num-0x80+1);
}


void activate_partition(DiskInfo *di,int index)
{
 int i;
 Part *p = & di->part[index];
 char tmp[80];

 sprintf_partition(tmp,di,index,PART_INFO_MODE_SHORT);

 if( !sure )
   {
    fprintf(stderr,"Activating partition (%s) on disk %d ...\r\n",
                    tmp, di->hd_num-0x80+1);
    if( !confirm() )
      {
       printf("\"/%s\" command is canceled by user\r\n","ACT");
       exit(1);
      }
   }

 if( debug || verbose ) 
   printf("Activating partition (%s) on disk %d ...\r\n",tmp,di->hd_num-0x80+1);

 for( i=1 ; i<P_ROWS ; i++ )
    {
     di->part[i].tag=0;
    }
 p->tag=0x80;

 if( write_disk_info(di)==-1 )
   {
    printf("ERROR: There was an error writing partition information to disk %d\r\n",di->hd_num-0x80+1);
    exit(1);
   }

 if( !quiet ) 
   printf("Partition  (%s) activated on disk %d.\r\n",tmp,di->hd_num-0x80+1);
}


void format_fat_partition(DiskInfo* di,int index,char *label)
{
 Part *p=&di->part[index];
 int x=(p->fs_type|0x1000);
 char tmp[80];

 sprintf_partition(tmp,di,index,PART_INFO_MODE_SHORT);


 if( !(x==0x1100|| x==0x1400|| x==0x1600|| x==0x1B00|| x==0x1C00|| x==0x1E00))
   {
    printf("ERROR: Cannot format partition of any type other than FAT !!!\r\n");
    exit(1);
   }

 if( !sure )
   {
    fprintf(stderr,"You are about to format partition (%s) on disk %d\r\n",
            tmp,di->hd_num-0x80+1);
    if( !confirm() )
      {
       printf("\"/%s\" command is canceled by user\r\n","FOR");
       exit(1);
      }
   }

 if( debug || verbose ) 
   printf("Formating partition (%s) on disk %d ...\r\n",tmp,di->hd_num-0x80+1);

 free_part_info(p);

 if( p->fs_type==0x0B00 || p->fs_type==0x0C00 ||
     p->fs_type==0x1B00 || p->fs_type==0x1C00 )  fat32_format(di,p,0,0,label);
                                            else fat16_format(di,p,0,0,label);
 if( !quiet ) 
   printf("Partition (%s) on disk %d was formated.\r\n",tmp,di->hd_num-0x80+1);
}


void create_partition(DiskInfo* di, int part_type, unsigned long part_size,
                                    int fat_type, int format, char *label )
{
 int i, part_index;
 Part *p;
 char tmp[40];
 
 if( part_type != PART_TYPE_EXT )
   {
    if( fat_type==FAT_TYPE_FAT16 && part_size!=0 )
        part_size=min(part_size,MAX_FAT16_SIZE);
    else if( fat_type==FAT_TYPE_NTFAT16 && part_size!=0 )
        part_size=min(part_size,MAX_NTFAT16_SIZE);
   }
 
 if( part_size!=0 )
   {
    part_size=max(part_size,di->sect_per_cyl-di->sect_per_track);
   }

 int n_pri=0;
 int ext_found=0;
 for( i=1 ; i<P_ROWS ; i++ )
  if( !UNUSED(&(di->part[i])) )
    {
     if( EXTENDED(&(di->part[i])) ) ext_found=1;
     if( !LOGICAL(&(di->part[i])) ) n_pri++;
    }

 if( n_pri==4  && part_type!=PART_TYPE_LOG ) 
   {
    printf("ERROR: Disk already has 4 primary partitions!\r\n");
    exit(1);
   }

 if( ext_found && part_type==PART_TYPE_EXT )
   {
    printf("ERROR: Extended partition already exists on the disk!\r\n");
    exit(1);
   }

 if( !ext_found && part_type==PART_TYPE_LOG )
   {
    printf("ERROR: To create /LOG partition you must create /EXT first!\r\n");
    exit(1);
   }

 int i_max=0;
 Sector max=0;
 for( i=1 ; i<P_ROWS ; i++ )
    {
     p=&(di->part[i]);

     if( !UNUSED(p) )
         continue;

     if( part_type==PART_TYPE_LOG && !LOGICAL(p) ||
         part_type!=PART_TYPE_LOG &&  LOGICAL(p) )
       {
        continue;
       }
     if( part_size!=0 )
       {
        if( p->num_sect >= part_size ||
            fat_type==FAT_TYPE_FAT16 && p->num_sect>=MAX_FAT16_SIZE ||
            fat_type==FAT_TYPE_NTFAT16 && p->num_sect>=MAX_NTFAT16_SIZE )
          {
           i_max=i;
           max=p->num_sect;
           break;
          }
       }
     // if part_size==0 - request to find largest match
     if( p->num_sect > max )
       {
        i_max=i;
        max=p->num_sect;
       }
    }
 
 if( i_max==0 || part_size==0 && max < di->sect_per_cyl - di->sect_per_track ||
                 part_size!=0 && max < part_size )
   {
    if( part_size==0 ) part_size=di->sect_per_cyl - di->sect_per_track;
    printf("ERROR: Not enough space on disk %d to create %,d.%d Mb partition.\r\n"
           "       Maximum size available for %s partition is %,d.%d Mb = %,d%%.\r\n",
            di->hd_num-0x80+1,
            part_size/2048, (part_size%2048)*10/2048,
                  (part_type==PART_TYPE_PRI)?"/PRI":
                  ((part_type==PART_TYPE_EXT)?"/EXT":"/LOG"),
                  max/2048,       (max%2048)*10/2048,
                  (max/di->sect_per_track*100)/((di->total_sects-di->sect_per_track)/di->sect_per_track) );
    exit(1);
   }

 part_index=i_max;
 if( part_size==0 )
   {
    part_size=max;
   }


 if( part_type != PART_TYPE_EXT )
   {
    if( fat_type==FAT_TYPE_FAT16 && part_size!=0 )
        part_size=min(part_size,MAX_FAT16_SIZE);
    else if( fat_type==FAT_TYPE_NTFAT16 && part_size!=0 )
        part_size=min(part_size,MAX_NTFAT16_SIZE);
   }
 

 int fs_type;
 if( part_type==PART_TYPE_EXT )
   {
    fs_type=0x0500; // Extended
   }
 else
   {
    if( fat_type==FAT_TYPE_NTFS )
      {
       fs_type=0x0700; // NTFS
      }
    else if( fat_type==FAT_TYPE_FAT16 ||
             fat_type==FAT_TYPE_NTFAT16 ||
             fat_type==FAT_TYPE_ANY && part_size <= MAX_FAT16_SIZE )
      {
       fs_type=0x0600; // FAT-16
      }
    else
      {
       fs_type=0x0B00; // FAT-32
      }
   }

 p=&(di->part[part_index]);
 p->fs_type=fs_type;
 find_fs_num(p);
 p->num_sect=part_size;
 recalculate_partition(di,p,MODE_LBA);
 if( p->end_sect!=di->sect_per_track ||
     p->end_head!=di->num_heads-1 &&
     p->start_cyl<p->end_cyl )
   {
    p->end_sect=di->sect_per_track;
    p->end_head=di->num_heads-1;
    p->end_cyl--;
    recalculate_partition(di,p,MODE_CHS);
   }
 part_size=p->num_sect;
 if( part_type==PART_TYPE_PRI && // p is the only primary partition
     (n_pri - ext_found)==0 )
   {
    p->tag=0x80;
   }

 if( fat_type == FAT_TYPE_FAT32 &&
     part_size < MIN_FAT32_SIZE )
   {
    printf("FAT-32 must be at least 512M. Use FAT-16 instead.");
    exit(1);
   }

 sprintf(tmp,"%s %,d.%d Mb",
            (fs_type==0x0500)?"Extended":((fs_type==0x0600)?"FAT-16":
            ((fs_type==0x0700)?"NTFS":"FAT-32")),
            part_size/2048, (part_size%2048)*10/2048 );

 if( !sure )
   {
    fprintf(stderr,"You are about to create %s%s partition on disk %d\r\n",
            format ? "and format ":"",
            tmp,
            di->hd_num-0x80+1);
    if( !confirm() )
      {
       printf("\"/%s\" command is canceled by user\r\n","CRE");
       exit(1);
      }
   }
 
 if( debug || verbose ) 
   printf("Creating %s partition on disk %d ...\r\n",tmp,di->hd_num-0x80+1);
 
 part_ins_wiz(di,&part_index);
 sort_disk_info(di,&part_index);

 if( write_disk_info(di)==-1 )
   {
    printf("ERROR: There was an error writing partition information to disk %d\r\n",di->hd_num-0x80+1);
    exit(1);
   }
 
 p=&(di->part[part_index]);

 if( format && fs_type!=0x0500 )
   {
    if( debug || verbose ) 
        printf("Formatting %s partition on disk %d...\r\n",tmp,di->hd_num-0x80+1);

    if( fs_type==0x0700 ) // NTFS
      {
       char tmp[512];
       memset(tmp,0,512);
       write_part(di,p,0,tmp,1);
      }
    else 
		if( fs_type==0x0B00 || fs_type==0x0C00) {
			fat32_format(di,p,0,0,label);
		} else {
			fat16_format(di,p,0,0,label);
		}
   }

 if( !quiet ) 
   printf("Partition (%s) was created on disk %d\r\n",tmp,di->hd_num-0x80+1);
}
