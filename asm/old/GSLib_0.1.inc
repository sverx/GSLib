

/*
 * GSLib : Generic Scroll Engine by Psidum
 * 
 * General purpose 8 way scrolling engine for use in games or demos. 
 * Uses 16x16 pixel Metatiles.
 * 
 * Thanks to everyone in the Sega Master System dev community.
 * Special thanks to Calindro, Kagesan, Efry, Maxim, Sverx, Bock, vbt and all the rest!
 * 
 */



.section "GSLib" free

.define GSL_GENERAL_RAM                 $C000       ; location of general purpose ram
.define GSL_HORIZONTAL_ADDITION         $0F         ; addition value required to push scrolltable pointer to right size of screen 
.define GSL_NAMETABLE_BASE_ADDRESS      $7800       ; base address of nametable in vram including write bit set (14) 
.define GSL_NAMETABLE_HIGH_BYTE_START   $78
.define GSL_NAMETABLE_HIGH_BYTE_END     $7F  
.define GSL_NAMETABLE_WIDTH_IN_BYTES    $40
.define GSL_MAX_MAP_HEIGHT_IN_METATILES 192
.define VDP_DATA_PORT                   $BE
.define VDP_CONTROL_PORT                $BF                


.enum GSL_GENERAL_RAM asc
        
        GSL_Scrolltable                 dw     
        ;GSL_ScrolltableRowLUT              dw          
        
        ; == Following is stored in scrolltable header.
        GSL_ScrolltableSize             dw
        GSL_WidthInMetatiles            dw          
        GSL_HeightInMetatiles           dw
        GSL_WidthInPixels               dw
        GSL_HeightInPixels              dw          
        GSL_VerticalAddition            dw          ; addition value required to push scrolltable point to bottom left.
        GSL_OptionByte                  db
        
        ; == Following is internal working ram
        GSL_ScrolltableCurrentPosition  dw          ; top-left position of current screen in scrolltable
        GSL_NametableCurrentPosition    dw          ; top-left position of current screen on vdp nametable
        GSL_MetatileTable               dw
        GSL_MetatileLookupBuffer        dw
        GSL_MetatileXBuffer             dw
        GSL_MetatileYBuffer             dw
        GSL_X                           dw          ; current x point in scrolltable
        GSL_Y                           dw          ; current y point in scroll table
        GSL_Y224                        db          ; stores Y%224. Needed to speed things up.
        GSL_YRefresh                    db          ; used in refresh window call for speed
        GSL_XUpdateRequest              db          ; requested x adjustment (signed)
        GSL_YUpdateRequest              db          ; requested y adjustment (signed)
        GSL_XUpdateStatus               db          ; represents required updates if any
        GSL_YUpdateStatus               db          ; ^^
        GSL_NametableUpdateColumn       dw          ; first entry in nametable for column update
        GSL_NametableUpdateRow          dw          ; first entry in nametable for row update
        GSL_NATColumnBuffer             dsb 64
        GSL_NATRowBuffer                dsb 72  
        GSL_MetatileUpdates             dsb 80      ; you need 10 bytes per maximum update, default here is 80 = 8 update!
        GSL_MetatileUpdatesCount        db
        GSL_MetatileUpdatesAddress      dw
        GSL_RAMJump                     dsb 3
        GSL_CollisionCount              db
        GSL_ScrolltableRowLUT           dsb GSL_MAX_MAP_HEIGHT_IN_METATILES * 2
        
.ende




/*
 * == Initializes GSLlib for rendering of new map
 * @in hl: Scrolltable Location
 * @in bc; Metatile Location
 */
GSL_InitialiseMap:          ; == Store GSL_ScrolltableRowLUT
                            ;ld (GSL_ScrolltableRowLUT), de
                            
                            ; == Store MetatileTable Address
                            ld (GSL_MetatileTable), bc
                            
                            ; == Store CollisionCount (3rd byte in metatile table)
                            inc bc
                            inc bc
                            inc bc
                            ld a, (bc)
                            ld (GSL_CollisionCount), a

                            ; == Copy Scrolltable Header to Ram (its structure is identical to enum)
                            ld de, GSL_GENERAL_RAM + 2
                            ld bc, 13
                            ldir
                            
                            ; == Store GSL_Scrolltable (ldir will have pushed past header to actual table)
                            ld (GSL_Scrolltable), hl

;                            ld a, (GSL_OptionByte)              ; bit 0 set = Metatile LUT Required.
;                            bit 0, a                            ; proceed only if set
;                            jr z, +                             
                            
                            ld iy, GSL_ScrolltableRowLUT
                            ld hl, (GSL_Scrolltable)
                            ld bc, GSL_MAX_MAP_HEIGHT_IN_METATILES
                            ld de, (GSL_WidthInMetatiles)
                            
_GenerateRowLUT:            ld (iy+0), h                        ; == Create Table Containing Start Address of Metatile Rows 
                            inc iy                              ; lut_entry = Scrolltable Location
                            ld (iy+0), l                        ; loop (height in metatiles)
                            inc iy                              ; { 
                            add hl, de                          ;   write lut_entry
                            dec bc                              ;   lut_entry += width in metatiles
                            ld a, b                             ; }
                            or c
                            jr nz, _GenerateRowLUT
                            
                            ; == Initialize Ram
                            ld hl, GSL_MetatileUpdates
                            ld (GSL_MetatileUpdatesAddress), hl
                            
                            xor a
                            ld (GSL_MetatileUpdatesCount), a
                            ld (GSL_XUpdateRequest), a
                            ld (GSL_YUpdateRequest), a
                            
                            ; == Set up GSL_RAMJump
                            ld a, 195  ; $C3
                            ld (GSL_RAMJump), a

                            ret




/* == Sets Internal Settings for Specified Window Position
 * 
 * - Stores X,Y
 * - Creates Pointers based on X,Y
 * 
 * TODO: 
 * - Only single modulo is needed!
 * - General Clean Up!
 */                                
GSL_PositionWindow:         ; **** Generate GSL_ScrolltableCurrentPosition
                            ; == Store (x, y) in ram
                            ld (GSL_Y), hl
                            ex de, hl
                            ld (GSL_X), hl
                            ex de, hl
                            
                            ; == Get Scrolltable location based on (x, y) and store as pointer
                            call GSL_MetatileLookup
                            ld (GSL_ScrolltableCurrentPosition), hl
                            
                            
                            ; **** Generate GSL_NametableCurrentPosition (vram pointer representing top left of window)
                            ; == Divide Y by 8
                            ld hl, (GSL_Y)
                            srl h                   ; divide y by 8 (get tile row)
                            rr l
                            srl h
                            rr l
                            srl h
                            rr l
                            
                            ; == Modulo 28 (height of nametable in tiles)
                            ld e, 28                
                            ld b, 8
_Modulo_1_Loop:              adc hl, hl
                            ld a, h
                            jr c, _Modulo_1_Carry1
                            cp e
                            jr c, _Modulo_1_Carry2
_Modulo_1_Carry1:           sub e
                            ld h, a
                            or a
