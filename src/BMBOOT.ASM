;
; boot.asm
;
PAGE 255,132
.MODEL SMALL
.DATA
LOCALS
PUBLIC	BM_BOOT
ASSUME	CS:_DATA, DS:_DATA
.386

BM_BOOT		PROC	NEAR
		;
		;  MBR loads BM_BOOT at 0000:7C00h
		;
		jmp	short @@xcode
		nop
		;
FS_MAGIC	DB     "?????64"; 7 characters - identifies this structure
SECT_SIZE	DW	512	; Works with 512, 1024, 2048, and 4096
FS_OPTIONS	DD	?	; File System features (bitmap)

PART_REL_SECT	DQ	?	; Number of sectors prior to partition
PART_NUM_SECT	DQ	?	; Number of sectors in the partition

FAT1_REL_SECT	DQ	?	; Offset of the first copy of FAT in partition
FAT2_REL_SECT	DQ	?	; Offset of the second copy of FAT in partition

LOADER_REL_SECT	DQ	?	; Loader relative sector
LOADER_NUM_SECT	DD	?	; Loader size in sectors
LOADER_CHK_SUM	DD	?

RESERVED	DB	64 Dup(0)

		;
@@xcode:
		xor	ax, ax
		mov	ds, ax
		mov	es, ax
		mov	ss, ax
		mov	sp, 7C00h
		;
		mov	bp, sp

		push	es
		push	dx
		
		mov	eax, [bp+16]	; part_rel_sect
		add	eax, [bp+48]	; loader_rel_sect
		push	eax
		mov	eax, [bp+56]	; loader_num_sect
		push	eax
		push	1000h		; segment destination address 64k
		call	READ_N_SECT
		jc	@@error
		
		add	sp, 10
		pop	dx
		pop	es

		;jmp	1000h:0000h
		 DB	0EAh
		 DW	0000, 1000h

		;
		; Print error, wait for key, reboot.
		;
@@error:
		lea	si, [bp+(ERRMSG-BM_BOOT)]
		call	PRINT
		xor	ax, ax
		int	16h
		mov	ax, 0E0Dh
		int	10h
		mov	al, 0Ah
		int	10h
		int	19h
		;
PRINT		PROC	NEAR

		mov	ah, 0Eh
		mov	bx, 0007h
@@pr1:
		lodsb
		or	al, al
		jz	@@pr2
		int	10h
		jmp	@@pr1
@@pr2:
		ret
		
PRINT		ENDP
		;
		;
READ_N_SECT	PROC	NEAR
		;
		;	Arguments: 
		;	  dl - disk number
		;	 [sp+8] - rel_sect
		;	 [sp+4] - num_sect
		;	 [sp+2] - segment
		;
		;	Returns: Flag CF set if error
		;
		pusha
		push	dx
		mov	ah, 8		; Get disk parameters
		push	es
		int	13h
		pop	es
		jnc	@@go_ahead
		pop	dx
		popa
		ret
@@go_ahead:
		and	cx, 3Fh
		mov	di, cx		; di = sect / track
		mov	ah, 0
		mov	al, dh
		inc	ax
		mul	cx
		mov	si, ax		; si = sect / cylinder

		pop	dx
		pusha
		mov	bp, sp		; save stack
		jmp	@@read_sector

REL_SECT_H	EQU	 word ptr [bp+42]
REL_SECT_L	EQU	 word ptr [bp+40]
REL_SECT	EQU	dword ptr [bp+40]
NUM_SECT	EQU	dword ptr [bp+36]
DEST_SEG	EQU	 word ptr [bp+34]
DISK		EQU	 byte ptr [bp+26]

@@next_sector:
		add	DEST_SEG, 512/16
		inc	REL_SECT
		dec	NUM_SECT
		jz	@@end
@@read_sector:
		mov	cl, 3		; We will try at most three times
@@next_try:
		mov	es, DEST_SEG

		mov	ah, 41h		; Is there LBA interface
		mov	bx, 055AAh
		int	13h
	;jmp	@@chs
		jc	@@chs
		cmp	bx, 0AA55h
		jne	@@chs
		test	cl, 1		; LBA read/write/... supported
		jnz	@@lba		; everything is ok - using LBA mode
@@chs:
		mov	ax, REL_SECT_L
		mov	dx, REL_SECT_H

		div	si		; ax=cylinder, dx=sector on cylinder
		mov	ch, al
		mov	cl, ah
		shl	cl, 6
		mov	ax, dx
		xor	dx, dx
		div	di		; ax=head, dx=sector on head
		inc	dx
		mov	dh, al
		or	cl, dl
		mov	ax, 0201h	; Read (AH=02) 1 Sector (AL=01)
		mov	dl, DISK	; Hard disk number
		xor	bx, bx
		jmp	short @@int13h
@@lba:
		push	ds	; 0	; Read using LBA mode
		push	ds	; 0
		push	REL_SECT	; rel_sect
		push	es		;  segment
		push	ds	; 0	;  address
		push	01h		; num_sect
		push	10h		; pkt_size
		mov	si, sp		; pkt_addr
		push	ds
		pop	es
		mov	ah, 42h
@@int13h:
		int	13h
		mov	sp, bp		; restore stack
		popa
		pusha
		mov	bp, sp
		jnc	@@next_sector	; no errors
@@reset:
		mov	ah, 0		; reset disk
		int	13h
		loop	@@next_try
		stc			; if we failed 3 times we give up
@@end:
		popa
		popa

		ret

READ_N_SECT	ENDP
		;
		;
		;
ERRMSG:		DB 0Dh,0Ah,"Error reading loader."
		DB 0Dh,0Ah,"Press a key to reboot ... "
		;

GAP		DB (1BEh-($-BM_BOOT)) Dup(0)
		DB 64 Dup(0)
		DW 0AA55h

COMMENT %
test:		mov	ax, cs
		mov	ds, ax
		mov	es, ax
		mov	ss, ax
		mov	sp, 1024
		add	ax, 1024/16
		push	0
		push	64
		push	0
		push	127
		push	ax
		mov	dl, 81h
		call	READ_N_SECT
		
		mov	ax, 4C00h
		int	21h
%
BM_BOOT		ENDP

		END	;test

