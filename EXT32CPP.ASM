;
; ext32cpp.asm - C++ support functions
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

extrn	fdprintf: near
extrn	exit: near

public	_pure_error_
public  _ftol

;----------------------------------------------------------------------

_DATA		segment

perr		DB 'Error: call to pure virtual function!!!', 13, 10, 0

_DATA		ends
;----------------------------------------------------------------------
_TEXT 		segment			; 32-bit protected mode code
;----------------------------------------------------------------------

_pure_error_	proc	C	near

		push	offset perr
		push	2  ; stderr
		call	fdprintf
		pop	eax
		pop	eax

		push	1
		call	exit
		pop	eax
		
		ret

_pure_error_	endp

;----------------------------------------------------------------------

_ftol		proc	C	near

		ret
		
_ftol		endp

;----------------------------------------------------------------------
_TEXT 		ends
;----------------------------------------------------------------------
		end
