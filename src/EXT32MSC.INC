;
; ext32msc.inc	Miscelanious real mode functions
;

;----------------------------------------------------------------------
_TEXT16		segment		; 16-bit code
;----------------------------------------------------------------------

check_os	proc	near
		;
		;  Arguments:	none
		;  Returns:	al - OS type
		;  Destroys:	ax, bx, cx, dx, si, di, es
		;
		;	1 - MS-DOS <5.00
		;	2 - MS-DOS  5.xx-7.xx
		;	3 - Windows 2.xx-3.xx
		;	4 - Windows 95/98/...
		;	5 - Windows NT
		;
		mov	si, 1		; DOS < 5.00

		mov	ax, 3000h	; Get DOS version
		int	21h
		cmp	al, 5
		jb	@@check_win

		mov	si, 2		; DOS 5.xx - 7.xx

		mov	ax, 3306h	; Get true DOS version
		int	21h
		
		cmp	bx, 3205h	; Win NT dos box returns v5.50
		jne	@@check_win
		
		mov	si, 5		; Win NT
		jmp	@@end
@@check_win:
		mov	ax, 1600h	; Get Windows version
		int	2Fh
		cmp	al, 0
		je	@@no_win
		cmp	al, 80h
		je	@@no_win

		mov	si, 3		; Windows 2.xx - 3.xx
		
		cmp	al, 0FFh	; Windows 2.xx
		je	@@end

		cmp	al, 3		; else al=major (>=3)
		je	@@end		;      ah=minor (.xx)

		mov	si, 4		; Windows 95/98 GUI
		jmp	@@end
@@no_win:
@@end:
		mov	ax, si

		ret
check_os	endp

;----------------------------------------------------------------------
_TEXT16		ends
;----------------------------------------------------------------------
