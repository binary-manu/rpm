.386p
	model small
	ifndef	??version
	?debug	macro
	endm
	endif
	?debug	S "ext32mem.cpp"
	?debug	T "ext32mem.cpp"
_TEXT	segment dword public use32 'CODE'
_TEXT	ends
_DATA	segment dword public use32 'DATA'
_DATA	ends
_BSS	segment dword public use32 'BSS'
_BSS	ends
DGROUP	group	_BSS,_DATA
_BSS	segment dword public use32 'BSS'
	align	4
mem_list	label	dword
	db	4	dup(?)
_BSS	ends
_TEXT	segment dword public use32 'CODE'
	align	4
@mem_lib_init$qpcl	proc	near
?live1@0:
   ;	
   ;	void mem_lib_init(char *heap_start, long heap_size)   /* called before main */
   ;	
@1:
	push      ebp
	mov       ebp,esp
   ;	
   ;	{
   ;	 mem_list = (struct mem_node *)heap_start;
   ;	
   ;	 mem_list -> size = heap_size;
   ;	 mem_list -> is_free = 1;
   ;	 mem_list -> next = 0;
   ;	
?live1@16: ; EAX = @temp0
	xor       ecx,ecx
?live1@32: ; 
	mov       eax,dword ptr [ebp+8]
?live1@48: ; EAX = @temp0
	mov       edx,dword ptr [ebp+12]
?live1@64: ; 
	mov       dword ptr [mem_list],eax
?live1@80: ; EAX = @temp0
	mov       dword ptr [eax],edx
   ;	
   ;	 mem_list -> prev = 0;
   ;	
	xor       edx,edx
	mov       dword ptr [eax+4],1
	mov       dword ptr [eax+8],ecx
	mov       dword ptr [eax+12],edx
   ;	
   ;	
   ;	}/* mem_lib_init */
   ;	
?live1@160: ; 
@2:
	pop       ebp
	ret 
@mem_lib_init$qpcl	endp
	align	4
@mem_left$qv	proc	near
?live1@176:
   ;	
   ;	int mem_left(void)
   ;	{
   ;	 int sum=0;
   ;	
@3:
	xor       edx,edx
   ;	
   ;	 struct mem_node *p;
   ;	
   ;	 for( p=mem_list ; p!=0 ; p=p->next )
   ;	
?live1@192: ; EDX = sum
	mov       eax,dword ptr [mem_list]
	test      eax,eax
	je        short @5
   ;	
   ;	  if( p->is_free )
   ;	
?live1@208: ; EAX = p, EDX = sum
@4:
	cmp       dword ptr [eax+4],0
	je        short @6
   ;	
   ;	    {
   ;	     sum += p->size-16;
   ;	
	mov       ecx,dword ptr [eax]
	add       ecx,-16
	add       edx,ecx
@6:
	mov       eax,dword ptr [eax+8]
	test      eax,eax
	jne       short @4
   ;	
   ;	    }
   ;	
   ;	 return sum; 
   ;	
?live1@240: ; EDX = sum
@5:
	mov       eax,edx
   ;	
   ;	}/* mem_left */
   ;	
?live1@256: ; 
@9:
@8:
	ret 
@mem_left$qv	endp
	align	4
@mem_largest$qv	proc	near
?live1@272:
   ;	
   ;	int mem_largest(void)
   ;	{
   ;	 int l=0;
   ;	
@10:
	xor       edx,edx
   ;	
   ;	 struct mem_node *p;
   ;	
   ;	 for( p=mem_list ; p!=0 ; p=p->next )
   ;	
?live1@288: ; EDX = l
	mov       eax,dword ptr [mem_list]
	test      eax,eax
	je        short @12
   ;	
   ;	  if( p->is_free && p->size-16 > l )
   ;	
?live1@304: ; EAX = p, EDX = l
@11:
	cmp       dword ptr [eax+4],0
	je        short @13
	mov       ecx,dword ptr [eax]
	add       ecx,-16
	cmp       edx,ecx
	jge       short @13
   ;	
   ;	    {
   ;	     l = p->size-16;
   ;	
?live1@320: ; EAX = p
	mov       edx,dword ptr [eax]
	add       edx,-16
@13:
	mov       eax,dword ptr [eax+8]
	test      eax,eax
	jne       short @11
   ;	
   ;	    }
   ;	
   ;	 return l; 
   ;	
?live1@336: ; EDX = l
@12:
	mov       eax,edx
   ;	
   ;	}/* mem_largest */
   ;	
