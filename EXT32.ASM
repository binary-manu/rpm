PAGE 255,132
LOCALS
.386p

PM_STACK_SIZE	EQU	10000h	; 64k protected-mode stack

;SCR_X EQU 720
SCR_X EQU 640
SCR_Y EQU 400

OWN_CODE_SIZE	EQU	127*512

SGROUP	GROUP _TEXT16, _DATA16, _ENDS16

_TEXT16	segment	para public use16 'START'
_TEXT16 ends
_DATA16	segment para public use16 'START'
_DATA16	ends
_ENDS16	segment para public use16 'START'
data16_size label byte
_ENDS16	ends

MODEL	SMALL	; Need to make offsets relative to the beginning of the groups

CODE	GROUP _TEXT, _TEXTEND
DGROUP	GROUP _DATA, _DATAEND, _BSS, _BSSEND, GRAFIX_DATA, MARKER_DATA, _INIT_, _EXIT_

_TEXT	 segment public use32 'CODE'
_TEXT	 ends
_TEXTEND segment para   use32 'CODE'
code32_size label byte
_TEXTEND ends
_DATA	 segment public use32 'DATA'
_DATA	 ends
GRAFIX_DATA	 segment public use32 'DATA'
GRAFIX_DATA	 ends
MARKER_DATA	 segment public use32 'DATA'
MARKER_DATA	 ends
_INIT_	 segment public use32 'INITDATA'
_INIT_	 ends
_EXIT_	 segment public use32 'EXITDATA'
_EXIT_	 ends
_DATAEND segment para   use32 'BSS' stack
data32_size label byte
	public	cmd_line, own_code
	cmd_line db 128 Dup(?)
	own_code db OWN_CODE_SIZE Dup(?)
bss_start   label byte
_DATAEND ends
_BSS	 segment public use32 'BSS'
_BSS	 ends
_BSSEND  segment para 	use32 'BSS'
bss_end     label byte
dgroup_size label byte
	 DB PM_STACK_SIZE Dup(?)
pm_stack_top label byte
min_mem_size label byte
_BSSEND  ends

;----------------------------------------------------------------------

seg_descriptor	struc		; 8 bytes
  limit0_15	dw 	0
  base0_15	dw	0
  base16_23	db	0
  access	db	0
  gd00lim16_19	db	0
  base24_31	db	0
seg_descriptor	ends     

int_descriptor	struc		; 8 bytes
  offset0_15	dw	0
  selector	dw	0
  zero_byte	db	0
  access	db	0
  offset16_31	dw	0
int_descriptor	ends

dt_reference	struc		; 6 bytes
  dt_limit	dw	0
  dt_base	dd	0
dt_reference	ends

;----------------------------------------------------------------------
check_point	macro	n
		mov	last_check_pt, n
endm
;----------------------------------------------------------------------
_DATA16		segment
;----------------------------------------------------------------------

HEX_DIGIT_TAB	DB "0123456789ABCDEF"

align 	16			;              DPL
			;code:	;limit   base  P11CRA   GD00limit  base
			;data:	; 0-15   0-23  P10EWA   GD0016-19  24-31
gdt		seg_descriptor  <>					; sel
zero_dsc	seg_descriptor  <0ffffh, 0, 0, 10010010b, 00001111b, 0>	;   8
code16_dsc	seg_descriptor  <0ffffh, 0, 0, 10011010b, 00000000b, 0>	;c:10
data16_dsc	seg_descriptor  <0ffffh, 0, 0, 10010010b, 00000000b, 0>	;d:18
buffer_dsc	seg_descriptor  <0ffffh, 0, 0, 10010010b, 00000000b, 0>	;b:20 
video_dsc	seg_descriptor  <00fffh, 0, 0, 10010010b, 00000000b, 0> ;  28
code32_dsc	seg_descriptor  <00000h, 0, 0, 10011010b, 01000000b, 0> ;c:30
data32_dsc	seg_descriptor  <00000h, 0, 0, 10010010b, 11000000b, 0>	;d:38

