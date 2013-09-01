;
; ext32lib.asm - standard library functions
;
.386p
MODEL SMALL
LOCALS

DGROUP	GROUP _DATA, _BSS

_TEXT	segment dword public use32 'CODE'
_TEXT	ends
_DATA	segment dword public use32 'DATA'
_DATA	ends
_BSS	segment dword public use32 'BSS'
_BSS	ends

FPRINTF_BUF_SIZE	EQU	4096

extrn	write: near
extrn	write_block: near

public	printf
public	cprintf
public	fdprintf
public	sprintf
public	vcprintf
public	vfdprintf
public	vsprintf

public	memset
public	memmove
public	memcmp

public	strlen
public	strcmp
public	strcmpi
public	strncmp
public	strncmpi

public	strcpy
public	strncpy

;----------------------------------------------------------------------

_DATA		segment

dec_tab		label	byte
hex_tab_l	DB '0123456789abcdef'
hex_tab_u	DB '0123456789ABCDEF'

_DATA		ends
;----------------------------------------------------------------------
_TEXT 		segment			; 32-bit protected mode code
;----------------------------------------------------------------------

printf		proc	C	near

		arg	@@fmt: dword

		lea	eax, @@fmt
		add	eax, 4
		push	eax
		push	@@fmt
		push	1  ;stdout
		call	vfdprintf
		add	esp, 12
		
		ret

printf		endp

;----------------------------------------------------------------------

fdprintf	proc	C	near

		arg	@@fd: dword, @@fmt: dword

		lea	eax, @@fmt
		add	eax, 4
		push	eax
		push	@@fmt
		push	@@fd
		call	vfdprintf
		add	esp, 12
		
		ret

fdprintf	endp

;----------------------------------------------------------------------

sprintf		proc	C	near

		arg	@@buf: dword, @@fmt: dword

		lea	eax, @@fmt
		add	eax, 4
		push	eax
		push	@@fmt
		push	@@buf
		call	vsprintf
		add	esp, 12
		
		ret

sprintf		endp

;----------------------------------------------------------------------

cprintf		proc	C	near

		arg	@@attr: dword, @@x:dword, @@y:dword, @@fmt: dword

		lea	eax, @@fmt
		add	eax, 4
		push	eax
		push	@@fmt
		push	@@y
		push	@@x
		push	@@attr
		call	vcprintf
		add	esp, 20
		
		ret

cprintf		endp

;----------------------------------------------------------------------

vfdprintf	proc	C	near

		arg	@@fd: dword, @@fmt: dword, @@list: dword

		push	ebx
		push	esi
		push	edi

		sub	esp, FPRINTF_BUF_SIZE
		mov	ebx, esp

		push	@@list
		push	@@fmt
		push	ebx
		call	vsprintf
		add	esp, 12
		
		push	eax	; number of bytes
		push	ebx	; buffer address
		push	@@fd
		call	write
		add	esp, 12 + FPRINTF_BUF_SIZE
		
		pop	edi
		pop	esi
		pop	ebx

		ret

vfdprintf	endp

;----------------------------------------------------------------------

vcprintf	proc	C	near

		arg	@@attr: dword, @@x:dword, @@y:dword, \
		                       @@fmt: dword, @@list: dword
		push	ebx
		push	esi
		push	edi

		sub	esp, FPRINTF_BUF_SIZE
		mov	ebx, esp

		push	@@list
		push	@@fmt
		push	ebx
		call	vsprintf
		add	esp, 12
		
		push	ebx	; buffer address
		push	@@y
		push	@@x
		push	@@attr
		call	write_block
		add	esp, 16 + FPRINTF_BUF_SIZE
		
		pop	edi
		pop	esi
		pop	ebx

		ret

vcprintf	endp


;----------------------------------------------------------------------

