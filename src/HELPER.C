#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_MSGS 300
#define BUF_SIZE 4096

#define TEMP_FILE        "helper.tmp"
#define MESG_FILE        "part_msg.msg"
#define BACKUP_MESG_FILE "part_msg.mbk"

int num_msgs;
char *msg_name[MAX_MSGS];
char *msg_body[MAX_MSGS];

char buf[BUF_SIZE];
char buf2[BUF_SIZE];
char buf3[BUF_SIZE];

/* ------------------------------------------------------------------------ */

void fix_asm(char *file_name);
void fix_exe(char *file_name);
void imp_msg(char *language, char *file_name);
void exp_msg(char *language, char *file_name, char *file_name2);


void read_msgs(FILE *f, char *lang, FILE *dmp_c, FILE *dmp_h);
void merge_msgs(FILE *inf, FILE *outf, char *lang);

/* ------------------------------------------------------------------------ */

void error(char *msg)
{
 fprintf(stderr,"helper: %s\n",msg);
 exit(1);
}

void usage(void)
{
 fprintf(stderr,"Usage: helper -fix_asm  file.asm\n"
                "   or  helper -fix_exe  file.exe\n"
                "   or  helper -imp_msg  language import_file\n"
                "   or  helper -exp_msg  language export_file.c export_file.h\n");
 exit(1);
}


void *m_malloc(int n)
{
 void *p;
 if( (p=malloc(n))==0 ) error("Cannot allocate memory");
 return p;
}

/* ------------------------------------------------------------------------ */

void main(int argc, char **argv)
{
      if( argc==3 && strcmp(argv[1],"-fix_asm")==0 ) fix_asm(argv[2]);
 else if( argc==3 && strcmp(argv[1],"-fix_exe")==0 ) fix_exe(argv[2]);
 else if( argc==4 && strcmp(argv[1],"-imp_msg")==0 ) imp_msg(argv[2],argv[3]);
 else if( argc==5 && strcmp(argv[1],"-exp_msg")==0 ) exp_msg(argv[2],argv[3],argv[4]);
 else usage();
 exit(0);
}
 
/* ------------------------------------------------------------------------ */

void fix_asm(char *file_name)  // replace "model flat" with "model small"
{
 long l;
 FILE *f;
 char tmp[201];

 if( (f=fopen(file_name,"r+"))==0 )
   error("Error opening asm file");

 while(1) // looking for the line "\tmodel flat\n"
    {
     l=ftell(f); // remember file position prior to the next line
     fgets(tmp,200,f);
     if( feof(f) ) break;
     if( strncmp(tmp,"\tmodel flat",11)==0 )
       {
        fseek(f,l,0);
        fputs("model small\n", f);
        break;
       }
    }
 fclose(f);
}


/* ------------------------------------------------------------------------ */

#pragma pack(push)
#pragma pack(1)

struct exe_header
    {
     char sign[2];
     unsigned short n_bytes;  /* in the last 512-byte page */
     unsigned short n_pages;  /* number of 512-byte pages */
     unsigned short n_reloc_entries;
     unsigned short header_size;
     unsigned short min_add_par;
     unsigned short max_add_par;
     unsigned short init_ss;
     unsigned short init_sp;
     unsigned short check_sum;
     unsigned short init_ip;
     unsigned short init_cs;
    } h;

#pragma pack(pop)

void fix_exe(char *file_name)
{
 FILE *f;
 long l, s;
 
 if( (f=fopen(file_name,"rb+"))==0 ) error("Error opening exe file");
 
 fread( &h, sizeof(h), 1, f);

 fseek(f,0,2);
 l=ftell(f);
 fseek(f,0,0);

 s=(l - h.header_size*16);

/*
 printf("\n");
 printf("Signature: %c%c\n", h.sign[0], h.sign[1] );
 printf("Relocation entries: %2u\n", h.n_reloc_entries );
 printf("Header size (par):  %2u\n", h.header_size );
 printf("Number of pages:     %3u  Code size\n", h.n_pages );
 printf("Bytes in last page:  %3u (%08lX)\n", h.n_bytes,512L*h.n_pages+h.n_bytes );
 printf("Exe file size:    %6lu (%08lX)\n", l, l );
 printf("Check sum: 0x%04X\n",  h.check_sum);
 printf("Min additional par: %04X (%08lX)\n", h.min_add_par, 16L*h.min_add_par);
 printf("Max additional par: %04X (%08lX)\n", h.max_add_par, 16L*h.max_add_par);
 printf("Initial CS:IP = %04X:%04X\n",  h.init_cs, h.init_ip );
 printf("Initial SS:SP = %04X:%04X\n",  h.init_ss, h.init_sp );
*/

 h.init_ss = s/16 + ((s%16==0)?0:1);
 h.init_sp = 1024;
 h.min_add_par = 1024/16;


 fwrite( &h, sizeof(h), 1, f);

 fclose(f);
}

