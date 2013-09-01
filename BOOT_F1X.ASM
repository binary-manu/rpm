PAGE 255,132
.MODEL SMALL
;.STACK  400h

public BOOT_FAT_1X
public BOOT_FAT_1X_NAMES

.DATA
LOCALS
ASSUME	CS:_DATA, DS:_DATA
.386

;------------------------------------------------------------------------------
; SECTOR_SIZE = 512 BYTES IS ASSUMED!!!
;------------------------------------------------------------------------------

ERR_MSG		EQU	(offset ErrMsg  - offset boot_sector + 7C00h)

boot_sector	label	byte
BOOT_FAT_1X	PROC	NEAR
		;
		; IPL loads this code to 0000:7C00h
		;
		jmp	short @@code_3Eh
		nop

;FAT1X_Reserved	DB	3Bh Dup(?)

		DB	"FAT_BOOT"
		DW	512 ; Sector size in bytes
		DB	?  ; Cluster size in sectors
		DW	?  ; Reserved sectors at the beginning of partition
		DB	?  ; Number of copies of FAT table
		DW	?  ; Number of entries in Root directory
		DW	?  ; Number of sectors in partition (Short if <= 64k)
		DB	?  ; Media descriptor byte (HD:F8, Floppy:F0,F9,FC-FF)
		DW	?  ; Number of sectors in each copy of FAT
		DW	?  ; Number of sectors per track
		DW	?  ; Number of tracks per cylinder (=heads, =sides)
		DD	?  ; Starting sector of the partition
		DD	?  ; Number of sectors in partition (Long if > 64k)
		DW	?  ; Disk number (Floppies:0,1,... HDs: 80h,81h,... )
		DB	29h; Extended boot record signature
		DD	12345678	; Volume serial number
		DB	"NO NAME    "	; Volume label (11 bytes, space padded)
		DB	"FAT12   "	; File system ID "FAT12" or "FAT16"
		
@@code_3Eh:	xor	ax, ax
		mov	ds, ax
		mov	ss, ax
		mov	sp, 7C00h
		mov	bp, sp
		cld
		sti
		mov	[bp+24h], dl	; save dl = disk number

		mov	al, [bp+10h]	; now ax = number of FAT copies (ah=0)
		mul	word ptr[bp+16h];*fat_size -> now dx:ax - size of FATs
		add	ax, [bp+0Eh]	; reserved sectors at the beginning of
		adc	dl, dh;=0	; the partition
		add	ax, [bp+1Ch]	; starting sector of the partition
		adc	dx, [bp+1Eh]	; now dx:ax - first sector of root

		;Read root directory and look for the names of boot files in it
		push	54h
		pop	es
		push	ds;=0	; di will be popped at the end from this place
		mov	bx, 053Ah
		mov	si, 7E00h	; last file name+10h
		mov	cx, [bp+11h]	; number of entries in root directory
@@next_root_sector:			; we will read one sector at a time
		push	es		; destination address es:bx = 050h:000h
		call	read_sector	; will advance sector number in dx:ax
		pop	es
		mov	di, 200h	; sector size
@@next_root_entry:
		sub	di, 20h		; previous root directory entry
		jc	@@next_root_sector
@@push_si:
		push	si
@@prev_name:
		lea	si, [si-10h] ; previous file name
		cmp	byte ptr [si+0Fh], bh
		jb	@@root_entry_done
		pusha
		ja	@@dont_check_keys
		;mov	al, [417h]	; get shift keys status
		db	0A0h, 17h, 04h
		and	al, 0Fh	; leave only "Ctrl, Alt, Shift"
		cmp	al, [si+0Eh]
		jnz	@@dont_check_name ; -> jnz  @@prev_name
@@dont_check_keys:
		mov	cx, 0Bh
		repe	cmpsb
@@dont_check_name:
		popa
		jnz	@@prev_name	; name not matched
		push	word ptr [bx-53Ah+540h+di+1Ah] ; first cluster of file
		cmp	[si+0Ch],bx;=53Ah non-boot file (MSDOS.SYS, IBMDOS.COM)
		jne	@@push_si ; boot file (WINBOOT.SYS, IO.SYS, IBMBIO.COM)
		pop	word ptr [bx-53Ah+53Ah]; non-boot file starting cluster
		jmp	short @@prev_name
@@root_entry_done:
		pop	si
		loop	@@next_root_entry
		pop	di	; starting cluster or 0 if no name has matched

		mov	sp, bp

		; di - first cluster of the file that we have to read
		; si - pointer to name, #sectors, and destination segment

		mov	[bx-53Ah+51Ah], di ; MS-DOS expects to find starting
	    	 ; clusters of IO.SYS at 000h:051Ah and MSDOS.SYS at 0:053Ah

		push	dx	; [bp-2] starting sector of cluster 2 on disk
		push	ax	; [bp-4] ( dx:ax = [bp-2]:[bp-4] )

		les	bx, [si+0Ah] ; now es - destination segment (cx is 0)
		xchg	bh, cl	; now cx - number of sectors to read, bh = 0

		push	es	; destination segment
		push	di	; first cluster of boot file

