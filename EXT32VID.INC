public	set_text_mode
public	set_graph_mode
public	put_pixel

;--------------------------------------------------------------	
_DATA	segment
;--------------------------------------------------------------	
gr640x400 label	word
	;	 3C4h	; Sequencer Index
	DW	0F02h	; Map Mask Register
	DW	0604h	; Memory Mode Register
	;	 3CEh	; Graphics Controller Index
	DW	0000h	; Set/Reset Register (default color)
	DW	0F01h	; Enable Set/Reset Register
	DW	0003h	; Data Rotate Register
	DW	0005h	; Mode Register
	DW	0406h	; Misc Register
	DW     0FF08h	; Bit Mask Register
	;	 3C0h	; Attribute Controller Register
	DB	  30h	; Enable Video / Select 10h register
	DB	  01h	; Attribute Mode Control Register / Go Graphics!!!
	;	 3D4h	; CRTC Controller Index
	DW	200Ch	; Start Address High Register
	DW	000Dh	; Start Address Low Register
	DW	2813h	; Offset Register
	DW	6009h	; Maximum Scan Line Register
	DW	0014h	; Underline Location Register
	DW     0E317h	; Mode Control Register
	DW	0E11h	; Unprotect Horizontal Registers (0-7)
	DW	5F00h	; Horizontal Total Register
	DW	4F01h	; Horizontal Displayed Register
	DW	5002h	; Horizontal Blank Start Register (5A)
	DW	8203h	; Horizontal Blank End Register (8E)
	DW	5404h	; Horizontal Retrace (Sync) Start Register (5F)
	DW	8005h	; Horizontal Retrace (Sync) End Register (8A)
	DW     0BF06h	; Vertical Total Register

gr720x400 label	word
	;	 3C4h	; Sequencer Index
	DW	0F02h	; Map Mask Register
	DW	0604h	; Memory Mode Register
	;	 3CEh	; Graphics Controller Index
	DW	0000h	; Set/Reset Register (default color)
	DW	0F01h	; Enable Set/Reset Register
	DW	0003h	; Data Rotate Register
	DW	0005h	; Mode Register
	DW	0406h	; Misc Register
	DW     0FF08h	; Bit Mask Register
	;	 3C0h	; Attribute Controller Register
	DB	  30h	; Enable Video / Select 10h register
	DB	  01h	; Attribute Mode Control Register / Go Graphics!!!
	;	 3D4h	; CRTC Controller Index
	DW	200Ch	; Start Address High Register
	DW	000Dh	; Start Address Low Register
	DW	2D13h	; Offset Register
	DW	6009h	; Maximum Scan Line Register
	DW	0014h	; Underline Location Register
	DW     0E317h	; Mode Control Register
	DW	0E11h	; Unprotect Horizontal Registers (0-7)
	DW	6B00h	; Horizontal Total Register (6B)
	DW	5901h	; Horizontal Displayed Register
	DW	5A02h	; Horizontal Blank Start Register (5A)
	DW	8E03h	; Horizontal Blank End Register (8E)
	DW	5F04h	; Horizontal Retrace (Sync) Start Register (5F)
	DW	8A05h	; Horizontal Retrace (Sync) End Register (8A)
	DW     0C006h	; Vertical Total Register (BF) [C0]

text80x25 label	word
	;	 3C4h	; Sequencer Index
	DW	0302h	; Map Mask Register
	DW	0204h	; Memory Mode Register
	;	 3CEh	; Graphics Controller Index
	DW	0000h	; Set/Reset Register
	DW	0001h	; Enable Set/Reset Register
	DW	0003h	; Data Rotate Register
	DW	1005h	; Mode Register
	DW	0E06h	; Misc Register
	DW     0FF08h	; Bit Mask Register
	;	 3C0h	; Attribute Controller Register
	DB	  30h	; Enable Video / Select 10h register
	DB	  04h	; Attribute Mode Control Register / Go to TextMode
	;	 3D4h	; CRTC Controller Index
	DW	4F09h	; Maximum Scan Line Register
	DW	000Ch	; Start Address High Register
	DW	000Dh	; Start Address Low Register
	DW	2813h	; Offset Register
	DW	1F14h	; Underline Location Register
	DW     0A317h	; Mode Control Register
	DW	0E11h	; Unprotect Horizontal Registers (0-7)
	DW	5F00h	; Horizontal Total Register
	DW	4F01h	; Horizontal Displayed Register
	DW	5002h	; Horizontal Blank Start Register
	DW	8203h	; Horizontal Blank End Register
	DW	5404h	; Horizontal Retrace (Sync) Start Register
	DW	8005h	; Horizontal Retrace (Sync) End Register
	DW     0BF06h	; Vertical Total Register
;--------------------------------------------------------------	
_DATA	ends
;--------------------------------------------------------------	

;--------------------------------------------------------------	
_TEXT	segment
;--------------------------------------------------------------	
COMMENT	%
		mov	ax, 1C00h	; Test for VGA
		mov	cx, 0001h
		int	10h
		cmp	al, 1Ch
		jne	@@exit		; Not a VGA BIOS