LEFT_JUSTIFY	EQU	1
ZERO_PRECEDE	EQU	2
PLUS_PRECEDE	EQU	4
MINUS_PRECEDE	EQU	8
SPACE_PRECEDE	EQU	16
COMA_SEPARATE	EQU	32


vsprintf	proc	C	near

		arg	@@buf: dword, @@fmt: dword, @@list: dword
		local	@@flags: dword, @@width: dword
		
		push	ebx
		push	esi
		push	edi

		mov	edi, @@buf
		mov	esi, @@fmt
		mov	ebx, @@list
		cld

		jmp	@@load_next_char
@@store:
		stosb
		cmp	al, 0		; was it end of the string?
		je	@@end
@@load_next_char:
		lodsb
		cmp	al, '%'
		jne	@@store

		xor	ecx, ecx	; clear flags
		xor	edx, edx	; clear width
@@read_next_flag:
		lodsb
		cmp	al, 'A'		; does it look like type?
		jae	@@check_type
		cmp	al, '1'		; does it look like width?
		jae	@@check_width3
@@check_flag1:
		cmp	al, '-'
		jne	@@check_flag2
		or	ecx, LEFT_JUSTIFY
		jmp	@@read_next_flag
@@check_flag2:
		cmp	al, '0'
		jne	@@check_flag3
		or	ecx, ZERO_PRECEDE
		jmp	@@read_next_flag
@@check_flag3:
		cmp	al, '+'
		jne	@@check_flag4
		or	ecx, PLUS_PRECEDE
		jmp	@@read_next_flag
@@check_flag4:
		cmp	al, ' '
		jne	@@check_flag5
		or	ecx, SPACE_PRECEDE
		jmp	@@read_next_flag
@@check_flag5:
		cmp	al, ','
		jne	@@check_percent
		or	ecx, COMA_SEPARATE
		jmp	@@read_next_flag
@@check_percent:
		cmp	al, '%'		; second '%'
		je	@@store
@@check_width:
		cmp	al, '*'
		je	@@rd_arg_width
@@check_width2:
		cmp	al, '0'
		jb	@@check_type
@@check_width3:
		cmp	al, '9'
		ja	@@check_type
		imul	edx, 10
		and	eax, 0Fh
		add	edx, eax
		lodsb
		jmp	@@check_width2
@@rd_arg_width:
		mov	edx, [ebx]
		add	ebx, 4
		lodsb
@@check_type:
		or	edx, edx
		jns	@@width_positive
		neg	edx
		or	ecx, LEFT_JUSTIFY
@@width_positive:
		mov	@@flags, ecx
		mov	@@width, edx
@@check_type1:
		cmp	al, 'c'
		jne	@@check_type2
		jmp	@@print_char
@@check_type2:
		cmp	al, 's'
		jne	@@check_type3
		jmp	@@print_string
@@check_type3:
		cmp	al, 'd'
		jne	@@check_type4
		jmp	@@print_signed
@@check_type4:
		cmp	al, 'u'
		jne	@@check_type5
		jmp	@@print_unsigned
@@check_type5:
		cmp	al, 'x'
		jne	@@check_type6
		jmp	@@print_hex_l
@@check_type6:
		cmp	al, 'X'
		jne	@@check_type7
		jmp	@@print_hex_u
@@check_type7:
		cmp	al, 'p'
		jne	@@check_type8
		jmp	@@print_ptr
@@check_type8:
		jmp	@@store		; unrecognized character
@@end:
		mov	eax, edi
		dec	eax
		sub	eax, @@buf
		
		pop	edi
		pop	esi
		pop	ebx
		
		ret


@@print_char:
		mov	eax, [ebx]
		add	ebx, 4
		stosb
		jmp	@@load_next_char

@@print_hex_l:
		push	esi
		mov	esi, offset hex_tab_l
		jmp	@@print_hex
@@print_hex_u:
		push	esi
		mov	esi, offset hex_tab_u
