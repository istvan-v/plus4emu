
.setcpu "6502"
.code
.word $1001, $100b, $000a
.byte $9e, "4109", $00, $00, $00
.org $100d

nLines = $d8
nLinesD2 = $d9
interlaceEnabled = $da
overscanEnabled = $db
addr0ZP = $dc
addr1ZP = $de
tmp1ZP = $e0
tmp2ZP = $e1
tmp3ZP = $e2
tmp4ZP = $e3
tmp5ZP = $e4
fli1CodeOffsMSB = $e5
fli0CodeStartMSB = $e6
fli1CodeStartMSB = $e7
tmp6ZP = $e8

field0XShiftTable = $1900
field0Color0Table = $1a00
field0Color3Table = $1b00
field1XShiftTable = $1d00
field1Color0Table = $1e00
field1Color3Table = $1f00
lineBlankFXEnabled = $1ffb
borderColor = $1ffc
interlaceFlags = $1ffd
nLinesMSB = $1ffe
nLinesLSB = $1fff
fliCodeAddrLSBTable = $2040
fliCodeAddrMSBTable = $6040

main:
        sei
        cld
        ldx #$ff
        txs
        lda #<resetRoutine
        sta $fffc
        lda #>resetRoutine
        sta $fffd
        jsr initFLI
        jsr doInitCallback
        ldx irqLineLSBTable
        dex
@l1:    cpx $ff1d
        beq @l1
        inx
@l2:    cpx $ff1d
        beq @l2
        stx $ff0b
        lda irqLineMSBTable
        sta $ff0a
        lda irqAddrLSBTable
        sta $fffe
        lda irqAddrMSBTable
        sta $ffff
        dec $ff09
        cli
        ldx #$00
@l3:    jsr endIRQWait
        lda lineBlankCnt1
        bne @l4
        lda lineBlankCnt2
        beq @l6
        bpl @l4
        lda #$7f
        sta $fd30
        sta $ff08
        lda $ff08
        and #$10
        bne @l5
        txa
        beq @l5
        lda lineBlankFXEnabled
        beq @l6
        lda nLinesD2
        lsr
        sta lineBlankCnt2
@l4:    txa
@l5:    pha
        jsr doFrameCallback
        pla
        tax
        bpl @l3
@l6:    sei
        lda #$0b
        sta $ff06
        sta $ff3e
        sta $fdd0
        jmp (spaceCallback)

doDecompressCallback:
        jmp (decompressCallback)

doInitCallback:
        jmp (initCallback)

doFrameCallback:
        ldx lineBlankCnt1
        beq @l1
        dex
        stx lineBlankCnt1
        jsr displayLine
        lda nLinesD2
        clc
        sbc lineBlankCnt1
        tax
        jsr displayLine
        jmp (frameCallback)
@l1:    ldx lineBlankCnt2
        bmi @l2
        beq @l2
        dex
        stx lineBlankCnt2
        jsr blankLine
        lda nLinesD2
        clc
        sbc lineBlankCnt2
        tax
        jsr blankLine
@l2:    jmp (frameCallback)

resetRoutine:
        sei
        sta $ff3e
        sta $fdd0
        jmp ($fffc)

vsyncWait:
        bit $ff07
        bvs @l1
        lda #$fa
        .byte $2c
@l1:    lda #$e1
        jmp lineWait

endIRQWait:
        lda endIRQLine

lineWait:
@l2:    cmp $ff1d
        bne @l2
@l3:    cmp $ff1d
        beq @l3
dummyCallback:
        rts

; -----------------------------------------------------------------------------

blankLine:
        stx tmp1ZP
        jsr initFLICodeAddrPointers
        bit overscanEnabled
        bmi @l1
        lda #$8e
        tax
        ldy #$00
        jsr writeFLICodeByte
        jsr findNextTEDRegisterWrite
        jsr incrementFLICodeAddressBy5
        lda #$15
        jsr clearBackgroundColor
        lda #$16
        jsr clearBackgroundColor
        jsr findNextTEDRegisterWrite
        jsr incrementFLICodeAddressBy5
        lda #$15
        jsr clearBackgroundColor
        lda tmp1ZP
        and #$03
        tax
        lda attribute0AddrTable, x
        bit interlaceFlags
        bvs @l2
        tay
        bne @l3
@l2:    ldy attribute1AddrTable, x
@l3:    tax
        lda #$14
        jmp changeTEDRegisterWrite