align	16
;				;  offset,   selector, 0, DPL,     offset
;				;    0-15                 P0D110  16-31
;idt		int_descriptor	<i_offset, code16_sel, 0, 10001110b, 0>

 idt	label	byte
i=0
rept 20h; int 00h - int 1Fh
    int_descriptor <offset int_00 + i, code16_dsc-gdt, 0, 10001110b, 0>
i=i+4
endm	; int 20h - 6Eh	Error messages will be owerwritten with zeros:
	;		if one of int 20h - int 6Eh  will be called it
	;		will generate General Protection Fault

msg0	DB "Need 80386 or better CPU.", 0
msg1	DB "Not enough conventional memory.",0

msg2	DB "DPMI has version prior to 0.90 or cannot run 32-bit programs.",0
msg3	DB "DPMI requested more conventional memory than available.",0
msg4	DB "DPMI failed to switch to proected mode.",0
msg5	DB "DPMI failed to provide memory information.",0
msg6	DB "DPMI failed to allocate memory.",0
msg7	DB "DPMI failed to lock memory.",0
msg8	DB "DPMI failed to initialize descriptors.",0
msg9	DB "DPMI failed to set interrupt handler.",0

msg10	DB "CPU is running in protected mode, but DPMI is not available.",0

msg11	DB "XMS failed to enable A20 line.",0
msg12	DB "XMS doesn't have enough memory.",0
msg13	DB "XMS failed to allocate memory.",0
msg14	DB "XMS failed to lock memory.",0

msg15	DB "Not enough extended memory.",0
msg16	DB "Cannot enable A20 line.",0

	DB ( (6Fh-20h)*8 - ($-msg0) ) Dup (0)

	; int 6Fh
	int_descriptor <offset int_6Fh,    code16_dsc-gdt, 0, 10001110b, 0>

rept 8	; int 70h - 77h
	int_descriptor <offset int_00 + i, code16_dsc-gdt, 0, 10001110b, 0>
i=i+4
endm

exc_msg	DB "Exception", 0, " -", 0
	DB "Divide Error", 0
	DB "Debug", 0
	DB "NMI", 0
	DB "Break Point", 0
	DB "Overflow", 0
	DB "BOUND Range", 0
	DB "Invalid Opcode", 0
	DB "Device Not Available", 0
	DB "Double Fault", 0
	DB "CoProcessor", 0
	DB "Invalid TSS", 0
	DB "Segment Not Present", 0
	DB "Stack Fault", 0
	DB "General Protection Fault", 0
	DB "Page Fault", 0
	DB "Reserved", 0
	DB "Floating-Point", 0
	DB "Alignment Check", 0
	DB "Machine Check", 0
exc_eip	DB "EIP",0, "EFLAGS",0, "ErrorCode",0
	DB "EAX",0, "EBX",0, "ECX",0, "EDX",0
	DB "ESI",0, "EDI",0, "EBP",0, "ESP",0
	DB "CS", 0,  "GS",0,  "FS",0
	DB "DS", 0,  "ES",0,  "SS",0
exc_ss	DB "SS:",0

startup_hd	db	?
detected_os	db	?
last_check_pt	db	?

align	4	; Don't change order of these variables !!!

code16_seg	label	word	; SGROUP 16-bit code, data, and stack segments
data16_seg	dw	?

rm_stack_16	label	fword	; Real-mode 16-bit stack pointer
rm_sp		label	word
rm_esp		dd	0	; Will wrap to 0FFFFh
rm_ss		label	word
buffer_seg	dw	?	; I/O buffer for real mode calls + rm_stack
conv_seg	dw	?	; Free conventional memory after buffer_seg
conv_len	dw	?	; its lenght in paragraphs (16 bytes/par)

zero_sel	dw	zero_dsc - gdt	; to reference first Meg of memory

pm_exit_32	label	fword	; Address for jumping from 32- to 16-bit code
		dd	offset exit_32_bit
code16_sel	dw	code16_dsc - gdt

data32_addr	dd	?	; Linear address of original copy of data32_seg
data16_sel	dw	data16_dsc - gdt

