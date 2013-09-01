//#include "ext32.h"

//------------------------------------------

#define MAX_DISKS 4

char *fdgets(char *buf, int max_len, int fd)
{
 char *p=buf;
 int x, n=0;
 while(1)
    {
     if( n==max_len-1 ) break;
     x=read(fd,p,1);
     if( x==0 && n==0 || x==-1 ) return 0;
     if( x==0 || *p=='\n' ) { p++; break; }
     p++;
     n++;
    }
 if( n!=max_len ) *p=0;
 return buf;
}


static struct disk_sim_desc
    {
     int cyls;
     int heads;
     int sects;
     char *file_name;
    } disk_sim_desc[MAX_DISKS];

static unsigned disk_sim_cfg_read=0;
static unsigned disk_sim_count=0;

#define ATOI(var,p) \
 for( (var)=0 ; *(p)>='0' && *(p)<='9' ; (var)=10*(var)+*(p++)-'0' );
#define SKIP_SPACES(p) \
         while( *(p)==' ' || *(p)=='\t' ) (p)++;
 
void read_disk_sim_cfg(void)
{
 int i;
 char tmp[80];
 int cfg=open("disk_sim.cfg",O_RDONLY);
 if( cfg!=-1 )
   {
    while( fdgets(tmp,sizeof(tmp),cfg)!=0 )
       {
        char *p=tmp;
        struct disk_sim_desc* dsim = & (disk_sim_desc[ disk_sim_count ]);
        SKIP_SPACES(p);
        if( p[0]==0 || p[0]=='\r' || p[0]=='\n' || p[0]=='#' ) continue;
        ATOI(dsim->cyls,p); SKIP_SPACES(p);
        ATOI(dsim->heads,p); SKIP_SPACES(p);
        ATOI(dsim->sects,p); SKIP_SPACES(p);
        for( i=strlen(p)-1 ; i>0 && (p[i]=='\r' || p[i]=='\n' ||
                                     p[i]=='\t' || p[i]==' ' ) ; i-- ) p[i]=0;
        if( (dsim->file_name=new char[i+2])==0 ) break;
        strcpy(dsim->file_name,p);
        disk_sim_count++;
        if( disk_sim_count==MAX_DISKS ) break;
       }
    close(cfg);
   }
 else // no cfg file
   {
    if( (cfg=open("disk_sim.cfg",O_WRONLY+O_TRUNC+O_CREAT))!=-1 )
      {
       fdprintf(cfg,"# Cylinders Heads Sectors DiskFileName\r\n");
       fdprintf(cfg,"        128     4      63 disk_sim.x80\r\n");
       fdprintf(cfg,"         24    16      63 disk_sim.x81\r\n");
       fdprintf(cfg,"#       ???    16      63 disk_sim.x82\r\n");
       close(cfg);
       disk_sim_desc[0].cyls  = 128;
       disk_sim_desc[0].heads =   4;
       disk_sim_desc[0].sects =  63;
       disk_sim_desc[0].file_name = "disk_sim.x80";
       disk_sim_desc[1].cyls  = 24;
       disk_sim_desc[1].heads = 16;
       disk_sim_desc[1].sects = 63;
       disk_sim_desc[1].file_name = "disk_sim.x81";
       disk_sim_count=2;
      }
   }
 disk_sim_cfg_read=1;
}

unsigned get_disk_sim_count(void)
{
 if( !disk_sim_cfg_read )
     read_disk_sim_cfg();
 return disk_sim_count;
}

struct disk_sim_desc* get_disk_sim_desc(unsigned hd)
{
 if( !disk_sim_cfg_read )
     read_disk_sim_cfg();
 if( hd<0x80 || hd-0x80 >= disk_sim_count )
     return 0;
 return & (disk_sim_desc[hd-0x80]);
}

//------------------------------------------

int sim_hard_disk_get_info(int hd, HardDiskInfo *hdi)
{
 int fd;
 struct disk_sim_desc *dsim;

 if( (dsim=get_disk_sim_desc(hd))==0 ) return -1;

 hdi->flags = 1; // LBA Enabled
 hdi->hd_num=hd;
 
 hdi->num_heads      = dsim->heads;
 hdi->sect_per_track = dsim->sects;
 hdi->num_cyls       = dsim->cyls;

 hdi->sect_per_cyl = hdi->num_heads * hdi->sect_per_track;
 hdi->total_sects  = (Sector)(hdi->num_cyls)*(hdi->sect_per_cyl);

 if( (fd=open(dsim->file_name,O_RDONLY))==-1 )
   {
    long i;
    char *buf=new char[512];
    if( buf==0 ) return -1;

    if( (fd=open(dsim->file_name,O_WRONLY+O_TRUNC+O_CREAT))==-1 )
      {
       delete [] buf;
       return -1;
      }
    cprintf(BakBlue+BrWhite,1,1,"Creating simulation file...");
    for( i=0 ; i<512 ; i++ ) buf[i]=0;
    for( i=0 ; i<hdi->total_sects ; i++ )
       {
        write(fd,buf,512);
       }
    close(fd);
    delete [] buf;
   }
 else close(fd);

 return get_disk_sim_count();
}

int hard_disk_read(HardDiskInfo *hdi, Sector rel_sect, void *buf, Sector n_sect)
{
 int fd;
 struct disk_sim_desc *dsim;

 if( (dsim=get_disk_sim_desc(hdi->hd_num))==0 ) return -1;

 if( (fd=open(dsim->file_name,O_RDONLY))==-1 )
   {
    return -1;
   }

 lseek(fd, rel_sect*512, SEEK_SET);
 
 if( read(fd, buf, 512*n_sect )==-1 )
   {
    close(fd);
    return -1;
   }
 
 close(fd);
 return 0;
}

int sim_hard_disk_write(HardDiskInfo *hdi, Sector rel_sect, void *buf, Sector n_sect)
{
 int fd;
 struct disk_sim_desc *dsim;

 if( (dsim=get_disk_sim_desc(hdi->hd_num))==0 ) return -1;

 if( (fd=open(dsim->file_name,O_RDWR))==-1 )
   {
    return -1;
   }

 lseek(fd, rel_sect*512, SEEK_SET);
 
 if( write(fd, buf, 512*n_sect )==-1 )
   {
    close(fd);
    return -1;
   }
 
 close(fd);
 return 0;
}

int sim_hard_disk_clear(HardDiskInfo *hdi, Sector rel_sect, Sector n_sect)
{
 int i, fd;
 struct disk_sim_desc *dsim;
 char* buf=new char[SECTOR_SIZE];

 if( buf==0 ) return 0;

 if( (dsim=get_disk_sim_desc(hdi->hd_num))==0 ) 
   {
    delete [] buf;
    return -1;
   }

 if( (fd=open(dsim->file_name,O_RDWR))==-1 )
   {
    delete [] buf;
    return -1;
   }

 for( i=0 ; i<SECTOR_SIZE ; i++ )
  buf[i]=0;

 lseek(fd, rel_sect*512, SEEK_SET);
 
 for( i=0 ; i<n_sect ; i++ )
  if( write(fd, buf, 512 )==-1 )
    {
     delete [] buf;
     close(fd);
     return -1;
    }
 
 close(fd);
 delete buf;
 return n_sect;
}

int sim_hard_disk_verify(HardDiskInfo *, Sector rel_sect, Sector n_sect)
{
 return n_sect;
}
