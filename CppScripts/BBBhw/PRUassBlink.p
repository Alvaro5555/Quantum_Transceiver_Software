// PRUassTrigSigScript.p  (PRU1: cuadradas en P8_39..P8_42)

.origin 0
.entrypoint INITIATIONS

#define GPIO0_BANK 0x44E07000
#define GPIO1_BANK 0x4804c000
#define GPIO2_BANK 0x481ac000
#define GPIO3_BANK 0x481AE000
#define GPIO_SETDATAOUToffset 0x194
#define GPIO_CLEARDATAOUToffset 0x190

#define PRU0_PRU1_INTERRUPT     17
#define PRU1_PRU0_INTERRUPT     18
#define PRU0_ARM_INTERRUPT      19
#define PRU1_ARM_INTERRUPT      20
#define ARM_PRU0_INTERRUPT      21
#define ARM_PRU1_INTERRUPT      22

#define CONST_PRUCFG   C4
#define CONST_PRUDRAM  C24
#define CONST_IETREG   C26

#define OWN_RAM        0x00000000
#define OWN_RAMoffset  0x00000200
#define PRU1_CTRL      0x240

; === MÁSCARAS DE SALIDA R30.b0 ===
; P8_39->bit6, P8_40->bit7, P8_41->bit4, P8_42->bit5  => 11110000b = 0xF0
#define AllOutputInterestPinsHigh 0xF0
#define AllOutputInterestPinsLow  0x00

; === DELAYS (ajusta para frecuencia/duty) ===
#define DELAY_ON   0x00500000
#define DELAY_OFF  0x00500000

; --- LED macros (USR0 en GPIO2_21) ---
.macro LED_OFF
MOV r28, 1<<21
MOV r29, GPIO2_BANK | GPIO_CLEARDATAOUToffset
SBBO r28, r29, 0, 4
.endm
.macro LED_ON
MOV r28, 1<<21
MOV r29, GPIO2_BANK | GPIO_SETDATAOUToffset
SBBO r28, r29, 0, 4
.endm

; r30 = out, r31 = in

INITIATIONS:
SET     r30.t11
LBCO    r0, CONST_PRUCFG, 4, 4
CLR     r0, r0, 4
SBCO    r0, CONST_PRUCFG, 4, 4

MOV     r0, OWN_RAM | OWN_RAMoffset
MOV     r10, 0x24000+0x20
SBBO    r0, r10, 0, 4

LDI     r4, 0
MOV     r2, 0x24924
SBCO    r2, CONST_PRUCFG, 0x10, 4
LDI     r0, 1
SBCO    r0, CONST_PRUCFG, 0x30, 4
MOV     r0, 0x111
SBCO    r0, CONST_IETREG, 0, 4
SBCO    r4, CONST_IETREG, 0x08, 4

LDI     r30, 0
LDI     r4, 0

; ====== BUCLE CONTINUO: ON delay / OFF delay ======
MAIN:
    ; ON
    MOV     r30.b0, AllOutputInterestPinsHigh
    LDI     r2, DELAY_ON
    LDI     r1, 0
DLY1:
    ADD     r1, r1, 1
    QBNE    DLY1, r1, r2

    ; OFF
    MOV     r30.b0, AllOutputInterestPinsLow
    LDI     r2, DELAY_OFF
    LDI     r1, 0
DLY2:
    ADD     r1, r1, 1
    QBNE    DLY2, r1, r2

    JMP     MAIN

EXIT:
SET     r30.t11
HALT

ERR:
SET     r30.t11
LED_ON
HALT
