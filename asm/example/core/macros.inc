


; == VDPRegisterWrite
; ===================================================================================================

; Designed to take 2+ arguments.
; \1  == vdp register to write to
; \2 == value to write. 
.macro VDPRegisterWrite
        
                            ld a, \2
                            out (VDP_CONTROL_PORT), a
                            ld a, $80 | \1
                            out (VDP_CONTROL_PORT), a
    
.endm


.macro CartRam0Enable

                            ld a, %0001000
                            ld ($FFFC), a
                            
.endm


.macro CartRam1Enable

                            ld a, %00001100
                            ld ($FFFC), a
                            
.endm


.macro CartRamDisable

                            xor a
                            ld ($FFFC), a
                            
.endm


.macro Slot1BankSwitch

                            ld a, \1
                            ld ($FFFE), a

.endm


.macro Slot2BankSwitch
    
                            ld a, \1
                            ld ($FFFF), a
        
.endm


.macro Slot1BufferedBankSwitch
    
                            ld a, \1
                            ld ($FFFE), a
                            ld (pageBuffer1), a

.endm


.macro Slot2BufferedBankSwitch
    
                            ld a, \1
                            ld ($FFFF), a
                            ld (pageBuffer2), a

.endm


.macro WriteToVDP 

                            ld hl, \1
                            call SetVDPAddress
                            
                            ld hl, \2
                            ld bc, \3
                            call CopyToVDP
    
.endm


.macro WriteTextToVDP

                            ld hl, \1
                            call SetVDPAddress
                            
                            ld hl, \2
                            call WriteTextToVDP
                            
.endm


.macro ClearVRAM

                            ld hl, \1                                        
                            ld bc, \2
                            call ClearVRAM  
                            
.endm


; Speed Code Version
.macro WriteToVDP_SC 

                            ld a, \1 # 256
                            out (VDP_CONTROL_PORT), a
                            ld a, \1 / 256  ; ??? need to test?
                            out (VDP_CONTROL_PORT), a
                            
                            ld hl, \2
                            ld bc, \3
-:                          ld a, (hl)   
                            out (VDP_DATA_PORT), a
                            inc hl       
                            dec bc
                            ld a, b
                            or c
                            jr nz, -
    
.endm


.macro IndiectionHL        

                            ld a, (hl)
                            inc hl
                            ld h, (hl)
                            ld l,a
                            
.endm


.macro IndiectionDE        

                            ld a, (de)
                            inc de
                            ld d, (de)
                            ld e,a
                            
.endm
                            
                            
.macro LoadInterruptHandler 

                            ld hl, \1
                            ld de, InteruptHandler
                            ld bc, \2
                            ldir
                            
.endm


.macro CopyToRAM 

                            ld hl, \2
                            ld de, \1
                            ld bc, \3
                            ldir
                            
.endm

.macro ClearRAM
                            ld hl, \1
                            ld bc, \2
                            ld d, 0
                            
                            
-:                          ld (hl), d
                            inc hl
                            dec bc
                            ld a, b
                            or c
                            jr nz, -
.endm

.macro RepeatNOP

                            .repeat \1
                            nop
                            .endr
                            
.endm


.macro InitaliseVDPRegisters

                            ld hl, VDPInitData                                  ; Initialize VDP Registers
                            ld b, VDPInitDataEnd-VDPInitData
                            ld c, VDP_CONTROL_PORT
                            otir
                            
.endm

.macro InitaliseGeneral
                            ld a, $C3                   ; Put jp instruction in ram (following two bytes are smc address)
                            ld (RAMJump), a
.endm

.bank 0 slot 0
.section "!vdp init data" free

VDPInitData:                
.db $06, $80, $00, $81, $ff, $82, $ff, $85, $ff, $86, $00, $87, $00, $88, $00, $89, $ff, $8a
VDPInitDataEnd:

.ends



