
startAddr = $e500

        .org startAddr - 2
        .byte <startAddr
        .byte >startAddr

        .export decompressDataBlock
        .export decompressData
        .export decompressFLI

shiftRegister = $20
crcValue = $20
prvDistanceLowTable = $26
prvDistanceHighTable = $2a
bytesRemainingLow = $2e
bytesRemainingHigh = $2f
decompressWriteAddrLow = $30
decompressWriteAddrHigh = $31
tmpLow = $32
tmpHigh = $33
inputDataStartAddrLow = $33
inputDataStartAddrHigh = $34
inputDataEndAddrLow = $35
inputDataEndAddrHigh = $36
lzMatchReadAddrLow = $34
lzMatchReadAddrHigh = $35
readAddrLow = $37
readAddrHigh = $38
prvDistanceTablePos = $39
savedReadCharAddrLow = $3e
huffmanLimitLowTable = $40
huffmanLimitHighTable = $50
huffmanOffsetLowTable = $60
huffmanOffsetHighTable = $70

borderColor = $ff19

        .proc huffmanDecode1
        ldx #$ff
        .endproc

        .proc huffmanDecode2
        lda #$01
        sty tmpHigh
l1:     inx
        asl shiftRegister
        beq l8
l2:     rol
        bcs l5
        cmp huffmanLimitLowTable, x
        bcs l1
        bcc l7
l3:     inx
        asl shiftRegister
        bne l4
        ldy #$00
        jsr readCompressedByte
l4:     rol
        rol tmpHigh
        bmi l6
l5:     cmp huffmanLimitLowTable, x
        tay
        lda tmpHigh
        sbc huffmanLimitHighTable, x
        tya
        bcs l3
l6:     ldy #$00
l7:     adc huffmanOffsetLowTable, x
        sta tmpLow
        lda tmpHigh
        adc huffmanOffsetHighTable, x
        sta tmpHigh
        eor #$02
        cmp (tmpLow), y
        sta tmpHigh
        lda (tmpLow), y
        rts
l8:     jsr readCompressedByte
        bne l2
        .endproc

        .proc read1Bit
        ldx #$01
        .byte $2c
        .endproc

        .proc read9Bits
        ldx #$09
        .byte $2c
        .endproc

        .proc read8Bits
        ldx #$08
        .byte $2c
        .endproc

        .proc read5Bits
        ldx #$05
        .endproc

        .proc readXBits
        tya
l1:     asl shiftRegister
        bne l2
        jsr readCompressedByte
l2:     rol
        dex
        bne l1
        tax
        rts
        .endproc

        .proc readLZMatchByteWithDelta
        lda (lzMatchReadAddrLow), y
        adc #$00
        clc
        .byte $2c
        .endproc
deltaValue = readLZMatchByteWithDelta + 3

        .proc readLZMatchByte
        lda (lzMatchReadAddrLow), y
        dex
        beq l2
        inc lzMatchReadAddrLow
        bne l1
        inc lzMatchReadAddrHigh
l1:     rts
l2:     ldx savedReadCharAddrLow
        stx readCharAddrLow
        rts
        .endproc

        .proc readCompressedByte
        sta $21
        lda $24
        rol
        sta shiftRegister
        inc $37
        bne l1
        inc $38
l1:     lda ($37), y
        sta $24
        lda $21
        rts
        .endproc

        .proc gammaDecode
        lda #$01
        sty $3f
l1:     asl shiftRegister
        bne l2
        jsr readCompressedByte
l2:     bcc gammaDecode - 1
        asl shiftRegister
        bne l3
        jsr readCompressedByte
l3:     rol
        rol $3f
        bcc l1
        .endproc

        .proc readLZMatchParameterBits
        pha
        lsr
        lsr
        tax
        dex
        pla
        and #$03
        ora #$04
l1:     asl shiftRegister
        bne l2
        jsr readCompressedByte
l2:     rol
        rol tmpHigh
        dex
        bne l1
        rts
        .endproc