@@print_hex:
		mov	eax, [ebx]
		add	ebx, 4
		push	ebx
		mov	ebx, 16
		jmp	@@start_divide

@@print_signed:
		mov	eax, [ebx]
		add	ebx, 4
		or	eax, eax
		jns	@@print_decimal
		neg	eax
		or	@@flags, MINUS_PRECEDE
		jmp	@@print_decimal

@@print_unsigned:
		mov	eax, [ebx]
		add	ebx, 4
@@print_decimal:
		push	esi
		push	ebx
		mov	ebx, 10
		mov	esi, offset dec_tab
@@start_divide:
		xor	ecx, ecx
		xchg	edi, @@flags
@@next_divide:
		xor	edx, edx
		div	ebx
		push	dword ptr [esi+edx]
		inc	ecx
		or	eax, eax
		jz	@@stop_divide
		test	edi, COMA_SEPARATE
		jz	@@next_divide
		cmp	cl, 3
		je	@@push_coma
		cmp	cl, 7
		je	@@push_coma
		cmp	cl, 11
		jne	@@next_divide
@@push_coma:
		push	','
		inc	ecx
		jmp	@@next_divide
@@stop_divide:
		test	edi, ZERO_PRECEDE
		jz	@@no_zeros
		cmp	ecx, @@width
		jae	@@no_zeros
		test	edi, (MINUS_PRECEDE or PLUS_PRECEDE or SPACE_PRECEDE)
		jz	@@push_zeros
		dec	@@width
@@push_zeros:
		cmp	ecx, @@width
		jae	@@no_zeros
		push	'0'
		inc	ecx
		jmp	@@push_zeros
@@zeros_done:
@@no_zeros:
		mov	al, '-'
		test	edi, MINUS_PRECEDE
		jnz	@@push_eax
		mov	al, '+'
		test	edi, PLUS_PRECEDE
		jnz	@@push_eax
		mov	al, ' '
		test	edi, SPACE_PRECEDE
		jz	@@dont_push_eax
@@push_eax:
		push	eax
		inc	ecx
@@dont_push_eax:
		test	edi, LEFT_JUSTIFY
		xchg	edi, @@flags
		jnz	@@dump_digits
@@write_space:
		cmp	ecx, @@width
		jae	@@dump_digits
		mov	al, ' '
		stosb
		dec	@@width
		jmp	@@write_space
@@dump_digits:
		mov	ebx, ecx
@@dump_next_digit:
		pop	eax
		stosb
		loop	@@dump_next_digit
@@write_space2:
		cmp	ebx, @@width
		jae	@@finally_end
		mov	al, ' '
		stosb
		dec	@@width
		jmp	@@write_space2
@@finally_end:
		pop	ebx
		pop	esi

		jmp	@@load_next_char


@@print_ptr:
		mov	edx, [ebx]
		add	ebx, 4
		mov	ecx, 8
		rol	edx, 4
@@next_hex_dg:
		mov	eax, edx
		and	eax, 0Fh
		mov	al, hex_tab_u[eax]
		stosb
		rol	edx, 4
		loop	@@next_hex_dg

		jmp	@@load_next_char


@@print_string:
		push	esi
		mov	esi, [ebx]
		add	ebx, 4
		xor	ecx, ecx
		
		test	@@flags, LEFT_JUSTIFY
		jnz	@@load_char
		cmp	@@width, 0
		jne	@@right_just_str
		jmp	@@load_char
@@dump_char:
		stosb
		inc	ecx
@@load_char:
		lodsb
		cmp	al, 0
		jne	@@dump_char

		mov	al, ' '
@@write_space3:
		cmp	ecx, @@width
		jae	@@str_end
		stosb
		inc	ecx
		jmp	@@write_space3
@@str_end:
		pop	esi
		jmp	@@load_next_char

@@right_just_str:
		push	esi
		push	edi
		jmp	@@count_next
