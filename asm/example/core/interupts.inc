

; == BOOTUP
; =====================================
.bank 0 slot 0
.org $0000
.section "Boot" force

                            di                                                  ; disable interrupts
                            im 1                                                ; Interrupt mode 1
                            jp InitializeSMS                                    ; jump to main program

.ends




; == MI Interrupt Handler
; =====================================
.bank 0 slot 0
.org $0038
.section "MI Interrupt handler" force

MI_InterruptHandler:        jp InteruptHandler
        
.ends   



; == NMI Interrupt Handler
; =====================================
.bank 0 slot 0
.org $0066
.section "NMI Interrupt Handler" force
    
NMI_InterruptHandler:       retn ; return
    
.ends