%
;--------------------------------------------------------------	
set_graph_mode	proc	C	near

		arg	@@comp: dword
		
		push	ebx
		push	esi
		push	edi

		mov	ax, 12h
		rm_int	10h
		jmp	@@end		
		test	byte ptr @@comp, 1
		;jz	@@fast
		;if(compartibility mode)
		  mov	dx, 3DAh
@@wait:		  in	al, dx
		  test	al, 8	; Is it doing vertical retrace ?
		  jz	@@wait  ; If not - then wait
		  
		  mov	ax, 13h		; Mode 0Eh/Don't clear memory
		  rm_int 10h		; Set mode
		  ;mov	ax, 0Eh		; Mode 0Eh/Don't clear memory
		  ;rm_int 10h		; Set mode
		  mov	dx, 3C2h	; Misc Video Register
		  mov	al, 67h		; Set 400-lines, 28 Hz
		  ;out	dx, al
		  ;mov	dl, 0D4h
		  ;mov	ax, 6009h
		  ;out	dx, ax
		  ;mov	ax, 200Ch
		  ;out	dx, ax
		  ;jmp	@@end
		  
@@fast:
		mov	esi, offset gr640x400
		;mov	esi, offset gr720x400
		call	set_video_mode
		;mov	???, video_sel
@@end:
		pop	edi
		pop	esi
		pop	ebx
		
		ret

set_graph_mode	endp
;--------------------------------------------------------------	
set_text_mode	proc	C	near

		arg	@@comp: dword
		
		push	ebx
		push	esi
		push	edi

		mov	ax, 03h
		rm_int	10h
		jmp	@@end		

		test	byte ptr @@comp, 1
		jz	@@fast
		;if(compartibility mode)
		  mov	dx, 3DAh
@@wait:		  in	al, dx
		  test	al, 1	; Is it doing horizontal retrace ?
		  jz	@@wait  ; If not - then wait
		  mov	ax, 03h		; Mode 03h/Don't clear memory
		  rm_int 10h		; Set mode
		  jmp	@@end
@@fast:
		mov	esi, offset text80x25
		call	set_video_mode
		;mov	???, video_sel
@@end:
		pop	edi
		pop	esi
		pop	ebx
		
		ret

set_text_mode	endp
;--------------------------------------------------------------	
set_video_mode	proc	near

		cld
		mov	edx, 3C4h	; Sequencer Index
		outsw
		outsw
		mov	dl, 0CEh	; Graphics Controller Index
		mov	ecx, 0006
		rep	outsw

		push	es
		xor	eax, eax	; Clear video memory
		mov	ecx, (SCR_X/8)*SCR_Y/4	; 720*400/(8 bits/byte)/(4 bytes/stosd)
		mov	edi, 0A2000h
		;mov	es, graph_sel
		mov	es, zero_sel
		rep	stosd
		pop	es

		mov	dl, 0DAh	; Video Status Register
		in	al, dx		; Reset 3C0's flip-flop
		mov	dl, 0C0h	; Attribute Controller Register
		mov	al, 0
		out	dx, al		; Diable video output

		mov	dl, 0DAh	; Video Controller Status Register
@@wait:		in	al, dx		; Get status / Reset 3C0's flip-flop
		test	al, 8		; Is it doing vertical retrace now ?
		jz	@@wait		; if not - wait until retrace starts

		;cli
		mov	dl, 0C0h	; Attribute Controller Register
		outsb			; Enable Video / Select 10h register
		outsb			; Mode Control / Switch Text/Graphics

		mov	dl, 0D4h	; CRT Controller Index
		mov	cl, 14
		rep	outsw
		;sti

		ret

set_video_mode	endp
;--------------------------------------------------------------	
put_pixel	proc	C	near

		arg	@@color: dword, @@x:dword, @@y:dword

		push	ebx
		push	esi
		push	edi

		mov	eax, SCR_X/8 ; bytes/line
		mul	@@y
	;add	eax, 2000h ; screen offset
		mov	ebx, @@x
		shr	ebx, 3	   ; 8 pixels/byte
		add	ebx, eax
		
		mov	dx, 3CEh ; Graphics Controller Register
		mov	ah, byte ptr @@color
		and	ah, 0Fh
		mov	al, 00h
		out	dx, ax		; Load color into Set/Reset Register
		 mov	ax, 0F01h
		 out	dx, ax
		mov	ecx, @@x
		and	cl, 7
		mov	ax, 8008h
		shr	ah, cl	 ; ah = Bit Mask, al = Bit Mask Register Index
		out	dx, ax

		push	es
		;mov	es, graph_sel
		 mov	es, zero_sel
		 add	ebx, 0A0000h
		mov	al, es:[ebx]	; Read latch register
		mov	es:[ebx], al	; Initiate write cycle
		pop	es
		
		;mov	ax, 0FF08h	; Restore Bit Mask Register
		;out	dx, ax
		;mov	ax, 0001h	; Restore Set/Reset Register
		;out	dx, ax

		pop	edi
		pop	esi
		pop	ebx

		ret
put_pixel	endp
;--------------------------------------------------------------	
_TEXT	ends
