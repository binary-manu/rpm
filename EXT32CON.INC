;
; ext32con.inc	Protected mode keyboard, cursor, and direct video functions
;

public	delay
public	kb_hit
public	get_key
public	put_key
public	move_cursor
public	save_cursor

public	write_string
public	write_block

public	save_window
public	load_window
public	clear_window
public	border_window
public	WriteString

;----------------------------------------------------------------------
_TEXT 		segment			; 32-bit protected mode code
;----------------------------------------------------------------------

delay		proc	C	near

		arg	@@t: dword
		
		mov	ah, 86h		; wait
		mov	edx, @@t
		imul	edx, 976	; timer granularity is 976 microseconds
		mov	ecx, edx
		shr	ecx, 16
		rm_int	15h		; cx:dx is wait time in microseconds

		ret

delay		endp

;----------------------------------------------------------------------

kb_hit		proc	C	near

		push	ebx
		push	esi
		push	edi

		mov	ah, 01h			; get keyboard status
		rm_int	16h
		jz	@@no_key
		movzx	eax, ax
		cmp	al, 0
		je	@@end
		mov	ah, 0
		jmp	@@end
@@no_key:
		xor	eax, eax
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret
kb_hit		endp

;----------------------------------------------------------------------
get_key		proc	C	near

		push	ebx
		push	esi
		push	edi

		cmp	detected_os, 0	; Is it a bare system?
		jne	@@get_key_int21	; No, there is DOS or Windows.
@@get_key:
		mov	ah, 00h			; keyboard read
		rm_int	16h
		movzx	eax, ax
		cmp	al, 0
		je	@@end
		mov	ah, 0
		jmp	@@end

@@get_key_int21:		; Reading keyboard via DOS call will not hog
		mov	ah, 07h	; CPU cycles, when we run under Windows
		rm_int	21h
		movzx	eax, al
		cmp	al, 0
		jne	@@end
		mov	ah, 07h
		rm_int	21h
		mov	ah, al
		mov	al, 0
		movzx	eax, ax
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret
get_key		endp

;----------------------------------------------------------------------

put_key		proc	C	near

		arg	@@k: dword

		push	ebx
		push	esi
		push	edi

		mov	ah, 05h			; put key in keyb buffer
		mov	ecx, @@k

		rm_int	16h
		
		movzx	eax, ax

		pop	edi
		pop	esi
		pop	ebx

		ret

put_key		endp

;----------------------------------------------------------------------

move_cursor	proc	C	near

		arg	@@x: dword, @@y: dword
		
		push	ebx
		push	esi
		push	edi

		mov	ah, 02h			; set cursor position
		mov	bh, 00h
		mov	dl, byte ptr @@x
		mov	dh, byte ptr @@y
		sub	dx, 0101h

		rm_int	10h

		pop	edi
		pop	esi
		pop	ebx

		ret

move_cursor	endp

;----------------------------------------------------------------------

save_cursor	proc	C	near

		arg	@@px: dword, @@py: dword
		
		push	ebx
		push	esi
		push	edi

		mov	ah, 03h			; get cursor position
		mov	bh, 00h

		rm_int	10h
		
		mov	esi, @@px
		movzx	eax, dl
		inc	eax
		mov	[esi], eax

		mov	esi, @@py
		movzx	eax, dh
		inc	eax
		mov	[esi], eax

		pop	edi
		pop	esi
		pop	ebx

		ret

save_cursor	endp

;----------------------------------------------------------------------

write_string	proc	C 	near

		arg	@@attr: dword, @@x: dword, @@y: dword, @@s:dword

		push	esi
		push	edi

		mov	ah, byte ptr @@attr
		mov	esi, @@s
		mov	edi, @@y
		dec	edi
		imul	edi, 80		; chars per row
		add	edi, @@x
		dec	edi
		shl	edi, 1		; two bytes per char

		push	es
		mov	es, video_sel
		cld
		jmp	@@read
    @@write:
    		cmp	edi, 4000	; max screen length 
    		ja	@@exit
		stosw
    @@read:
		lodsb
		cmp	al, 0
		jne	@@write
    @@exit:
		pop	es

		pop	edi
		pop	esi

		ret

write_string	endp

WriteString			proc		C near

		arg	@@attr: dword, @@x: dword, @@y: dword, @@s:dword, @@n:dword

		push	esi
		push	edi
		push	ecx
		
		mov ecx, @@n
		mov	ah, byte ptr @@attr
		mov	esi, @@s
		mov	edi, @@y
		dec	edi
		imul	edi, 80		; chars per row
		add	edi, @@x
		dec	edi
		shl	edi, 1		; two bytes per char

		push	es
		mov	es, video_sel
		cld
		jmp	@@read
    @@write:
    	cmp	edi, 4000	; max screen length 
    	ja	@@exit
		stosw
    @@read:
		jecxz @@exit
		dec ecx
		lodsb
		cmp	al, 0
		jne	@@write
    @@exit:
		pop	es

		pop ecx
		pop	edi
		pop	esi

		ret

WriteString			endp

;----------------------------------------------------------------------

write_block	proc	C 	near

		arg	@@attr: dword, @@x: dword, @@y: dword, @@s:dword

		push	es
		push	esi
		push	edi

		cld
		mov	es, video_sel
		mov	ah, byte ptr @@attr
		mov	esi, @@s
