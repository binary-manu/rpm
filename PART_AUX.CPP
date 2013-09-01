#define PART_AUX_C
#include "part.h"

#define REL_SECT(p)  ( (p)->start_cyl  * hdi->sect_per_cyl + \
                       (p)->start_head * hdi->sect_per_track + \
                       (p)->start_sect - 1 )

#define LAST_SECT(p) ( (p)->end_cyl  * hdi->sect_per_cyl + \
                       (p)->end_head * hdi->sect_per_track + \
                       (p)->end_sect - 1 )

#define CYL(abs_sect)  ( (abs_sect)/(hdi->sect_per_cyl) )
#define HEAD(abs_sect) ( (abs_sect)%(hdi->sect_per_cyl)/(hdi->sect_per_track) )
#define SECT(abs_sect) ( (abs_sect)%(hdi->sect_per_cyl)%(hdi->sect_per_track)+1 )

#ifdef MT4P
int prepare_mbr_records( Part *part, Part *part2)
{
 int i, n_pri, t;
 
 n_pri=0; /* n_pri - number of primary partitions */
 
 memset(part2,0,4*sizeof(Part));     /* copying primary partitions */

 for( i=0; i<P_ROWS; i++ ) 
  if( !UNUSED(&part[i]) && !LOGICAL(&part[i]) && part[i].fs_type!=FS_MBR ) {
     n_pri++;
     t = part[i].mbr_row;
     if (t) {
	if (part2[t-1].tag == 0) /* required only for t == 1; but keeping it */
		part2[t-1] = part[i];
     } else if (part[i].tag) { /* active part with no row; make it the 1st */
		part2[0] = part[i];
		part[i].mbr_row = reset_mbr_row(part, &part[i], 1);
	    }
  }

 return n_pri;
 
}/* prepare_mbr_records */
#else
int prepare_mbr_records( Part *part, Part *part2)
{
 int i, j, t, n_pri;
 Part tmp;

 n_pri=0; /* n_pri - number of primary partitions */

 memset(part2,0,4*sizeof(Part));     /* copying primary partitions */
 for( i=0, j=0 ; i<P_ROWS ; i++ )
  if( !UNUSED(&part[i]) && !LOGICAL(&part[i]) && part[i].fs_type!=FS_MBR )
    {
     n_pri++;
     if( n_pri>4 ) continue;
     part2[j]=part[i];
     j++;
    }

 for( i=0 ; i<4 ; i++ )              /* sorting them according to mbr_row */
  for( j=0 ; j<4 ; j++ )
     {
      t=part2[j].mbr_row;
      if( t>=1 && t<=4 && t!=j+1 )
        {
         tmp=part2[j];
         part2[j]=part2[t-1];
         part2[t-1]=tmp;
        }
     }

 return n_pri;

}/* prepare_mbr_records */
#endif /* MT4P */


void pack_part_rec( Part *part, MbrPartRec *part_rec)
{
 part_rec->active     = part->tag;
 part_rec->fs_type    = part->fs_type >> 8;

 part_rec->start_cylL = part->start_cyl & 0xFF;
 part_rec->start_cylH = part->start_cyl >> 8;
 part_rec->start_head = part->start_head;
 part_rec->start_sect = part->start_sect;

 part_rec->end_cylL   = part->end_cyl & 0xFF;
 part_rec->end_cylH   = part->end_cyl >> 8;
 part_rec->end_head   = part->end_head;
 part_rec->end_sect   = part->end_sect;

 part_rec->rel_sect   = part->rel_sect;
 part_rec->num_sect   = part->num_sect;

}/* pack_part_rec */


void unpack_part_rec( MbrPartRec *part_rec, Part *part)
{
 part->fs_type    = part_rec->fs_type << 8;
 part->flags      = 0;
 part->tag        = part_rec->active;

 part->start_cyl  = part_rec->start_cylL+(part_rec->start_cylH<<8);
 part->start_head = part_rec->start_head;
 part->start_sect = part_rec->start_sect;

 part->end_cyl    = part_rec->end_cylL+(part_rec->end_cylH<<8);
 part->end_head   = part_rec->end_head;
 part->end_sect   = part_rec->end_sect;

 part->rel_sect   = part_rec->rel_sect;
 part->num_sect   = part_rec->num_sect;

 find_fs_num( part );

}/* unpack_part_tab */


