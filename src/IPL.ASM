;
; ipl.asm - code for the Initial Program Loader that resides in MBR
;
PAGE 255,132
.MODEL SMALL
.DATA
LOCALS
ASSUME	CS:_DATA, DS:_DATA
PUBLIC	BM_IPL, STD_IPL, EMP_IPL
.386

VIRUS		EQU	(M1-start-100h)
ERROR		EQU	(M2-start-100h)
NL_BOOT		EQU	(M3-start-100h)
BOOTING		EQU	(M4-start-100h)

BM_CHS		EQU	79Eh
OPTIONS		EQU	byte ptr [di-FD_CHS+7A2h]
TIMEOUT		EQU	byte ptr [di-FD_CHS+7A3h]
DEFAULT		EQU	byte ptr [di-FD_CHS+7A4h]
FD_CHS		EQU	7AEh
NL_MSG		EQU	0B2h

HD_NUM		EQU	byte ptr [(di-FD_CHS)+(M5-start+600h)]

BM_NUM_SECT	EQU	66	; From hard drive always read 66 sectors

		;
		;  Known problems:
		;
		;     1. IPL will always use LBA mode iff Cyl=1023 and Sect>=1
		;

BM_IPL		PROC	NEAR
		;
		;  BIOS loads MBR at 0000h:7C00h
		;
		;  Hard disk number passed in DL
		;
start:
		xor	bp, bp
		mov	ds, bp
		mov	es, bp
		mov	ss, bp		; CPU disables ints for the next op
		mov	sp, 7C00h
		mov	si, sp
		push	si		; Return address to jump to boot sector
		mov	di, 0600h	; First we move the code to 0000h:0600h
		mov	cx, 0100h
		cld
		rep	movsw
		;jmp to the next instruction in the ipl's new code location
		 DB	0E9h
		 DW	-(7C00h-0600h)
		;
		;  Set DI to beginning of partition table minus 16 bytes
		;
		mov	di, FD_CHS	; From now on DI = FD_CHS
		;
		;  Check hard disk number (some BIOSes pass 0 in DL)
		;
		cmp	dl, 80h
		jae	@@hd_ok
		mov	dl, 80h
@@hd_ok:
		mov	al, dl		; We want to display it
		sub	al, 80h-'1'
		mov	HD_NUM, al
		;
		;
		mov	bl, BOOTING
		test	OPTIONS, 1
		jz	@@no_virus	; Don't check for viruses
		;
		;  Check interrupt vectors (for hooked viruses)
		;
		mov	si, 04*1Ch+03	; Check interrupts 0 to 1Ch
		mov	al, 0C0h	; they must be >= C000:0000
		cmp	al, [si+0B4h]	; int 4Ah - User Alarm
		ja	@@int_changed
		cmp	al, [si+14Ch]	; int 70h - Real-Time Clock
		ja	@@int_changed
@@next_int:
		sub	si, 4
		jc	@@no_virus	; all checked - no virus found
		cmp	al, [si]
		jbe	@@next_int
@@int_changed:
		mov	bl, VIRUS
		call	print
@@get_a_key:
		mov	ah, 0		;  Get a key
		int	16h
		cmp	al, 0Dh		;  Is it Enter?
		jne	@@get_a_key
		mov	bl, NL_BOOT
@@no_virus:
		;
		;  If boot manager is configured - load it
		;
		mov	si, BM_CHS	; From now on SI -> active partition
		cmp	[si], ch;0
		jne	@@found		; Boot Manager active - load it

		call	print		; else print "Booting..." message

		push	di
		add	al, DEFAULT	; al=0 after call to print
		jnz	@@test_part	; DEFAULT partition is set
		pop	di

		;
		;  Find active partition
		;
		mov	al, '0'
@@next_part:
		add	si, 10h		; Advance to next record
		cmp	[si], ch;0	; If boot flag is set 
		jne	@@found		; Then this is active partition
		inc	ax
		cmp	al, '4'
		jbe	@@next_part
		;
		jmp	short @@read_key; No active partition found - ask user
@@m0:
		xor	bx, bx		; This will indicate first loop
		cmp	al, ' '		; If user hit Space print prompt & wait
		jne	@@process_key	; If not the Space - check other keys
		;
		; Select new partition
		;
@@m1:
		mov	bl, BOOTING
@@m2:
		call	print		; Print prompt
@@read_key:
		mov	ah, 0
		int	16h		; Read character
@@process_key:
		push	di
		cmp	al, 09		; Tab - Next hard drive
		je	@@boot_hd

		or	al, 20h		; Capital letter 'A' will become 'a'
		cmp	al, 'a'		; Floppy disk
		je	@@boot_fd

@@test_part:	sub	al, '0'
		jz	@@wrong_key

		mov	cl, 05		; Clear 4 partition records
		cmp	al, cl		; One of the partitions ('1'-'4')?
		jb	@@clear