@l1:    lda #$14
        ldx #$08
        ldy #$08
        jsr changeTEDRegisterWrite
        jsr findNextTEDRegisterWrite
        jsr incrementFLICodeAddressBy5
        lda #$15
        jsr clearBackgroundColor
        jsr findNextTEDRegisterWrite
        lda #$16
        jsr clearBackgroundColor
        lda #$15
        jsr clearBackgroundColor
        jsr findNextTEDRegisterWrite
        lda #$16
clearBackgroundColor:
        ldx borderColor
        ldy borderColor
        jmp changeTEDRegisterWrite

displayLine:
        stx tmp1ZP
        jsr initFLICodeAddrPointers
        bit overscanEnabled
        bmi @l1
        lda tmp1ZP
        and #$03
        bne @l2
        lda #$8d
        .byte $2c
@l2:    lda #$ee
        tax
        ldy #$00
        jsr writeFLICodeByte
        jmp @l3
@l1:    lda tmp1ZP
        cmp #$64
        and #$03
        bcc @l4
        ora #$04
@l4:    tax
        lda attribute0AddrTable, x
        bit interlaceFlags
        bvs @l5
        tay
        bne @l6
@l5:    ldy attribute1AddrTable, x
@l6:    tax
        lda #$14
        jsr changeTEDRegisterWrite
@l3:    asl tmp1ZP
        jsr findNextTEDRegisterWrite
        jsr incrementFLICodeAddressBy5
        jsr findNextTEDRegisterWrite
        ldx tmp1ZP
        ldy field1Color0Table, x
        lda field0Color0Table, x
        tax
        lda #$15
        jsr changeTEDRegisterWrite
        jsr findNextTEDRegisterWrite
        ldx tmp1ZP
        ldy field1Color3Table, x
        lda field0Color3Table, x
        tax
        lda #$16
        jsr changeTEDRegisterWrite
        jsr findNextTEDRegisterWrite
        ldx tmp1ZP
        bit overscanEnabled
        bmi @l7
        jsr incrementFLICodeAddressBy5
        jsr findNextTEDRegisterWrite
        ldx tmp1ZP
        ldy field1Color0Table + 1, x
        lda field0Color0Table + 1, x
        tax
        lda #$15
        jsr changeTEDRegisterWrite
        jmp @l8
@l7:    lda field0XShiftTable + 1, x
        cmp field0XShiftTable, x
        bne @l9
        lda #$15
        sta tmp3ZP
        lda field0Color0Table + 1, x
        sta tmp4ZP
        jmp @l10
@l9:    sta tmp4ZP
        lda #$07
        sta tmp3ZP
@l10:   lda field1XShiftTable + 1, x
        cmp field1XShiftTable, x
        bne @l11
        lda #$15
        sta tmp5ZP
        lda field1Color0Table + 1, x
        sta tmp6ZP
        jmp @l12
@l11:   sta tmp6ZP
        lda #$07
        sta tmp5ZP
@l12:   ldy #$01
        lda tmp4ZP
        ldx tmp6ZP
        jsr writeFLICodeByte
        ldy #$03
        lda tmp3ZP
        ldx tmp5ZP
        jsr writeFLICodeByte
        jsr incrementFLICodeAddressBy5
@l8:    jsr findNextTEDRegisterWrite
        ldx tmp1ZP
        ldy field1Color3Table + 1, x
        lda field0Color3Table + 1, x
        tax
        lda #$16
        jmp changeTEDRegisterWrite

initFLICodeAddrPointers:
        lda fliCodeAddrLSBTable, x
        sta addr0ZP
        sta addr1ZP
        lda fliCodeAddrMSBTable, x
        sta addr0ZP + 1
        clc
        adc fli1CodeOffsMSB
        sta addr1ZP + 1
        rts

findNextTEDRegisterWrite:
@l5:    ldy #$00
@l6:    lda (addr0ZP), y
        cmp #$a0
        bne @l1
        rts
@l1:    cmp #$ea
        beq @l2
        cmp #$24
        beq @l3
        cmp #$a2
        beq @l3
        cmp #$a9
        beq @l3
        cmp #$4c
        bne @l4
        iny
        lda (addr0ZP), y
        pha
        iny
        lda (addr0ZP), y
        sta addr0ZP + 1
        clc
        adc fli1CodeOffsMSB
        sta addr1ZP + 1
        pla
        sta addr0ZP
        sta addr1ZP
        bcc @l5