void pack_vtoc_part_rec( Part *part, VtocPartRec *part_rec)
{
 part_rec->fs_type  =  part->fs_type;
 part_rec->part_options = (part->fs_flags | part->flags & P_LOGICAL);
 part_rec->mbr_row  =  part->mbr_row;

 part_rec->rel_sect = part->rel_sect;
 part_rec->num_sect = part->num_sect;
 
}/* pack_vtoc_part_tab */

void unpack_vtoc_part_rec( VtocPartRec *part_rec, Part *part)
{
 part->fs_type  =  part_rec->fs_type;
 part->fs_flags =  part_rec->part_options;
 part->mbr_row  =  part_rec->mbr_row;

 part->rel_sect = part_rec->rel_sect;
 part->num_sect = part_rec->num_sect;

 find_fs_num( part );

 part->flags |=  (part_rec->part_options & P_LOGICAL);

}/* unpack_vtoc_part_tab */


void recalculate_partition( HardDiskInfo *hdi, Part *p, int mode )
{
 Sector rel_sect;
 Sector num_sect;
 Sector last_sect;

 if( p->start_sect==0 )
   {
    p->start_sect = 1;
   }

 if( p->end_sect==0 )
   {
    p->end_sect = hdi->sect_per_track;
    if( p->end_head==0 )
        p->end_head = hdi->num_heads-1;
   }

 rel_sect  = REL_SECT(p);              /* calculated from CHS */
 last_sect = LAST_SECT(p);

 num_sect = ( (last_sect < rel_sect) ? (0) : (last_sect - rel_sect + 1) );

 if( mode==MODE_CHS )      /* CHS -> LBA */
   { 
    p->rel_sect = rel_sect;
    p->num_sect = num_sect;
   }
 else if( mode==MODE_LBA ) /* LBA -> CHS */
   {
    rel_sect = p->rel_sect;
    num_sect = p->num_sect;
    
    p->start_cyl=0;	p->end_cyl=0;
    p->start_head=0;	p->end_head=0;
    p->start_sect=0;	p->end_sect=0;

    if( rel_sect!=0 || num_sect!=0 )
      {
       p->start_cyl  = CYL(rel_sect);
       p->start_head = HEAD(rel_sect);
       p->start_sect = SECT(rel_sect);

       if( num_sect!=0 )
         {
          last_sect = rel_sect + num_sect - 1 ;

          p->end_cyl  = CYL(last_sect);
          p->end_head = HEAD(last_sect);
          p->end_sect = SECT(last_sect);
         }
      }
   }

 p->last_sect = ( num_sect==0 ? rel_sect : (rel_sect + num_sect - 1) );

 if( p->fs_type!=0 || p->rel_sect!=0 || p->num_sect!=0 )
    p->flags |=  P_NOT_EMPTY;
 else
    p->flags &= ~P_NOT_EMPTY;
}/* recalculate_partition */