_Modulo_1_Carry2:           djnz _Modulo_1_Loop
                            ld a, l
                            rla
                            cpl
                            
                            ; == Multiply by 64 (length of data in nametable row)
                            ld l, h                 
                            xor a
                            ld d, a                     ; this is for next step!
                            ld h, a
                            add hl, hl              
                            add hl, hl
                            add hl, hl
                            add hl, hl
                            add hl, hl
                            add hl, hl
                            
                            ; == Add X worth of nametable entries.
                            ; Divide by 8 then << 1 (below is optimization of this)
                            ld a, (GSL_X)
                            and 248 ; %11111000
                            rra
                            rra
                            ld e, a
                            add hl, de
                            
                            ; == Add vdp base address of nametable 
                            ld de, GSL_NAMETABLE_BASE_ADDRESS       
                            add hl, de
                            
                            ; == Store GSL_NametableCurrentPosition
                            inc hl
                            inc hl
                            ld (GSL_NametableCurrentPosition), hl     
                            
                            
                            ; **** Generate GSL_Y224
                            ; == Y Modulo 224 (height of window in pixels)
                            ld hl, (GSL_Y)
                            ld e, 224             
                            ld b, 8
_Modulo_2_Loop:                          adc hl, hl
                            ld a, h
                            jr c, _Modulo_2_Carry1
                            cp e
                            jr c, _Modulo_2_Carry2
_Modulo_2_Carry1:           sub e
                            ld h, a
                            or a
_Modulo_2_Carry2:           djnz _Modulo_2_Loop
                            ld a, l
                            rla
                            cpl
                            
                            ; == Store GSL_Y224
                            ld a, h
                            ld (GSL_Y224), a
                            
                            ret
                            
                            
                            
                            
 /* == Updates Nametable contents (based on current internal window possition)
 * 
 * - Active Display Safe
 *
 * TODO:
 * - Optimizations are possible.
 */                             
GSL_RefreshScreen:          ; == Set VDP Nametable Pointer
                            ld hl, (GSL_NametableCurrentPosition)
                            ld (GSL_NametableUpdateRow), hl

                            
                            ; == Keep a Modified Y to reflect rows being updated
                            ld hl, (GSL_Y)
                            ld (GSL_YRefresh), hl ; using this as temp ram entry
                            
                            ld a, (GSL_Y224)
                            srl a
                            srl a
                            srl a
                            sub 28
                            ld (GSL_NATColumnBuffer), a
                            
                            ld a, 28
                            ld (GSL_NATColumnBuffer + 1), a
                            
                            
_RefreshLoop:               ; == Set VDP Pointer at Start of Row
                            ld hl, (GSL_NametableUpdateRow)
                            ld c, VDP_CONTROL_PORT
                            out (c), l
                            out (c), h
                            
                            ; == Lookup Metatile for this Row
                            ld hl, (GSL_YRefresh)
                            ld de, (GSL_X)
                            call GSL_MetatileLookup
                            
                            ; == Store address in iy
                            ld a, h
                            ld iyh, a
                            ld a, l
                            ld iyl, a
                            
                            ; == Put Y Offset in to ixl and Unroll a row of Nametable data.
                            ld a, (GSL_YRefresh)
                            and 8 ; %00001000
                            rrca
                            ld ixl, a
                            call _unrollRowForNTUpdate          ; Unroll buffer table column for nametable updates!
                            
                            ; == Add Offset to Start location in Buffer.
                            ld a, (GSL_X)
                            add a, 8
                            and 8 ; %00001000
                            rrca
                            rrca
                            ld hl, GSL_NATRowBuffer
                            ld e, a
                            xor a
                            ld d, a
                            add hl, de
                            
                            ; == Calculate Number of Bytes to be Written in first Write, store in b.
                            ld a, (GSL_X)
                            add a, 8
                            and 248 ; %11111000
                            rrca
                            rrca
                            ld d, a
                            ld a, 64
                            sub d
                            and 63 ; %00111111
                            ld d, a
                            
                            ; == Write Buffered Data to VDP Nametable
                            ld c, VDP_DATA_PORT
_WriteRow1Loop:             outi
                            dec d
                            jp nz, _WriteRow1Loop
                            
                            ; == Set VDP Address to Start of Row for Second Write
                            ld de, (GSL_NametableUpdateRow)
                            ld c, VDP_CONTROL_PORT
                            ld a, 192 ; %11000000
                            and e
                            out (c), a
                            out (c), d
                            
                            ; == Calculate Number of Bytes to be Written in scond Write, store in b.
                            ld a, (GSL_X)
                            add a, 8
                            and 248 ; %11111000
                            rrca
                            rrca
                            ld d, a
                            ld c, VDP_DATA_PORT
                            
                            ; == Write Buffered Data to VDP Nametable
_WriteRow2Loop:             outi
                            dec d
                            jp nz, _WriteRow2Loop
                            
                            ; **** Prep for Next Write.
                            ; == Push VDP Nametable address to next Row.
                            ld hl, (GSL_NametableUpdateRow)
                            ld de, GSL_NAMETABLE_WIDTH_IN_BYTES
                            add hl, de
                            ld (GSL_NametableUpdateRow), hl
                            
                            
                            ; == Have we written rows? Return if true;
                            ld a, (GSL_NATColumnBuffer + 1)
                            dec a
                            ret z
                            ld (GSL_NATColumnBuffer + 1), a
                            
                            ; == Is next row at top of VDP Nametable, reset Address if true;
                            ld a, (GSL_NATColumnBuffer)
                            inc a
                            ld (GSL_NATColumnBuffer), a
                            jp nz, _AdjustTempYValue
                            ld hl, (GSL_NametableUpdateRow)
                            ld h, GSL_NAMETABLE_HIGH_BYTE_START
                            ld a, 63 ; %00111111
                            and l
                            ld l, a
                            ld (GSL_NametableUpdateRow), hl
                            
                            
                            ; == Adjust Temp Y Value for Next Write.
_AdjustTempYValue:          ld hl, (GSL_YRefresh)
                            ld d, 0
                            ld e, 8
                            add hl, de
                            ld (GSL_YRefresh), hl
                            
                            jp _RefreshLoop
                            
                            
                            
                            
/* == Retreives from Scrolltable, Metatile ID and address
 * 
 * - Stores address in buffer GSL_MetatileLookupBuffer.
 * - Requires GSL_MetatileLUT to have been created.
 * - GSL_MetatileLUT has Scrolltable base address baked in so no need do add this!
 * 
 * @in hl: Y
 * @in de: X
 * 
 * @out a:  metatile id
 * @out hl: address of metatile in scrolltable
 * 
 */
GSL_MetatileLookup:         ; == Convert Y to LUT Entry and retrieve value
                            ld (GSL_MetatileXBuffer), de
                            ld (GSL_MetatileYBuffer), hl
                            ld a, 240 ; %11110000
                            and l
                            srl h
                            rra
                            srl h
                            rra
                            srl h
                            rra
                            ld l, a
                            
                            ld bc, GSL_ScrolltableRowLUT
                            add hl, bc
                            ld a, (hl)                      ; indirection      
                            inc hl                       
                            ld l, (hl)                   
                            ld h, a    


                            ; == Divide X by 16 then add to above address
                            ld a, e
                            srl d
                            rra
                            srl d
                            rra
                            srl d
                            rra
                            srl d
                            rra
                            ld e, a
                            add hl, de
                            
                            ; == Put Metatile ID in a and store address in buffer.
                            ld a, (hl)
                            ld (GSL_MetatileLookupBuffer), hl
                            
                            ret
                            
                            
                            
                            
/* == Retreives from Scrolltable, Nametable Entry
 * 
 * - Stores address of surrounding metatile in buffer GSL_MetatileLookupBuffer.
 * - Requires GSL_MetatileLUT to have been created.
 * - GSL_MetatileLUT has Scrolltable base address baked in so no need do add this!
 * 
 * @in hl: Y
 * @in de: X
 * 
 * @out hl: Nametable entry
 */                      