@@next_row:
		mov	edi, @@y
		dec	edi
		imul	edi, 80		; chars per row
		add	edi, @@x
		dec	edi
		shl	edi, 1		; two bytes per char

		jmp	@@read
    @@write:
    		cmp	edi, 4000	; max screen length 
    		ja	@@exit
		stosw
    @@read:
		lodsb
		cmp	al, 0
		je	@@exit
		cmp	al, 13		; '\r' - ignoring it
		je	@@read
		cmp	al, 10		; '\n' - go to the new line
		jne	@@write
		inc	@@y
		jmp	@@next_row
    @@exit:
		pop	edi
		pop	esi
		pop	es

		ret

write_block	endp

;----------------------------------------------------------------------

save_window	proc	C 	near

		arg		@@x: dword, @@y: dword, \
				@@w: dword, @@h: dword, @@buf:dword
		push	ds
		push	esi
		push	edi

		cld
		mov	edi, @@buf
		mov	esi, @@y
		dec	esi
		imul	esi, 80		; chars per row
		add	esi, @@x
		dec	esi
		shl	esi, 1		; two bytes per char
		push	ds
		pop	es
		mov	ds, video_sel
		
		mov	ecx, @@h
@@next_row:
		push	ecx
		mov	ecx, @@w
		rep
		  movsw
		add	esi, 160	; Advance to the next row
		sub	esi, @@w
		sub	esi, @@w
		pop	ecx

		loop	@@next_row

		pop	edi
		pop	esi
		pop	ds
		
		ret

save_window	endp

;----------------------------------------------------------------------

load_window	proc	C 	near

		arg		@@x: dword, @@y: dword, \
				@@w: dword, @@h: dword, @@buf:dword
		push	es
		push	esi
		push	edi

		cld
		mov	esi, @@buf
		mov	edi, @@y
		dec	edi
		imul	edi, 80		; chars per row
		add	edi, @@x
		dec	edi
		shl	edi, 1		; two bytes per char
		mov	es, video_sel
		
		mov	ecx, @@h
@@next_row:
		push	ecx
		mov	ecx, @@w
		rep
		  movsw
		add	edi, 160	; Advance to the next row
		sub	edi, @@w
		sub	edi, @@w
		pop	ecx

		loop	@@next_row

		pop	edi
		pop	esi
		pop	es
		
		ret

load_window	endp

;----------------------------------------------------------------------

clear_window	proc	C 	near

		arg	@@attr:dword, @@x: dword, @@y: dword, \
				      @@w: dword, @@h: dword
		push	es
		push	esi
		push	edi

		cld
		mov	edi, @@y
		dec	edi
		imul	edi, 80		; chars per row
		add	edi, @@x
		dec	edi
		shl	edi, 1		; two bytes per char
		mov	es, video_sel
		
		mov	al, ' '
		mov	ah, byte ptr @@attr

		mov	ecx, @@h
@@next_row:
		push	ecx
		mov	ecx, @@w
		rep
		  stosw
		add	edi, 160	; Advance to the next row
		sub	edi, @@w
		sub	edi, @@w
		pop	ecx

		loop	@@next_row

		pop	edi
		pop	esi
		pop	es

		ret

clear_window	endp

;----------------------------------------------------------------------

border_window	proc	C 	near	; Min window w=3, h=1

		arg	@@attr:dword, @@x: dword, @@y: dword, \
				      @@w: dword, @@h: dword, @@b: dword
		push	es
		push	esi
		push	edi

		cld
		mov	esi, @@b
		mov	edi, @@y
		dec	edi
		imul	edi, 80		; chars per row
		add	edi, @@x
		dec	edi
		shl	edi, 1		; two bytes per char
		mov	es, video_sel
		
		mov	ah, byte ptr @@attr
@@upper_row:
		lodsb			; Upper row - left corner
		stosw
		mov	ecx, @@w
		sub	ecx, 2
		lodsb			; Upper row - middle char
		rep
		  stosw
		lodsb			; Upper row - right corner
		stosw

		add	edi, 160	; Advance to the next row
		sub	edi, @@w
		sub	edi, @@w
		
		mov	ecx, @@h
		cmp	ecx, 1
		jbe	@@end
		sub	ecx, 2
		jz	@@bottom_row
@@middle_rows:
@@next_row:
		push	ecx
		lodsb			; Left side
		stosw
		mov	ecx, @@w
		sub	ecx, 2
		lodsb			; Filling char
		rep
		  stosw
		lodsb			; Right side
		stosw
		add	edi, 160	; Advance to the next row
		sub	edi, @@w
		sub	edi, @@w
		sub	esi, 3		; Repeat same characters
		pop	ecx

		loop	@@next_row

		add	esi, 3
@@bottom_row:
		lodsb			; Bottom row - left corner
		stosw
		mov	ecx, @@w
		sub	ecx, 2
		lodsb			; Bottom row - middle char
		rep
		  stosw
		lodsb			; Bottom row - right corner
		stosw
@@end:
		pop	edi
		pop	esi
		pop	es

		ret

border_window	endp

;----------------------------------------------------------------------
_TEXT 		ends
;----------------------------------------------------------------------