void import_partition(HardDiskInfo *hdi, Part *p, Sector ext_base, Sector log_base)
{
 Sector rel_sect;
 Sector num_sect;
 Sector last_sect;

 rel_sect  = REL_SECT(p);              /* calculated from CHS */
 last_sect = LAST_SECT(p);

 num_sect = ( (last_sect < rel_sect) ? (0) : (last_sect - rel_sect + 1) );

 if( p->start_cyl==0 && p->start_head==0 && p->start_sect==0 &&
     p->end_cyl==0   && p->end_head==0   && p->end_sect==0   &&
     p->rel_sect==0  && p->num_sect==0 )
   {
    /* partition record is empty */
   }
 else if( p->rel_sect==rel_sect && p->num_sect==num_sect &&
         (!LOGICAL(p) || p->rel_sect>log_base) /* must be relative to 0 */ )
   {
    /* CHS and LBA adresses are consistent - so nothing needs to be done  */
   }
 else if( p->rel_sect==0 && p->num_sect==0 ) /* what if only CHS were set */
   {
    p->rel_sect = rel_sect;
    p->num_sect = num_sect;
   }
 else /* they are inconsistent */
   {
    /* if this is logical disk lets try to find correct rel_sect */

    if( LOGICAL(p) )
      {
       /*  Inside extended partition we have to do some extra checks because
        *  everybody treats it differently. The expected behaviour, according
        *  to Microsoft, should be:
        *    - for extended partitions rel_sect is relative to the beginning
        *      of the most outer (primary) extended partition.
        *    - for logical disks - it is from the beginning of the parent
        *      extended partition (the closest one that enncloses ours)
        *
        *  Normally to make rel_sect relative to the beginning of the disk 
        *  we would have to: p->rel_sect += EXTENDED(p) ? ext_base : log_base;
        */
          
       if( rel_sect == p->rel_sect + ext_base ||  /* is it rel to ext_base? */
           rel_sect == p->rel_sect + log_base )   /* is it rel to log_base? */
         {
          p->rel_sect = rel_sect;
         }
       else /* none of them - then we follow default behaviour */
         {
          p->rel_sect += EXTENDED(p) ? ext_base : log_base;
         }
      }/* logical */

      /* do they both match now? */
           
    if( p->rel_sect == rel_sect  &&  p->num_sect == num_sect )
      {
       /* CHS and LBA adresses are consistent now - so do nothing */
      }
    else if( p->num_sect == num_sect &&         /* what if num_sect is good */
             !( (p->start_cyl==1022 ||          /* but make sure we are not */
                 p->start_cyl==1023 ) &&        /* dealing with over 8G LBA */
                 p->start_cyl==p->end_cyl ) &&  /* partition                */
              !( p->rel_sect+p->num_sect >= 1024*hdi->sect_per_cyl ) && 
             p->start_head < hdi->num_heads &&   /* and heads and sectors    */
             p->end_head < hdi->num_heads &&     /* have valid values       */
             p->start_sect <= hdi->sect_per_track &&
             p->end_sect <= hdi->sect_per_track )
      {
       p->rel_sect = rel_sect;              /* then we could trust CHS  */
      }
    else  /* nothing we could with CHS now, so we just trust LBA values */
      {
       recalculate_partition(hdi,p,MODE_LBA);
      }
   }/* inconsistent */

 if( p->fs_type!=0 || p->rel_sect!=0 || p->num_sect!=0 )
    p->flags |= P_NOT_EMPTY;
 else
    p->flags &= ~P_NOT_EMPTY;

 p->last_sect = ( (p->num_sect==0) ? (p->rel_sect) :
                                     (p->rel_sect + p->num_sect - 1) );

}/* import_partition */


void export_partition(HardDiskInfo *hdi, Part *p, Sector ext_base, Sector log_base)
{
 if( p->start_cyl > 1023 )
   {
    p->start_cyl = 1023;
 // p->start_head = 0;
 // p->start_sect = 1;
    p->start_head = hdi->num_heads - 1;
    p->start_sect = hdi->sect_per_track;
   }

 if( p->end_cyl > 1023 )
   {
    p->end_cyl = 1023;
    p->end_head = hdi->num_heads - 1;
    p->end_sect = hdi->sect_per_track;
   }

 if( !EMPTY(p) )
   {
    p->rel_sect -= EXTENDED(p) ? ext_base : log_base;
   }
 
}/* export_partition */