GSL_TileLookup:             ; == Create offset for within Metatile (which nametable entry does x,y point to)
                            ld a, 1 ; %00000001
                            and l
                            rla
                            rla
                            ld c, a
                            ld a, 1 ; %00000001
                            and e
                            rla
                            or c
                            push af
                            
                            ; == Retrieve Metatile ID
                            call GSL_MetatileLookup
                            
                            ; == Resolve in to Metatile Address - ((ld << 3) + Offset + GSL_METATILE_TABLE)
                            ld l, a
                            xor a
                            ld h, a
                            add hl, hl
                            add hl, hl
                            add hl, hl
                            pop af
                            add a, l
                            ld l, a
                            ld de, (GSL_MetatileTable)
                            add hl, de
                            
                            ; == Get Nametable entry using resolved address
                            ld a, (hl)                      ; indirection      
                            inc hl
                            ld h, (hl)
                            ld l, a
                            
                            ret
                            
                            
                            

/* == Update user defined Metatile Entry to VDP.
* This is used in cases where user knows the x,y, and array index of metatile to update.
* Useful for animation type stuff.
* NOTE: this is a more advanced feature that puts the onus on the coder to manage properly.
* 
* @in hl: Y
* @in de: X
* 
* @in bc: Srolltable Array Offset (pointing at entry to be updated)
*/
 
GSL_MetatileUpdateSpecific: ; == Populate Buffered Values with Custom Values.
                            ld (GSL_MetatileYBuffer), hl
                            ld (GSL_MetatileXBuffer), de
                            
                            ; == Add Array offset to Scrolltable bass address then store results.
                            ld hl, (GSL_Scrolltable)
                            add hl, bc
                            ld (GSL_MetatileLookupBuffer), hl
                            
                            ; == Flow to GSL_MetatileUpdate Below 
                            
                          
                          
                          
/* == Write last accessed Scrolltable Metatile entry to VDP.
 * 
 * NOTE: This Routine assumes that the scrolltable entry in question is in current display.
 * If you call this on an entry that is not in the current window space it will update
 * the metatile relative to current display creating garbage
 * 
 * In Short... ONLY CALL THIS ROUTINE IF YOU ARE SURE THE SCROLLTABLE ENTRY IS CURRENTLY BEING
 * DISPLAYED IN FULL ON SCREEN!
 * 
 * - GSL_MetatileLookup stores buffer of last entry looked up, this routine uses this to update
 * last looked up entry.
 */                          

GSL_MetatileUpdate:         ; == Y >> 4
                            ld hl, (GSL_MetatileYBuffer)
                            ld a, l
                            srl h
                            rra
                            srl h
                            rra
                            srl h
                            rra 
                            srl h 
                            rra
                            ld l, a
                            
                            ; == Add GSL_ModTable base table address 
                            ld de, GSL_ModTable
                            add hl, de
                            
                            ; == Indirection + Shifts to get address of Y Row in Nametable
                            ld h, (hl)
                            xor a
                            ld l, a
                            ld d, a             ; Prep for X
                            srl h
                            rr l
                            
                            ; == Isolate metatile location in X and add
                            ld a, (GSL_MetatileXBuffer)
                            and 240 ; %11110000
                            rra
                            rra
                            ld e, a
                            add hl, de
                            
                            ; == Write Resolved Address of Metatile on VDP (including register high bits)
                            ex de, hl
                            ld hl, (GSL_MetatileUpdatesAddress)
                            ld (hl), e
                            inc hl
                            ld (hl), d
                            inc hl
                            ex de, hl
                            
                            ; == Resolve Metatile Location from ID
                            ld hl, (GSL_MetatileLookupBuffer)
                            ld l, (hl)
                            ld a, l
                            and 7 ; %111                          
                            ;or GSL_METATILE_TABLE_HIGH_BYTE     ; add high bits of metatile address (table must be on 2k boundary)
                            ld h, a
                            ld a, l
                            and 248 ; %11111000
                            ld l, a   
                            ld bc, (GSL_MetatileTable)
                            add hl, bc
                            
                            ; == Copy Contents of Metatile To Ram Buffer
                            ldi
                            ldi
                            ldi
                            ldi
                            ldi
                            ldi
                            ldi
                            ldi
                            
                            ; == Update Count and Address for more Metatile Updates if any 
                            ld (GSL_MetatileUpdatesAddress), de
                            ld hl, GSL_MetatileUpdatesCount
                            inc (hl)
                            
                            ret
                            
                            
                            
                            
 /* == Routine to be Called During Active Display
 * 
 * - Checked XUpdateRequest + YUpdateRequest Status for updates
 * - Uses pointers to speed up nametable and scrolltable lookups, updates.
 * - Checks boundary crosses and updates pointers as required.
 * 
 * NOTES: Somewhat Optimized.
 */                              
GSL_ActiveDisplayRoutine:   ; == Initialize
                            ld iy, GSL_GENERAL_RAM
                            xor a
                            ld (GSL_XUpdateStatus), a
                            ld (GSL_YUpdateStatus), a
                            
                            
_xScroll:                   ; == Check Left / Right Scroll... 
                            ld a, (GSL_XUpdateRequest)              
                            and a                               
                            jp z, _yScroll                      ; no value = no horizontal scroll
                            jp m, _leftScroll                   ; signed = left scroll.
                                  
                                     
_rightScroll:               ; == Update currentXScroll Value.
                            ld hl, (GSL_X)                      ; add to current x value
                            ld b, l
                            add a, l
                            jr nc, _noRightHighByteCarry
                            inc h
_noRightHighByteCarry:      ld l, a
                            xor b
                            ld d, a                             ; store low byte different pre / post scroll (for boundary tests)
                            ld (GSL_X), hl      
                                   

_rightNametableCheck:       ; == Check for NameTable Boundary Cross
                            and 248 ; %11111000
                            jp z, _yScroll
                            
                            ; == NameTable Bounary Crossed... Process
                            ld (iy + GSL_XUpdateStatus - GSL_GENERAL_RAM), 1       ; Flag boundary cross
                            
                            ld a, (GSL_NametableCurrentPosition)            ; Update NameTable Pointer
                            ld c, a                             ; (addition bound within low 6 bits!!!)
                            and 192 ; %11000000                       
                            ld b, a
                            ld a, c
                            add a, 2
                            and 63 ; %111111
                            or b
                            ld (GSL_NametableCurrentPosition), a


_rightScrolltableCheck:     ; == Check for BufferTable Boundary Cross
                            ld a, d                                 
                            and 240 ; %11110000                    
                            jp z, _yScroll
                            
                            ; == BufferTable Boundary Crossed... Process
                            ld hl, (GSL_ScrolltableCurrentPosition)          ;  Update buffer table pointer.
                            inc hl
                            ld (GSL_ScrolltableCurrentPosition), hl

                            jp _yScroll

                                                    
_leftScroll:                ; == Update currentXScroll Value.
                            ld hl, (GSL_X)                      ; add to current x value
                            ld b, l
                            add a, l
                            jr c, _noLeftHighByteCarry
                            dec h
_noLeftHighByteCarry:       ld l, a
                            xor b
                            ld d, a                             ; store low byte different pre / post scroll (for boundary tests)
                            ld (GSL_X), hl             
                            
_leftNametableCheck:        ; == Check for NameTable Boundary Cross
                            and 248 ; %11111000
                            jp z, _yScroll
                            
                            ; == NameTable Bounary Crossed... Process
                            ld (iy + GSL_XUpdateStatus - GSL_GENERAL_RAM), 129
                            
                            ld a, (GSL_NametableCurrentPosition)        ; Update name table pointer.
                            ld c, a                             ; (subtraction bound within low 6 bits!!!)
                            and 192 ; %11000000                       
                            ld b, a
                            ld a, c
                            sub 2
                            and 63 ; %111111
                            or b
                            ld (GSL_NametableCurrentPosition), a
                            
                            
