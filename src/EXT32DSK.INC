;
; ext32dsk.inc	Protected mode hard disk I/O functions
;

public	hard_disk_get_info
public	hard_disk_read
public	hard_disk_write
public	hard_disk_clear
public	hard_disk_verify

public	floppy_disk_read
public	floppy_disk_write

PUBLIC	StoreMBR
PUBLIC	LoadMBR
PUBLIC	ReadCMOSData
PUBLIC	WriteCMOSData

disk_info		struc
 hd_flags	dd	?
 hd_num		dd	?
 num_cyls	dd	?
 num_heads	dd	?
 sect_per_track	dd	?
 sect_per_cyl	dd	?
 total_sects	dd	?
ends

disk_lba_info	struc
 lba_inf_buf_sz	DW	?
 lba_inf_flags	DW	?
 lba_inf_cyls	DD	?
 lba_inf_heads	DD	?
 lba_inf_sects	DD	?
 lba_inf_total	DQ	?
 lba_inf_ssize	DW	?
ends

disk_lba_addr	struc
 lba_buf_size	DB	?
 lba_reserved	DB	?
 lba_num_sect	DW	?
 lba_buf_addr	DD	?
 lba_rel_sect	DQ	?
ends

;----------------------------------------------------------------------
_TEXT 		segment			; 32-bit protected mode code
;----------------------------------------------------------------------

hard_disk_get_info	proc	C	near

		arg	@@hd: dword, @@disk_info: dword
		local	@@num_disks: dword

		push	ebx
		push	esi
		push	edi
		
		mov	edi, @@disk_info
		mov	[edi].hd_flags, 0

		mov	ah, 08h			; get hard disk info
		mov	dl, byte ptr @@hd
		movzx	edx, dl
		mov	[edi].hd_num, edx
		push	edi
		rm_int	13h
		pop	edi
		jc	@@error		

		movzx	eax, dl
		mov	@@num_disks, eax

		movzx	eax, dh
		inc	eax
		mov	[edi].num_heads, eax

		movzx	eax, cl
		and	al, 3Fh
		mov	[edi].sect_per_track, eax

		mov	eax, ecx
		xchg	ah, al
		shr	ah, 6
		movzx	eax, ax
		inc	eax
		mov	[edi].num_cyls, eax
COMMENT %
		;
		;  Now we try to read last sector on last+1 cylinder of disk
		;  If we can do that - BIOS hides an extra cylinder
		;
		add	ch, 1
		jnc	@@skip
		add	cl, 40h
	@@skip:
		jc	@@count_total	; there is already 1024 cylinders

		mov	ax, 0201	; read 1 sector
		mov	bx, 0		; beginning of the buffer
		mov	dl, byte ptr @@hd
		rm_int	13h
		jc	@@count_total	; error - no extra cylinder

		inc	[edi].num_cyls
%
@@count_total:
		mov	eax, [edi].sect_per_track
		mul	[edi].num_heads
		mov	[edi].sect_per_cyl, eax
		mul	[edi].num_cyls
		mov	[edi].total_sects, eax
		;
		; Now lets check LBA interface
		;
		mov	ah, 41h		; Installation check
		mov	bx, 055AAh
		mov	dl, byte ptr @@hd
		rm_int	13h
		jc	@@no_lba
		
		cmp	bx, 0AA55h
		jne	@@no_lba

		test	cl, 1	; read/write/verify/seek/get_info supported
		jz	@@no_lba

		mov	ah, 48h		; Get drive parameters
		xor	esi, esi	; beginning of the buffer
		mov	fs:[esi].lba_inf_buf_sz, 1Ah
		mov	fs:[esi].lba_inf_flags,   0h
		mov	dl, byte ptr @@hd
		rm_int	13h
		jc	@@no_lba

 		or	[edi].hd_flags, 1	; LBA supported

		mov	eax, dword ptr fs:[esi].lba_inf_total
		cmp	eax, [edi].total_sects
		;jb	@@dont_save_total  ; LBA reports less sectors than CHS
		jb	@@dont_save_cyls   ; LBA reports less sectors than CHS
		mov	[edi].total_sects, eax

		; Need to comment it because some BIOSes report 255 heads
		; for CHS interface and 14-16 for LBA - we have to use 255
;@@dont_save_total:
		;mov	ax, fs:[esi].lba_inf_flags
		;test	ax, 2	; Does LBA info provide Cyl, Head, Sect
		;jz	@@calc_chs
		
		;mov	eax, fs:[esi].lba_inf_heads
		;mov	[edi].num_heads, eax
		;mov	ebx, fs:[esi].lba_inf_sects
		;mov	[edi].sect_per_track, ebx
		;mul	bx
		;mov	[edi].sect_per_cyl, eax
		;mov	eax, fs:[esi].lba_inf_cyls
		;jmp	@@check_cyls