int sort_disk_info( DiskInfo *di, int *row )
{
 int i, j, sv_row_pos, n;
 Part *p, *q, part_tmp, *part=di->part;
 Sector sv_rel_sect;
 Sector first_free_sect, first_not_free_sect, first_non_ext_sect;

 sv_row_pos = 0;

 if( !UNUSED(&part[*row]) )    /* lets remember which partition was active */
   {
    sv_rel_sect = part[*row].rel_sect;
        
    for( i=0 ; i<=*row ; i++ )
     if( !UNUSED(&part[i]) && sv_rel_sect == part[i].rel_sect ) sv_row_pos++;
   }

 n=0;
 
 for( i=0 ; i<P_ROWS ; i++ )       /* First we bring all USED records to the */
  if( !UNUSED(&part[i]) )          /* beginning of array in the sorted order */
    {
     part_tmp = part[i];
     j=n;
     while( j>0 && part[j-1].rel_sect > part_tmp.rel_sect )
        {
         part[j] = part[j-1];
         j--;
        }
     part[j] = part_tmp;
     n++;
    }
 
 for( i=0 ; i<n ; i++ )               /* Now assume that there are no errors */
    {
     part[i].flags &= ~P_LOC_ERROR;
    }

 for( p=part, i=0 ; i<n ; i++, p++ )   /* Lets check if there are any errors */
    {
     p->ext_base=0;

     if( p->num_sect==0 )                /* partition may not have zero size */
       {
        p->flags |= P_ERR_ZERO;
       }

     if( p->start_head >= di->num_heads ||       /* checking fo valid ranges */
         p->end_head   >= di->num_heads ||
         p->start_sect >  di->sect_per_track ||
         p->end_sect   >  di->sect_per_track ||
         p->last_sect  >= di->total_sects )
       {
        p->flags |= P_ERR_RANGE;
       }

     if( !LOGICAL(p) )        /* first we deal only with primary partitions */
       {
        for( q=p+1, j=i+1 ; j<n ; j++, q++ )
         if( !LOGICAL(q) &&
             p->last_sect >= q->rel_sect )             /* checking overlap */
           {
            p->flags |= P_ERR_OVERLAP;
            q->flags |= P_ERR_OVERLAP;
           }
       }
     else if( EXTENDED(p) )   /* now dealing with non primary ext partitions */
       {
        /* first make sure that they are inside primary extended partition */

        for( q=p-1, j=i-1 ; j>0 ; j--, q-- )
         if( EXTENDED(q) && !LOGICAL(q) &&
             p->rel_sect >  q->rel_sect &&
             p->rel_sect <= q->last_sect )
           {
            p->ext_base = q->rel_sect;
            if( p->last_sect <= q->last_sect ) break;
           }

        if( j==0 ) p->flags |= P_ERR_LOG_ORPH;

        for( q=p+1, j=i+1 ; j<n ; j++, q++ )     /* now checking for overlap */
         if( EXTENDED(q) && LOGICAL(q) &&
             p->last_sect >= q->rel_sect )
           {
            p->flags |= P_ERR_OVERLAP;
            q->flags |= P_ERR_OVERLAP;
           }

        if( i>0 && !EXTENDED(p-1) && (p-1)->last_sect > p->rel_sect )
          {
            p->flags |= P_ERR_OVERLAP;
            (p-1)->flags |= P_ERR_OVERLAP;
          }
       }
      else /* LOGICAL(p) - pure logical drives (FAT, Linux, NTFS, ... ) */
       {
        /* first make sure that they are inside some extended partition */

        if( i>0 && EXTENDED(p-1) )
          {
           if( p->rel_sect == (p-1)->rel_sect ||
               p->last_sect > (p-1)->last_sect )
             {
               p->flags    |= P_ERR_LOG_STRU;
              (p-1)->flags |= P_ERR_LOG_STRU;
             }
           else p->ext_base = (p-1)->rel_sect;
          }
        else /* there is no extended partition before p */
          {
           p->flags |= P_ERR_LOG_STRU;
           if( i>1 && EXTENDED(p-2) )
                 (p-2)->flags |= P_ERR_LOG_STRU;
          }
       }/* logical */
    }/* for */

 /* now calling file system specific functions to validate partitions' data */

 for( i=0 ; i<n ; i++ )
    {
     if( part[i].flags & OP_VALIDATE )
        OP_HANDLER( OP_VALIDATE, di, &part[i], 0 );
    }

 /* now we insert unused partitions where there is free space */

 first_non_ext_sect=0;
 first_free_sect=1;

 for( i=0 ; i<=n ; i++ )
    {
     first_not_free_sect = ( i<n ? part[i].rel_sect : di->total_sects );
     
     if( first_free_sect     > di->total_sects ) break;
     if( first_not_free_sect > di->total_sects ) first_not_free_sect = di->total_sects;

     if( first_free_sect < first_non_ext_sect &&
                           first_non_ext_sect < first_not_free_sect )
       {
        first_not_free_sect = first_non_ext_sect;
       }
     
     if( first_free_sect < first_not_free_sect )  /* insert unused partition */
       {
        if( n==P_ROWS ) break; /* Not enough unused partitions !!! */
        n++;
        for( j=n ; j>i ; j-- ) part[j]=part[j-1];
        memset(&part[i],0,sizeof(Part));
        part[i].rel_sect = first_free_sect;
        part[i].num_sect = first_not_free_sect - first_free_sect;
        recalculate_partition(di,&part[i],MODE_LBA);
        if( first_free_sect < first_non_ext_sect ) part[i].flags |= P_LOGICAL;
        first_free_sect = first_not_free_sect;
       }
     else
       {
        if( EXTENDED(&part[i]) )
          {
           first_free_sect = max( first_free_sect, part[i].rel_sect + di->sect_per_track );
           if( !LOGICAL(&part[i]) ) first_non_ext_sect = part[i].last_sect + 1;
          }
        else 
          {
           first_free_sect = max( first_free_sect, part[i].last_sect + 1 );
          }
       }
    }
 
 if( n!=P_ROWS ) memset(&part[n],0,(P_ROWS-n)*sizeof(Part));

 if( sv_row_pos!=0 )       /* now we are looking for the row that was active */
   {
    for( i=0 ; i<n ; i++ )
     if( !UNUSED(&part[i]) && sv_rel_sect == part[i].rel_sect )
       {
        sv_row_pos--;
        if( sv_row_pos == 0 ) break;
       }
    while( EMPTY(&part[i]) ) i--;
    *row=i;
   }

 di->num_part=n;
 
 return 0;
}/* sort_disk_info */

