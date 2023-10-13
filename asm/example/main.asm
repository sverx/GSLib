
.memorymap
defaultslot 0
slotsize $4000
slot 0 $0000
slot 1 $4000
slot 2 $8000
slot 3 $c000
.endme
  

.rombankmap
bankstotal 4
banksize $4000
banks 4
.endro


.sdsctag 0.1,"GSL", "Generic Scroll Engine","Psidum" 



; == RAM SETUP
; =====================================

; stack pointer @ $dff0 < - space given for 60 16 bit entries.
; interrupt handler @ de7f > - 256 bytes given.
.define InteruptHandler     $DE80
.define CoreSupport         $DE7F
.define ScratchPad          $DE00 ; used as generic ram space for various routines! 64 bytes
.define RAMJump             $DDFD
.define EffectRAM           $DDFC ; effects will use memory $DDFC descending

.enum CoreSupport desc      ; general hardware
    pageBuffer1 db          ; paging (everdrive bug on slot 0. only 1 & 2)
    pageBuffer2 db
    stackSwapBufffer dw     ; used to store stack location when using stack for other things.
.ende


; == INCLUDES GENERIC
; =====================================
.include "core/defines.inc"
.include "core/interupts.inc"
.include "core/generic_routines.inc"
.include "core/macros.inc"
.include "libs/GSLib_1.0.inc"
.include "libs/aplib-z80-fast.inc"



; == RESOURCES
.include "resources/Graphics.inc"
.include "resources/ScrollData.inc"

; == ASCII SETUP
; =====================================
.asciitable
map " " to "~" = 0
.enda


; == START UP
; =====================================
.bank 0 slot 0
.section "Initialize SMS" free

InitializeSMS:              ld sp, $DFF0                       ; set up stack pointer


                            ; == Initialize System
                            VDPRegisterWrite 1, DISPLAY_OFF
                            InitaliseVDPRegisters
                            InitaliseGeneral
                            ClearVRAM $4000, $4000
                            ClearVRAM $C000, $0032
                            
                            
                            ; == write interrupt handler to ram
                            ; Basic interrupt for testing, returns to address last pushed on stack.
                            LoadInterruptHandler genericPushInterrupt, genericPushInterruptEnd - genericPushInterrupt  
                            
                            
                            ; == Load Tiles and Palette
                            Slot2BankSwitch :PhantasyStarTiles
                            WriteToVDP $4000, PhantasyStarTiles, PhantasyStarTilesEnd - PhantasyStarTiles
                            WriteToVDP $C000, PhantasyStarPalette, PhantasyStarPaletteEnd - PhantasyStarPalette
                            
                            CopyToRAM $C000, Scrolltable, ScrolltableEnd - Scrolltable
                            
                            
                            ex af, af'
                            xor a
                            ex af, af'
                            
                            ; == Initalise Scrolltable.
                            ; HL = Address of Scrolltable Data
                            ; DE = RAM Location to generate LUT for this Map (LUT used to speed metatile lookups).
                            ; BC = Address of Metatile Data
                            ld hl, $C000
                            ;ld de, $D000 
                            ld bc, Metatiles
                            call GSL_InitialiseMap
                            
                            
                            ; == Position Window at location (0,0) Top left most.
                            ; HL = Y
                            ; DE = X
                            ld bc, 106
                            ld hl, 106
                            call GSL_PositionWindow
                            
                            
                            ; == Update Screen Contents 
                            ; Only needs to be called at beginning or when you modify position manually using GSL_PositionWindow.
                            call GSL_RefreshScreen
                            
                            
--:                         ; **** Game Loop Begins!
                            ; == Call ActiveDisplayRoutine each frame after vblank has occurred (can be during vblank if afte vblank routine!)
                            call GSL_ActiveDisplayRoutine
                            
                             
                            ; == EXAMPLE OF METATILE LOOKUP
                            ; Just an example of use, we don't actually do anything with it.
                            
                            ; For testing purposes I have map on rom however if you have map in ram you can modify 
                            ; the metatile entry in scrolltable and call for that update to be updated on screen!
                            
                            ; @in HL: Y
                            ; @in DE: X
                            ; @out A: Metatile ID
                            ; @out HL: Address of Metatile in Scrolltable.