@l2:    lda #$01
        .byte $2c
@l3:    lda #$02
        .byte $2c
@l4:    lda #$03
        jsr incrementFLICodeAddress
        jmp @l6

changeTEDRegisterWrite:
        sta tmp3ZP
        sty tmp4ZP
        txa
        ldx tmp4ZP
        ldy #$01
        jsr writeFLICodeByte
        lda tmp3ZP
        tax
        ldy #$03
        jsr writeFLICodeByte

incrementFLICodeAddressBy5:
        lda #$05
incrementFLICodeAddress:
        clc
        adc addr0ZP
        sta addr0ZP
        sta addr1ZP
        bcc @l1
        inc addr0ZP + 1
        inc addr1ZP + 1
@l1:    rts

; -----------------------------------------------------------------------------

initFLI:
        sei
        sta $ff3e
        sta $fdd0
        lda $ff07
        and #$df
        sta $ff07
        jsr vsyncWait
        jsr $ff84
        lda borderColor
        sta $ff19
        lda #$0b
        sta $ff06
        sta $ff3f
        lda compressedDataSizeMSB
        ora compressedDataSizeLSB
        beq @l1
        jsr doDecompressCallback
        lda #$00
        sta compressedDataSizeMSB
        sta compressedDataSizeLSB
@l1:    ldx nLinesMSB
        cpx #$01
        lda #$00
        ror
        sta interlaceEnabled
        lda nLinesLSB
        cpx #$01
        bcc @l2
        ror
@l2:    lsr
        and #$7e
        cmp #$40
        bcs @l3
        lda #$40
@l3:    sta nLinesD2
        bit $ff07
        bvs @l21
        lda #$7c
        .byte $2c
@l21:   lda #$74
        cmp nLinesD2
        bcc @l4
        lda nLinesD2
@l4:    sta nLinesD2
        asl
        sta nLines
        lda lineBlankFXEnabled
        beq @l5
        lda borderColor
        and #$70
        sta tmp1ZP
        lsr
        lsr
        lsr
        lsr
        ora tmp1ZP
        ldx #$00
@l6:    sta $0800, x
        sta $0900, x
        sta $0a00, x
        sta $0b00, x
        inx
        bne @l6
        lda borderColor
        and #$0f
        sta tmp1ZP
        asl
        asl
        asl
        asl
        ora tmp1ZP
@l7:    sta $0c00, x
        sta $0d00, x
        sta $0e00, x
        sta $0f00, x
        inx
        bne @l7
        lda nLinesD2
        lsr
        .byte $2c
@l5:    lda #$00
        sta lineBlankCnt1
        lda #$80
        sta lineBlankCnt2
        lda interlaceFlags
        and #$c0
        sta interlaceFlags
        lda $ff07
        and #$40
        sta tmp1ZP
        ldx nLines
        lda #$00
@l26:   dex
        ora $1900, x
        ora $1d00, x
        cpx #$00
        bne @l26
        and #$07
        bne @l27
        lda tmp1ZP
        ora #$08
        sta tmp1ZP
@l27:   lda $1900, x
        and #$17
        ora tmp1ZP
        sta $1900, x
        lda $1d00, x
        and #$17
        ora tmp1ZP
        sta $1d00, x
        inx
        cpx #$f8
        bne @l27
        lda nLines
        cmp #$c9
        lda #$00
        ror
        sta overscanEnabled
        asl
        lda interlaceFlags
        rol
        rol
        rol
        tax
        lda fli0CodeStartMSBTable, x
        sta fli0CodeStartMSB
        sta addr0ZP + 1
        lda fli1CodeStartMSBTable, x
        sta fli1CodeStartMSB
        sta addr1ZP + 1
        sec
        sbc fli0CodeStartMSBTable, x
        sta fli1CodeOffsMSB
        lda fliCodeSplitCntTable, x
        sta tmp2ZP
        ldx #$00
        stx addr0ZP
        stx addr1ZP
        bit overscanEnabled
        bmi @l9
        lda #$08
        jsr writeFLICodeBlock
        stx tmp1ZP
        ldy #$ff
        lda #$40
        bit interlaceFlags
        bvs @l13
        ldx #$40
        .byte $2c
@l13:   ldx #$80
        jsr writeFLICodeByte
        ldx tmp1ZP
@l8:    txa
        and #$03
        bne @l10
        lda #$09
        .byte $2c