?live1@352: ; 
@16:
@15:
	ret 
@mem_largest$qv	endp
	align	4
@malloc$qi	proc	near

public		@$bnew$qui
@$bnew$qui	proc	near
@$bnew$qui	endp

public		@$bnwa$qui
@$bnwa$qui	proc	near
@$bnwa$qui	endp

?live1@368:
   ;	
   ;	void *malloc(int len)
   ;	
@17:
	push      ebp
	mov       ebp,esp
	push      ebx
	mov       edx,dword ptr [ebp+8]
   ;	
   ;	{
   ;	 struct mem_node *p, *r;
   ;	
   ;	 // align_len_to_next_16 + 16
   ;	 
   ;	 len = aligned(len,16)+16;
   ;	
?live1@384: ; EDX = len
	add       edx,15
	and       edx,-16
	add       edx,16
   ;	
   ;	 
   ;	 for( p=mem_list ; p!=0 ; p=p->next )
   ;	
	mov       eax,dword ptr [mem_list]
	test      eax,eax
	je        short @19
   ;	
   ;	  if( p->is_free && p->size>=len )
   ;	
?live1@416: ; EAX = p, EDX = len
@18:
	cmp       dword ptr [eax+4],0
	je        short @20
	cmp       edx,dword ptr [eax]
	jg        short @20
   ;	
   ;	    {
   ;	     if( p->size-len >= 32 ) /* split */
   ;	
	mov       ecx,dword ptr [eax]
	sub       ecx,edx
	cmp       ecx,32
	jl        short @21
   ;	
   ;	       {
   ;	        r = (struct mem_node*)( ((char*)p)+len );
   ;	
	lea       ecx,dword ptr [edx+eax]
   ;	
   ;	        r->prev=p;
   ;	
?live1@464: ; EAX = p, EDX = len, ECX = r
	mov       dword ptr [ecx+12],eax
   ;	
   ;	        r->next=p->next;
   ;	
	mov       ebx,dword ptr [eax+8]
	mov       dword ptr [ecx+8],ebx
   ;	
   ;	        p->next=r;
   ;	
	mov       dword ptr [eax+8],ecx
   ;	
   ;	        if( r->next!=0 ) r->next->prev=r;
   ;	
	mov       ebx,dword ptr [ecx+8]
	test      ebx,ebx
	je        short @22
	mov       dword ptr [ebx+12],ecx
   ;	
   ;	        r->size=p->size-len;
   ;	
@22:
	mov       ebx,dword ptr [eax]
	sub       ebx,edx
	mov       dword ptr [ecx],ebx
   ;	
   ;	        p->size=len;
   ;	
	mov       dword ptr [eax],edx
   ;	
   ;	        // mark_free(r);
   ;	        r->is_free=1;
   ;	
?live1@560: ; EAX = p, ECX = r
	mov       dword ptr [ecx+4],1
   ;	
   ;	        // put_on_free_list(r);
   ;	       }
   ;	     // mark_taken(p);
   ;	     p->is_free=0;
   ;	
?live1@576: ; EAX = p
@21:
	xor       edx,edx
	mov       dword ptr [eax+4],edx
   ;	
   ;	     // remove_from_free_list(p);
   ;	     break;
   ;	
	jmp       short @23
@20:
	mov       eax,dword ptr [eax+8]
	test      eax,eax
	jne       short @18
   ;	
   ;	    }
   ;	 
   ;	 if( p==0 ) return 0;
   ;	
@19:
@23:
	test      eax,eax
	jne       short @25
	xor       eax,eax
@28:
	pop       ebx
	pop       ebp
	ret 
   ;	
   ;	 
   ;	 return (void *)(((char*)p) + 16);
   ;	
@25:
	add       eax,16
   ;	
   ;	}/* malloc */
   ;	
