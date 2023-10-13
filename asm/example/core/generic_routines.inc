

; == Common Routines.
; == Some copied from Maxim (if it ain't broke...)
; ===================================================================================================

.bank 0 slot 0
.section "Generic Routines" free


; == Sets the VDP address
; Parameters: hl = address

; Affects: 	  a, hl
SetVDPAddress:				ld 	a, l
							out (VDP_CONTROL_PORT),a
							ld	a, h
							out (VDP_CONTROL_PORT),a
						        
							ret



; == Copies data to the VDP
; Parameters: hl = data address
;			  bc = data length

; Affects: 	  a, hl, bc
CopyToVDP:					ld a, (hl)    ; Get data byte
							out (VDP_DATA_PORT), a
							inc hl       ; Point to next letter
							dec bc
							ld a, b
							or c
							jr nz, CopyToVDP
							    
							ret
							
							
							
; == Clears user specified location and length of vdp vram
; Parameters: hl = vram address
;			  bc = size to clear (in bytes)

; Affects: 	  a, hl, bc
ClearVRAM:					call SetVDPAddress
-:							xor a
							out (VDP_DATA_PORT), a
							dec bc
							ld a, b
							or c
							jr nz, -
    
							ret	



; Clears user specified area of ram (can be $0000 - $FFFF)
; Parameters: hl = ram address
;			  bc - size to clear (in bytes)

; Affects: 	  a, hl, bc
ClearRAM:					xor a
							ld (hl), a
							dec bc
							ld a, b
							or c
							jr nz, ClearRAM
							
							ret
							
							
; Outs ascii text to vdp. Copied and modified from Maxim
; $FF = end of text.
; Parameters: hl = text address 	

; Affects:	  a, hl
WriteTextToVDP:				ld a,(hl)
						    cp $ff
						    ret z
						    out (VDP_DATA_PORT),a
						    xor a
						    out (VDP_DATA_PORT),a
						    inc hl
						    jr WriteTextToVDP
						    
						
						
CallHL:                     jp (hl)





/* ===================================================================================================
 * ** 256 Long Block of outi
 * - Call as follows; call OutBlock - n (n being number of outi to complete)
 */                   
                            .repeat 256
                            outi
                            .endr
OutBlock:                   ret
                            



genericPushInterrupt:         
                            in a, (VDP_CONTROL_PORT)
                            inc sp   ; << event handler
                            inc sp
                            ei 
                            reti
genericPushInterruptEnd:

.ends