@@pre_write_char:
		stosb
		inc	ecx
@@count_next:
		lodsb
		cmp	al, 0
		jne	@@pre_write_char

		cmp	ecx, @@width
		jb	@@need_spaces
		pop	eax	; string was too long to be right justifyed
		pop	eax
		pop	esi
		jmp	@@load_next_char
		
@@need_spaces:
		pop	edi
		pop	esi
		mov	edx, ecx
		mov	ecx, @@width
		sub	ecx, edx
		mov	al, ' '
		rep
		  stosb
		mov	ecx, edx
		rep
		  movsb
		pop	esi
		jmp	@@load_next_char
		
		
vsprintf	endp

;----------------------------------------------------------------------

memset		proc	C	near

		arg	@@s: dword, @@c: dword, @@n: dword
		
		push	esi
		push	edi
		
		mov	edi, @@s
		push	edi
		mov	al, byte ptr @@c
		mov	ah, al
		push	ax
		shl	eax, 16
		pop	ax
		mov	ecx, @@n
		push	ecx
		shr	ecx, 2
		cld
		rep
		  stosd
		pop	ecx
		and	ecx, 3
		rep
		  stosb
		pop	eax	; returning destination address

		pop	edi
		pop	esi
		
		ret
		
memset		endp

;----------------------------------------------------------------------

memmove		proc	C	near

		arg	@@dst: dword, @@src: dword, @@n: dword
		
		push	esi
		push	edi
		
		mov	ecx, @@n
		mov	esi, @@src
		mov	edi, @@dst
		push	edi
		
		cld
		cmp	esi, edi
		jae	@@copy
@@reverse:
		add	esi, ecx
		add	edi, ecx
		dec	esi
		dec	edi
		std
		push	ecx
		and	ecx, 3
		rep
		  movsb
		pop	ecx
		and	cl, 0FCh
		sub	esi, 3
		sub	edi, 3
@@copy:
		push	ecx
		shr	ecx, 2
		rep
		  movsd
		pop	ecx
		and	ecx, 3
		rep
		  movsb
@@end:
		cld
		pop	eax	; returning destination address

		pop	edi
		pop	esi
		
		ret
		
memmove		endp

;----------------------------------------------------------------------

memcmp		proc	C	near

		arg	@@s1: dword, @@s2: dword, @@n: dword
		
		push	esi
		push	edi
		
		cld
		xor	eax, eax
		mov	ecx, @@n
		mov	esi, @@s1
		mov	edi, @@s2

		rep
		  cmpsb
		jz	@@end
		dec	esi
		dec	edi
		lodsb
		sub	al, [edi]
		movsx	eax, al
@@end:
		pop	edi
		pop	esi
		
		ret
		
memcmp		endp

;----------------------------------------------------------------------

strlen		proc	C	near

		arg	@@s: dword
		
		push	esi
		push	edi
		
		cld
		xor	eax, eax
		xor	ecx, ecx
		dec	ecx
		mov	edi, @@s
		repne
		  scasb
		mov	eax, edi
		dec	eax
		sub	eax, @@s

		pop	edi
		pop	esi
		
		ret
		
strlen		endp
		
;----------------------------------------------------------------------

strcmp		proc	C	near

		arg	@@s1: dword, @@s2: dword
		
		push	esi
		push	edi
		
		cld
		xor	eax, eax
		mov	esi, @@s1
		mov	edi, @@s2

@@next_char:
		lodsb
		mov	ah, [edi]
		inc	edi
		cmp	al, ah
		jne	@@end
		cmp	al, 0
		je	@@end
		cmp	ah, 0
		je	@@end
		jmp	@@next_char
@@end:
		sub	al, ah
		mov	ah, 0

		pop	edi
		pop	esi
		
		ret
		
strcmp		endp

;----------------------------------------------------------------------