#ifdef MT4P
int can_insert(DiskInfo *di, Part *ptmp)
{

	int i, j, n, int_ext = -1, logie = -1;
	Part *p, *part = di->part;
	unsigned int extlog = P_EXTENDED | P_LOGICAL;

	n = di->num_part;

	if (ptmp->num_sect == 0) /* partition may not have zero size */
		return 0;

	if (ptmp->start_head >= di->num_heads || /* checking fo valid ranges */
		ptmp->end_head   >= di->num_heads ||
		ptmp->start_sect >  di->sect_per_track ||
		ptmp->end_sect   >  di->sect_per_track ||
		ptmp->last_sect  >= di->total_sects)
		return 0;

	if (!LOGICAL(ptmp)) { /* Primary & primary extended partitions */
		for (i = 0; i < n; i++)
			if (!UNUSED(&part[i]) && !LOGICAL(&part[i]) && 
			    !non_overlap_check(&part[i], ptmp))
				return 0;
		return 1;
	}

	/* 
	 * NOTE: The entire code below is based on what I observed after
         * creating the extended & logical partitions using fdisk of Linux
         * and windows & verified for consistency with RPM. I did not have 
	 * prior knowledge on ext. partitions before. It is possible that
	 * I still might have missed something here though I feel it may not 
	 * be the case. It is also that there may be additional checks in
	 * this code which may not be really required.
	 * - Muthu
	 */

	for (i = 0; i < n; i++) {
		if (!LOGICAL(&part[i]) && EXTENDED(&part[i]))
			if (ptmp->rel_sect > part[i].rel_sect &&
			    ptmp->rel_sect <= part[i].last_sect &&
			    ptmp->last_sect > part[i].rel_sect &&
			    ptmp->last_sect <= part[i].last_sect)
				break;
	}

	if (i == n) /* No encompassing primary extended partition */
		return 0;

	/* Internal extended & logical partitions */
	for (i = 0; i < n; i++) {
		if (UNUSED(&part[i]) || !LOGICAL(&part[i]))
			continue;

		/* both are either logical OR internal ext. part. */
		if ((ptmp->flags & extlog) == (part[i].flags & extlog)) {
			if (!non_overlap_check(&part[i], ptmp))
				return 0;
		} else {
			/* Logical inside an internal ext. part */
			if (LOGICAL(ptmp) && !EXTENDED(ptmp))
				if (ptmp->rel_sect > part[i].rel_sect &&
			      	    ptmp->rel_sect <= part[i].last_sect &&
			      	    ptmp->last_sect > part[i].rel_sect &&
			            ptmp->last_sect <= part[i].last_sect) {
					logie = i;	
					if (int_ext == -1)
						int_ext = i;
					continue;
				}

			if (!non_overlap_check(&part[i], ptmp))
				return 0;
		
		}
		/* Record the first internal ext. part. */
		if (LOGICAL(&part[i]) && EXTENDED(&part[i]) && int_ext == -1)
			int_ext = i; 
	}

	/* 
         * Check whether the logical partition is the first inside the
	 * primary extended partition; in this case no need 
	 * to check for an inner extended partition. Otherwise
         * there needs to be an inner extended partition which encompasses
         * this logical partition
         */
	if (LOGICAL(ptmp) && !EXTENDED(ptmp) && (logie == -1) && 
	    (int_ext != -1) && !(ptmp->rel_sect < part[int_ext].rel_sect))
		return 0;

	return 1;
}