;@@calc_chs:
		mov	eax, [edi].total_sects
		xor	edx, edx
		div	[edi].sect_per_cyl
;@@check_cyls:
		;cmp	eax, [edi].num_cyls
		;jb	@@dont_save_cyls  ; LBA reported less cyls than CHS
		mov	[edi].num_cyls, eax
@@dont_save_cyls:
@@no_lba:
		mov	eax, @@num_disks
		jmp	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret
hard_disk_get_info	endp

;----------------------------------------------------------------------

disk_operation	proc	near
		;
		;  Input:    ah  - operation (read/write/verify)
		;            ecx - number of sectors
		;            edi - relative sector
		;            esi - address of hd_info
		;
		;  Output:   CF  - set on error
		;
		;  Destroys: eax, ebx, edx, esi
		;

		test	[esi].hd_flags, 1
		jz	@@chs_mode
@@lba_mode:
		push	eax
		mov	edx, [esi].hd_num
		mov	esi, 126*512 ; area following I/O buffer
		mov	fs:[esi].lba_buf_size, 16
		mov	fs:[esi].lba_reserved,  0
		mov	fs:[esi].lba_num_sect, cx
		mov	ax, buffer_seg
		mov	word ptr fs:[esi].lba_buf_addr, 0
		mov	word ptr fs:[esi].lba_buf_addr+2, ax
		mov	dword ptr fs:[esi].lba_rel_sect, edi
		mov	dword ptr fs:[esi].lba_rel_sect+4, 0
		pop	eax
		add	ah, 40h	; LBA operations
		mov	al, 00h ; zero flags if writing (no verify)
		jmp	@@int13
@@chs_mode:
		mov	al, cl	  ; num_sect
		push	eax
		mov	eax, edi  ; relative sector
		xor	edx, edx
		div	[esi].sect_per_track
		mov	cl, dl	; sector
		inc	cl
		xor	edx, edx
		div	[esi].num_heads
		cmp	eax, 1023 ; cyl
		jbe	@@cyl_ok
		stc
		jmp	@@end
	@@cyl_ok:
		mov	dh, dl	; head
		mov	ch, al	; cyl-lo
		shr	eax, 2
		and	al, 0C0h
		or	cl, al	; cyl-hi
		mov	dl, byte ptr [esi].hd_num
		pop	eax
		xor	ebx, ebx	; beginning of the buffer
@@int13:
		rm_int  13h
@@end:
		ret

disk_operation	endp

;----------------------------------------------------------------------

hard_disk_read	proc	C	near

		arg	@@hd_info: dword, @@rel: dword, @@buf: dword, \
							@@num: dword
		push	ebx
		push	esi
		push	edi

@@read_more:
		mov	ecx, @@num
		mov	edi, @@rel
		mov	esi, @@hd_info
		
		cmp	ecx, 126  ; max num of sectors that 63k buffer can hold
		jbe	@@num_sect_ok
		mov	ecx, 126
@@num_sect_ok:

		push	ecx
		mov	ah, 2   	; read
		call	disk_operation
		pop	ecx		; number of sectors read
		jc	@@error

		push	ecx		; copy read data to _DATA segment
		shl	ecx, 7		; 128 dwords per sector
		xor	esi, esi	; beginning of the I/O buffer
		mov	edi, @@buf	; destination in _DATA segment
		push	ds
		mov	ds, buffer_sel
		cld
		rep
		  movsd
		pop	ds
		mov	@@buf, edi	; remember next position
		pop	ecx

		add	@@rel, ecx
		sub	@@num, ecx
		jnz	@@read_more

		xor	eax, eax

		jmp	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret

hard_disk_read	endp

;----------------------------------------------------------------------

hard_disk_write	proc	C	near

		arg	@@hd_info: dword, @@rel: dword, @@buf: dword, \
							@@num: dword
		push	ebx
		push	esi
		push	edi
		
@@write_more:
		mov	ecx, @@num
		cmp	ecx, 126  ; max num of sectors that 63k buffer can hold
		jbe	@@num_sect_ok
		mov	ecx, 126
