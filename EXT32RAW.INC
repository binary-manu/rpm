;
; ext32raw.inc	Protected mode RAW initialization functions
;

;----------------------------------------------------------------------

set_desc_base1	macro	dsc	; eax = base	;(if base is under 1M)
		or	dword ptr dsc.base0_15, eax  ; sets base 0-23
endm

set_desc_base	macro	dsc	; eax=base
		push	eax
		mov	dsc.base0_15, ax
		shr	eax, 16
		mov	dsc.base16_23, al
		mov	dsc.base24_31, ah
		pop	eax
endm

set_desc_limit	macro	dsc	; ecx=limit
		push	ecx
		dec	ecx	; this is limit - not the size
		mov	dsc.limit0_15, cx
		shr	ecx, 16
		mov	ch, dsc.gd00lim16_19
		and	ch, 0F0h
		and	cl, 00Fh
		or	ch, cl
		mov	dsc.gd00lim16_19, ch
		pop	ecx
endm

;----------------------------------------------------------------------
_TEXT16		segment		; 16-bit code
;----------------------------------------------------------------------

raw_init	proc	near
		
		;
		; Get size of available memory
		;
		check_point 15
		
		mov	ah, 88h		; Get amount of memory above 1M in K
		int	15h
		;jc	@@error

		movzx	eax, ax		; convert to bytes
		shl	eax, 10
		and	ax, 0F000h	; Align on 4k page boundary
		mov	mem_size, eax
		;mov	mem_addr, 100000h ; segment base = 1M (default value)

		cmp	eax, offset min_mem_size ; Do we have enough memory?
		jb	@@error
		
		;
		; Enable A20 line
		;
		check_point 16
	        call	enableA20
		jc	@@error

		call	prepare_protected_mode
		call	switch_to_protected_mode ; switches to 32-bit stack

		call	call_32_bit		; returns exit code in al
; clts ???
		call	switch_to_real_mode	; restores 16-bit stack
		push	ax
		
		;
		; Disable A20 line
		;
		call	disableA20
		
		pop	ax	; exit code in al
		clc
		ret

@@error:	stc
		ret

raw_init	endp

;----------------------------------------------------------------------

prepare_protected_mode	proc	near

		sidt	rm_idtr		; saving real mode IDT

		mov	ax, code16_seg	; setting segment addresses
		mov	code16_seg_jmp, ax
		mov	ax, code16_sel	; for jumps in mode switches
		mov	code16_sel_jmp, ax

		;
		; Clear IDT table for interrupts 20h - 6Eh
		;
		push	ds
		pop	es
		mov	al, 0
		mov	di, offset idt + 20h*8
		mov	cx, (6Fh-20h)*8
		cld
		rep
			stosb
setup_descriptors:
		;
		;  Setting up descriptors
		;
		movzx	eax, data16_seg
		shl	eax, 4	; seg -> linear address of data16_seg
		
		add	pm_gdtr.dt_base, eax	; Set correct linear addresses 
		add	pm_idtr.dt_base, eax	; for pointers to GDT and IDT

		set_desc_base1	code16_dsc	; with eax
		set_desc_base1	data16_dsc	; with eax

		add	eax, offset data16_size	; eax = linear addr of code32
		mov	ecx, offset code32_size

		set_desc_base1	code32_dsc	; with eax
		set_desc_limit	code32_dsc	; with ecx

		add	eax, ecx		; eax = linear addr of data32
		mov	data32_addr, eax	;    under 1M (original copy)
		
		add	eax, offset data32_size	; eax = linear addr of buffer

		set_desc_base1	buffer_dsc	; with eax

		mov	eax, mem_addr	
		mov	ecx, mem_size
		shr	ecx, 12	; 4k page granular

		set_desc_base	data32_dsc ; with eax
		set_desc_limit	data32_dsc ; with ecx

		ret

prepare_protected_mode	endp

;----------------------------------------------------------------------

switch_to_protected_mode	proc	near
		;
		; Switches CPU to protected mode (following Intel specs)
		;
		; Assuming that on entry DS=data16_seg
		; On exit       ES=FS=GS=DS=data16_sel
		;		    SS:ESP=pm_stack_32
		pushf
		cli			; 1. Disable interrupts

		mov	tmp_eax, eax
		pop	tmp_flag_ret	; flags and return address

		mov	rm_esp, esp	; saving 16-bit real-mode stack ptr

		lgdt	pm_gdtr		; 2. Load GDT
		
		mov	eax, cr0	; 3. Set PE (protection enable) bit
		or	al,  01h
		mov	cr0, eax
		
		;jmp	code16_seg:????	; 4. Jump to a protected mode selector 
		DB	0EAh
		DW	$+4