?live1@640: ; 
@27:
@26:
	pop       ebx
	pop       ebp
	ret 
@malloc$qi	endp
	align	4
@free$qpv	proc	near

public		@$bdele$qpv
@$bdele$qpv	proc	near
@$bdele$qpv	endp

public		@$bdla$qpv
@$bdla$qpv	proc	near
@$bdla$qpv	endp
		
?live1@656:
   ;	
   ;	void free(void *xxx)
   ;	
@29:
	push      ebp
	mov       ebp,esp
	mov       eax,dword ptr [ebp+8]
   ;	
   ;	{
   ;	 struct mem_node *p;
   ;	 
   ;	 if( xxx==0 ) return;
   ;	
?live1@672: ; EAX = xxx
	test      eax,eax
	je        short @31
   ;	
   ;	 
   ;	 p = (struct mem_node *)( ((char*)xxx) - 16 );
   ;	
	add       eax,-16
   ;	
   ;	
   ;	 if( p->is_free ) return;
   ;	
?live1@704: ; EAX = p
	cmp       dword ptr [eax+4],0
	jne       short @31
   ;	
   ;	 
   ;	 // mark_free(p)
   ;	
   ;	 p->is_free=1;
   ;	
	mov       dword ptr [eax+4],1
   ;	
   ;	  
   ;	 if( p->next!=0 && p->next->is_free )  /* merge right */
   ;	
	mov       edx,dword ptr [eax+8]
	test      edx,edx
	je        short @33
	cmp       dword ptr [edx+4],0
	je        short @33
   ;	
   ;	   {
   ;	    // remove_from_free_list(p->next)
   ;	    
   ;	    p->size += p->next->size;
   ;	
?live1@752: ; EAX = p, EDX = @temp1
	mov       ecx,dword ptr [edx]
	add       dword ptr [eax],ecx
   ;	
   ;	    p->next  = p->next->next;
   ;	
?live1@768: ; EAX = p
	mov       edx,dword ptr [eax+8]
	mov       edx,dword ptr [edx+8]
	mov       dword ptr [eax+8],edx
   ;	
   ;	    if( p->next!=0 ) p->next->prev = p;
   ;	
?live1@784: ; EAX = p, EDX = @temp4
	test      edx,edx
	je        short @34
	mov       dword ptr [edx+12],eax
   ;	
   ;	   }
   ;	 
   ;	 if( p->prev!=0 && p->prev->is_free ) /* merge left */
   ;	
?live1@800: ; EAX = p
@34:
@33:
	mov       edx,dword ptr [eax+12]
	test      edx,edx
	je        short @35
	cmp       dword ptr [edx+4],0
	je        short @35
   ;	
   ;	   {
   ;	    // remove_from_free_list(p)
   ;	
   ;	    p = p->prev;
   ;	
?live1@816: ; EAX = p, EDX = @temp2
	mov       eax,edx
   ;	
   ;	    p->size += p->next->size;
   ;	
?live1@832: ; EAX = p
	mov       ecx,dword ptr [eax+8]
	mov       edx,dword ptr [ecx]
	add       dword ptr [eax],edx
   ;	
   ;	    p->next  = p->next->next;
   ;	
	mov       ecx,dword ptr [eax+8]
	mov       edx,dword ptr [ecx+8]
	mov       dword ptr [eax+8],edx
   ;	
   ;	    if( p->next!=0 ) p->next->prev = p;
   ;	
?live1@864: ; EAX = p, EDX = @temp5
	test      edx,edx
	je        short @36
	mov       dword ptr [edx+12],eax
   ;	
   ;	   }
   ;	
   ;	 // put_on_free_list(p);
   ;	
   ;	}/* free */
   ;	
?live1@880: ; 
@36:
@35:
@37:
@31:
	pop       ebp
	ret 
@free$qpv	endp
_TEXT	ends
	public	@mem_lib_init$qpcl
	public	@mem_left$qv
	public	@mem_largest$qv
	public	@malloc$qi
	public	@free$qpv
	?debug	D "ext32mem.cpp" 9573 47145
	end