@@num_sect_ok:
		push	ecx		; copy data from _DATA to I/O buffer
		shl	ecx, 7		; 128 dwords per sector
		mov	esi, @@buf	; starting location in _DATA segment
		xor	edi, edi	; beginning of the I/O buffer
		push	es
		mov	es, buffer_sel
		cld
		rep
		  movsd
		pop	es
		mov	@@buf, esi	; remember next position
		pop	ecx		; number of sectors to write

		mov	edi, @@rel
		mov	esi, @@hd_info

		push	ecx
		mov	ah, 3  ; write
		call	disk_operation
		pop	ecx
		jc	@@error

		add	@@rel, ecx
		sub	@@num, ecx
		jnz	@@write_more

		xor	eax, eax

		jmp	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret

hard_disk_write	endp

;----------------------------------------------------------------------

hard_disk_clear	proc	C	near

		arg	@@hd_info: dword, @@rel: dword, @@num: dword
		push	ebx
		push	esi
		push	edi

		mov	ecx, @@num	; first	we have to zero-out the buffer
		cmp	ecx, 126
		jbe	@@under_126
		mov	ecx, 126
@@under_126:
		shl	ecx, 7		; 128 dwords per sector
		xor	edi, edi	; beginning of the I/O buffer
		xor	eax, eax
		push	es
		mov	es, buffer_sel
		cld
		rep
		  stosd
		pop	es
@@clear_more:
		mov	ecx, @@num
		cmp	ecx, 126  ; max num of sectors that 63k buffer can hold
		jbe	@@num_sect_ok
		mov	ecx, 126
@@num_sect_ok:
		mov	edi, @@rel
		mov	esi, @@hd_info

		push	ecx
		mov	ah, 3  ; write
		call	disk_operation
		pop	ecx
		jc	@@error

		add	@@rel, ecx
		sub	@@num, ecx
		jnz	@@clear_more

		xor	eax, eax

		jmp	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret

hard_disk_clear	endp

;----------------------------------------------------------------------

hard_disk_verify	proc	C	near

		arg	@@hd_info: dword, @@rel: dword, @@num: dword
		local	@@num_done

		push	ebx
		push	esi
		push	edi
		
		mov	@@num_done, 0

		mov	ebx, 126	; max number of sectors to verify
@@verify_more:				; (even though the manual says 255
		cmp	ebx, @@num	;  it works only if num_sect <= 127)
		jbe	@@max_sect_ok
		mov	ebx, @@num
@@max_sect_ok:	mov	ecx, ebx
		mov	edi, @@rel
		mov	esi, @@hd_info
		push	ebx
		push	ecx
		mov	ah, 4 ; verify
		call	disk_operation
		pop	ecx
		pop	ebx
		jc	@@error
		
		add	@@num_done, ecx
		add	@@rel, ecx
		sub	@@num, ecx
		jnz	@@verify_more

		jmp	@@end
@@error:			; Lets cut num sectors in half and try again
		cmp	ecx, 1
		je	@@end	; Nothing is left; this is the bad sector
		shr	ecx, 1	 ; ecx = ecx/2;
		sub	ebx, ecx  ; Leave the rest for the next iteration
		jmp	@@verify_more
@@end:
		mov	eax, @@num_done

		pop	edi
		pop	esi
		pop	ebx

		ret

hard_disk_verify	endp

;----------------------------------------------------------------------

get_floppy_info	proc	near
		;
		;  	Arg:  dl - floppy drive number
		;	Ret: ebx - sect/cyl
		;            ecx - sect/track
		;
		mov	ah, 8		; Get floppy disk info
		rm_int	13h		; if there is no floppy and drive type
		movzx	eax, dh		; is unknown all registers are set to 0
		inc	eax		; number of heads
		and	ecx, 3Fh	; ecx - sectors per track
		mul	ecx		; eax - sectors per cylinder
		mov	ebx, eax	; ebx - sectors per cylinder
		cmp	ebx, 1		; if( ebx<1 ) CF=1  /same as if(ebx==0)
		ret
		
get_floppy_info	endp

;----------------------------------------------------------------------

floppy_operation proc	near
		;
		;	ah - operation, al - number of sectors
		;	cl - sect per track, dl - floppy number
		;	ebx - sect per cylinder
		;	edi - relative sector
		;
		push	edx
		mov	dh, 3		; will try at most three times
