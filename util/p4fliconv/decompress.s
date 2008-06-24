
; NOTE: the start address should be chosen so that the routines from
; huffmanDecode1 to read5Bits are on the same 256-byte page
; ((startAddr & $f8) = 0 is recommended)

startAddr = $e504

; do not verify checksum if this is set to any non-zero value
.define NO_CRC_CHECK            0
; do not read ahead one byte of compressed data
.define NO_READ_BUFFER          0
; disable border effect and saving/restoring of the border color
.define NO_BORDER_EFFECT        0
; do not clear the color memory ($0800-$0B5F) after decompression
.define NO_COLOR_MEMORY_CLEAR   0
; do not restore RAM/ROM paging after decompression, return in RAM ($FF3F) mode
.define NO_ROM_ENABLE_RESTORE   0
; do not blank display, and do not save/restore $FF06
.define NO_BLANK_DISPLAY        0

        .setcpu "6502"
        .code
        .byte <startAddr
        .byte >startAddr
        .org startAddr

        .export decompressDataBlock
        .export decompressData
        .export decompressFLI

shiftRegister = $20
crcValue = $20
tmpValue = $21
readByteBuffer = $24
huffmanInitTmp = $25
huffTableWriteAddrLow = $26
huffTableWriteAddrHigh = $27
huffCodeSizesRemaining = $28
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
huffmanDecodedValueLow = $3a
huffmanDecodedValueHigh = $3b
huffSymbolsRemainingLow = $3c
huffSymbolsRemainingHigh = $3d
gammaDecodedValueHigh = $3f
huffmanLimitLowTable = $40
huffmanLimitHighTable = $50
huffmanOffsetLowTable = $60
huffmanOffsetHighTable = $70

borderColor = $ff19

; -----------------------------------------------------------------------------

compressedFLIDataSizeMSB = $17fe
compressedFLIDataSizeLSB = $17ff
compressedFLIDataStart = $1800
compressedFLIDataEnd = $e504

        .proc decompressFLI
        ldx #$e0
        lda #<compressedFLIDataStart
        sta $00, x
        clc
        adc compressedFLIDataSizeLSB
        sta $02, x
        lda #>compressedFLIDataStart
        sta $01, x
        adc compressedFLIDataSizeMSB
        sta $03, x
        lda #<compressedFLIDataEnd
        sta $04, x
        lda #>compressedFLIDataEnd
        sta $05, x
        jmp decompressData
        .endproc

; -----------------------------------------------------------------------------

        .proc decompressDataBlock
        ldx #$03                        ; read address and 65536 - data length
l2:     stx prvDistanceTablePos
        jsr read8Bits
        ldx prvDistanceTablePos
        sta bytesRemainingLow, x
        dex
        bpl l2
        ldx #$02
        jsr readXBits
        lsr
        pha                             ; save last block flag
        lda #<read8Bits
        sta readCharAddrLow
        bcc l3                          ; is compression enabled ?
        jsr huffmanInit
l3:
        .if NO_BORDER_EFFECT = 0
        inc borderColor                 ; border effect at LZ match
        .endif
l4:     jsr huffmanDecode1              ; read next character
        bcs l6
        sta (decompressWriteAddrLow), y ; store decompressed data
        inc decompressWriteAddrLow
        beq l17
l5:     inc bytesRemainingLow
        bne l4
l16:    inc bytesRemainingHigh
        bne l3                          ; border effect at 256 byte blocks
        pla                             ; return with A=1, Z=0 on last block
        rts
l17:    inc decompressWriteAddrHigh
        bcc l5
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
l8:     ldx #$00
l9:     stx deltaValue
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
l11:    tax
        inx                             ; adjust length to 2..256 range
        inx
        .byte $a9
l14:    iny
        lda (lzMatchReadAddrLow), y
        clc
        adc #$00
        sta (decompressWriteAddrLow), y
        dex
        bne l14
        tya
        sec
        adc decompressWriteAddrLow
        sta decompressWriteAddrLow
        bcc l15
        inc decompressWriteAddrHigh
l15:    tya
        ldy #$00
        sec
        adc bytesRemainingLow
        sta bytesRemainingLow
        bcc l3
        bcs l16
l12:    cmp #$40                        ; check special match codes
        bcs l13
        and #$03                        ; LZ match with delta value
        pha
        ldx #$07
        jsr readXBits
        sbc #$3f
        adc #$00
        tax
        pla
        bcc l9
l13:    adc prvDistanceTablePos         ; LZ match with recent offset
        and #$03
        tax
        lda prvDistanceLowTable, x
        ldy prvDistanceHighTable, x
        bcc l8
        .endproc

readCharAddrLow = decompressDataBlock::l4 + 1
readLengthAddrLow = decompressDataBlock::l10 + 1
deltaValue = decompressDataBlock::l14 + 5

; -----------------------------------------------------------------------------

        .proc huffmanInit
        jsr l1
        iny
l1:     sty huffmanInitTmp
        ldy #$00
        ldx #$01
        jsr readXBits
        ldy huffmanInitTmp
        lsr
        bcs l3
        lda addrTable + 2, y
l2:     ldx addrTable, y
        sta decompressDataBlock, x
        ldy #$00
        rts
l3:     tya
        adc #$09
        tay
        ldx #$05