decompressDataBlock = decompressDataBlock_ + 6

        .proc decompressDataBlock_
l1:     inc bytesRemainingHigh
        bne l5
        plp                             ; return with carry set on last block
        rts
        ldx #$03                        ; read address and 65536 - data length
l2:     stx prvDistanceTablePos
        jsr read8Bits
        ldx prvDistanceTablePos
        sta bytesRemainingLow, x
        dex
        bpl l2
        jsr read1Bit
        lsr
        php                             ; save last block flag
        lda #<read8Bits
        sta readCharAddrLow
        jsr read1Bit                    ; is compression enabled ?
        beq l3
        jsr huffmanInit
l3:     inc borderColor                 ; border effect at LZ match
l4:     jsr huffmanDecode1              ; read next character
        bcs l6
        sta (decompressWriteAddrLow), y ; store decompressed data
        inc bytesRemainingLow
        beq l1
l5:     inc decompressWriteAddrLow
        bne l4
        inc decompressWriteAddrHigh
        bcc l3                          ; border effect at 256 byte blocks
l6:     cmp #$3c                        ; LZ match code
        bcs l12
        cmp #$08
        bcc l8
        sty tmpHigh                     ; read offset
        jsr readLZMatchParameterBits
        ldx prvDistanceTablePos         ; store in recent offset buffer
        ldy tmpHigh
        sta prvDistanceLowTable, x
        sty prvDistanceHighTable, x
        dex
        bpl l7
        ldx #$03
l7:     stx prvDistanceTablePos
l8:     ldx #<readLZMatchByte
l9:     stx readCharAddrLow             ; set character read routine address
        eor #$ff                        ; calculate match address
        adc decompressWriteAddrLow
        sta lzMatchReadAddrLow
        tya
        eor #$ff
        adc decompressWriteAddrHigh
        sta lzMatchReadAddrHigh
        ldy #$00                        ; read match length code
        ldx #$3f
l10:    jsr huffmanDecode2
        cmp #$08
        bcc l11
        jsr readLZMatchParameterBits    ; read extra bits if length >= 10 bytes
        clc
l11:    tax
        inx                             ; adjust length to 2..256 range
        inx
        bcc l3
l12:    cmp #$40                        ; check special match codes
        bcs l13
        and #$03                        ; LZ match with delta value
        pha
        ldx #$07
        jsr readXBits
        sbc #$3f
        adc #$00
        sta deltaValue
        ldx #<readLZMatchByteWithDelta
        pla
        bcc l9
l13:    adc prvDistanceTablePos         ; LZ match with recent offset
        and #$03
        tax
        lda prvDistanceLowTable, x
        ldy prvDistanceHighTable, x
        bcc l8
        .endproc

readCharAddrLow = decompressDataBlock_::l4 + 1
readLengthAddrLow = decompressDataBlock_::l10 + 1

        .proc huffmanInit
        jsr l1
        iny
l1:     sty $25
        ldy #$00
        jsr read1Bit
        ldy $25
        lsr
        bcs l3
        lda addrTable + 2, y
l2:     ldx addrTable, y
        sta savedReadCharAddrLow, y
        sta decompressDataBlock, x
        ldy #$00
        rts
l3:     tya
        adc #$09
        tay
        ldx #$05
l4:     sta $24, x
        dey
        dey
        lda addrTable + 4, y
        dex
        bne l4
        jsr l2
        ldx $25
        lda #$01
l5:     asl
        sta huffmanLimitLowTable, x
        lda $28
        eor #$09
        beq l6
        tya
        rol
l6:     sta huffmanLimitHighTable, x
        ldy #$fe
        sty $3b
        iny
        sty $3a
        iny
        lda $26
        sec
        sbc huffmanLimitLowTable, x
        sta huffmanOffsetLowTable, x
        lda $27
        sbc huffmanLimitHighTable, x
        sta huffmanOffsetHighTable, x
        jsr gammaDecode
        sta $3c
        cmp #$01
        lda $3f
        adc #$00
        sta $3d