@l10:   lda #$0a
        jsr startFLICodeBlock
        lda #$0d
        jsr writeFLICodeBlock
        ldy #$f2
        sec
        jsr writeXShift
        inx
        cpx nLinesD2
        bcs @l11
        lda #$0b
        jsr writeFLICodeBlock
        lda #$0d
        jsr writeFLICodeBlock
        ldy #$f2
        clc
        jsr writeXShift
        jmp @l8
@l11:   lda #$0c
        jsr writeFLICodeBlock
        lda #$0d
        jsr writeFLICodeBlock
        ldy #$f2
        clc
        jsr writeXShift
        jmp @l20
@l16:   lda #$01
@l18:   jsr writeFLICodeBlock
@l19:   lda #$07
        jsr writeFLICodeBlock
@l9:    lda #$00
        jsr startFLICodeBlock
        ldy #$f7
        sec
        jsr writeXShift
        inx
        dec tmp2ZP
        beq @l14
        cpx nLinesD2
        bcs @l15
        cpx #$65
        bne @l16
        lda #$04
        bne @l18
@l14:   lda #$15
        sta tmp2ZP
        cpx nLinesD2
        bcs @l17
        lda #$02
        .byte $2c
@l17:   lda #$06
        jsr writeFLICodeBlock
        stx tmp1ZP
        lda addr1ZP + 1
        clc
        adc #$02
        pha
        tax
        sec
        sbc fli1CodeOffsMSB
        pha
        ldy #$ff
        jsr writeFLICodeByte
        ldx tmp1ZP
        lda #$00
        sta addr0ZP
        sta addr1ZP
        pla
        sta addr0ZP + 1
        pla
        sta addr1ZP + 1
        cpx nLinesD2
        bcc @l19
        bcs @l12
@l15:   lda #$05
        jsr writeFLICodeBlock
@l12:   lda #$07
        jsr writeFLICodeBlock
@l20:   ldy #$00
        lda #$60
        tax
        jsr writeFLICodeByte
        bit $ff07
        bvs @l22
        lda #$ff
        ldx #$36
        ldy #$f9
        bne @l23
@l22:   lda #$ff
        ldx #$04
        ldy #$e0
@l23:   sta startIRQLineMSB
        stx startIRQLineLSB
        and #$a3
        pha
        lda nLinesD2
        cmp #$75
        bcs @l31
        sbc #$63
        pha
        clc
        adc #$ce
@l35:   sta endIRQLine
        pla
        bmi @l32
        eor #$ff
        sec
        adc startIRQLineLSB
@l34:   tax
        pla
        sbc #$00
        bne @l33
@l32:   eor #$ff
        sec
        sbc #$01
        clc
        bcc @l34
@l31:   sbc #$66
        pha
        adc #$d1
        bcc @l35
@l33:   sta irqLineMSBTable
        stx irqLineLSBTable
        bit interlaceEnabled
        bmi @l24
        sta irqLineMSBTable + 1
        stx irqLineLSBTable + 1
        lda #>fliIRQ
        sta irqAddrMSBTable + 1
        lda #<fliIRQ
        sta irqAddrLSBTable + 1
        jmp @l25
@l24:   lda #$a2
        sta irqLineMSBTable + 1
        sty irqLineLSBTable + 1
        lda #>interlaceIRQ
        sta irqAddrMSBTable + 1
        lda #<interlaceIRQ
        sta irqAddrLSBTable + 1
@l25:   ldx #$00
        stx currentInterlaceField
        lda interlaceFlags
        cmp #$40
        txa
        rol
        sta fliInterlaceEnabled
        bit interlaceFlags
        bmi @l36
        lda #$c8
        ldx #$e8
        bne @l37
@l36:   lda #$d8
        ldx #$f0
@l37:   sta bitmapAddrTable + 1
        stx bitmapAddrTable2 + 1
        ldx nLinesD2
        dex
@l28:   txa
        pha
        lda lineBlankFXEnabled
        bne @l29
        jsr displayLine
        jmp @l30
@l29:   jsr blankLine
@l30:   pla
        tax
        dex
        bpl @l28
        rts

writeXShift:
        txa
        pha
        sty tmp1ZP
        txa
        rol
        tax
        ldy field0XShiftTable - 1, x
        lda field1XShiftTable - 1, x
        tax
        tya
        ldy tmp1ZP
        jsr writeFLICodeByte
        pla
        tax
        rts

; -----------------------------------------------------------------------------