@@calc_start_sector:
		mov	bl, [bp+0Dh]	; cluster size
		lea	ax, [di-2]
		mul	bx
		add	ax, [bp-4]	; starting sector of cluster 2 on disk
		adc	dx, [bp-2]

		or	di, di		; is cluster number valid ? (not 0)
		jz	print_error	; if not then there is nothing to boot
@@read_sector:
		call	read_sector
		dec	bx		; bx - sectors left in cluster
		loopnz	@@read_sector	; cx - sectors left in file
		jcxz	@@read_done	; cx=0 no nore sectors in file
		
		; Need to set (di) to the next cluster in the file
		push	cx

		mov	ax, di
		mov	al, ah
		and	ax, 00F0h
		cmp	ax, si
		je	@@fat_in_memory
		mov	si, ax
		push	es
		push	7E0h	; read 16 sectors of FAT to 7E0h:0h = 0:7E00h
		pop	es
		cwd		; now dx=0
		add	ax, [bp+0Eh]	; reserved sectors at the beginning of
		adc	dx, dx		; the  partition
		add	ax, [bp+1Ch]	; starting sector of the partition
		adc	dx, [bp+1Eh]	; now dx:ax - first + si sector of FAT
		mov	cx, 16
@@read_fat:	call	read_sector
		loop	@@read_fat
		pop	es
@@fat_in_memory:
		mov	cx, 0FFFh
		and	di, cx
		mov	ax, di
		add	di, ax
		add	di, ax
		
		cmp	byte ptr [bp+3Ah], '6'	; is it FAT-16 ?
		jne	@@fat12	
@@fat16:
		add	di, ax
		mov	ch, cl
@@fat12:
		shr	di, 1
		mov	di, [di+7E00h]
		jnc	@@no_shift
		shr	di, 4
@@no_shift:
		and	di, cx	; now di is next cluster number
		cmp	di, cx
		pop	cx
		jne	@@calc_start_sector
@@read_done:
		pop	di	; first cluster of boot file
		pop	es	; destination segment
		mov	dl, [bp+24h] ; disk number
		mov	ch, [bp+15h] ; DOS expects media descriptor byte in ch
		lea	si, [bp+0Bh] ; NTLDR expects DS:SI to point to [bp+0Bh]
		xor	bx, bx
		cmp	word ptr es:[bx], 5A4Dh ; 'MZ' Win95/98, part.exe, ...
		je	@@win95
		pop	bx		; MS-DOS, PC-DOS, DR-DOS, NTLDR, ...
		pop	ax		; DOS expects ax:bx=start of cluster 2
		push	es
		push	ss;=0
		retf			; transfer control to the loaded file
;------------------------------------------------------------------------------
disk_reset:
		pop	dx	; dl=disk number
		mov	ah, 0
		int	13h
		cmp	sp, si	; is there anything left on stack
		jne	disk_get_info	; still have more retries
print_error:
		mov	si, ERR_MSG
print_next_char:
		lodsb
halt:		test	al, al
		jz	halt
		mov	ah, 0Eh
		;xor	bh, bh	; display page in text mode (assumed that bh=0)
		mov	bl, 07h	; in case of the graphic mode set color to grey
		int	10h
		jmp	short print_next_char
@@win95:
		push	dword ptr [bx+78h] ; address of int 1Eh (FD params)
		push	bp	 ; Should be pointer to int 1Eh = 000h:078h
		push	ss	 ; but Win95/98 seems to ignore these values
		push	es
		push	200h
		retf			; transfer control to the loaded file

read_sector	proc	near
		;
		;  dx:ax - sector on disk   (on return dx:ax will be advanced)
		;  es:00 - destination address (on return es will be advanced)
		;
		;  byte ptr ss:[bp+24h] - disk number
		;  assuming ss=ds=0
		;
@@read:		
		pusha
		push	es

		push	ss;=0	; rel_sect7,6 ; Prepare parameters for LBA read
		push	ss;=0	; rel_sect5,4
		push	dx	; rel_sect3,2
		push	ax	; rel_sect1,0
		push	es	;  segment
		push	ss;=0	;  address
		push	01h	; num_sect
		push	10h	; pkt_size
		mov	si, sp	; pkt_addr

		mov	dl, [bp+24h]	; disk number
		push	dx	; giving chance for up to 2 retries
		push	dx
		push	dx
disk_get_info:
		mov	ah, 8		; Get disk parameters
		int	13h
		les	bx, [si+04h]	; destination address