tmp_esp		dd	?
buffer_sel	dw	buffer_dsc - gdt

tmp_eax		dd	?
video_sel	dw	video_dsc - gdt

pm_enter_32	label	fword	; Address for jumping from 16- to 32-bit code
		dd	offset enter_32_bit
code32_sel	dw	code32_dsc - gdt

pm_stack_32	label	fword	; Protected-mode 32-bit stack pointer
pm_esp		dd	offset pm_stack_top
data32_sel	dw	data32_dsc - gdt

pm_gdtr		dt_reference	<   8 * 8 - 1, offset gdt >
pm_idtr		dt_reference	< 78h * 8 - 1, offset idt >
rm_idtr		dt_reference	< >	; to save real mode IDTR

align	4

mem_addr	dd	100000h ; data32 segment base (default value = 1M)
mem_size	dd	?	; data32 segment size
mem_handle	dd	?	; handle for that memory given by DPMI or XMS

DPMI_switch	label	dword	; temporary pointer to DPMI entry point
XMS_call	dd	?	; pointer to XMS manager service handler
tmp_flag_ret	dd	?

exit_jump_addr	dd	0	; On bare system jmp on exit if not zero
exit_jump_dl	db	0	; and put this value in dl register (hd)

vmode		db	0
IFDEF MOUSE_AND_VIDEO
mouse_type	db	0
mou_buttons	db	0
mou_x		dw	0
mou_y		dw	0
ENDIF

;----------------------------------------------------------------------
_DATA16		ends
;----------------------------------------------------------------------

 ASSUME	CS:SGROUP, DS:SGROUP

;----------------------------------------------------------------------
_TEXT16		segment	; 16-bit real-mode BIOS and DOS startup code
;----------------------------------------------------------------------

startup		proc	near

		call	@@next_line	; if 
@@next_line:	pop	si
		sub	si, 3
		jz	@@offset0
		mov	ax, cs
		mov	cl, 4
		shr	si, cl
		add	ax, si
		push	ax	; segment
		xor	ax, ax
		push	ax	; offset
		mov	es, ax
		retf
@@offset0:
		mov	ax, es	; If ES=0 we are loaded at 1000h:0000h
		or	ax, ax	; on a bare system with no OS (only BIOS)
		jnz	@@dos_startup
@@bare_startup:
		int	12h		; Get conventional memory size
		mov	cl, 6		; ax = conventional memory size in kb
		shl	ax, cl		; Converting to paragraphs (16 bytes)
		mov	bx, ax		; bx = top+1 paragraph of conventional
					;      memory available to the program
		mov	al, 0		; detected_os=0 (bare system = no DOS)

		jmp	short @@common_startup
		;
		;  Entry point if code is loaded by DOS to ????h:0000h
		;
@@dos_startup:
		mov	bx, 2		; bx = top+1 paragraph of conventional
		mov	bx, es:[bx]	;      memory available to the program

		push	es
		push	bx
		call	check_os	; returns detected_os in al
		pop	bx
		pop	es
		;
@@common_startup:
		mov	cx, cs
		mov	ds, cx
		mov	data16_seg, cx
		mov	detected_os, al
		mov	startup_hd, dl
		mov	conv_len, bx	; Conventional memory (in paragraphs)
		;
		;  Checking that CPU is 386+
		;
		check_point 0

		push	sp	; CPUs prior to 386 would decrement SP
		pop	ax	; and then push it.
		cmp	ax, sp	; On 386+ these values should be equal
		jne	@@error

		;
		;  Setting up segments
		;
		mov	eax, offset data16_size	; Calculate total size of
		add	eax, offset code32_size	; loaded data
		add	eax, offset data32_size
		mov	ecx, eax
		shr	eax, 4		; Convert to number of paragraphs
		add	ax, data16_seg
		mov	buffer_seg, ax
		add	ax, 1000h	; 64k buffer

		check_point 1
		mov	conv_seg, ax	; free conventional memory after buffer
		sub	conv_len, ax
		jc	@@error		; not enough memory for buffer

		lss	esp, rm_stack_16 ; Set real-mode 16-bit stack

		mov	ah,0Fh		; Check current video mode
		int	10h
		cmp	al, 07h
		mov	word ptr video_dsc+3, 0B00h ; Monochrome video base
		je	@@mono
		cmp	al, 03h
		mov	word ptr video_dsc+3, 0B80h ; Color video base
		je	@@color
		mov	ax,03		; If unknown set to "Color 80x25"
		int	10h