.proc fliIRQ
        sta l12 + 1
        lda $ff1e
        and #$0e
        lsr
        sta l1 - 1
        bpl l1
l1:     lda #$a9
        lda #$a9
        lda #$a9
        lda $ea
l10:    lda #$36
        sta $ff1d
        lda #$ff
        sta $ff1c
        txa
        pha
        tya
        pha
        ldx #$0d
l11:    lda #$3b
        dex
        bne l11
        sta $ff06
        ldx #$17
l2:     dex
        bne l2
        nop
l3:     lda $ff1f
        ldy #$18
        ldx #$00
        sty $ff1b
        stx $ff1a
        and #$fe
        sta $ff1f
        ora #$07
        nop
        nop
        ldx #$7f
        ldy #$d1
        stx $ff1e
        nop
        nop
        nop
        nop
        sty $ff1e
        ldx #$03
l5:     ldy #$00
        dex
        bne l5
        sta $ff1f
        ldy #$51
        sty $ff1e
        ldx #$05
l7:     lda #$00
        dex
        bne l7
        tax
        eor #$01
l9:     and #$01
        sta currentInterlaceField
        lda fli0CodeStartMSB, x
        sta fliCodeAddrMSB
        lda bitmapAddrTable, x
        sta $ff12
        lda bitmapAddrTable2, x
        ldx #$0a
l8:     jsr $a000
        ldx currentInterlaceField
        lda irqLineLSBTable, x
        sta $ff0b
        lda irqLineMSBTable, x
        sta $ff0a
        lda irqAddrLSBTable, x
        sta $fffe
        lda irqAddrMSBTable, x
        sta $ffff
        lda #$ce
l4:     cmp $ff1d
        bne l4
        lda #$ce
        sta $ff1d
        lda #$0b
        sta $ff06
        dec $ff09
        pla
        tay
        pla
        tax
l12:    lda #$00
        rti
.endproc

endIRQLine = fliIRQ::l4 + 6
currentInterlaceField = fliIRQ::l7 + 1
fliCodeAddrMSB = fliIRQ::l8 + 2
fliInterlaceEnabled = fliIRQ::l9 + 1
startIRQLineLSB = fliIRQ::l10 + 1
startIRQLineMSB = fliIRQ::l10 + 6

; -----------------------------------------------------------------------------

interlaceIRQ:
        sta @l4 + 1
        lda $ff1e
        and #$0e
        lsr
        sta @l1 - 1
        bpl @l1
@l1:    lda #$a9
        lda #$a9
        lda #$a9
        lda $ea
        txa
        pha
        ldx #$0d
@l2:    lda #$6d
        dex
        bne @l2
        clc
        sta $ff1e
        lda $ff0b
        adc #$06
        sta $ff0b
        bcc @l3
        inc $ff0a
@l3:    lda #<interlaceIRQ2
        sta $fffe
        lda #>interlaceIRQ2
        sta $ffff
        dec $ff09
        pla
        tax
@l4:    lda #$00
        rti

interlaceIRQ2:
        sta @l4 + 1
        lda $ff1e
        and #$0e
        lsr
        sta @l1 - 1
        bpl @l1
@l1:    lda #$a9
        lda #$a9
        lda #$a9
        lda $ea
        txa
        pha
        lda #<interlaceIRQ3
        sta $fffe
        lda #>interlaceIRQ3
        sta $ffff
        ldx #$07
@l2:    lda #$ad
        dex
        bne @l2
        clc
        sta $ff1e
        lda $ff0b
        adc #$06
        sta $ff0b
        bcc @l3
        inc $ff0a
@l3:    dec $ff09
        pla
        tax
@l4:    lda #$00
        rti

interlaceIRQ3:
        pha
        dec $ff1d
        lda irqLineLSBTable
        sta $ff0b
        lda irqLineMSBTable
        sta $ff0a
        lda irqAddrLSBTable
        sta $fffe
        lda irqAddrMSBTable
        sta $ffff
        dec $ff09
        pla
        rti

; -----------------------------------------------------------------------------

startFLICodeBlock:
        pha
        lda addr0ZP
        sta fliCodeAddrLSBTable, x
        lda addr0ZP + 1
        sta fliCodeAddrMSBTable, x
        pla
writeFLICodeBlock:
        stx tmp1ZP
        tax
        tya
        pha
        ldy fliCodeBytesTable, x
        tya
        pha
        dey
        clc
        adc fliCodeOffsetTable, x
        tax
        dex
        lda interlaceFlags
        bne @l2
