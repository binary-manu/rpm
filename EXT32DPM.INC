;
; ext32dpm.inc	Protected mode DPMI initialization functions
;

registers	struc
reg_edi		dd	?
reg_esi		dd	?
reg_ebp		dd	?
reg_reserved	dd	?
reg_ebx		dd	?
reg_edx		dd	?
reg_ecx		dd	?
reg_eax		dd	?
reg_flags	dw	?
reg_es		dw	?
reg_ds		dw	?
reg_fs		dw	?
reg_gs		dw	?
reg_ip		dw	?
reg_cs		dw	?
reg_sp		dw	?
reg_ss		dw	?
		ends

DPMI_mem_info	struc
largest_block	dd	?	; largest available block in bytes
max_unlocked_pg	dd	?	; maximum unlocked page allocation
max_locked_pg	dd	?	; maximum locked page allocation
total_linear_pg	dd	?	; total linear address space in pages
total_unlocked	dd	?	; total unlocked pages
free_pages	dd	?	; free pages
total_phy_pg	dd	?	; total physical pages
free_linear_pg	dd	?	; free linear address space in pages
swap_size_pg	dd	?	; size of paging file/partition in pages
dpmi_mi_resrvd	db  12 Dup(?) 	; reserved
		ends

;----------------------------------------------------------------------
_TEXT16		segment		; 16-bit code
;----------------------------------------------------------------------

dpmi_init	proc	near

		mov	ax, 1687h	; Get DPMI host address
		int	2fh
		cmp	ax, 0
		jne	@@no_dpmi	; DPMI host is not installed

		mov	word ptr DPMI_switch,   di
		mov	word ptr DPMI_switch+2, es
		
		check_point 1
		cmp	dx, 90		; DPMI version	dh = major
		jb	@@rm_error	;		dl = minor
		
		test	bl, 1
		jz	@@rm_error	; DPMI cannot run 32-bit code
		
		check_point 3

		sub	conv_len, si	; SI memory needed by DPMI (paragraphs)
		jc	@@rm_error	; not enough conventional memory
		mov	ax, conv_seg	; Giving memory to DPMI from the 
		add	ax, conv_len	; top of free conventional memory

		check_point 4
		mov	es, ax		; start of requested memory
		mov	ax, 1		; 32-bit application
		call	DPMI_switch	; switch to protected mode
		jc	@@rm_error

		;
		;  Now we are in protected mode
		;

		;
		;   Allocate and locks largest block of available memory
		;
		check_point 5

		mov	bx, sp		; saving stack ptr
		sub	sp, 48		; making space for DPMI_mem_info

		mov	ax, 0500h	; Get memory info
		mov	edi, esp	; address of the structure
		push	ss
		pop	es
		int	31h
		pop	eax		; largest_block of available memory
		mov	sp, bx		; restoring stack ptr
		jc	@@pm_error
		
		check_point 6

		cmp	detected_os, 3
		jb	@@not_windows	; Under Windows we will take only
		shr	eax, 1		; half of the available memory
@@decrease_size:
		sub	eax, 10000h	; Leaving out 64k of memory
@@not_windows:
		and	ax, 0F000h	; Align on 4k page boundary
		mov	mem_size, eax
		cmp	eax, offset min_mem_size
		jb	@@pm_error	; not enough memory
		
		mov	ax, 0501h	; Allocate memory block
		mov	bx, word ptr mem_size+2
		mov	cx, word ptr mem_size
		int	31h
		jc	@@decrease_size
		
		mov	word ptr mem_addr+2, bx
		mov	word ptr mem_addr,   cx
		mov	word ptr mem_handle+2, si
		mov	word ptr mem_handle,   di
		
		check_point 7
		mov	ax, 0600h	; Lock linear region
		;mov	bx, word ptr mem_addr+2
		;mov	cx, word ptr mem_addr
		mov	si, word ptr mem_size+2
		mov	di, word ptr mem_size
		int	31h
		jc	@@pm_error

		;
		;  Setting up descriptors
		;
		check_point 8

		call	setup_descriptors	; base and limit

		xor	edi, edi
		push	ds
		pop	es

		mov	bx, offset zero_sel	; First of 7 selectors
		mov	di, offset zero_dsc	; First of 7 descriptors
		mov	cx, 7
