;
; ext32dsk.inc	Protected mode DOS file system functions
;

public  open
public  stclose
public  close
public  lseek
public  read
public  write

;----------------------------------------------------------------------
_TEXT 		segment			; 32-bit protected mode code
;----------------------------------------------------------------------

open		proc	C	near

		arg	@@path: dword, @@mode: dword

		push	ebx
		push	esi
		push	edi
		
		mov	esi, @@path	; copy path to buffer
		xor	edi, edi
		push	es
		mov	es, buffer_sel
		cld
@@next_char:
		lodsb
		stosb
		or	al, al
		jnz	@@next_char
		pop	es

		test	@@mode, 0200h ; O_TRUNC
		jnz	@@create

		mov	eax, @@mode
		mov	ah, 3Dh  ; open file
		xor	edx, edx ; address of the path
		rm_int	21h
		movzx	eax, ax
		jnc	@@end
		
		; Error
		
		cmp	eax, 2   ; File not found
		jne	@@error
		
		test	@@mode, 0100h ; O_CREAT
		jz	@@error
@@create:
		mov	ah, 3Ch  ; create a new file / truncate existing file 
		xor	ecx, ecx ; normal attributes
		xor	edx, edx ; address of the path
		rm_int	21h
		movzx	eax, ax
		jnc	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret
open		endp

;----------------------------------------------------------------------
stclose		proc	C	near	; sets file date/time and close file

		arg	@@fd: dword, @@date: dword, @@time: dword

		push	ebx
		push	esi
		push	edi
		
		mov	eax, 5701h	; set file date/time
		mov	ebx, @@fd
		mov	ecx, @@time	; time
		mov	edx, @@date	; date

		rm_int	21h
		
		jmp	stclose_reentry
		
stclose		endp

;----------------------------------------------------------------------

close		proc	C	near

		arg	@@fd: dword

		push	ebx
		push	esi
		push	edi
stclose_reentry:
		xor	eax, eax
		mov	 ah, 3Eh	; close file
		mov	ebx, @@fd
		
		rm_int	21h
		
		mov	eax, 0

		jnc	@@end
		
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret
close		endp

;----------------------------------------------------------------------

;----------------------------------------------------------------------

lseek		proc	C	near

		arg	@@fd: dword, @@pos: dword, @@mode: dword

		push	ebx
		push	esi
		push	edi
		
		mov	eax, @@mode
		mov	 ah, 42h	; lseek
		mov	ebx, @@fd
		mov	ecx, @@pos	; pos in CX:DX
		mov	edx, ecx
		shr	ecx, 16
		
		rm_int	21h
		
		jc	@@error
		
		shl	edx, 16		; returns pos in DX:AX
		mov	 dx, ax
		mov	eax, edx
		
		jmp	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret
lseek		endp

;----------------------------------------------------------------------

read		proc	C	near

		arg	@@fd: dword, @@buf: dword, @@len: dword
		local	@@read: dword

		push	ebx
		push	esi
		push	edi
		
		mov	@@read, 0
@@read_more:
		mov	ah, 3Fh		; read from a file
		mov	ebx, @@fd
		mov	ecx, @@len
		cmp	ecx, 126*512
		jbe	@@num_bytes_ok
		mov	ecx, 126*512
@@num_bytes_ok:
		xor	edx, edx	; beginning of the buffer
		rm_int	21h
		jc	@@error
		
		movzx	eax, ax
		mov	ecx, eax	; number of bytes read
		xor	esi, esi	; beginning of the buffer
		mov	edi, @@buf
		push	ds
		mov	ds, buffer_sel
		push	ecx
		shr	ecx, 2
		cld
		rep
		  movsd
		pop	ecx
		and	ecx, 3
		rep
		  movsb
		pop	ds
		mov	@@buf, edi
	
		add	@@read, eax
		sub	@@len, eax
		cmp	eax, 0
		je	@@eof
		cmp	@@len, 0
		jne	@@read_more
@@done:
@@eof:
		mov	eax, @@read
		jmp	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret
read		endp

;----------------------------------------------------------------------

write		proc	C	near

		arg	@@fd: dword, @@buf: dword, @@len: dword
		local	@@written: dword

		push	ebx
		push	esi
		push	edi
		
		mov	@@written, 0
@@write_more:
		mov	ah, 40h  ; write
		mov	ebx, @@fd
		mov	ecx, @@len
		cmp	ecx, 126*512
		jbe	@@num_bytes_ok
		mov	ecx, 126*512
@@num_bytes_ok:
		push	ecx		; number of bytes to write
		mov	esi, @@buf
		xor	edi, edi	; beginning of the buffer
		push	es
		mov	es, buffer_sel
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
		mov	@@buf, esi
		pop	ecx
		
		xor	edx, edx ; address of the buffer
		rm_int	21h
		jc	@@error
		movzx	eax, ax

		add	@@written, eax
		sub	@@len, eax
		cmp	eax, ecx
		jne	@@done   ; not all bytes were written - disk full
		cmp	@@len, 0
		jne	@@write_more
@@done:
		mov	eax, @@written
		jmp	@@end
@@error:
		mov	eax, -1
@@end:
		pop	edi
		pop	esi
		pop	ebx

		ret
write		endp

;----------------------------------------------------------------------
_TEXT 		ends
;----------------------------------------------------------------------