strncmp		proc	C	near

		arg	@@s1: dword, @@s2: dword, @@n: dword
		
		push	esi
		push	edi
		
		cld
		xor	eax, eax
		mov	ecx, @@n
		mov	esi, @@s1
		mov	edi, @@s2

@@next_char:
		lodsb
		mov	ah, [edi]
		inc	edi
		cmp	al, ah
		jne	@@end
		cmp	al, 0
		je	@@end
		cmp	ah, 0
		je	@@end
		loop	@@next_char
@@end:
		sub	al, ah
		mov	ah, 0

		pop	edi
		pop	esi
		
		ret
		
strncmp		endp

;----------------------------------------------------------------------

strcmpi		proc	C	near

		arg	@@s1: dword, @@s2: dword
		
		push	esi
		push	edi
		
		cld
		xor	eax, eax
		mov	esi, @@s1
		mov	edi, @@s2

@@next_char:
		lodsb
		mov	ah, [edi]
		inc	edi
		cmp	al, ah
		jne	@@not_equ
@@char_equ:
		cmp	al, 0
		je	@@end
		jmp	@@next_char
@@not_equ:
		cmp	al, 'a'
		jb	@@skip1
		cmp	al, 'z'
		ja	@@skip1
		sub	al, 'a'-'A'
	@@skip1:
		cmp	ah, 'a'
		jb	@@skip2
		cmp	ah, 'z'
		ja	@@skip2
		sub	ah, 'a'-'A'
	@@skip2:
		cmp	al, ah
		je	@@next_char
@@end:
		sub	al, ah
		mov	ah, 0

		pop	edi
		pop	esi
		
		ret
		
strcmpi		endp

;----------------------------------------------------------------------

strncmpi	proc	C	near

		arg	@@s1: dword, @@s2: dword, @@n: dword
		
		push	esi
		push	edi
		
		cld
		xor	eax, eax
		mov	ecx, @@n
		mov	esi, @@s1
		mov	edi, @@s2

@@next_char:
		lodsb
		mov	ah, [edi]
		inc	edi
		cmp	al, ah
		jne	@@not_equ
@@char_equ:
		cmp	al, 0
		je	@@end
		loop	@@next_char
		jmp	@@end
@@not_equ:
		cmp	al, 'a'
		jb	@@skip1
		cmp	al, 'z'
		ja	@@skip1
		sub	al, 'a'-'A'
	@@skip1:
		cmp	ah, 'a'
		jb	@@skip2
		cmp	ah, 'z'
		ja	@@skip2
		sub	ah, 'a'-'A'
	@@skip2:
		cmp	al, ah
		je	@@char_equ
@@end:
		sub	al, ah
		mov	ah, 0h

		pop	edi
		pop	esi

		ret
		
strncmpi	endp

;----------------------------------------------------------------------

strcpy		proc	C	near

		arg	@@s1: dword, @@s2: dword
		
		push	esi
		push	edi
		
		mov	esi, @@s2
		mov	edi, @@s1
		
		push	edi
		cld
@@next_char:
		lodsb
		stosb
		cmp	al, 0
		jne	@@next_char
		
		pop	eax

		pop	edi
		pop	esi
		
		ret
		
strcpy		endp

;----------------------------------------------------------------------

strncpy		proc	C	near

		arg	@@s1: dword, @@s2: dword, @@n: dword
		
		push	esi
		push	edi
		
		xor	ecx, ecx
		mov	esi, @@s2
		mov	edi, @@s1
		
		push	edi
		cld
@@next_char:
		cmp	ecx, @@n
		je	@@stop
		lodsb
		stosb
		inc	ecx
		cmp	al, 0
		jne	@@next_char
		jmp	@@done
@@stop:
		mov	al, 0
		stosb
@@done:
		
		pop	eax

		pop	edi
		pop	esi
		
		ret
		
strncpy		endp

;----------------------------------------------------------------------
_TEXT 		ends
;----------------------------------------------------------------------
		end