@@next_dsc:
		push	cx
		push	bx
		push	di

		mov	ax, 0000h	; Allocate LDT descriptor
		mov	cx, 0001h
		int	31h
		jc	@@pm_error

		mov	[bx], ax	; Saving selector number
		mov	bx, ax
		or	[di].access,60h	; Set DPL to the lowest level

		mov	ax, 000Ch	; Set LDT descriptor 
		int	31h		; Selector in bx, Address in di
		jc	@@pm_error
		
		pop	di
		pop	bx
		pop	cx
		add	bx, 6		; next selector
		add	di, 8		; next descriptor

		loop	@@next_dsc

		;
		;  Set interrupt handler for int 6Fh 
		;
		check_point 9

		mov	ax, 0205h	; Set protected-mode interrupt
		mov	bl, 6Fh
		mov	cx, code16_sel
		mov	edx, offset dpmi_int6Fh
		int	31h
		jc	@@pm_error

		;
		;  Ready to switch to 32-bit code
		;
		mov	rm_esp, esp		; Saving 16-bit stack ptr
		lss	esp, pm_stack_32	; Switching to 32-bit stack
	
		call	call_32_bit
		
		mov	ah, 4Ch	; application exit code was passed in al
		int	21h	; DPMI will clean up everything for us

		;
		;  Protected mode error handling - print error and exit
		;
@@pm_error:
		push	buffer_seg  ; ss	(see structure registers)
		push	200	    ; sp
		push	code16_seg         ; cs
		push	offset print_error ; ip
		push	0	    ; gs
		push	0           ; fs
		push	data16_seg  ; ds
		push	0	    ; es
		pushf		    ; flags
		pushad		    ; eax, ebx, ecx, edx, ... esi, edi

		mov	edi, esp    ; now es:edi points to struct registers
		push	ss
		pop	es

		mov	ax, 0301h	; Call real mode function
		mov	bh, 0		; flags, must be 0 (reserved)
		mov	cx, 0		; copy 0 bytes from PM to RM stack
		int	31h		; print_error will exit to DOS

		mov	ax, 4C01h	; but if int 31 fails we exit here and
		int	21h		; DPMI will clean up everything for us

		;
		; Real mode error handling - set CF and return
		;
@@rm_error:	stc
		ret

@@no_dpmi:
		clc
		ret

dpmi_init	endp

;----------------------------------------------------------------------

dpmi_int6Fh	proc	near
		;
		;  This function calls real-mode interrupts under DPMI
		;
		push	ebp
		mov	ebp, esp
		push	ds
		push	es
		push	fs
		push	gs

		mov	ds, cs:data16_sel
		
		push	rm_ss	    ; ss	(see structure registers)
		push	rm_sp	    ; sp
		push	0	    ; cs - not in use
		push	0	    ; ip - not in use
		push	0	    ; gs
		push	0           ; fs
		push	buffer_seg  ; ds
		push	buffer_seg  ; es
		pushf		    ; flags
		pushad		    ; eax, ebx, ecx, edx, ... esi, edi

		mov	edi, esp    ; now es:edi points to struct registers
		push	ss
		pop	es
		
		mov	ax, 0300h   ; Generate real mode int
		mov	bl, [ebp+16]; int_num
		mov	bh, 0 	    ; flags must be 0
		xor	cx, cx	    ; copy 0 bytes from PM to RM stack
		int	31h

		mov	ax, [esp+32]	; importing lower 16 flags
		mov	[ebp+16], al	; saving lower 8 flags (CF,ZF,...)
		popad		    	; eax, ebx, ecx, edx, ..., esi, edi
		add	esp, 18		; flags, ds, es, fs, gs, cs:ip, ss:sp

		pop	gs
		pop	fs
		pop	es
		pop	ds
		pop	ebp

		iretd

dpmi_int6Fh	endp

;----------------------------------------------------------------------
_TEXT16		ends
;----------------------------------------------------------------------