@@color:
@@mono:		;
		;  Copy cmd_line and own_code to the buffer segment
		;
		push	cx	; code lenght
		push	ds	; data16_seg
		push	es	; psp_seg
		mov	es, buffer_seg
		pop	ds	; ds=psp
		cld
		xor	di, di		; first clear buffer
		mov	al, 0
		mov	cx, 128+OWN_CODE_SIZE
		rep	stosb

		mov	si, 80h		; Copy command line arguments
		xor	di, di
		lodsb
		mov	cl, al		; cmd_line lenght
		mov	al, '*'		; fake argv[0]
		stosb
		rep
		  movsb
		pop	ds	; data16_seg
		pop	cx	; code lenght

		xor	si, si		; Copy our own code and data
		mov	di, 80h
		rep
		  movsb

		;
		;
		call	mouse_init
		;
		;  Now switching CPU into protected mode
		;
		cmp	detected_os, 0	; Is it a bare system (no OS running)
		je	@@raw_init

		call	dpmi_init	; if it finds DPMI - it jumps to 32-bit
		jc	@@error

		check_point 10
		mov	eax, cr0	; If CPU is already running in the
		test	al, 1		; protected mode then there is no way
		jnz	@@error		; we could switch it to PM ourselves.

		call	xms_init	; if it finds XMS - it jumps to 32-bit
		jc	@@error
@@raw_init:
		call	raw_init
		jc	@@error
		jmp	@@exit
@@error:
print_error:
		mov	cl, last_check_pt
		mov	ch, 0
		mov	si, offset msg0
		jcxz	@@print		; looking for CX zero-terminated string
@@next_char:
		lodsb
		cmp	al, 0
		jne	@@next_char
		dec	cx		; Another message just was passed
		jnz	@@next_char
@@print:
		lodsb
		cmp	al, 0
		je	@@eol
		mov	ah, 0Eh		; print a character
		mov	bh, 00h
		int	10h
		jmp	@@print
@@eol:
		mov	al, 13
		int	10h
		mov	al, 10
		int	10h
@@exit:		;
		call	mouse_done
		;

		cmp	detected_os, 0	; if there is no OS then we will enter
		jne	@@dos_exit	; exit to dos

		cmp	al, 0	; exit code
@@halt:		jne	@@halt	; If errors halt (user will need to reboot)

		cmp	exit_jump_addr, 0
		je	@@reboot
		
		mov	dl, exit_jump_dl ; passing hard disk number
		xor	ax, ax
		mov	ds, ax
		mov	es, ax
		mov	ss, ax
		mov	sp, 7C00h
		jmp	cs:exit_jump_addr
@@reboot:
		int	19h	; reboot (bootstrap)
@@dos_exit:
		mov	ah, 4Ch ; exit code was passed in al
		int	21h

startup		endp

;----------------------------------------------------------------------

IFDEF MOUSE_AND_VIDEO

mou_handler	proc	far	; Handler for PS/2 mouse
		
	MOU_Z	EQU	word ptr [bp+6]
	MOU_Y	EQU	byte ptr [bp+8]
	MOU_X	EQU	byte ptr [bp+10]
	MOU_ST	EQU	byte ptr [bp+12]

		push	bp
		mov	bp, sp
		push	ds
		;mov	ds, cs:data16_seg
		push	cs ;cs=ds
		pop	ds
		push	ax
		push	bx
		
		mov	al, MOU_X
		cbw
		add	mou_x, ax
		js	@@x_zero
		cmp	mou_x, SCR_X
		jb	@@x_good
		mov	mou_x, SCR_X-1
		jmp	@@x_good