code16_sel_jmp	DW	?
					; 5. Load LDT - we don't use it.

		mov	ax, cs:data16_sel ; 6. Load segment registers with
		mov	ds, ax		  ; valid protected mode selectors
		mov	es, ax
		mov	fs, ax
		mov	gs, ax
		lss	esp, pm_stack_32

		;ltr	...		; 7. Load task register
		
		lidt	pm_idtr		; 8. Load protected mode IDT
		
		push	tmp_flag_ret	; flags and return address
		mov	eax, tmp_eax

		;sti			; 9. Enable interrupts
					; (popf will do that for us)
		popf

		ret

switch_to_protected_mode	endp

;----------------------------------------------------------------------

switch_to_real_mode	proc	near

		;
		; Switches CPU to real mode (following Intel specifications)
		;
		; Assuming that on entry DS=data16_sel
		; On exit       DS=ES=FS=GS=data16_seg (=CS)
		;		    SS:ESP=rm_stack_16
		pushf

		cli	; 1. Disable interrupts
			; 2. Disable paging - we didn't use paging.
			; 3. Transfer control to segment with linear address
			; equal to physical (this is the case for code16_sel)
		
		mov	tmp_eax, eax
		pop	tmp_flag_ret	; flags and return address

		mov	pm_esp, esp	; saving 32-bit protected-mode stack

		mov	ax, ds		; 4. Load all segment registers with
		mov	ss, ax		;    selectors which have limit=64k
		mov	es, ax		;    and are     byte granular (G=0),
		mov	fs, ax		;    expand up (E=0), writable (W=1)
		mov	gs, ax		;                  and present (P=1)

		lidt	rm_idtr		; 5. Restore original real mode IDT

		mov	eax, cr0	; 6. Clear PE (protection enable) bit
		and	al, 0FEh
		mov	cr0, eax
		
		;jmp	code16_seg:????	; 7. Jump to a real mode segment 
		DB	0EAh
		DW	$+4
code16_seg_jmp	DW	?		; Must be initialized during startup
		;
		; Previous instruction jumps here
		;
		mov	ax, cs		; 8. Load segment registers with valid
		mov	ds, ax		;    real mode segment addresses
		mov	es, ax
		mov	fs, ax
		mov	gs, ax
		lss	esp, rm_stack_16

		push	tmp_flag_ret	; flags and return address
		mov	eax, tmp_eax

		;sti			; 9. Enable interrupts
					; (popf will do that for us)
		popf
		
		ret

switch_to_real_mode	endp

;----------------------------------------------------------------------

int_6Fh		proc	near

		call	real_int  ; Handler for int 78h - call real-mode int
		iretd

int_6Fh		endp

real_int	proc	near
		;
		;  Call real-mode interrupt handlers from the protected-mode
		;
		;	int_num	 [esp+24] ; pushed by caller to the stack
		;	flags	 [esp+20]
		;	cs	 [esp+16]
		;	eip	 [esp+12]
		;	ip	 [esp+10] ; Local call return address
		push	ds	;[esp+8]
		push	es	;[esp+6]
		push	fs	;[esp+4]
		push	gs	;[esp+2]
		push	ax	;[esp+0]
		mov	ds, cs:data16_sel
		mov	al, [esp+24]	  ; int_num
		mov	int_num, al
		pop	ax

		call	switch_to_real_mode ; switches to 16-bit stack
		push	ds
		mov	es, buffer_seg
		mov	ds, buffer_seg

		db	0cdh	; call actual int ? handler
	int_num	db	?

		pop	ds
		call	switch_to_protected_mode ; restores 32-bit stack
		
		push	ax
		pushf			; Some interrupt services may change
		pop	ax		; low 8-bit flags: CF, AF, ZF, SF, ...
		mov	[esp+24], al	; saving those flags so that user could
		pop	ax		; execute "popf" when he gets control

		pop	gs
		pop	fs
		pop	es
		pop	ds
		ret