@l1:    lda fliCode0, x
        sta (addr0ZP), y
        dex
        dey
        bpl @l1
        bmi @l3
@l2:    lda fliCode0, x
        sta (addr0ZP), y
        sta (addr1ZP), y
        dex
        dey
        bpl @l2
@l3:    pla
        clc
        adc addr0ZP
        sta addr0ZP
        sta addr1ZP
        bcc @l4
        inc addr0ZP + 1
        inc addr1ZP + 1
@l4:    pla
        tay
        ldx tmp1ZP
        rts

writeFLICodeByte:
        cpy #$80
        bcc @l1
        dec addr0ZP + 1
        dec addr1ZP + 1
@l1:    sta (addr0ZP), y
        lda interlaceFlags
        beq @l2
        txa
        sta (addr1ZP), y
@l2:    cpy #$80
        bcc @l3
        inc addr0ZP + 1
        inc addr1ZP + 1
@l3:    rts

fliCode0:
        ldy #$00
        sty $ff14
        ldy #$00
        sty $ff07
        ldy #$00
        sty $ff15
fliCode1:
        nop
        bit $ea
fliCode2:
        nop
        jmp $0000
fliCode3:
fliCode8:
        lda #$00
        bit $ea
fliCode4:
        sta $ff08, x
fliCode5:
        ldx #$ca
        bit $ea
fliCode6:
        ldx #$ca
        jmp $0000
fliCode7:
        ldy #$00
        sty $ff16
        ldy #$00
        sty $ff07
        stx $ff1d
        ldy #$00
        sty $ff16
fliCode9:
        sta $ff14
fliCode10:
        inc $ff14
fliCode12:
        ldx #$ca
fliCode11:
        stx $ff1d
fliCode13:
        ldy #$00
        sty $ff07
        ldy #$00
        sty $ff15
        ldy #$00
        sty $ff16
fliCodeEnd:

; -----------------------------------------------------------------------------

endCode:
        .res $17a4 - endCode, $00

fliCodeOffsetTable:
        .byte fliCode0 - fliCode0, fliCode1 - fliCode0
        .byte fliCode2 - fliCode0, fliCode3 - fliCode0
        .byte fliCode4 - fliCode0, fliCode5 - fliCode0
        .byte fliCode6 - fliCode0, fliCode7 - fliCode0
        .byte fliCode8 - fliCode0, fliCode9 - fliCode0
        .byte fliCode10 - fliCode0, fliCode11 - fliCode0
        .byte fliCode12 - fliCode0, fliCode13 - fliCode0

fliCodeBytesTable:
        .byte fliCode1 - fliCode0, fliCode2 - fliCode1
        .byte fliCode3 - fliCode2, fliCode4 - fliCode3
        .byte fliCode5 - fliCode4, fliCode6 - fliCode5
        .byte fliCode7 - fliCode6, fliCode9 - fliCode7
        .byte 2, fliCode10 - fliCode9
        .byte fliCode12 - fliCode10, fliCode13 - fliCode11
        .byte fliCode13 - fliCode12, fliCodeEnd - fliCode13

fliCodeSplitCntTable:
        .byte $ff, $ff, $ff, $15, $ff, $15, $15, $15
fli0CodeStartMSBTable:
        .byte $61, $61, $80, $a0, $61, $a9, $a9, $a9
fli1CodeStartMSBTable:
        .byte $70, $70, $8f, $af, $73, $c9, $c9, $c9

attribute0AddrTable:
        .byte $40, $48, $50, $58, $18, $a8, $b0, $b8
attribute1AddrTable:
        .byte $80, $88, $90, $98, $e0, $c8, $d0, $d8

bitmapAddrTable:
        .byte $c8, $d8
bitmapAddrTable2:
        .byte $e8, $f0
irqLineLSBTable:
        .byte $36, $f9
irqLineMSBTable:
        .byte $a3, $a2
irqAddrLSBTable:
        .byte <fliIRQ, <interlaceIRQ
irqAddrMSBTable:
        .byte >fliIRQ, >interlaceIRQ

decompressCallback:
        .addr resetRoutine
initCallback:
        .addr dummyCallback
frameCallback:
        .addr dummyCallback
spaceCallback:
        .addr main

lineBlankCnt1:
        .byte $00
lineBlankCnt2:
        .byte $00
compressedDataSizeMSB:
        .byte $00
compressedDataSizeLSB:
        .byte $00