@@x_zero:
		mov	mou_x, 0
@@x_good:	

		mov	al, MOU_Y
		cbw
		neg	ax
		add	mou_y, ax
		js	@@y_zero
		cmp	mou_y, SCR_Y
		jb	@@y_good
		mov	mou_y, SCR_Y-1
		jmp	@@y_good
@@y_zero:
		mov	mou_y, 0
@@y_good:	

		mov	al, MOU_ST
		and	al, 3h
		mov	mou_buttons, al

comment %
	push	es
	;mov	ax, mou_y
	;shr	ax, 2
	;mov	bx, 80h
	;mul	bx
	mov	bx, mou_x
	shr	bx, 3
	;add	bx, ax
	mov	ax, 0B800h
	mov	es, ax
	mov	ax, 741h
	shl	bx, 1
	mov	es:[bx],ax
	pop	es
%	
		pop	bx
		pop	ax
		pop	ds
		pop	bp
		retf

mou_handler	endp


mouse_init	proc	near

		push	es
		pusha

		mov	mouse_type, 0

		xor	ax, ax		; Reset mouse
		int	33h
		cmp	ax, 0FFFFh
		je	@@driver_installed

;@@initialize_ourselves:

		mov	mou_buttons, 0
		mov	mou_x, SCR_X/2
		mov	mou_y, SCR_Y/2
		
		mov	ax, 0C201h	; Reset PS/2 mouse
		int	15h
		jc	@@end

		mov	ax, 0C207h
		mov	bx, offset mou_handler
		push	cs
		pop	es
		int	15h
		jc	@@end

		mov	ax, 0C205h
		mov	bh, 3	; packet size
		int	15h
		jc	@@end

		mov	ax, 0C200h
		mov	bh, 1		; Enable PS/2 mouse
		int	15h
		jc	@@end

		mov	mouse_type, 2
comment %
	NUM	EQU	30

	mov	cx, NUM
@@again:
	push	cx
	push	es
	mov	bx, 0B800h
	mov	es, bx
	xor	bx, bx
	mov	ax, NUM
	sub	ax, cx
	mov	cl, 10
	div	cl
	mov	cl, ah
	mov	ah, 7
	mov	ch, 7
	add	al, 30h
	add	cl, 30h
	mov	es:[bx+10], ax
	mov	es:[bx+12], cx
	pop	es
	cmp	mou_buttons, 0
	jne	@@break
	mov	ah, 86h
	mov	cx, 4
	int	15h
	pop	cx
	loop	@@again
@@break:
	call	mouse_done
	
	mov	ah, 4Ch
	int	21h
%
		jmp	@@end
@@driver_installed:
		mov	mouse_type, 1
		mov	ax, 7		; Set horizontal mouse range
		mov	cx, 0
		mov	dx, SCR_X-1
		int	33h

		mov	ax, 8		; Set vertical mouse range
		mov	cx, 0
		mov	dx, SCR_Y-1
		int	33h
		
		mov	ax, 4		; Set mouse position
		mov	cx, SCR_X/2
		mov	dx, SCR_Y/2
		int	33h
@@end:
		popa
		pop	es
		ret
mouse_init	endp

mouse_done	proc	near

		cmp	mouse_type, 2
		jb	@@end
		push	ax
		push	bx

		;mov	ax, 0C200h	
		;mov	bh, 0		; Disable PS/2 mouse
		;int	15h

		mov	ax, 0C201h	; Reset PS/2 mouse
		int	15h

		pop	bx
		pop	ax
@@end:
		ret
mouse_done	endp
ELSE
mouse_init	proc	near
		ret
mouse_init	endp
mouse_done	proc	near
		ret
mouse_done	endp
ENDIF

;----------------------------------------------------------------------
_TEXT16		ends
;----------------------------------------------------------------------
include		ext32msc.inc	; miscelanious functions 
include		ext32a20.inc	; A20 line enabling/disabling
include		ext32raw.inc	; RAW protected-mode initialization 
include		ext32xms.inc	; XMS protected-mode initialization 
include		ext32dpm.inc	; DPMI protected-mode initialization 
;----------------------------------------------------------------------
_TEXT16		segment		; 16-bit protected mode code 
;----------------------------------------------------------------------

