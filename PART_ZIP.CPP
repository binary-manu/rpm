#define PART_ZIP_C
#include "part.h"

#define N_SECT 1024

#if 0
void zip_example( HardDiskInfo *hdi, Part *p )
{
 int fd;
 char *buf=(char*)malloc(N_SECT*512); /* 512 sectors */
 char tmp[40];
 
 if( buf==0 )
   {
    show_error(ERROR_MALLOC);
    return;
   }

 progress("^Dumping partition to file \"part_dmp\"...           (Press ESC to Cancel)");

 if( (fd=open("part_dmp",O_WRONLY+O_CREAT+O_TRUNC))==-1 )
   {
    show_error("Error opening file");
   }
 else
   {
  
    Sector rel_sect=0;
    while( rel_sect<p->num_sect )
       {
        Sector num_sect = p->num_sect-rel_sect;
        if( num_sect>N_SECT ) num_sect=N_SECT;
        if( read_part(hdi,p,rel_sect,buf,num_sect)==-1 )
          {
           show_error("Error reading disk");
           break;
          }
        if( write(fd,buf,num_sect*512)!=num_sect*512 )
          {
           show_error("Error writing file");
           break;
          }
        rel_sect+=num_sect;

        sprintf(tmp,"%% %3d%% %,11dk", rel_sect*100/p->num_sect, rel_sect/2 );
        if( progress(tmp)==EV_CANCEL )
           break;
       }
    close(fd);
   }
 free(buf);
}/* zip_example */
#endif