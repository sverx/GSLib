; == DEFINES
; =====================================

; VDP NAT Defines
.define VDP_NAT                 $0F
.define VDP_NAT00               $01
.define VDP_NAT01               $03
.define VDP_NAT02               $05
.define VDP_NAT03               $07
.define VDP_NAT04               $09
.define VDP_NAT05               $0B
.define VDP_NAT06               $0D
.define VDP_NAT07               $0F

; VRAM NAT Defines
.define VRAM_NAT                $7800
.define VRAM_NAT00              $4000
.define VRAM_NAT01              $4800
.define VRAM_NAT02              $5000
.define VRAM_NAT03              $5800
.define VRAM_NAT04              $6000
.define VRAM_NAT05              $6800
.define VRAM_NAT06              $7000
.define VRAM_NAT07              $7800

; SAT
.define VDP_SAT                 $7F
.define VRAM_SAT                $7F00

; VDP Base Address
.define VRAM                   $4000
.define CRAM                   $C000

; Ports
.define VDP_CONTROL_PORT         $BF
.define VDP_DATA_PORT            $BE
.define V_COUNTER_PORT           $7E

; VDP 00 Functionality
.define LINE_INTERRUPT_ON           %00010100
.define LINE_INTERRUPT_OFF          %00000100
.define RIGHT_SCROLL_LOCKED         %10000100
.define RIGHT_SCROLL_UNLOCKED       %00000100
.define TOP_SCROLL_LOCKED           %01000100
.define TOP_SCROLL_UNLOCKED         %00000100
.define SCROLL_MASK_ON              %00100100
.define SCROLL_MASK_OFF             %00000100
.define SPRITE_LEFT_SHIFT_ON        %00001100
.define SPRITE_LEFT_SHIFT_OFF       %00000100

; VDP 01 Functionality
.define SPRITE_STRETCH_ON           %00000001
.define SPRITE_STRETCH_OFF          $00
.define SPRITE_TALL_ON              %00000010
.define SPRITE_TALL_OFF             $00
.define FRAME_INTERRUPT_ON          %00100000
.define FRAME_INTERRUPT_OFF         %00
.define DISPLAY_ON                  %01000000
.define DISPLAY_OFF                 $00

; VDP 06
.define SPRITE_LOCATION_0           %11111011
.define SPRITE_LOCATION_1           %11111111

; Paging
.define PAGE_0                  $FFFD
.define PAGE_1                  $FFFE
.define PAGE_2                  $FFFF