call_32_bit	proc	near
		;
		;  After CPU enters PM we call this function
		;  At this point there is already 32-bit stack
		;
		mov	tmp_esp, esp		; Saving stack ptr
		and	esp, -4			; Align esp on 4-byte boundary

		mov	gs, data16_sel		; Setting up selectors
		mov	fs, buffer_sel
		mov	es, data32_sel
		mov	ds, data32_sel

		jmp	gs:pm_enter_32		; Jumping to 32-bit code

exit_32_bit:	; 32-bit code returns here with the exit code in al

		mov	ds, gs:data16_sel 	; Restoring 16-bit data segment
		mov	esp, tmp_esp		; Restoring stack ptr

		ret	; Whoever called us will cleanup and exit from PM
		
call_32_bit	endp

;----------------------------------------------------------------------
_TEXT16		ends
;----------------------------------------------------------------------

ASSUME	CS:CODE, DS:DGROUP, GS:SGROUP	; Assumptions for 32-bit code

;----------------------------------------------------------------------
rm_int		macro	int_num
		pushfd
		mov	byte ptr [esp], int_num
		int	6Fh
		popfd
endm
;----------------------------------------------------------------------
_TEXT		segment			; 32-bit protected mode code
;----------------------------------------------------------------------

enter_32_bit	proc	far

		public	exit
		public  get_startup_hd
		public  get_detected_os
		public	copy_to_zero_sel
		public  set_on_exit_jump

		;extrn	main: near
		;extrn	mem_lib_init: near
		extrn	@main$qippc: near
		extrn	@mem_lib_init$qpcl: near

		;
		;  Copy initialized DATA to memory above 1M and clear BSS
		;  
		push	ds

		mov	ds, zero_sel
		mov	esi, data32_addr
		xor	edi, edi
		mov	ecx, offset bss_start
		shr	ecx, 2
		cld
		rep	movsd		; copy data

		xor	eax, eax
		mov	ecx, offset bss_end
		sub	ecx, edi
		shr	ecx, 2
		rep	stosd		; clear bss
		
		pop	ds

		;
		; Initialize memory management
		;
		mov	ebx, mem_size
		mov	eax, offset pm_stack_top
		sub	ebx, eax
		push	ebx		; heap_size
		push	eax		; heap_start
		call	@mem_lib_init$qpcl
		add	esp, 8
		
		;
		;  Splitting command line into arguments
		;
		xor	ebx, ebx	; number of arguments (argc)
		push	ebx		; last argv pointer is zero
		cmp	detected_os, 0	; Are we running under some OS?
		je	@@cmd_done	; No OS - no arguments
@@parse_cmd_line:
		mov	ecx, offset cmd_line
		mov	edi, offset cmd_line+80h
@@skip_zeros:	cmp	edi, ecx
		jbe	@@cmd_done
		dec	edi
		cmp	byte ptr[edi], 0
		jne	@@skip_zeros
@@kill_spaces:
		mov	byte ptr[edi], 0
		cmp	edi, ecx ; cmd_line
		jbe	@@cmd_done
		dec	edi
		cmp	byte ptr[edi], 32	; Space
		je	@@kill_spaces
		cmp	byte ptr[edi], 9	; Tab
		je	@@kill_spaces
@@skip_words:
		mov	esi, edi
		cmp	edi, ecx ; cmd_line
		jbe	@@last_arg
		dec	edi
		cmp	byte ptr[edi], 32	; Space
		je	@@white_space
		cmp	byte ptr[edi], 9	; Tab
		jne	@@skip_words
@@white_space:
		push	esi		; Remember argument address 
		inc	ebx		; Increase argument's count
		jmp	@@kill_spaces
@@last_arg:
		push	esi
		inc	ebx