@@try_again:
		push	eax
		push	ebx
		push	ecx
		push	edx

		push	eax
		push	edx
		xor	edx, edx
		mov	eax, edi	; relative sector
		div	ebx		; eax - cylinder, edx - sect on cyl
		push	eax
		mov	eax, edx
		div	cl		; al - head, ah - sector
		mov	cl, ah
		inc	cl
		mov	dh, al
		pop	eax
		mov	ch, al
		shl	ah, 6
		or	cl, ah
		pop	eax
		mov	dl, al
		pop	eax
		xor	ebx, ebx	; beginning of the buffer 
		rm_int	13h
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		jnc	@@end
		
		push	eax
		mov	ah, 0		; reset floppy disk
		rm_int	13h
		pop	eax
		jc	@@end

		dec	dh
		jnz	@@try_again	
		stc
@@end:
		pop	edx		
		ret
		
floppy_operation	endp

;----------------------------------------------------------------------

floppy_disk_read	proc	C	near

		arg	@@fd_num: dword, @@rel: dword, @@buf: dword, \
						       @@num: dword
		push	ebx
		push	esi
		push	edi
		
		mov	edx, @@fd_num
		call	get_floppy_info
		jc	@@error
@@read_more:				; Reading one sector at a time
		mov	edi, @@rel
		mov	edx, @@fd_num
		mov	ax, 0201h   	; read 1 sector
		call	floppy_operation
		jc	@@error

		push	ds		; copy read data to _DATA segment
		mov	ds, buffer_sel
		mov	ecx, 128	; 128 dwords per sector
		xor	esi, esi	; beginning of the I/O buffer
		mov	edi, @@buf	; destination in _DATA segment
		cld
		rep
		  movsd
		mov	@@buf, edi	; remember next position
		pop	ds

		inc	@@rel
		dec	@@num
		jnz	@@read_more

		xor	eax, eax

		jmp	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret

floppy_disk_read	endp

;----------------------------------------------------------------------

floppy_disk_write	proc	C	near

		arg	@@fd_num: dword, @@rel: dword, @@buf: dword, \
						       @@num: dword
		push	ebx
		push	esi
		push	edi
		
		mov	edx, @@fd_num
		call	get_floppy_info
		jc	@@error
@@write_more:				; Writing one sector at a time
		push	ecx
		push	es		; copy data from _DATA to I/O buffer
		mov	es, buffer_sel
		mov	ecx, 128	; 128 dwords per sector
		mov	esi, @@buf	; starting location in _DATA segment
		xor	edi, edi	; beginning of the I/O buffer
		cld
		rep
		  movsd
		mov	@@buf, esi	; remember next position
		pop	es
		pop	ecx

		mov	edi, @@rel
		mov	edx, @@fd_num
		mov	ax, 0301h   	; write 1 sector
		call	floppy_operation
		jc	@@error

		inc	@@rel
		dec	@@num
		jnz	@@write_more

		xor	eax, eax

		jmp	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret

floppy_disk_write	endp

PushadButEAX	MACRO
				push		ebx
				push		ecx
				push		edx
				push		esi
				push		edi
				ENDM

PopadButEAX		MACRO
				pop			edi
				pop			esi
				pop			edx
				pop			ecx
				pop			ebx
				ENDM

; Saves a 512 byte data block pointed to by @@data to the file named by @@path.
; Uses the DOS file functions for writing to files.
StoreMBR		PROC		C NEAR
				ARG			@@path:dword, @@data:dword
				
				pushfd
				PushadButEAX
				
				; Copy the MBR and the path to the real-mode area
				; buffer_seg:0000h = MBR
				; buffer_seg:0200h = Pathname
				push		es
				mov			esi, @@data
				mov			edi, 0
				push		fs
				pop			es
				mov			ecx, 512 / 4
				cld
				rep			movsd
				mov			esi, @@path
@@NextChar:		lodsb
				stosb
				cmp			al, 0
				jne			@@NextChar
				pop			es
				
				; Create a DOS file
				mov			eax, 3C00h
				mov			ecx, 0
				mov			edx, 512
				; ES is set to buffer_seg by real_int: no need to set it here
				rm_int		21h
				jc			@@Error
				
				; Write the MBR to file
				mov			ebx, eax	; BX = file handle
				mov			ecx, 512	; CX = bytes to read
				mov			edx, 0		; DS:DX = I/O buffer
@@WriteLp:		mov			eax, 4000h
				rm_int		21h
				jc			@@BreakLp
				add			edx, eax
				sub			ecx, eax
				jnz			@@WriteLp
				clc
@@BreakLp:		setc		al		; Save the return code (CF) to the stack
				movzx		eax, al
				push		eax
				
				; Close file
				mov			eax, 3E00h
				rm_int		21h
				pop			ecx
				rcl			ecx, 1
				jecxz		@@OK
				
				
@@Error:		mov			eax, -1
				jmp			@@Common
