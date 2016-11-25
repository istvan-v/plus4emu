#!/usr/bin/python

import string
import re
import os

def assembleFile(infileName, outfileName):
    tmpName = '__ca65_tmp__' + str(os.getpid()) + '__.o'
    try:
        os.remove(tmpName)
    except:
        pass
    os.spawnvp(os.P_WAIT, 'ca65', ['ca65', '--target', 'none',
                                   infileName, '-o', tmpName])
    os.spawnvp(os.P_WAIT, 'ld65', ['ld65', '--target', 'none',
                                   tmpName, '-o', outfileName])
    os.remove(tmpName)

def assembleDecompressor(decompMode):
    f = open('decompress2_sfx.s', 'r')
    outputText = ''
    c16Mode = int((decompMode & 1) != 0)
    noCRCCheck = int((decompMode & 2) != 0)
    noReadBuffer = int((decompMode & 4) != 0)
    borderEffectType = int((decompMode & 24) / 8)
    noBlankDisplay = int((decompMode & 32) != 0)
    noZPSaveRestore = int((decompMode & 64) != 0)
    sfxStartAddress = 4109
    if not (c16Mode or noCRCCheck):
        sfxStartAddress = 4117
    for s in f:
        t = re.sub('define SFX_C16_MODE  *0',
                   'define SFX_C16_MODE ' + str(c16Mode), s)
        t = re.sub('define NO_CRC_CHECK  *0',
                   'define NO_CRC_CHECK ' + str(noCRCCheck), t)
        t = re.sub('define NO_READ_BUFFER  *0',
                   'define NO_READ_BUFFER ' + str(noReadBuffer), t)
        t = re.sub('define BORDER_EFFECT_TYPE  *2',
                   'define BORDER_EFFECT_TYPE ' + str(borderEffectType), t)
        t = re.sub('define NO_BLANK_DISPLAY  *0',
                   'define NO_BLANK_DISPLAY ' + str(noBlankDisplay), t)
        t = re.sub('define NO_ZP_SAVE_RESTORE  *0',
                   'define NO_ZP_SAVE_RESTORE ' + str(noZPSaveRestore), t)
        t = re.sub('define NO_ROM_ENABLE_RESTORE  *0',
                   'define NO_ROM_ENABLE_RESTORE 1', t)
        t = re.sub('define SFX_NO_CLI  *0',
                   'define SFX_NO_CLI 1', t)
        t = re.sub('define SFX_RUN_BASIC_PROGRAM  *1',
                   'define SFX_RUN_BASIC_PROGRAM 0', t)
        t = re.sub('sfxStartAddress = .*',
                   'sfxStartAddress = ' + str(sfxStartAddress), t)
        outputText = outputText + t
    f.close()
    asmFileName = '__ca65_tmp__' + str(os.getpid()) + '__.s'
    prgFileName = '__' + str(os.getpid()) + '__' + str(decompMode) + '__.prg'
    try:
        os.remove(asmFileName)
    except:
        pass
    try:
        os.remove(prgFileName)
    except:
        pass
    f = open(asmFileName, 'w')
    f.write(outputText)
    f.flush()
    f.close()
    assembleFile(asmFileName, prgFileName)
    os.remove(asmFileName)

outputData = []
outputDataPos = 0
for i in range(0, 512):
    outputData += [0]
    outputDataPos = outputDataPos + 1

for i in range(0, 128):
    assembleDecompressor(i)
    prgFileName = '__' + str(os.getpid()) + '__' + str(i) + '__.prg'
    f = open(prgFileName, 'rb')
    fileData = []
    while 1:
        c = f.read(1)
        if c.__len__() != 1:
            break
        fileData = fileData + [ord(c)]
    f.close()
    os.remove(prgFileName)
    outputData[(i * 4) + 0] = outputDataPos & 0xFF
    outputData[(i * 4) + 1] = (outputDataPos & 0xFF00) / 256
    outputData[(i * 4) + 2] = fileData.__len__() & 0xFF
    outputData[(i * 4) + 3] = (fileData.__len__() & 0xFF00) / 256
    for i in range(0, fileData.__len__()):
        outputData = outputData + [fileData[i]]
        outputDataPos = outputDataPos + 1

tmpFileName = '__' + str(os.getpid()) + '__.bin'
try:
    os.remove(tmpFileName)
except:
    pass
f = open(tmpFileName, 'wb')
for i in range(0, outputData.__len__()):
    f.write(chr(outputData[i]))
f.flush()
f.close()

try:
    os.remove('sfxdecomp2.bin')
except:
    pass
os.spawnvp(os.P_WAIT,
           '../../p4compress',
           ['../../p4compress', '-m2', '-noprg', '-9',
            tmpFileName, 'sfxdecomp2.bin'])
os.remove(tmpFileName)