real_int	endp
;----------------------------------------------------------------------

int_handler	proc	near

@@redir1:	; Redirected interrupts 0 - 7 (Exceptions)

		jmp	@@exc

		i=0
int_00:		rept	8	; Interrupt handlers  0 - 7 (Exceptions)
		 push	i
		 jmp	short @@redir1
		 i=i+1
		endm

		rept	8	; Interrupt handlers  8 - 15 (Exceptions/IRQs)
		 push	i
		 jmp	short @@redir2
		 i=i+1
		endm

		rept	16	; Interrupt handlers 16 - 31 (Exceptions)
		 push	i
		 jmp	short @@redir4a
		 i=i+1
		endm

		i=70h
		rept	8	; Interrupt handlers 70h - 77h (High IRQs)
		 push	i
		 jmp	short @@redir3
		 i=i+1
		endm

@@redir4a:	jmp	short @@redir4 ; Exceptions 16-31

@@redir2:	; Redirected interrupts 8 - 15 (Exceptions or IRQs)
		;
		; We have to check whether it is an IRQ or an Exception
		;
		push	ax
		push	bx
		mov	bx, [esp+4]	; IRQ number
		sub	bx, 8

		mov	al, 0Bh	; Read Interrupt Request Register
		out	20h, al	; from 8259A interrupt controller
		in	al, 20h
		
		bt	ax, bx	; CF = ax_bit[bx]
		pop	bx
		pop	ax
		jnc	@@exc	; Not a pending IRQ - must be an Exception

@@redir3:	; Redirected interrupts 70h - 77h (IRQs)

		;push	int_num	 ; int_num alread pushed on stack
		sub	esp, 12	 ; instead of pushing eflags, cs, eip
		call	real_int ; calling IRQ's real-mode handler
		add	esp, 14

		iretd

@@redir4:	; Redirected interrupts 16 - 31 (Exceptions)

		sv_flg	EQU	[ebp+58]
		sv_cs	EQU	[ebp+54]
		sv_eip	EQU	[ebp+50]
		sv_err	EQU	[ebp+46]
		sv_exc	EQU	[ebp+44] ; 2
		sv_ss	EQU	[ebp+42] ; 2
		sv_es	EQU	[ebp+40] ; 2
		sv_ds	EQU	[ebp+38] ; 2
		sv_fs	EQU	[ebp+36] ; 2
		sv_gs	EQU	[ebp+34] ; 2
		sv_cs2	EQU	[ebp+32] ; 2
		sv_esp	EQU	[ebp+28]
		sv_ebp	EQU	[ebp+24]
		sv_edi	EQU	[ebp+20]
		sv_esi	EQU	[ebp+16]
		sv_edx	EQU	[ebp+12]
		sv_ecx	EQU	[ebp+8]
		sv_ebx	EQU	[ebp+4]
		sv_eax	EQU	[ebp+0]

		    X	EQU	6*160+24
		  ATTR	EQU	  4Fh

@@exc:		; Exceptions - print debug information and halt CPU

		; Check if exception pushes error code onto the stack
		; The bit is set for such exceptions 
		; 32 28 24 20 16 12  8  4  0
	EXC_TABL EQU 00000000000000100111110100000000b

		push	eax
		push	ebx
		mov	ax, [esp+8]	; int_num
		movzx	eax, ax
		mov	ebx, EXC_TABL
		bt	ebx, eax	; CF = exc_has_err_code[eax]
		pop	ebx
		pop	eax
		jc	@@err_code_present
		push	dword ptr [esp]		; If not we duplicate exc_num
		mov	dword ptr [esp+2], 0	; And put 0 instead of err_code
@@err_code_present:
		push	ss
		push	es
		push	ds
		push	fs
		push	gs
		push	word ptr [esp+20] ; cs
		push	esp
		push	ebp
		push	edi
		push	esi
		push	edx
		push	ecx
		push	ebx
		push	eax

		mov	ebp, esp
		mov	eax, esp
		add	eax, 62
		mov	sv_esp, eax

		mov	ds, cs:data16_sel
		mov	es, video_sel

		cld

		mov	cx, 12			; clear 12 screen lines
		mov	di, X
		mov	al, ' '
		mov	ah, ATTR