@@OK:			mov			eax, 0
@@Common:		PopadButEAX
				popfd
				ret
StoreMBR		ENDP


; Loads a 512 byte data block pointed to by @@data with the first 512 bytes
; from the file named by @@path.
; Uses the DOS file functions for writing to files.
LoadMBR			PROC		C NEAR
				ARG			@@path:dword, @@data:dword
				
				pushfd
				PushadButEAX
				
				; Copy the path to the buffer
				mov			esi, @@path
				mov			edi, 0
				push		es
				push		fs
				pop			es
				cld
@@NextChar:		lodsb
				stosb
				test		al, 0FFh
				jnz			@@NextChar
				pop			es
				
				; Open a DOS file
				mov			eax, 3D00h
				mov			edx, 0
				; ES is set to buffer_seg by real_int: no need to set it here
				rm_int		21h
				jc			@@Error
				
				; Load the MBR from file to buffer_seg:0000
				mov			ebx, eax
				mov			ecx, 512
				mov			edx, 0
@@ReadLp:		mov			eax, 3F00h
				rm_int		21h
				jc			@@BreakLp
				add			edx, eax
				sub			ecx, eax
				jnz			@@ReadLp
				clc
@@BreakLp:		setnc		al		; Save the return code (~CF) to the stack
				movzx		eax, al
				push		eax
				
				; Close file
				mov			eax, 3E00h
				rm_int		21h
				; NOTE: it is not an error if we can't close the file; even
				; if it is a waste of resources, no harm can be done by having
				; a read-only unused file
				
				; Now that the file is closed, let's see if there was an error
				pop			ecx
				jecxz		@@Error
				
				; Now move the loaded data to the real buffer
				push		ds
				push		fs
				pop			ds
				mov			esi, 0
				mov			edi, @@data
				mov			ecx, 512 / 4
				rep			movsd
				pop			ds
				
@@OK:			mov			eax, 0
				jmp			@@Common
@@Error:		mov			eax, -1
@@Common:		PopadButEAX
				popfd
				ret
LoadMBR			ENDP


CMOS_ADDR_PORT	EQU			70h
CMOS_DATA_PORT	EQU			71h
CMOS_DATA_DELAY	EQU			10000h
CMOS_XFER_BYTES	EQU			128
NMI_DISABLE_MSK	EQU			80h

ReadCMOSData	PROC		C NEAR
				ARG			@@path:dword
				
				; Save all GP registers & flags, but not EAX
				pushfd
				PushadButEAX
				cli
				cld
				
				; ES:EDI points to a buffer where CMOS data are stored
				mov			edi, @@path
				push		es
				push		ds
				pop			es
				
				; Wait for the RTC to be in non-update state
@@WaitForRTC:	mov			al, 0Ah + NMI_DISABLE_MSK
				out			CMOS_ADDR_PORT, al
				mov			ecx, CMOS_DATA_DELAY
@@Here1:		loop		@@Here1
				in			al, CMOS_DATA_PORT
				test		al, 80h
				jnz			@@WaitForRTC
				
				
				mov			eax, NMI_DISABLE_MSK
@@CMOSByte:		out			CMOS_ADDR_PORT, al
				push		eax
				mov			ecx, CMOS_DATA_DELAY
@@Here:			loop		@@Here
				in			al, CMOS_DATA_PORT
				stosb
				pop			eax
				inc			eax
				cmp			eax, CMOS_XFER_BYTES + NMI_DISABLE_MSK
				jne			@@CMOSByte
				
				; Enable NMI
				xor			al, al
				out			CMOS_ADDR_PORT, al
				
				pop			es
				PopadButEAX
				popfd
				ret
ReadCMOSData	ENDP

WriteCMOSData	PROC		C NEAR
				ARG			@@path:dword
				ARG			@@size:dword
				
				pushfd
				PushadButEAX
				
				cli
				cld
				mov			esi, @@path
				mov			eax, NMI_DISABLE_MSK
				
@@CMOSByte:		out			CMOS_ADDR_PORT, al
				push		eax
				mov			ecx, CMOS_DATA_DELAY
@@Here:			loop		@@Here
				lodsb
				out			CMOS_DATA_PORT, al
				pop			eax
				inc			eax
				cmp			eax, CMOS_XFER_BYTES + NMI_DISABLE_MSK
				jne			@@CMOSByte
				
				PopadButEAX
				popfd
				ret
WriteCMOSData	ENDP
;----------------------------------------------------------------------
_TEXT 		ends
;----------------------------------------------------------------------