/* ------------------------------------------------------------------------ */


void imp_msg(char *language, char *file_name)
{
 FILE *mf, *f, *tf;

 if( (mf=fopen(MESG_FILE,"r"))==0 )
   error("Error opening master_message_file file");

 if( (f=fopen(file_name,"r"))==0 )
   error("Error opening import_msg file");

 if( (tf=fopen(TEMP_FILE,"w"))==0 )
   error("Error opening temporary file");

 read_msgs(f,language,0,0);
 merge_msgs(mf,tf,language);

 fclose(mf);
 fclose(f);
 fclose(tf);
/* remove(BACKUP_MESG_FILE); 
 rename(MESG_FILE,BACKUP_MESG_FILE);
 rename(TEMP_FILE,MESG_FILE);
 */
}

/* ------------------------------------------------------------------------ */

void exp_msg(char *language, char *file_name_c, char *file_name_h)
{
 FILE *mf, *c, *h;

 if( (mf=fopen(MESG_FILE,"r"))==0 )
   error("Error opening master_message_file file");

 if( (c=fopen(file_name_c,"w"))==0 )
   error("Error opening export_msg.c file");
 if( (h=fopen(file_name_h,"w"))==0 )
   error("Error opening export_msg.h file");

 read_msgs(mf,language,c,h);

 fclose(mf);
 fclose(c);
 fclose(h);
}

/* ------------------------------------------------------------------------ */

void read_msgs(FILE *f, char *lang, FILE *dmp_c, FILE *dmp_h)
{
 char *p, *q;
 int i, n=0, msg_in_buf=0, append_allowed=0;

 while(1)
    {
     fgets(buf,BUF_SIZE,f);
     if( feof(f) ) break;
     p=buf+strlen(buf);
     while( p!=buf && (*p==0 || *p==' ' || *p=='\t' || *p=='\r' || *p=='\n') )
          {
           *p=0; p--;
          }
     p=buf;
     while( *p==' ' || *p=='\t' || *p=='\r' || *p=='\n' ) p++;

     if( *p=='#' || *p==0 )
       {
        if( strncmp(p,"#define",7)==0 && dmp_c!=0 ) fprintf(dmp_c,"%s\n",p);
        if( strncmp(p,"#-STOP-",7)==0 ) break;
        continue;
       }

     if( *p!='[' && *p!='\"' )  /* saving mesg name to buf2 */
       {
        if( msg_in_buf )
          {
           msg_name[n]=m_malloc(strlen(buf2)+1); strcpy(msg_name[n],buf2);
           msg_body[n]=m_malloc(strlen(buf3)+1); strcpy(msg_body[n],buf3);
           n++;
           msg_in_buf=0;
           append_allowed=0;
          }
        q=buf2;
        while( *p!=' ' && *p!='\t' && *p!='\r' && *p!='\n' && *p!='#' && *p!=0 )
           *(q++)=*(p++);
        *q=0;
       }
     else if( p[0]=='[' && p[3]==']' )  /* message body in a new language */
       {
        if( p[1]=='e' && p[2]=='n' && !msg_in_buf ||
            p[1]==lang[0] && p[2]==lang[1] ) 
          {
           msg_in_buf=1;
           append_allowed=1;
           strcpy(buf3,p+4);
          }
        else append_allowed=0;
       }
     else if( *p=='\"' && append_allowed )
       {
        strcat(buf3,"\n ");
        strcat(buf3,p);
       }
    }/* while */

 if( msg_in_buf )
   {
    msg_name[n]=m_malloc(strlen(buf2)+1); strcpy(msg_name[n],buf2);
    msg_body[n]=m_malloc(strlen(buf3)+1); strcpy(msg_body[n],buf3);
    n++;
   }

 num_msgs=n;
 
 if( dmp_c!=0 )
   for( i=0 ; i<n ; i++ )
      {
       fprintf(dmp_h,"extern char %s[];\n", msg_name[i]);
       fprintf(dmp_c,"char %s[] = %s;\n", msg_name[i], msg_body[i]);
      }
}

/* ------------------------------------------------------------------------ */

void merge_msgs(FILE *inpf, FILE *outpf, char *lang)
{
 int i;
 char *p, tmp[80];

 while(1)
    {
     fgets(buf,BUF_SIZE,inpf);
     if( feof(inpf) ) break;
     p=buf;
     while( *p==' ' || *p=='\t' || *p=='\r' || *p=='\n' ) p++;
     if( *p=='#' || *p==0 )
       {
        fputs(buf,outpf);
        continue;
       }
     if( *p!='[' ) strcpy(tmp,buf);
#if 0
     else if( n==0 || p[3]!=']' ) error(buf);
     else if( p[1]==lang[0] && p[2]==lang[1] )
       {
        msg_body[n-1]=m_malloc(strlen(buf)+1);
        strcpy(msg_body[n-1],buf);
       }
#endif
     fputs(buf,outpf);
    }

}