l4:     sta huffmanInitTmp - 1, x
        dey
        dey
        lda addrTable + 4, y
        dex
        bne l4
        jsr l2
        ldx huffmanInitTmp
        lda #$01
l5:     asl
        sta huffmanLimitLowTable, x
        lda huffCodeSizesRemaining
        eor #$09
        beq l6
        tya
        rol
l6:     sta huffmanLimitHighTable, x
        ldy #$fe
        sty huffmanDecodedValueHigh
        iny
        sty huffmanDecodedValueLow
        iny
        lda huffTableWriteAddrLow
        sec
        sbc huffmanLimitLowTable, x
        sta huffmanOffsetLowTable, x
        lda huffTableWriteAddrHigh
        sbc huffmanLimitHighTable, x
        sta huffmanOffsetHighTable, x
        jsr gammaDecode
        sta huffSymbolsRemainingLow
        cmp #$01
        lda gammaDecodedValueHigh
        adc #$00
        sta huffSymbolsRemainingHigh
l7:     dec huffSymbolsRemainingLow
        bne l8
        dec huffSymbolsRemainingHigh
        beq l11
l8:     inc huffmanLimitLowTable, x
        bne l9
        inc huffmanLimitHighTable, x
l9:     jsr gammaDecode
        adc huffmanDecodedValueLow
        sta huffmanDecodedValueLow
        lda gammaDecodedValueHigh
        adc huffmanDecodedValueHigh
        sta huffmanDecodedValueHigh
        sta (huffTableWriteAddrLow), y
        inc huffTableWriteAddrHigh
        inc huffTableWriteAddrHigh
        lda huffmanDecodedValueLow
        sta (huffTableWriteAddrLow), y
        inc huffTableWriteAddrLow
        beq l10
        dec huffTableWriteAddrHigh
l10:    dec huffTableWriteAddrHigh
        bne l7
l11:    dec huffCodeSizesRemaining
        beq l12
        lda huffmanLimitLowTable, x
        ldy huffmanLimitHighTable, x
        inx
        bne l5
l12:    rts
        .endproc

; -----------------------------------------------------------------------------

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
        rts
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

        .proc readCompressedByte
        sta tmpValue
        inc readAddrLow
        bne l1
        inc readAddrHigh
l1:
        .if NO_READ_BUFFER = 0
        lda readByteBuffer
        rol
        sta shiftRegister
        lda (readAddrLow), y
        sta readByteBuffer
        .else
        lda (readAddrLow), y
        rol
        sta shiftRegister
        .endif
        lda tmpValue
        rts
        .endproc

        .proc gammaDecode
        lda #$01
        sty gammaDecodedValueHigh
l1:     asl shiftRegister
        bne l2
        jsr readCompressedByte
l2:     bcc gammaDecode - 1
        asl shiftRegister
        bne l3
        jsr readCompressedByte
l3:     rol
        rol gammaDecodedValueHigh
        bcc l1
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
        ldy #$00
        .if NO_BLANK_DISPLAY = 0
        lda $ff06                       ; save TED registers
        pha
        sty $ff06
        .endif
        .if NO_ROM_ENABLE_RESTORE = 0
        lda $ff13
        eor #$01
        and #$01
        pha
        .endif
        .if NO_BORDER_EFFECT = 0
        lda borderColor
        pha
        .endif
l1:     lda $0000, y
        sta $0940, y
        iny
        bne l1
        .if NO_CRC_CHECK = 0
        sty crcValue
        .endif
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
        ldx inputDataStartAddrLow
        stx inputDataEndAddrLow
        bcs l3
        dec inputDataEndAddrHigh
l3:     eor #$ff
        sec
        adc readAddrLow
        sta readAddrLow
        bcs l5
        .byte $2c
l4:     dec inputDataEndAddrHigh
        dec readAddrHigh
l5:     dey
        lda (inputDataEndAddrLow), y
        sta (readAddrLow), y
        .if NO_CRC_CHECK = 0
        eor crcValue
        asl
        adc #$c4
        sta crcValue
        .endif
        tya
        bne l5
l6:     lda inputDataEndAddrHigh
        cmp inputDataStartAddrHigh
        bne l4
l7:     lda #$80                        ; NOTE: this also initializes the
        .if NO_CRC_CHECK = 0
        cmp crcValue                    ; shift register (which is the same
        beq l8                          ; variable)
;       sty $ff3e                       ; reset machine on CRC error
        jmp ($fffc)
        .else
        sta shiftRegister
        .endif
l8:
        .if NO_READ_BUFFER = 0
        jsr read8Bits                   ; skip CRC byte
        .endif
l9:     jsr decompressDataBlock         ; decompress all data blocks
        beq l9
        .if NO_BORDER_EFFECT = 0
        pla                             ; restore TED registers
        sta borderColor
        .endif
        .if NO_ROM_ENABLE_RESTORE = 0
        pla
        tax
        sta $ff3e, x
        .endif
        ldx #$a0                        ; restore zeropage variables
l10:    lda $095f, x
        sta $1f, x
        dex
        bne l10
        .if NO_COLOR_MEMORY_CLEAR = 0
        lda $053b                       ; clear color memory
l11:    sta $0800, y
        sta $0900, y
        sta $0a00, y
        sta $0a60, y
        iny
        bne l11
        .endif
        .if NO_BLANK_DISPLAY = 0
        pla
        sta $ff06
        .endif
        plp
        rts
        .endproc