_leftScrolltableCheck:      ; == Check for BufferTable Boundary Cross
                            ld a, d                                 
                            and 240 ; %11110000                       
                            jp z, _yScroll
                            
                            ; == BufferTable Boundary Crossed... Process
                            ld hl, (GSL_ScrolltableCurrentPosition)          ;  Update buffer table pointer.
                            dec hl
                            ld (GSL_ScrolltableCurrentPosition), hl
                                

_yScroll:                   ; == Check Up / Down Scroll... 
                            ld a, (GSL_YUpdateRequest)
                            and a                           
                            jp z, _processUpdateBuffers          ; no value = no vertical scroll
                            jp m, _upScroll                     ; signed = left scroll.


_downScroll:                ; == Update dummyYScroll Value (wraps around 224 NameTable height!)
                            ld b, a
                            ld a, (GSL_Y224)
                            add a, b
                            cp 224
                            jp c, _downNoCrossBoundary
                            sub 224
_downNoCrossBoundary:       ld (GSL_Y224), a
                            ld a, b
                            
                            ; == Update currentYScroll Value.
                            ld hl, (GSL_Y)             ; add to current y value
                            ld b, l
                            add a, l
                            jr nc, _noDownHighByteCarry
                            inc h
_noDownHighByteCarry:       ld l, a
                            xor b
                            ld d, a                             ; store low byte different pre / post scroll (for boundary tests)
                            ld (GSL_Y), hl             
                            
                            
_downNametableCheck:        ; == Check for NameTable Boundary Cross
                            and 248 ; %11111000
                            jp z, _processUpdateBuffers 
                            
                            ; == NameTable Bounary Crossed... Process
                            ld (iy + GSL_YUpdateStatus - GSL_GENERAL_RAM), 1 ; ????
                                                
                            ld hl, (GSL_NametableCurrentPosition)                  ; Update name table pointer.
                            ld bc, 64                           
                            add hl, bc
                            ld a, GSL_NAMETABLE_HIGH_BYTE_END                                      ; TODO shift this to .defines for variable nat positioning
                            cp h
                            jp nz, _downNoResetNTHighByte
                            ld h, GSL_NAMETABLE_HIGH_BYTE_START
_downNoResetNTHighByte:     ld (GSL_NametableCurrentPosition), hl

_downScrolltableCheck:      ; == Check for BufferTable Boundary Cross
                            ld a, d                                 
                            and 240 ; %11110000                       ; !SMC metatile width bitmask
                            jp z, _processUpdateBuffers
                            
                            ; == BufferTable Boundary Crossed... Process
                            
                            ld hl, (GSL_ScrolltableCurrentPosition)
                            ld bc, (GSL_WidthInMetatiles)
                            add hl, bc
                            ld (GSL_ScrolltableCurrentPosition), hl

                            jp _processUpdateBuffers
                            
                            
_upScroll:                  ; == Update dummyYScroll Value (wraps around 224 NameTable height!)
                            ld b, a
                            ld a, (GSL_Y224)
                            add a, b
                            jp c, _upNoCrossBoundary
                            sub 32
_upNoCrossBoundary:         ld (GSL_Y224), a
                            ld a, b
                            
                            ; == Update currentYScroll Value.
                            ld hl, (GSL_Y)             ; add to current y value
                            ld b, l
                            add a, l
                            jr c, _noUpHighByteCarry
                            dec h
_noUpHighByteCarry:         ld l, a
                            xor b
                            ld d, a                             ; store low byte different pre / post scroll (for boundary tests)
                            ld (GSL_Y), hl             
                            
_upNametableCheck:          ; == Check for NameTable Boundary Cross
                            ld a, d
                            and 248 ; %11111000
                            jp z, _processUpdateBuffers

                            ; == NameTable Bounary Crossed... Process
                            ld (iy + GSL_YUpdateStatus - GSL_GENERAL_RAM), 129
                            
                            ld hl, (GSL_NametableCurrentPosition)                  ; Update name table pointer.
                            ld bc, 64
                            or a
                            sbc hl, bc
                            ld a, GSL_NAMETABLE_HIGH_BYTE_START - 1
                            cp h
                            jp nz, _upNoResetNTHighByte
                            ld h, GSL_NAMETABLE_HIGH_BYTE_END - 1
_upNoResetNTHighByte:       ld (GSL_NametableCurrentPosition), hl

_upScrolltableCheck:        ; == Check for BufferTable Boundary Cross
                            ld a, d                                 
                            and 240 ; %11110000                       ; !SMC metatile width bitmask
                            jp z, _processUpdateBuffers
                            
                            ; == BufferTable Boundary Crossed... Process
                            
                            ld hl, (GSL_ScrolltableCurrentPosition)
                            ld bc, (GSL_WidthInMetatiles)
                            xor a
                            sbc hl, bc
                            ld (GSL_ScrolltableCurrentPosition), hl

                            
_processUpdateBuffers:       
_updateX:                   ; == Left / Right Scroll Occur?
                            ld a, (GSL_XUpdateStatus)
                            or a
                            jp z, _updateY
                            jp m, _updateBufferLeft
                            
_updateBufferRight:               ; == Create Adjusted btPointer for _unrollColumnForNTUpdate call
                            ld iy, (GSL_ScrolltableCurrentPosition)
                            ld de, 16
                            add iy, de

                            ; == Create X offset for _unrollColumnForNTUpdate call
                            ld a, (GSL_X)                    ; get x offset for meta tile. Writing column so need to adjust for this.
                            and 8 ; %00001000
                            rrca
                            rrca
                            ld ixl, a
                            
                            ; == Unroll MetaTiles and get NameTable data for update
                            call _unrollColumnForNTUpdate   
                            
                            ; == Store Adjusted NameTable Address for Update.
                            ld a, (GSL_NametableCurrentPosition)
                            ld c, a
                            and 192 ; %11000000                       ; update name table pointer.
                            ld b, a
                            ld a, c
                            sub 2
                            and 63 ; %111111
                            or b
                            ld (GSL_NametableUpdateColumn), a
                            ld a, (GSL_NametableCurrentPosition + 1)
                            ld (GSL_NametableUpdateColumn+ 1), a    
                            
                            jp _updateY
                            
                     
_updateBufferLeft:          ; == NameTable Update Required?
                            rra
                            jp nc, _updateY
                            
                            ; == Create Adjusted btPointer for _unrollColumnForNTUpdate call
                            ld iy, (GSL_ScrolltableCurrentPosition)
                            ld a, (GSL_X) 
                            add a, 8
                            and 8 ; %00001000
                            jp nz, _createXOffset
                            inc iy
                            
                            ; == Create X offset for _unrollColumnForNTUpdate call
_createXOffset:             rrca
                            rrca
                            ld ixl, a

                            ; == Unroll MetaTiles and get NameTable data for update                        
                            call _unrollColumnForNTUpdate
                                 
                            ; == Store NameTable Address for Update (no adjustment required)
                            ld hl, (GSL_NametableCurrentPosition)
                            ld (GSL_NametableUpdateColumn), hl

                           
_updateY:                   ; == Left / Right Scroll Occur?
                            ld a, (GSL_YUpdateStatus)
                            or a
                            ret z
                            jp m, _updateBufferUp
                           