@@wrong_key:	pop	di
		dec	bx		; if bx==0 we are only checking the key
		jns	@@read_key	;    bx!=0 -> wrong key, lets ask again
		jmp	@@save		;    bx==0 -> wrong key, but not for us
@@boot_fd:
		pop	 si
		mov	[si], ch;0	; To boot floppy disk
		jmp	short @@print_char
		;
		;  Clear all boot flags and point SI to the selected partitions
		;
@@clear_next:	add	si, 10h
		push	si
		dec	al
		jz	@@clear_skip	; AL will be 0 for selected partition
@@clear:	pop	 si		; then its address will remain on stack
		mov	[si], ch;0	; Clear boot flag for current partition
@@clear_skip:	loop	@@clear_next
		add	al, '0'+ 4	; Restore partition number
@@boot_hd:
		pop	 si
		cmp	[si], ch;0	; Is boot flag already set?
		jne	@@no_inc
		inc	bp		; If no then we have to save changes
@@no_inc:
@@found:	mov	[si], dl	; Set boot flag on the selected part.
		cmp	si, di
		jb	@@load_sect	; In case of boot manager no printing
		ja	@@print_char
		inc	byte ptr [si]	; Next hard disk number
		mov	al, 26
@@print_char:
		dec	bx		; if bx=0 we were only checking the key
		js	@@m1		; in this case come back and read it in

		mov	ah, 0Eh		; Print what we are booting (A,1-4,->)
		mov	bh, 00h
		int	10h		; Print a character

		;
		;  Load boot sector from disk
		;
@@load_sect:
		pop	bx	 	; Destination Address = 7C00h
		push	bx

		mov	cl, 3		; We will try at most three times
@@read_again:
		pusha
		mov	bp, sp

		mov	ax,  0201h	; Read using CHS mode
		mov	cx, [si+02]	; Cyl#, Sect#
		mov	dx, [si+00]	; Head, Drive in DL

		cmp	cl, al		; if partition is empty (sect=0)
		jb	@@skip_int13

		cmp	dl, al		; Are we on hard drive
		jb	@@call_int13	; No - this is a floppy
		mov	al, BM_NUM_SECT	; From hard disk read BM_NUM_SECT

		cmp	cx, 0FFC1h	; Cyl=1023, Sect=1..63
		jb	@@call_int13

		push	ds	; 0	; Read using LBA mode
		push	ds	; 0
		push	dword ptr[si+8]	; rel_sect
		push	ds		;  segment
		push	bx		;  address
		push	BM_NUM_SECT	; num_sect
		push	10h		; pkt_size
		mov	ah, 42h
		mov	si, sp		; pkt_addr
@@call_int13:
		int	13h
@@skip_int13:
		mov	sp, bp
		jnc	@@read_ok
@@disk_reset:
		mov	ah, 0		; We get here if there was an error
		int	13h		; We will try to reset device
		popa

		loop	@@read_again
		mov	bl, ERROR	; We tried three times, so we give up
		jmp	@@m2
@@read_ok:
		popa

		cmp	si, di		; if we are loading boot manager
		jb	@@boot		; boot immediately

		;
		;  Print dots and check if key is pressed
		;
		mov	ax, 092Eh	; Print 30 dots
		mov	bx, 0007h
		mov	cl, 30
		int	10h
@@check_key:
		mov	ah, 01h		; Check if key is pressed
		int	16h
		jz	@@print_sp	; no key

		cmp	al, 1Bh		; There is a key - is it ESC?
		jne	@@end

		mov	ah, 00		; Remove ESC from keyboard buffer
		int	16h
		jmp	short @@end
@@print_sp:
		mov	ax, 0E20h
		int	10h		; Print a SPACE
		pusha
		mov	ah, 86h		; Wait for a while
		mov	cl, TIMEOUT
		shr	cx, 1
		sar	dh, 1
		int	15h
		popa

		loop	@@check_key

		mov	al, 1Bh		; tells that time run out
@@end:
		push	ax
		mov	ax, 0920h	; Print rest of SPACEs
		int	10h
		mov	bl, NL_MSG	; Print new line
		call	print
		;mov	ax, 0E0Dh	; Print carrige return
		;int	10h
		;mov	al, 0Ah		; Print line feed
		;test	OPTIONS, 02	; If messages are not suppressed
		;jz	@@print_lf
		;mov	ax, 0920h	; Remove the message instead
		;mov	cl, 13
;@@print_lf:
		;int	10h
		pop	ax

		cmp	al, 1Bh		; User hit ESC or time run out
		;je	@@save
		jne	@@m0

		;
		; Save changes to disk
		;