/*
 * non_overlap_check()
 *	returns 1 if there is no overlap
 *	retunrs 0 otherwise
 */
int non_overlap_check(Part *parti, Part *ptmp)
{

	/*
	 * DON'T CHANGE THE ORDER OF THE CONDITIONS for this 'if' statement.
	 * The 3rd & 4th conditions depend on the 1st two conditions being
	 * false & so don't check for the unnecessary additional checks
	 * e.g.: ptmp->rel_sect <= parti->last_sect in condition 3.
	 * If you change the order, it may not be true anymore.
	 */

		/*
		 * New partition begins in the middle of an
		 * existing partition 
		 */
	if ((ptmp->rel_sect >= parti->rel_sect && 
		ptmp->rel_sect <= parti->last_sect) ||

		/* New partition ends in the middle */
		(ptmp->last_sect >= parti->rel_sect &&
		ptmp->last_sect <= parti->last_sect) ||

		/* New partition encompasses an existing one */ 
		(ptmp->rel_sect <= parti->rel_sect &&
		ptmp->last_sect >= parti->last_sect) ||

		/*
 		 * Existing partition contains the new one;
		 * not needed since the first condition will take 
		 * care of this. But included for completion
		 */
		(ptmp->rel_sect >= parti->rel_sect &&
		ptmp->last_sect <= parti->last_sect))
			return 0;

	return 1;
}

int reset_mbr_row(Part *part, Part *p, int mbr_row)
{

	int i;
	
	if (LOGICAL(p)) /* 
			 * We don't worry about MBR/EMBR rows
			 * for logical/internal extended paritions.
			 * Should we need to??? I don't know.
			 * - Muthu
			 */
		return 0;

	for (i = 1; i < P_ROWS; i++) {
		if (!UNUSED(&part[i]) && part[i].mbr_row == mbr_row && 
		    memcmp(part+i, p, sizeof(Part)) != 0)
			if (!part[i].tag)
				part[i].mbr_row = 0;
			else
				return 5;

	}

	return mbr_row;
}
#endif /* MT4P */

void sprintf_partition( char *tmp, int num, Part *p, int mode )
{
 char tmp1[24], tmp2[8];
 char *fs_name, *prt_type=tmp2;
 char tag = (p->tag==0 ? ' ' : '*');

 fs_name = fs_desc[p->fs_num].name;

 if( ( fs_desc[p->fs_num].fs_id == FS_HIDDEN ||
       fs_desc[p->fs_num].fs_id == FS_UNKNOWN ) && mode!=MODE_MINI )
   {
    sprintf(tmp1,"%s (0x%02X)", fs_name, p->fs_type>>8 );
    fs_name=tmp1;
   }

      if( p->fs_type==FS_MBR ) prt_type = "MBR  ";
 else if( EMPTY(p)         ) prt_type = "     ";
 else if( LOGICAL(p)       )
   {
    prt_type =(EXTENDED(p) ? PART_TYPE_EXT : PART_TYPE_LOG );
   }
 else
   {
    sprintf(tmp2,PART_TYPE_PRI, p->mbr_row==0 ? ' ' : (p->mbr_row + '0') );
   }


 if( mode==MODE_CHS)
   {
    sprintf(tmp,"%2d %c%s %-21s%,6d %4d %4d %,6d %4d %4d %,11d ",
            num,
            tag, prt_type, fs_name,
            p->start_cyl,  p->start_head, p->start_sect,
            p->end_cyl,    p->end_head,   p->end_sect,
            p->num_sect/2 );
   }
 else if( mode==MODE_LBA )
   {
    sprintf(tmp,"%2d %c%s %-21s %,10d %,10d %,10d %,10d ",
            num,
            tag, prt_type, fs_name,
            p->rel_sect,   p->num_sect,  p->last_sect,
            p->num_sect/2 );
    }
 else if( mode==MODE_BM )
   {
    sprintf(tmp,"%2d  %s %-21s %,10d ",
            num,
            prt_type, fs_name,
            p->num_sect/2 );
    }
 else if( mode==MODE_MISC )
   {
    sprintf(tmp,"%2d %c%s %02X %,6d %3d %2d %,6d %3d %2d %,11d %,11d %,11d",
            num,
            tag, prt_type, p->fs_type>>8,
            p->start_cyl,  p->start_head, p->start_sect,
            p->end_cyl,    p->end_head,   p->end_sect,
            p->rel_sect,   p->num_sect,   p->last_sect );
    }
 else if( mode==MODE_EXACT )
   {
    sprintf(tmp,"%2d  %02X  %02X %,6d %4d %4d %,6d %4d %4d %,11d %,11d",
            num,
            p->tag, p->fs_type>>8,
            p->start_cyl,  p->start_head, p->start_sect,
            p->end_cyl,    p->end_head,   p->end_sect,
            p->rel_sect,   p->num_sect);
    }
 else if( mode==MODE_TITLE )
   {
    sprintf(tmp,"%-21s  CHS=(%d,%d,%d)  %,d k  %,d sectors",
            fs_name,
            p->start_cyl,  p->start_head, p->start_sect,
            p->num_sect/2, p->num_sect );
    }
 else if( mode==MODE_MINI )
   {
    int len;

    fs_name = fs_desc[p->fs_num].name;
    len = strlen(fs_name);
    if( len>10 ) fs_name += len+1;
    
    sprintf(tmp,"%d%c%-10s%,6d",
            num,
            tag, fs_name,
            p->num_sect/2048);
   }
 else if( mode==MODE_EXT )
   {
    sprintf(tmp,"%d %-21s%,6d %4d %4d %,6d %4d %4d",
            num,
            fs_name,
            p->start_cyl, p->start_head, p->start_sect,
            p->end_cyl,   p->end_head,   p->end_sect );
   }

}/* sprintf_partition */