_updateBufferDown:          ; == NameTable Update Required?
                            rra
                            ret nc

                            ; == Create Adjusted btPointer for _unrollColumnForNTUpdate call
                            ld iy, (GSL_ScrolltableCurrentPosition)
                            ld de, (GSL_VerticalAddition)
                            add iy, de
                            
                            ld de, (GSL_WidthInMetatiles)
                            ld a, (GSL_Y)              ; get x offset for meta tile. Writing column so need to adjust for this.
                            sub 8
                            and 8 ; %00001000
                            jp nz, _createYOffset
                            add iy, de
                            
_createYOffset:             rrca
                            ld ixl, a
                                        
                            ; == Unroll MetaTiles and get NameTable data for update                             
                            call _unrollRowForNTUpdate          
                            
                            
                            ; == Store Adjusted NameTable Address for Update.
                            ld hl, (GSL_NametableCurrentPosition)
                            ld de, 64
                            or a
                            sbc hl, de
                            ld a, GSL_NAMETABLE_HIGH_BYTE_START - 1
                            cp h
                            jp nz, _noYNametableWrap
                            ld h, GSL_NAMETABLE_HIGH_BYTE_END - 1
_noYNametableWrap:          ld (GSL_NametableUpdateRow), hl
                            
                            ret

_updateBufferUp:            ; == NameTable Update Required?
                            rrca
                            ret nc
                            
                            ; == Get btPointer for _unrollColumnForNTUpdate call
                            ld iy, (GSL_ScrolltableCurrentPosition)
                            
                            ; == Create Y offset for _unrollColumnForNTUpdate call
                            ld a, (GSL_Y)              ; get x offset for meta tile. Writing column so need to adjust for this.
                            and 8 ; %00001000
                            rrca
                            ld ixl, a
                            call _unrollRowForNTUpdate          ; Unroll buffer table column for nametable updates!
                            
                            ; == Store NameTable Address for Update (no adjustment required)
                            ld hl, (GSL_NametableCurrentPosition)
                            ld (GSL_NametableUpdateRow), hl
                            
                            ret



          
/* == Vblank call writes required updates to Nametable in vdp.
 * 
 * - Not Active Display Safe!
 */                             
GSL_VBlankRoutine:          ; == Check if row update required.
                            ld a, (GSL_YUpdateStatus)
                            and a
                            jp z, _updateColumn
                            
_writeRow:                  ; == Put VDP Pointer Address in Shadow Register and Set VDP Pointer.
                            exx
                            ld hl, (GSL_NametableUpdateRow)                  
                            ld c, VDP_CONTROL_PORT
                            out (c), l
                            out (c), h
                            exx
                            ld c, VDP_DATA_PORT

                            ; == Construct LUT address For First Write
                            ld a, (GSL_X)              
                            and 248 ; %11111000
                            rrca
                            ld hl, GSL_LUTRowUpdate
                            add a, l
                            jp nc, _finaliseLUTRowAddress
                            inc h
_finaliseLUTRowAddress:     ld l, a
                            push hl                             ; Story copy on stack
                            
                            ; == Extract Buffer Location from LUT and store in DE
                            ; (where to start copying data from buffer to vdp)
                            ld e, (hl)                          
                            inc hl
                            ld d, (hl)
                            inc hl
                            
                            ; == Extract JUMP address from LUT, write to RAM and call as Jump Address
                            ; outi code is unrolled, this controls number of writes before ret.
                            ld a, (hl)                          
                            ld (GSL_RAMJump + 1), a
                            inc hl
                            ld a, (hl)
                            ld (GSL_RAMJump + 2), a
                            ex de, hl
                            call GSL_RAMJump
                            
                            ; == Update VDP Pointer For Second Write
                            ; Second write always starts at left most side of Nametable so clear
                            ; low bytes of address and update.
                            exx
                            ld a, 192 ; %11000000
                            and l
                            out (c), a
                            out (c), h
                            exx
                            
                            ; == Move Forward 130 bytes in LUT
                            ex de, hl
                            pop hl
                            ld a, 130
                            add a, l
                            ld l, a
                            jp nc, _rowExtractJumpAddress
                            inc h
                            
                            ; == Extract JUMP address from LUT, write to RAM and call as Jump Address
                            ; outi code is unrolled, this controls number of writes before ret.
_rowExtractJumpAddress:     ld a, (hl)                          
                            ld (GSL_RAMJump + 1), a
                            inc hl
                            ld a, (hl)
                            ld (GSL_RAMJump + 2), a
                            ex de, hl
                            call GSL_RAMJump

                            
_updateColumn:              ; == Check if row update required.
                            ld a, (GSL_XUpdateStatus)
                            and a
                            jp z, _MetatileUpdates
                            
                            ; == Put VDP Pointer Address in Shadow Register and Set VDP Pointer.
                            exx
                            ld hl, (GSL_NametableUpdateColumn)            
                            ld de, 64                           ; Each new column entry is 64 bytes ahead, store here for addition
                            ld c, VDP_CONTROL_PORT
                            exx
                            
                            ; == Construct LUT address For First Write
                            ld a, (GSL_Y224)
                            and 248 ; %11111000
                            rrca
                            ld hl, GSL_LUTColumnUpdate
                            add a, l
                            jp nc, _finaliseLUTColumnAddress
                            inc h
_finaliseLUTColumnAddress:  ld l, a
                            push hl                             ; Story copy on stack
                            
                            ; == Extract Buffer Location from LUT and store in DE
                            ; (where to start copying data from buffer to vdp)
                            ld e, (hl)                          
                            inc hl
                            ld d, (hl)
                            inc hl
                            
                            ; == Extract JUMP address from LUT, write to RAM and call as Jump Address
                            ; outi code is unrolled, this controls number of writes before ret.
                            ld a, (hl)                          
                            ld (GSL_RAMJump + 1), a
                            inc hl
                            ld a, (hl)
                            ld (GSL_RAMJump + 2), a
                            ex de, hl
                            ld c, VDP_DATA_PORT
                            call GSL_RAMJump
                            
                            ; == Update VDP Pointer For Second Write
                            ; Second write always starts at top row of Nametable so reset high byte 
                            ; to GSL_NAMETABLE_HIGH_BYTE_START, keep low bytes of address and update.
                            exx
                            ld a, (GSL_NametableUpdateColumn)                  ; Get vram nametable pointer
                            and 63 ; %00111111
                            ld h, GSL_NAMETABLE_HIGH_BYTE_START
                            ld l, a
                            exx

                            ; == Move Forward 114 bytes in LUT
                            ex de, hl
                            pop hl
                            ld a, 114
                            add a, l
                            ld l, a
                            jp nc, _columnExtractJumpAddress
                            inc h
                            
                            ; == Extract JUMP address from LUT, write to RAM and call as Jump Address
                            ; outi code is unrolled, this controls number of writes before ret.
_columnExtractJumpAddress:  ld a, (hl)                          
                            ld (GSL_RAMJump + 1), a
                            inc hl
                            ld a, (hl)
                            ld (GSL_RAMJump + 2), a
                            ex de, hl
                            call GSL_RAMJump
                            
                            
_MetatileUpdates:           ; == Check if Metatile Updates are Required.
                            ld a, (GSL_MetatileUpdatesCount)
                            or a
                            jp z, _VBlankCleanup
                            
                            ld hl, GSL_MetatileUpdates
                            ld b, a
                            