l7:     dec $3c
        bne l8
        dec $3d
        beq l11
l8:     inc huffmanLimitLowTable, x
        bne l9
        inc huffmanLimitHighTable, x
l9:     jsr gammaDecode
        adc $3a
        sta $3a
        lda $3f
        adc $3b
        sta $3b
        sta ($26), y
        inc $27
        inc $27
        lda $3a
        sta ($26), y
        inc $26
        beq l10
        dec $27
l10:    dec $27
        bne l7
l11:    dec $28
        beq l12
        lda huffmanLimitLowTable, x
        ldy huffmanLimitHighTable, x
        inx
        bne l5
l12:    rts
        .endproc

addrTable:
        .byte <(readCharAddrLow - decompressDataBlock)
        .byte <(readLengthAddrLow - decompressDataBlock)
        .byte <read9Bits
        .byte <read5Bits
        .byte <huffmanDecode1
        .byte <huffmanDecode2
        .byte $00, $40
        .byte $00, $44
        .byte $08, $09
        .byte $10, $10

; -----------------------------------------------------------------------------

        .proc decompressData
        php
        sei
        cld
        ldy #$0b
        lda $ff06                       ; save TED registers
        pha
        sty $ff06
        lda $ff13
        eor #$01
        and #$01
        pha
        lda borderColor
        pha
l1:     lda $0000, y
        sta $0940, y
        iny
        bne l1
        sty crcValue
        sty $ff3f
l2:     lda $0940, x
        sta inputDataStartAddrLow, y
        inx
        iny
        cpy #$06
        bne l2
        lda inputDataEndAddrLow
        sec
        sbc inputDataStartAddrLow
        tay
        beq l6
        ldx #<inputDataEndAddrLow
l3:     tya
        eor #$ff
        sec
        adc $00, x
        sta $00, x
        bcs l4
        dec $01, x
l4:     cpx #<readAddrLow
        ldx #<readAddrLow
        bcc l3
l5:     dey
        lda (inputDataEndAddrLow), y
        sta (readAddrLow), y
        eor crcValue
        asl
        adc #$c4
        sta crcValue
        tya
        bne l5
l6:     lda inputDataEndAddrHigh
        cmp inputDataStartAddrHigh
        beq l7
        dec inputDataEndAddrHigh
        dec readAddrHigh
        bne l5
l7:     lda #$80                        ; NOTE: this also initializes the
        cmp crcValue                    ; shift register (which is the same
        beq l8                          ; variable)
;       sty $ff3e                       ; reset machine on CRC error
        jmp ($fffc)
l8:     jsr read8Bits                   ; skip CRC byte
l9:     jsr decompressDataBlock         ; decompress all data blocks
        bcc l9
        pla                             ; restore TED registers
        sta borderColor
        pla
        tax
        sta $ff3e, x
        ldy #$a0                        ; restore zeropage variables
l10:    lda $095f, y
        sta $001f, y
        dey
        bne l10
        lda $053b
l11:    sta $0800, y                    ; clear color memory
        sta $0900, y
        sta $0a00, y
        sta $0b00, y
        iny
        bne l11
        pla
        sta $ff06
        plp
        rts
        .endproc

; -----------------------------------------------------------------------------

compressedFLIDataSizeMSB = $17fe
compressedFLIDataSizeLSB = $17ff
compressedFLIDataStart = $1800
compressedFLIDataEnd = $e500

        .proc decompressFLI
        lda #<compressedFLIDataStart
        sta $e0
        clc
        adc compressedFLIDataSizeLSB
        sta $e2
        lda #>compressedFLIDataStart
        sta $e1
        adc compressedFLIDataSizeMSB
        sta $e3
        lda #<compressedFLIDataEnd
        sta $e4
        lda #>compressedFLIDataEnd
        sta $e5
        ldx #$e0
        jmp decompressData
        .endproc