;                            ld bc, 128
;                            ld hl, 96
;                            call GSL_MetatileLookup
;                            
;                            ex af, af'
;                            ld (hl), a
;                            ex af, af'
;                            
;                            call GSL_MetatileUpdate
;                            
;                            
;                            ld bc, 96
;                            ld hl, 100
;                            call GSL_MetatileLookup
;                            
;                            ex af, af'
;                            ld (hl), a
;                            inc a
;                            ex af, af'
;                            
;                            call GSL_MetatileUpdate
;                            
;                            
;                            ; == EXAMPLE OF TILE LOOKUP
;                            ; Just an example of use, we don't actually do anything with it.
;                            
;                            ; @in HL: Y
;                            ; @in DE: X
;                            ; @out HL: Nametable Entry
;                            ld hl, 24
;                            ld de, 24
;                            call GSL_TileLookup
;                            
;                            
                            ; == Active Interrupt For Next Vblank (not related to scroll table).
                            ld hl, _vblankInterrupt
                            push hl
                            VDPRegisterWrite 0, SCROLL_MASK_ON
                            VDPRegisterWrite 1, DISPLAY_ON | FRAME_INTERRUPT_ON
                            in a, (VDP_CONTROL_PORT)
                            ei
                            -: halt
                            jr -
                            
                            
                            
                            ; == VBLANK Interrupt has occoured!
_vblankInterrupt:           di

                            ; == Call GSL_VBlankRoutine every vblank.
                            call GSL_VBlankRoutine
                            
                            
                            ; *** BELOW IS SOME GENERIC CODE TO TAKE IN CONTROLER INPUT AND SCROLL SCREEN
                            ; When we want to scroll we write signed values to 
                            ; GSL_YUpdateRequest and GSL_XUpdateRequest values -8 to 8.
                            
                            ; == process user input
                            in a, ($dc)                         ; get joypad 1 input      
                            ld d, a                      
                            
_joypad_test_b2:            ; == Test input for reset (button 2)
                            bit 5, a                            ; condition ::  button 2 pressed?
                            jp z, $0000                         ; yes :: reset console to menu
                            
                            

                            ; == scrolling.
_joypad_test_left1:         bit 2, d                            ; condition :: is left pressed?
                            jr nz, _joypad_test_right1                    
                            
                                ld hl, (GSL_X)
                                ld a, h
                                or l
                                jp z, _joypad_test_up1
                                ld a, -1
                                ld (GSL_XUpdateRequest), a      ; PUT -8 INTO GSL_XUpdateRequest for LEFT SCROLL
                                jp _joypad_test_up1
              
              
_joypad_test_right1:        bit 3, d                            ; condition :: is right pressed?
                            jr nz, _joypad_test_up1
                                
                                ld hl, (GSL_X)
                                ld bc, 256
                                add hl, bc
                                ld bc, (GSL_WidthInPixels)
                                sbc hl, bc
                                ld a, h
                                or l
                                jp z, _joypad_test_up1
                                ld a, 1
                                ld (GSL_XUpdateRequest), a      ; PUT 8 INTO GSL_XUpdateRequest for RIGHT SCROLL
                                    
                                    
_joypad_test_up1:           bit 0, d                            ; condition :: is up pressed?
                            jr nz, _joypad_test_down1                    
                            
                                ld hl, (GSL_Y)
                                ld a, h
                                or l
                                jp z, _joypad_test_end
                                ld a, -1
                                ld (GSL_YUpdateRequest), a      ; PUT -8 INTO GSL_YUpdateRequest for UP SCROLL
                                jp _joypad_test_end
                                
                                
_joypad_test_down1:           bit 1, d                            ; condition :: is up pressed?
                              jr nz, _joypad_test_end                    
                                
                                ld hl, (GSL_Y)
                                ld bc, 192
                                add hl, bc
                                ld bc, (GSL_HeightInPixels)
                                sbc hl, bc
                                ld a, h
                                or l
                                jp z, _joypad_test_end              ; PUT 8 INTO GSL_YUpdateRequest for DOWN SCROLL
                                ld a, 1
                                ld (GSL_YUpdateRequest), a                    


_joypad_test_end:            


                            ; == VBLANK is done, loop back to Active Display Code.
                            jp --

.ends