_MetatileUpdatesLoop:       ; == Process each metatile Update
                            ld a, 7                 ; count = number of metatile to update.
                            add a, b                ; add 7 to current number, outi will reduce this by 8
                            ld b, a                 ; end result will be to subtract 1 from current number 
                            
                            ; == First word is VDP address of update. Update VDP Pointer.
                            ld d, (hl)
                            inc hl
                            ld e, (hl)
                            inc hl
                            ld c, VDP_CONTROL_PORT
                            out (c), d
                            out (c), e
                            
                            ; == Write first two entries of Metatile.
                            dec c
                            outi
                            outi
                            outi
                            outi
                            
                            ; == Update VDP pointer to be one nametable row lower.
                            ld a, 64 ; $40
                            add a, d
                            ld d, a
                            inc c
                            out (c), d
                            out (c), e
                            
                            ; == Write last two entries.
                            dec c
                            outi
                            outi
                            outi
                            outi
                            
                            ; == Loop if required.
                            ld a, b
                            or a
                            jp nz, _MetatileUpdatesLoop
                            
                            
_VBlankCleanup:             ; == Reset RAM Variables for next Active Display.
                            ld hl, GSL_MetatileUpdates
                            ld (GSL_MetatileUpdatesAddress), hl
                            
                            xor a
                            ld (GSL_MetatileUpdatesCount), a
                            ld (GSL_XUpdateRequest), a
                            ld (GSL_YUpdateRequest), a
                            
                            ; == Update Screen X,Y Scroll
                            ld a, (GSL_X)
                            neg
                            out (VDP_CONTROL_PORT), a
                            ld a, 136 ; $88
                            out (VDP_CONTROL_PORT), a
                            
                            ld a, (GSL_Y224)
                            out (VDP_CONTROL_PORT), a
                            ld a, 137 ; $89
                            out (VDP_CONTROL_PORT), a 
                            
                            ret
                              



/* == Populates Buffer for VBlank Updates
 * 
 * - Extracts row of nametable entries from scrolltable
 * - Stored in Buffer for Vblank routine
 * - Metatile ids in scrolltable are pre processed to speed the resolving of their address (which is very time consuming)
 *      * (metatile_id << 3) & 248) + ((metatile_id >> 5) & 7)
 * 
 * @in iy: scrolltable pointer pointing to first metatile in update
 * @in ixl: X offset, needed to identify which row in metatile to extract.
 */  
_unrollRowForNTUpdate:      ld de, GSL_NATRowBuffer         
                            ld c, 17*4   
                            
_unrollMetaTileRow:         ld l, (iy+0)                        ; == Resolve metatile address, current metatile pointed to by iy
                            ld a, l                             ; optimized <<3 shift, bits are stored as '43210765' for speed
                            and 7 ; %111                          
                            ld h, a
                            ld a, l
                            and 248 ; %11111000
                            or ixl                              ; add X offset (in ixl) to get desired column in meta tile.
                            ld l, a                             ; hl = pointer to metatile nametable entry we need to write to vdp
                            push bc
                            ld bc, (GSL_MetatileTable)
                            add hl, bc
                            pop bc
                            
                            ldi                                 ; == Write 2x nametable entries to buffer.
                            ldi                                 ; metatiles are 2 entries wide so l+=2 to move to second entry.
                            ldi
                            ldi
                            
                            xor a                               ; loop
                            or c
                            ret z
                            
                            inc iy                              ; Update Scrolltable Pointer
                            
                            jp _unrollMetaTileRow
                                                        
                            
                            
                            
/* == Populates Buffer for VBlank Updates
 * 
 * - Extracts column of nametable entries from scrolltable
 * - Stored in Buffer for Vblank routine
 * - Metatile ids in scrolltable are pre processed to speed the resolving of their address (which is very time consuming)
 *      * (metatile_id << 3) & 248) + ((metatile_id >> 5) & 7)
 * 
 * @in iy: scrolltable pointer pointing to first metatile in update
 * @in ixl: X offset, needed to identify which column in metatile to extract.
 */                   
_unrollColumnForNTUpdate:   ld de, GSL_NATColumnBuffer         
                            ld c, 15*4   
                            
_unrollMetaTileColumn:      ld l, (iy+0)                        ; == Resolve metatile address, current metatile pointed to by iy
                            ld a, l                             ; optimized <<3 shift, bits are stored as '43210765' for speed
                            and 7 ; %111                            
                            ;or GSL_METATILE_TABLE_HIGH_BYTE     ; add high bits of metatile address (table must be on 2k boundary)
                            ld h, a
                            ld a, l
                            and 248 ; %11111000
                            or ixl                              ; add X offset (in ixl) to get desired column in meta tile.
                            ld l, a                             ; hl = pointer to metatile nametable entry we need to write to vdp
                            push bc
                            ld bc, (GSL_MetatileTable)
                            add hl, bc
                            pop bc
                            
                            ldi                                 ; == Write 2x nametable entries to buffer.
                            ldi                                 ; metatiles are 2 entries wide so l+=2 to move to second entry.
                            inc hl                   
                            inc hl
                            ldi
                            ldi
                            
                            xor a                               ; loop
                            or c
                            ret z
                            
                            ld hl, (GSL_WidthInMetatiles)       ; Update Scrolltable Pointer
                            ex de, hl
                            add iy, de
                            ex de, hl
                            
                            jp _unrollMetaTileColumn                             



                            
/* == Unrolled VDP Row Write
 * Used to speed write rows of data on VBlank row update.
 */                            
                            .repeat 32
                            outi
                            outi
                            .endr
_UnrolledRowWrites:         ret




/* == Unrolled VDP Column Write
 * Used to speed write columns of data on VBlank row update.
 */ 
                            .repeat 28
                            exx
                            out (c), l
                            out (c), h
                            add hl, de
                            exx
                            outi
                            outi
                            .endr
_UnrolledColumnWrites:      ret
                            
 
 
 
/*
 * == LookUp Table for Row Updates.
 * 
 * Row Updates are split in two. 
 * - First is from left side of window (as seen on screen)
 * - Second is from left side of Nametable (as seen in vram)
 * 
 * This table contains location in buffer to start write and 
 * a jump address to unrolled vdp row update code representing
 * number of writes required for each update.
 */                           