@@save:
		dec	bp		; Do we have to save (if bp<=0 don't)
		js	@@boot
		mov	ax, 0301h	; Write (AH=03) 1 Sector (AL=01)
		mov	bx, 0600h	; Source located at address 600h
		mov	cl, 01h		; Cyl = 0, Sect = 1
		mov	dh, 00h		; Head = 0, Drive in DL
		int	13h
@@boot:
		mov	dl, [si]	; Boot sector expects Drive# in DL
		;jmp	7C00h		; Transfer control to the boot sector
		ret			; We've pushed 7C00h at the beginning
		;
		;  Print
		;
@@next_char:
		mov	bh, 00h
		int	10h
		inc	bx
print:
		mov	ax, 0E00h
		mov	bh, 07h
		add	al, [bx]
		jnz	@@next_char
		ret
		;
		;
		;
M1:		DB "Virus!",0
M2:		DB " Error!"
M3:		DB 13, 10
M4:		DB "Booting HD"
M5:		DB "1/",0

GAP:
GAPLEN		EQU	(19Eh-(GAP-start))
IF GAPLEN
	DB	GAPLEN DUP(0)
ENDIF

;BM_CHS  DD	0		; 79Eh
;OPTIONS DB	0		; 7A2h
;TIMEOUT DB	0		; 7A3h
;DEFAULT DB	0		; 7A4h
;SELECTD DB	0		; 7A5h
;BM_REL  DQ	0		; 7A6h
;FD_CHS  DD	0		; 7AEh
;NL_MSG  DD	0		; 7B2h
;VT_REL  DQ	0		; 7B6h

;	DB	40h DUP(0)
;	DB	055h, 0AAh

	DB	0Ah DUP(0)	; Align to multiple of 16 bytes

BM_IPL		ENDP



STD_IPL		PROC	NEAR

		;
		;   IPL that comes with MS-DOS's fdisk.exe
		;

		CLI
		XOR	AX,AX
		MOV	SS,AX
		MOV	SP,7C00h
		MOV	SI,SP
		PUSH	AX
		POP	ES
		PUSH	AX
		POP	DS
		STI
		CLD
		MOV	DI,0600h
		MOV	CX,0100h
		REPNZ
		MOVSW
	      ;	JMP	0000:061D
		DB	0EAh
		DW	061Dh, 0000h
@@_061D:
		MOV	SI,07BEh
		MOV	BL,04h
@@_0622:
		CMP	BYTE PTR [SI],80h
		JZ	@@_0635
		CMP	BYTE PTR [SI],00h
		JNZ	@@_0648
		ADD	SI,10h
		DEC	BL
		JNZ	@@_0622
		INT	18h
@@_0635:
		MOV	DX,[SI]
		MOV	CX,[SI+02]
		MOV	BP,SI
@@_063C:
		ADD	SI,10h
		DEC	BL
		JZ	@@_065D
		CMP	BYTE PTR [SI],00
		JZ	@@_063C
@@_0648:
		MOV	SI,068Bh
@@_064B:
		LODSB
		CMP	AL,00
		JZ	@@_065B
		PUSH	SI
		MOV	BX,0007
		MOV	AH,0Eh
		INT	10h
		POP	SI
		JMP	@@_064B
@@_065B:
		JMP	@@_065B
@@_065D:
		MOV	DI,0005h
@@_0660:
		MOV	BX,7C00h
		MOV	AX,0201h
		PUSH	DI
		INT	13h
		POP	DI
		JNB	@@_0678
		XOR	AX,AX
		INT	13h
		DEC	DI
		JNZ	@@_0660
		MOV	SI,06A3h
		JMP	@@_064B
@@_0678:
		MOV	SI,06C2h
		MOV	DI,7DFEh
		CMP	WORD PTR [DI],0AA55h
		JNZ	@@_064B
		MOV	SI,BP
	      ;	JMP	0000:7C00
		 DB	0EAh
		 DW	7C00h, 0000h

@@_068B:	DB	"Invalid partition table",0
@@_06A3:	DB	"Error loading operating system",0
@@_06C2:	DB	"Missing operating system",0

              ;	DB	291 DUP(0)	; Align to 510 bytes
	      ;	DB	055h, 0AAh

		DB	  5 DUP(0)	; Align to multiple of 16 bytes

STD_IPL		ENDP


EMP_IPL		PROC
		;
		;  Code for empty boot record to display message like
		;
		;  "Partition is not bootable ..."
		;
		;  Then wait until user hits a key and reboot.
		;
		push	cs
		pop	ds
		jmp	short @@call
@@print:	pop	si
		cld
		mov	ah, 0Eh
		mov	bh, 00h
@@next_char:
		lodsb
		int	10h
		cmp	[si], bh
		jne	@@next_char
		
		mov	ah, 0
		int	16h
		mov	ax, 0E0Dh
		int	10h
		mov	al, 0Ah
		int	10h
		int	19h
@@call:
		call	@@print
@@msg:
		; Put the null terminated message here (after the code)

EMP_IPL		ENDP

END