void find_fs_num( Part *p )
{
 int i=0;

 while( fs_desc[i].fs_id!=p->fs_type && fs_desc[i].fs_id!=FS_UNKNOWN ) i++;
 
 if( p->fs_type==0xFF00 )
   while( fs_desc[i].fs_id!=FS_HIDDEN ) i++;

 p->flags = (p->flags & ~P_FS_FLAGS) | fs_desc[i].flags;
 
 p->fs_num=i;
}/* find_fs_num */


int find_ipl_type( Mbr *mbr )
{
 if( memcmp(mbr,STD_IPL,STD_IPL_SIZE)==0 ) return IPL_TYPE_STD;
 if( memcmp(mbr,BM_IPL,BM_IPL_SIZE-7)==0 ) return IPL_TYPE_BM;
                            /* last 7 bytes could be modified */
 if( memcmp(mbr,BM_IPL,11)==0 ) return IPL_TYPE_XBM; /* Old version */
 return IPL_TYPE_UNKN;
}/* find_ipl_type */

void free_part_info( Part *p )
{
 p->rel_sect_loaded = 0;
 free(p->part_boot_sect);
 free(p->part_misc_info);
 p->part_boot_sect=0;
 p->part_misc_info=0;
 p->flags &= ~P_BTS_LOADED;
 p->flags &= ~P_BTS_ERROR;
}/* free_part_info */


int count_zeros(void *buf, int len)
{
 char *p;
 int i, z;

 z=0;
 p=(char*)buf;
 
 for( i=0 ; i<len ; i++, p++ )
  if( (*p)==0 ) z++;

 return z;
}/* count_zeros */



unsigned long calc_hash(void *buf, int n)
{
 int i;
 unsigned long s;
 unsigned char *p=(unsigned char *)buf;
 
 if( n==0 ) return 0;

 s=12345;
 
 for( i=0 ; i<n ; i++ )
   s=(p[i]^s)+((p[i]<<2)+7)*((s>>1)+3);
 
 return s;
}/* calc_hash */

char *get_partition_label(DiskInfo* di, Part*prt)
{
 int i;
 char *p, *tmp;
 static char lbl[20];
 int x=((prt->fs_type)|0x1000);

 if( !(x==0x1100|| x==0x1400|| x==0x1600|| x==0x1B00|| x==0x1C00|| x==0x1E00))
   {
    return "";
   }

 if( read_boot_sect(di,prt,1)==-1 )
   {
    return "";
   }
 
 tmp=(char*)prt->part_boot_sect;
 p = ( (x==0x1B00||x==0x1C00) ? tmp+71 :   // Fat-32 label offset = 71
                                tmp+43 );  // Fat-16 label offset = 43
 if( *p==0 ) p="NO LABEL";

 for( i=0 ; i<11 && *p!=0 ; i++, p++ )
     lbl[i]=*p;
 lbl[i]=0;

 return lbl;
}