GSL_LUTRowUpdate:
.dw GSL_NATRowBuffer + ((00 * 2) & 3) + 2, _UnrolledRowWrites - (31 * 4)
.dw GSL_NATRowBuffer + ((01 * 2) & 3) + 2, _UnrolledRowWrites - (30 * 4)
.dw GSL_NATRowBuffer + ((02 * 2) & 3) + 2, _UnrolledRowWrites - (29 * 4)
.dw GSL_NATRowBuffer + ((03 * 2) & 3) + 2, _UnrolledRowWrites - (28 * 4)
.dw GSL_NATRowBuffer + ((04 * 2) & 3) + 2, _UnrolledRowWrites - (27 * 4)
.dw GSL_NATRowBuffer + ((05 * 2) & 3) + 2, _UnrolledRowWrites - (26 * 4)
.dw GSL_NATRowBuffer + ((06 * 2) & 3) + 2, _UnrolledRowWrites - (25 * 4)
.dw GSL_NATRowBuffer + ((07 * 2) & 3) + 2, _UnrolledRowWrites - (24 * 4)
.dw GSL_NATRowBuffer + ((08 * 2) & 3) + 2, _UnrolledRowWrites - (23 * 4)
.dw GSL_NATRowBuffer + ((09 * 2) & 3) + 2, _UnrolledRowWrites - (22 * 4)
.dw GSL_NATRowBuffer + ((10 * 2) & 3) + 2, _UnrolledRowWrites - (21 * 4)
.dw GSL_NATRowBuffer + ((11 * 2) & 3) + 2, _UnrolledRowWrites - (20 * 4)
.dw GSL_NATRowBuffer + ((12 * 2) & 3) + 2, _UnrolledRowWrites - (19 * 4)
.dw GSL_NATRowBuffer + ((13 * 2) & 3) + 2, _UnrolledRowWrites - (18 * 4)
.dw GSL_NATRowBuffer + ((14 * 2) & 3) + 2, _UnrolledRowWrites - (17 * 4)
.dw GSL_NATRowBuffer + ((15 * 2) & 3) + 2, _UnrolledRowWrites - (16 * 4)
.dw GSL_NATRowBuffer + ((16 * 2) & 3) + 2, _UnrolledRowWrites - (15 * 4)
.dw GSL_NATRowBuffer + ((17 * 2) & 3) + 2, _UnrolledRowWrites - (14 * 4)
.dw GSL_NATRowBuffer + ((18 * 2) & 3) + 2, _UnrolledRowWrites - (13 * 4)
.dw GSL_NATRowBuffer + ((19 * 2) & 3) + 2, _UnrolledRowWrites - (12 * 4)
.dw GSL_NATRowBuffer + ((20 * 2) & 3) + 2, _UnrolledRowWrites - (11 * 4)
.dw GSL_NATRowBuffer + ((21 * 2) & 3) + 2, _UnrolledRowWrites - (10 * 4)
.dw GSL_NATRowBuffer + ((22 * 2) & 3) + 2, _UnrolledRowWrites - (09 * 4)
.dw GSL_NATRowBuffer + ((23 * 2) & 3) + 2, _UnrolledRowWrites - (08 * 4)
.dw GSL_NATRowBuffer + ((24 * 2) & 3) + 2, _UnrolledRowWrites - (07 * 4)
.dw GSL_NATRowBuffer + ((25 * 2) & 3) + 2, _UnrolledRowWrites - (06 * 4)
.dw GSL_NATRowBuffer + ((26 * 2) & 3) + 2, _UnrolledRowWrites - (05 * 4)
.dw GSL_NATRowBuffer + ((27 * 2) & 3) + 2, _UnrolledRowWrites - (04 * 4)
.dw GSL_NATRowBuffer + ((28 * 2) & 3) + 2, _UnrolledRowWrites - (03 * 4)
.dw GSL_NATRowBuffer + ((29 * 2) & 3) + 2, _UnrolledRowWrites - (02 * 4)
.dw GSL_NATRowBuffer + ((30 * 2) & 3) + 2, _UnrolledRowWrites - (01 * 4)
.dw GSL_NATRowBuffer + ((31 * 2) & 3) + 2, _UnrolledRowWrites - (00 * 4)
.dw GSL_NATRowBuffer + (00 * 2) + ((00 * 2) & 3), _UnrolledRowWrites - (01 * 4)
.dw GSL_NATRowBuffer + (01 * 2) + ((01 * 2) & 3), _UnrolledRowWrites - (02 * 4)
.dw GSL_NATRowBuffer + (02 * 2) + ((02 * 2) & 3), _UnrolledRowWrites - (03 * 4)
.dw GSL_NATRowBuffer + (03 * 2) + ((03 * 2) & 3), _UnrolledRowWrites - (04 * 4)
.dw GSL_NATRowBuffer + (04 * 2) + ((04 * 2) & 3), _UnrolledRowWrites - (05 * 4)
.dw GSL_NATRowBuffer + (05 * 2) + ((05 * 2) & 3), _UnrolledRowWrites - (06 * 4)
.dw GSL_NATRowBuffer + (06 * 2) + ((06 * 2) & 3), _UnrolledRowWrites - (07 * 4)
.dw GSL_NATRowBuffer + (07 * 2) + ((07 * 2) & 3), _UnrolledRowWrites - (08 * 4)
.dw GSL_NATRowBuffer + (08 * 2) + ((08 * 2) & 3), _UnrolledRowWrites - (09 * 4)
.dw GSL_NATRowBuffer + (09 * 2) + ((09 * 2) & 3), _UnrolledRowWrites - (10 * 4)
.dw GSL_NATRowBuffer + (10 * 2) + ((10 * 2) & 3), _UnrolledRowWrites - (11 * 4)
.dw GSL_NATRowBuffer + (11 * 2) + ((11 * 2) & 3), _UnrolledRowWrites - (12 * 4)
.dw GSL_NATRowBuffer + (12 * 2) + ((12 * 2) & 3), _UnrolledRowWrites - (13 * 4)
.dw GSL_NATRowBuffer + (13 * 2) + ((13 * 2) & 3), _UnrolledRowWrites - (14 * 4)
.dw GSL_NATRowBuffer + (14 * 2) + ((14 * 2) & 3), _UnrolledRowWrites - (15 * 4)
.dw GSL_NATRowBuffer + (15 * 2) + ((15 * 2) & 3), _UnrolledRowWrites - (16 * 4)
.dw GSL_NATRowBuffer + (16 * 2) + ((16 * 2) & 3), _UnrolledRowWrites - (17 * 4)
.dw GSL_NATRowBuffer + (17 * 2) + ((17 * 2) & 3), _UnrolledRowWrites - (18 * 4)
.dw GSL_NATRowBuffer + (18 * 2) + ((18 * 2) & 3), _UnrolledRowWrites - (19 * 4)
.dw GSL_NATRowBuffer + (19 * 2) + ((19 * 2) & 3), _UnrolledRowWrites - (20 * 4)
.dw GSL_NATRowBuffer + (20 * 2) + ((20 * 2) & 3), _UnrolledRowWrites - (21 * 4)
.dw GSL_NATRowBuffer + (21 * 2) + ((21 * 2) & 3), _UnrolledRowWrites - (22 * 4)
.dw GSL_NATRowBuffer + (22 * 2) + ((22 * 2) & 3), _UnrolledRowWrites - (23 * 4)
.dw GSL_NATRowBuffer + (23 * 2) + ((23 * 2) & 3), _UnrolledRowWrites - (24 * 4)
.dw GSL_NATRowBuffer + (24 * 2) + ((24 * 2) & 3), _UnrolledRowWrites - (25 * 4)
.dw GSL_NATRowBuffer + (25 * 2) + ((25 * 2) & 3), _UnrolledRowWrites - (26 * 4)
.dw GSL_NATRowBuffer + (26 * 2) + ((26 * 2) & 3), _UnrolledRowWrites - (27 * 4)
.dw GSL_NATRowBuffer + (27 * 2) + ((27 * 2) & 3), _UnrolledRowWrites - (28 * 4)
.dw GSL_NATRowBuffer + (28 * 2) + ((28 * 2) & 3), _UnrolledRowWrites - (29 * 4)
.dw GSL_NATRowBuffer + (29 * 2) + ((29 * 2) & 3), _UnrolledRowWrites - (30 * 4)
.dw GSL_NATRowBuffer + (30 * 2) + ((30 * 2) & 3), _UnrolledRowWrites - (31 * 4)
.dw GSL_NATRowBuffer + (31 * 2) + ((31 * 2) & 3), _UnrolledRowWrites - (32 * 4)                            
                            
                            
  
    
/*
 * == LookUp Table for Column Updates.
 * 
 * Column Updates are split in two. 
 * - First is from top of window (as seen on screen)
 * - Second is from top of Nametable (as seen in vram)
 * 
 * This table contains location in buffer to start write and 
 * a jump address to unrolled vdp column update code representing
 * number of writes required for each update.
 */                        