@@cmd_done:
		mov	eax, esp	; argv
		;
		; Calling main
		;
		push	eax		; Array of arguments  (argv)
		push	ebx		; Number of arguments (argc)
		call	@main$qippc
		add	esp, 8
		;
		; Exiting
		;
		push	eax
		call	exit

enter_32_bit	endp

;----------------------------------------------------------------------

exit		proc	C	near

		arg	@@status: dword

		mov	eax, @@status
		jmp	pm_exit_32	; Exiting from 32-bit to 16-bit code

exit		endp

;----------------------------------------------------------------------

get_detected_os	proc	near

		movzx	eax, detected_os
		ret

get_detected_os	endp

;----------------------------------------------------------------------

get_startup_hd	proc	near

		movzx	eax, startup_hd
		ret

get_startup_hd	endp

;----------------------------------------------------------------------

set_on_exit_jump	proc	C	near

		arg	@@jump_addr: dword, @@jump_dl

		mov	eax, @@jump_addr
		mov	exit_jump_addr, eax
		mov	eax, @@jump_dl
		mov	exit_jump_dl, al
		ret

set_on_exit_jump	endp

;----------------------------------------------------------------------

copy_to_zero_sel	proc	C	near

		arg	@@addr: dword, @@buf: dword, @@len: dword
		
		push	esi
		push	edi
		push	es
		mov	es, zero_sel

		mov	esi, @@buf
		mov	edi, @@addr		
		mov	ecx, @@len
		push	ecx
		shr	ecx, 2
		cld
		rep
		  movsd
		pop	ecx
		and	ecx, 3
		rep
		  movsb

		pop	es
		pop	edi
		pop	esi

		ret

copy_to_zero_sel	endp

;----------------------------------------------------------------------
IFDEF MOUSE_AND_VIDEO

public		get_mouse, set_mouse_resolution

get_mouse	proc	C	near
		
		arg	@@x_ptr: dword, @@y_ptr:dword, @@bt_ptr:dword

		push	esi
		push	edi
		push	ebx
		
		xor	ebx, ebx
		xor	ecx, ecx
		xor	edx, edx

		cmp	mouse_type, 1
		jb	@@end		; 0 - no mouse support 
		je	@@tsr_driver	; 1 - mouse driver installed
		;ja	@@own_driver	; 2 - using our own driver
@@own_driver:
		mov	bl, mou_buttons
		mov	cx, mou_x
		mov	dx, mou_y
		jmp	@@end
@@tsr_driver:
		mov	ax, 0003h
		rm_int	33h
@@end:
		mov	eax, @@x_ptr
		mov	[eax], ecx
		mov	eax, @@y_ptr
		mov	[eax], edx
		mov	eax, @@bt_ptr
		mov	[eax], ebx

		pop	ebx
		pop	edi
		pop	esi

		
		ret

get_mouse	endp


set_mouse_resolution	proc	C	near
		
		arg	@@x: dword, @@y:dword

		push	esi
		push	edi
		push	ebx
		
		cmp	mouse_type, 1
		jne	@@end

		mov	eax, 7		; Set horizontal mouse range
		mov	ecx, 0
		mov	edx, @@x
		dec	edx
		rm_int	33h

		mov	eax, 8		; Set vertical mouse range
		mov	ecx, 0
		mov	edx, @@y
		dec	edx
		rm_int	33h
		
		mov	eax, 4		; Set mouse position
		mov	ecx, @@x
		shr	ecx, 1
		mov	edx, @@y
		shr	edx, 1
		int	33h
@@end:
		pop	ebx
		pop	edi
		pop	esi

		
		ret

set_mouse_resolution	endp
ENDIF

;----------------------------------------------------------------------
_TEXT		ends
;----------------------------------------------------------------------
include		ext32con.inc	; 32-bit console I/O functions
include		ext32dsk.inc	; 32-bit hard disk I/O functions
include		ext32dos.inc	; 32-bit DOS file system functions
IFDEF MOUSE_AND_VIDEO
include		ext32vid.inc
ENDIF
;----------------------------------------------------------------------

		end	startup