@@next_line:
		push	cx
		push	di
		mov	cx, 58
		rep	stosw
		pop	di
		pop	cx
		add	di, 160
		loop	@@next_line

		mov	si, offset exc_msg	; Print "Exception"
		mov	di, X+160+2
		call	wr_string

		mov	ax, sv_exc	; Printing exception number as decimal
		push	ax
		mov	bl, 10
		div	bl
		add	ax, 3030h
		mov	bl, ah
		cmp	al, '0'
		jne	@@not_zero
		mov	al, ' '
@@not_zero:
		mov	ah, ATTR
		stosw
		mov	al, bl
		stosw
		call	wr_string	; Print " - "
		pop	cx

		cmp	cx, 18		; Printing exception description
		jbe	@@scan
		mov	cx, 15		; Int 15, & 19-31 are reserved by Intel
@@scan:
		jcxz	@@print
@@next_char:	lodsb
		cmp	al, 0
		jne	@@next_char
		dec	cx		; zero found - passed another string
		jmp	@@scan
@@print:
		call	wr_string

		mov	di, X+1*160+86		; Printing EIP
		mov	si, offset exc_eip
		call	wr_string_eq
		mov	edx, sv_eip
		call	wr_reg32

		mov	di, X+9*160+80		; Printing EFLAGS
		call	wr_string_eq
		mov	edx, sv_flg
		call	wr_reg32

		mov	di, X+10*160+78		; Printing Error Code
		call	wr_string
		mov	edx, sv_err
		call	wr_reg32

		mov	di, X+3*160+2		; Printing 32-bit registers
		mov	cx, 8
		mov	ebx, ebp
@@next_reg32:
		call	wr_string_eq
		mov	edx, ss:[ebx]
		call	wr_reg32
		add	ebx, 4
		add	di, 130
		loop	@@next_reg32

		mov	di, X+3*160+78		; Printing segment registers
		mov	cx, 6
@@next_seg_reg:
		call	wr_string_eq
		mov	dx, ss:[ebx]
		push	dx
		call	wr_reg16
		pop	dx
		movzx	edx, dx
		lsl	edx, edx		; get segment limit
		call	wr_reg32
		add	ebx, 2
		add	di, 122
		loop	@@next_seg_reg

		mov	di, X+3*160+34		; Printing stack
		mov	ebx, sv_esp
		and	ebx, 0FFFFFFFCh		; align 4
		add	ebx, 7*4
		mov	cx, 8
@@next_item:
		push	cx
		mov	si, offset exc_ss
		call	wr_string
		sub	di, 2
		mov	edx, ebx
		call	wr_reg32
		mov	dx, ss
		movzx	edx, dx
		lsl	edx, edx	; load segment limit
		sub	edx, 3
		cmp	ebx, edx	; does it fit in the stack
		ja	@@print_dots
		mov	edx, ss:[ebx]
		call	wr_reg32
		jmp	@@advance
@@print_dots:
		mov	cl, 8
		mov	al, '.'
		rep	stosw
		mov	al, ' '
		stosw
@@advance:
		add	di, 118
		sub	ebx, 4
		pop	cx
		loop	@@next_item

		;clts	; Clear Task Switched bit (if it is set)
		sti
		mov	al, 1		; exit code
		jmp	exit_32_bit	; Exit protected mode and cleanup

int_handler	endp

;----------------------------------------------------------------------

wr_video	proc	near

wr_reg16:
		push	cx
		shl	edx, 16
		mov	cx, 4
		jmp	short @@wr_hex
wr_reg32:
		push	cx
		mov	cx, 8
@@wr_hex:
		push	bx
		mov	bx, offset HEX_DIGIT_TAB
@@next_dig:
		mov	ah, ATTR
		rol	edx, 4
		mov	al, dl
		and	al, 0Fh
		xlat   ;al=[bx+al]
		stosw
		loop	@@next_dig
		
		pop	bx
		pop	cx
		jmp	short @@tail_sp

wr_string_eq:
		call	wr_string
		mov	al, '='
		stosw
		jmp	short	@@tail_sp
@@next_char:
		stosw
wr_string:
		mov	ah, ATTR
		lodsb
		or	al, al
		jne	@@next_char
@@tail_sp:
		mov	al, ' '
		stosw

		ret

wr_video	endp

;----------------------------------------------------------------------
_TEXT16		ends
;----------------------------------------------------------------------
