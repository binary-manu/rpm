;
; ext32xms.inc	Protected mode XMS initialization functions
;

;----------------------------------------------------------------------
_TEXT16		segment		; 16-bit code
;----------------------------------------------------------------------

xms_init	proc	near

		mov	ax, 4300h	; Check if XMS manager is installed
		int	2Fh
		cmp	al, 80h
		jne	@@no_xms	; XMS manager is not installed

		;
		; Allocating memory via XMS
		;

		mov	ax, 4310h	; Get XMS driver entry point
		int	2Fh
		mov	word ptr XMS_call, bx
		mov	word ptr XMS_call+2, es
		
		;mov	ah, 00h		; Get XMS version
		;call	XMS_call	; AH=Major, AL=Minor
		;cmp	ah, 3		; Version 3+ supprts >64M 
		;jb	@@xms2only	; of extended memory
		;...
		;@@xms2only:
		
		check_point 11
		mov	ah, 05h		; Local enable A20 line
		call	XMS_call
		or	ax, ax		; AX=0001 success
		jz	@@error		; AX=0000 failure
		
		check_point 12
		mov	ah, 08h		; Query free extended memory
		call	XMS_call	; AX = largest block size (K)
					; DX = total extended memory (K)

		movzx	eax, ax		; convert to bytes
		shl	eax, 10
		and	ax, 0F000h	; Align on 4k page boundary
		mov	mem_size, eax
		cmp	eax, offset min_mem_size ; Do we have enough memory?
		jb	@@error
		
		check_point 13
		mov	ah, 09h		; Allocate extended memory block
		mov	edx, mem_size
		shr	edx, 10		; convert to Kb
		call	XMS_call
		or	ax, ax
		jz	@@error

		mov	word ptr mem_handle, dx
		
		check_point 14
		mov	ah, 0Ch		; Lock extended memory block
		mov	dx, word ptr mem_handle
		call	XMS_call
		or	ax, ax
		jz	@@deallocate
		mov	word ptr mem_addr, bx
		mov	word ptr mem_addr+2, dx

		;
		;  Switching to protected mode
		;
		call	prepare_protected_mode
		call	switch_to_protected_mode ; switches to 32-bit stack

		call	call_32_bit		; returns exit code in al

		call	switch_to_real_mode	; restores 16-bit stack
		push	ax
		;
		; Deallocating XMS memory
		;
		mov	ah, 0Dh		; Unlock extended memory block
		mov	dx, word ptr mem_handle
		call	XMS_call
		;or	ax, ax		; there is nothing we could or should
		;jz	@@error		; do with errors now, so we ingnoring
					; them
		mov	ah, 0Ah		; Free extended memory block
		mov	dx, word ptr mem_handle
		call	XMS_call
		;or	ax, ax
		;jz	@@error
		
		mov	ah, 06h		; Local disable A20 line
		call	XMS_call
		;or	ax, ax
		;jz	@@error

		call	mouse_done
		;
		; Exit to DOS
		;
		pop	ax
		mov	ah, 4Ch  ; exit code was passed in al
		int	21h
		
		;
		;  Errors ...
		;
@@deallocate:
		mov	ah, 0Ah		; Free extended memory block
		mov	dx, word ptr mem_handle
		call	XMS_call
		;or	ax, ax
		;jz	@@error
@@error:
		stc
		ret

@@no_xms:
		clc
		ret

xms_init	endp

;----------------------------------------------------------------------
_TEXT16		ends
;----------------------------------------------------------------------