GSL_LUTColumnUpdate:
.dw GSL_NATColumnBuffer + ((00 * 2) & 3), _UnrolledColumnWrites - (28 * 11)
.dw GSL_NATColumnBuffer + ((01 * 2) & 3), _UnrolledColumnWrites - (27 * 11)
.dw GSL_NATColumnBuffer + ((02 * 2) & 3), _UnrolledColumnWrites - (26 * 11)
.dw GSL_NATColumnBuffer + ((03 * 2) & 3), _UnrolledColumnWrites - (25 * 11)
.dw GSL_NATColumnBuffer + ((04 * 2) & 3), _UnrolledColumnWrites - (24 * 11)
.dw GSL_NATColumnBuffer + ((05 * 2) & 3), _UnrolledColumnWrites - (23 * 11)
.dw GSL_NATColumnBuffer + ((06 * 2) & 3), _UnrolledColumnWrites - (22 * 11)
.dw GSL_NATColumnBuffer + ((07 * 2) & 3), _UnrolledColumnWrites - (21 * 11)
.dw GSL_NATColumnBuffer + ((08 * 2) & 3), _UnrolledColumnWrites - (20 * 11)
.dw GSL_NATColumnBuffer + ((09 * 2) & 3), _UnrolledColumnWrites - (19 * 11)
.dw GSL_NATColumnBuffer + ((10 * 2) & 3), _UnrolledColumnWrites - (18 * 11)
.dw GSL_NATColumnBuffer + ((11 * 2) & 3), _UnrolledColumnWrites - (17 * 11)
.dw GSL_NATColumnBuffer + ((12 * 2) & 3), _UnrolledColumnWrites - (16 * 11)
.dw GSL_NATColumnBuffer + ((13 * 2) & 3), _UnrolledColumnWrites - (15 * 11)
.dw GSL_NATColumnBuffer + ((14 * 2) & 3), _UnrolledColumnWrites - (14 * 11)
.dw GSL_NATColumnBuffer + ((15 * 2) & 3), _UnrolledColumnWrites - (13 * 11)
.dw GSL_NATColumnBuffer + ((16 * 2) & 3), _UnrolledColumnWrites - (12 * 11)
.dw GSL_NATColumnBuffer + ((17 * 2) & 3), _UnrolledColumnWrites - (11 * 11)
.dw GSL_NATColumnBuffer + ((18 * 2) & 3), _UnrolledColumnWrites - (10 * 11)
.dw GSL_NATColumnBuffer + ((19 * 2) & 3), _UnrolledColumnWrites - (09 * 11)
.dw GSL_NATColumnBuffer + ((20 * 2) & 3), _UnrolledColumnWrites - (08 * 11)
.dw GSL_NATColumnBuffer + ((21 * 2) & 3), _UnrolledColumnWrites - (07 * 11)
.dw GSL_NATColumnBuffer + ((22 * 2) & 3), _UnrolledColumnWrites - (06 * 11)
.dw GSL_NATColumnBuffer + ((23 * 2) & 3), _UnrolledColumnWrites - (05 * 11)
.dw GSL_NATColumnBuffer + ((24 * 2) & 3), _UnrolledColumnWrites - (04 * 11)
.dw GSL_NATColumnBuffer + ((25 * 2) & 3), _UnrolledColumnWrites - (03 * 11)
.dw GSL_NATColumnBuffer + ((26 * 2) & 3), _UnrolledColumnWrites - (02 * 11)
.dw GSL_NATColumnBuffer + ((27 * 2) & 3), _UnrolledColumnWrites - (01 * 11)

.dw GSL_NATColumnBuffer + (00 * 2) + ((00 * 2) & 3), _UnrolledColumnWrites - (00 * 11)
.dw GSL_NATColumnBuffer + (01 * 2) + ((01 * 2) & 3), _UnrolledColumnWrites - (01 * 11)
.dw GSL_NATColumnBuffer + (02 * 2) + ((02 * 2) & 3), _UnrolledColumnWrites - (02 * 11)
.dw GSL_NATColumnBuffer + (03 * 2) + ((03 * 2) & 3), _UnrolledColumnWrites - (03 * 11)
.dw GSL_NATColumnBuffer + (04 * 2) + ((04 * 2) & 3), _UnrolledColumnWrites - (04 * 11)
.dw GSL_NATColumnBuffer + (05 * 2) + ((05 * 2) & 3), _UnrolledColumnWrites - (05 * 11)
.dw GSL_NATColumnBuffer + (06 * 2) + ((06 * 2) & 3), _UnrolledColumnWrites - (06 * 11)
.dw GSL_NATColumnBuffer + (07 * 2) + ((07 * 2) & 3), _UnrolledColumnWrites - (07 * 11)
.dw GSL_NATColumnBuffer + (08 * 2) + ((08 * 2) & 3), _UnrolledColumnWrites - (08 * 11)
.dw GSL_NATColumnBuffer + (09 * 2) + ((09 * 2) & 3), _UnrolledColumnWrites - (09 * 11)
.dw GSL_NATColumnBuffer + (10 * 2) + ((10 * 2) & 3), _UnrolledColumnWrites - (10 * 11)
.dw GSL_NATColumnBuffer + (11 * 2) + ((11 * 2) & 3), _UnrolledColumnWrites - (11 * 11)
.dw GSL_NATColumnBuffer + (12 * 2) + ((12 * 2) & 3), _UnrolledColumnWrites - (12 * 11)
.dw GSL_NATColumnBuffer + (13 * 2) + ((13 * 2) & 3), _UnrolledColumnWrites - (13 * 11)
.dw GSL_NATColumnBuffer + (14 * 2) + ((14 * 2) & 3), _UnrolledColumnWrites - (14 * 11)
.dw GSL_NATColumnBuffer + (15 * 2) + ((15 * 2) & 3), _UnrolledColumnWrites - (15 * 11)
.dw GSL_NATColumnBuffer + (16 * 2) + ((16 * 2) & 3), _UnrolledColumnWrites - (16 * 11)
.dw GSL_NATColumnBuffer + (17 * 2) + ((17 * 2) & 3), _UnrolledColumnWrites - (17 * 11)
.dw GSL_NATColumnBuffer + (18 * 2) + ((18 * 2) & 3), _UnrolledColumnWrites - (18 * 11)
.dw GSL_NATColumnBuffer + (19 * 2) + ((19 * 2) & 3), _UnrolledColumnWrites - (19 * 11)
.dw GSL_NATColumnBuffer + (20 * 2) + ((20 * 2) & 3), _UnrolledColumnWrites - (20 * 11)
.dw GSL_NATColumnBuffer + (21 * 2) + ((21 * 2) & 3), _UnrolledColumnWrites - (21 * 11)
.dw GSL_NATColumnBuffer + (22 * 2) + ((22 * 2) & 3), _UnrolledColumnWrites - (22 * 11)
.dw GSL_NATColumnBuffer + (23 * 2) + ((23 * 2) & 3), _UnrolledColumnWrites - (23 * 11)
.dw GSL_NATColumnBuffer + (24 * 2) + ((24 * 2) & 3), _UnrolledColumnWrites - (24 * 11)
.dw GSL_NATColumnBuffer + (25 * 2) + ((25 * 2) & 3), _UnrolledColumnWrites - (25 * 11)
.dw GSL_NATColumnBuffer + (26 * 2) + ((26 * 2) & 3), _UnrolledColumnWrites - (26 * 11)
.dw GSL_NATColumnBuffer + (27 * 2) + ((27 * 2) & 3), _UnrolledColumnWrites - (27 * 11)                            



; == MOD table to speed identifying VDPNametable loc for metatile updates.
; - Table is %28 but only for even numbers.
; - Each row represents 1x192 height (width does not matter!).          
GSL_ModTable:
.repeat (GSL_MAX_MAP_HEIGHT_IN_METATILES /12)
.db $F0, $F1, $F2, $F3, $F4, $F5, $F6, $F7, $F8, $F9, $FA, $FB, $FC, $FD
.endr

                            
.ends