@@jc_reset:	jc	disk_reset

		and	cx, 3Fh	; Now cx = number of sectors per track
		mov	[bp+18h], cx

		movzx	ax, dh	; Now al = number of heads - 1
		inc	ax	; Now al = number of heads
		mov	[bp+1Ah], ax
		mul	cx	; Now ax = number of sectors per cylinder
		xchg 	ax, di	; Now di = number of sectors per cylinder
		mov	ax, [si+08h]	; rel_sect low
		mov	dx, [si+0Ah]	; rel_sect high
		jz	@@lba	; one of CHS dimensions is zero - must use LBA
		cmp	dx, di
		jae	@@lba	; if cylinder >= 65536 must use LBA
		div	di	; Now ax = cylinder, dx = sector on cylinder
		xchg	ax, dx	; Now ax = sector on cylinder, dx = cylinder
		div	cl	; Now al = head, ah = sector on head - 1 
		mov	cl, 2
		xchg	ch, dl	; Now dh = cyl:15-8,  dl = 0, ch = cyl:7-0
		shr	dx, cl	; Now dh = cyl:15-10, dl = cyl:9,8 in bits 7,6
		xchg	ah, cl	; Now ah = 2 (CHS read), cl = sector on head-1 
		inc	cx	; Now ah = 2 (CHS read), cl = sector on head
		or	cl, dl	; Now cl = cyl:9,8(bits 7,6) and sector in 5-0
		xchg	dh, al	; Now dh = head, al = cyl:15-10 (0 in bits 7,6)
		or	al, al	; Is cylinder >= 1024
		jz	@@int13	; No, it is less than 1024 - we may use CHS
	@@lba:
		mov	ah, 42h
		push	ss
		pop	es
		mov	byte ptr [bp+2], 0Eh	; Force Win95/98 to use LBA
	@@int13:
		inc	ax  ; for CHS turn al=0 into 1 - read one sector

		mov	dl, [bp+24h] ; dl = disk number
		int	13h
		jc	@@jc_reset
		lea	sp, [si+10h]
		add	word ptr [si+10h], 20h ; advance es for the next sector
		pop	es
		popa
		inc	ax
		jnc	@@no_carry	; WIN_MSG depends on this jump's offset
		inc	dx
@@no_carry:
		ret

read_sector	endp
;------------------------------------------------------------------------------
WIN_MSG	EQU	ERR_MSG-3 ;Needed by Win95 bootstrap code to copy err msgs.
ErrMsg	DB	"Error!",0
;------------------------------------------------------------------------------
GAP	PROC
GAPLEN	EQU	(1D0h-(GAP-boot_sector))
IF GAPLEN GT 0
	DB	GAPLEN DUP(0)	
ENDIF
GAP	ENDP
;------------------------------------------------------------------------------
FirstNm: ;	"FileNameExt", #sectors, destination segment

; +0F	0..4 - no record, 5 - check keys, >= 5 - don't check keys
; +0E	key mask: (will check for file name only if mask matches)
;	0 - no shift keys pressed
;       mask bits: 1 - Right Shift pressed
;                  2 - Left Shift pressed
;                  4 - Ctrl pressed
;                  8 - Alt pressed

BOOT_FAT_1X_NAMES:

	DB	"WINBOOT SYS",     4,     70h, 00h, 0, 6
	DB	"MSDOS   SYS",     4,     3Ah, 05h, WIN_MSG-7D00h, 7Dh
	DB	"IO      SYS",     4,     70h, 00h, 055h, 0AAh

	DB	"??????  ???",     4,     70h, 00h, 0, 0
	DB	"IBMDOS  COM",    58,     3Ah, 05h, WIN_MSG-7D00h, 7Dh
	DB	"IBMBIO  COM",    58,     70h, 00h, 055h, 0AAh

	DB	"SETUPLDRBIN",     0,     00h, 20h, 4, 5
	DB	"$LDR$      ",     0,     00h, 20h, 4, 5
	DB	"NTLDR      ",     0,     00h, 20h, 055h, 0AAh

	DB	"PART    EXE",     0,     00h, 10h, 0, 5
	DB	"MSDOS   SYS",     4,     3Ah, 05h, WIN_MSG-7D00h, 7Dh
	DB	"IO      SYS",     4,     70h, 00h, 055h, 0AAh

	DB	"PART    EXE",     0,     00h, 10h, 0, 5
	DB	"IBMDOS  COM",    58,     3Ah, 05h, WIN_MSG-7D00h, 7Dh
	DB	"IBMBIO  COM",    58,     70h, 00h, 055h, 0AAh

	DB	"PART    EXE",     0,     00h, 10h, 0, 5
	DB	"SETUPLDRBIN",     0,     00h, 20h, 0, 6
	DB	"NTLDR      ",     0,     00h, 20h, 055h, 0AAh

;	- Last record has lower priority
;	- If destination segment is 0 then starting cluster will be stored
;	  to 0000h:053Ah but file will not be used for booting.
;	  (ex: MSDOS.SYS accompanies IO.SYS)
;	- Files could be loaded to dest seg = 0050h ... 07B0h, 09E0h ... 9FFFh
;	- If number of sectors is 0 it means "load entire file."
;	- Memory overruns are not checked. Make sure file is short enough to
;	  fit in memory or specify number of sectors correctly.


BOOT_FAT_1X	ENDP

;------------------------------------------------------------------------------

		END ;install

