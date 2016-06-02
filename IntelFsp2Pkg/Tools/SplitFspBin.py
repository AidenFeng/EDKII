## @ FspTool.py
#
# Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

import os
import sys
import uuid
import copy
import struct
import argparse
from   ctypes import *

"""
This utility supports some operations for Intel FSP 2.0 image.
It supports:
    - Display FSP 2.0 information header
    - Split FSP 2.0 image into individual FSP-T/M/S/O component
    - Rebase FSP 2.0 components to a different base address
    - Generate FSP mapping C header file
"""

CopyRightHeaderFile = """/*
 *
 * Automatically generated file; DO NOT EDIT.
 * FSP mapping file
 *
 */
"""

class c_uint24(Structure):
    """Little-Endian 24-bit Unsigned Integer"""
    _pack_   = 1
    _fields_ = [('Data', (c_uint8 * 3))]

    def __init__(self, val=0):
        self.set_value(val)

    def __str__(self, indent=0):
        return '0x%.6x' % self.value

    def __int__(self):
        return self.get_value()

    def set_value(self, val):
        self.Data[0:3] = Val2Bytes(val, 3)

    def get_value(self):
        return Bytes2Val(self.Data[0:3])

    value = property(get_value, set_value)

class EFI_FIRMWARE_VOLUME_HEADER(Structure):
    _fields_ = [
        ('ZeroVector',           ARRAY(c_uint8, 16)),
        ('FileSystemGuid',       ARRAY(c_uint8, 16)),
        ('FvLength',             c_uint64),
        ('Signature',            ARRAY(c_char, 4)),
        ('Attributes',           c_uint32),
        ('HeaderLength',         c_uint16),
        ('Checksum',             c_uint16),
        ('ExtHeaderOffset',      c_uint16),
        ('Reserved',             c_uint8),
        ('Revision',             c_uint8)
        ]

class EFI_FIRMWARE_VOLUME_EXT_HEADER(Structure):
    _fields_ = [
        ('FvName',               ARRAY(c_uint8, 16)),
        ('ExtHeaderSize',        c_uint32)
        ]

class EFI_FFS_INTEGRITY_CHECK(Structure):
    _fields_ = [
        ('Header',               c_uint8),
        ('File',                 c_uint8)
        ]

class EFI_FFS_FILE_HEADER(Structure):
    _fields_ = [
        ('Name',                 ARRAY(c_uint8, 16)),
        ('IntegrityCheck',       EFI_FFS_INTEGRITY_CHECK),
        ('Type',                 c_uint8),
        ('Attributes',           c_uint8),
        ('Size',                 c_uint24),
        ('State',                c_uint8)
        ]

class EFI_COMMON_SECTION_HEADER(Structure):
    _fields_ = [
        ('Size',                 c_uint24),
        ('Type',                 c_uint8)
        ]

class FSP_COMMON_HEADER(Structure):
     _fields_ = [
        ('Signature',            ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint32)
        ]

class FSP_INFORMATION_HEADER(Structure):
     _fields_ = [
        ('Signature',            ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint32),
        ('Reserved1',            c_uint16),
        ('SpecVersion',          c_uint8),
        ('HeaderRevision',       c_uint8),
        ('ImageRevision',        c_uint32),
        ('ImageId',              ARRAY(c_char, 8)),
        ('ImageSize',            c_uint32),
        ('ImageBase',            c_uint32),
        ('ImageAttribute',       c_uint16),
        ('ComponentAttribute',   c_uint16),
        ('CfgRegionOffset',      c_uint32),
        ('CfgRegionSize',        c_uint32),
        ('Reserved2',            c_uint32),
        ('TempRamInitEntryOffset',     c_uint32),
        ('Reserved3',                  c_uint32),
        ('NotifyPhaseEntryOffset',     c_uint32),
        ('FspMemoryInitEntryOffset',   c_uint32),
        ('TempRamExitEntryOffset',     c_uint32),
        ('FspSiliconInitEntryOffset',  c_uint32)
    ]

class FSP_PATCH_TABLE(Structure):
    _fields_ = [
        ('Signature',            ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint16),
        ('HeaderRevision',       c_uint8),
        ('Reserved',             c_uint8),
        ('PatchEntryNum',        c_uint32)
        ]

class EFI_IMAGE_DATA_DIRECTORY(Structure):
    _fields_ = [
        ('VirtualAddress',       c_uint32),
        ('Size',                 c_uint32)
        ]

class EFI_TE_IMAGE_HEADER(Structure):
    _fields_ = [
        ('Signature',            ARRAY(c_char, 2)),
        ('Machine',              c_uint16),
        ('NumberOfSections',     c_uint8),
        ('Subsystem',            c_uint8),
        ('StrippedSize',         c_uint16),
        ('AddressOfEntryPoint',  c_uint32),
        ('BaseOfCode',           c_uint32),
        ('ImageBase',            c_uint64),
        ('DataDirectoryBaseReloc',  EFI_IMAGE_DATA_DIRECTORY),
        ('DataDirectoryDebug',      EFI_IMAGE_DATA_DIRECTORY)
        ]

class PE_RELOC_BLOCK_HEADER(Structure):
    _fields_ = [
        ('PageRVA',              c_uint32),
        ('BlockSize',            c_uint32)
        ]

class EFI_FV_FILETYPE:
    ALL                        = 0x00
    RAW                        = 0x01
    FREEFORM                   = 0x02
    SECURITY_CORE              = 0x03
    PEI_CORE                   = 0x04
    DXE_CORE                   = 0x05
    PEIM                       = 0x06
    DRIVER                     = 0x07
    COMBINED_PEIM_DRIVER       = 0x08
    APPLICATION                = 0x09
    SMM                        = 0x0a
    FIRMWARE_VOLUME_IMAGE      = 0x0b
    COMBINED_SMM_DXE           = 0x0c
    SMM_CORE                   = 0x0d
    OEM_MIN                    = 0xc0
    OEM_MAX                    = 0xdf
    DEBUG_MIN                  = 0xe0
    DEBUG_MAX                  = 0xef
    FFS_MIN                    = 0xf0
    FFS_MAX                    = 0xff
    FFS_PAD                    = 0xf0

class EFI_SECTION_TYPE:
    """Enumeration of all valid firmware file section types."""
    ALL                        = 0x00
    COMPRESSION                = 0x01
    GUID_DEFINED               = 0x02
    DISPOSABLE                 = 0x03
    PE32                       = 0x10
    PIC                        = 0x11
    TE                         = 0x12
    DXE_DEPEX                  = 0x13
    VERSION                    = 0x14
    USER_INTERFACE             = 0x15
    COMPATIBILITY16            = 0x16
    FIRMWARE_VOLUME_IMAGE      = 0x17
    FREEFORM_SUBTYPE_GUID      = 0x18
    RAW                        = 0x19
    PEI_DEPEX                  = 0x1b
    SMM_DEPEX                  = 0x1c

def AlignPtr (offset, alignment = 8):
    return (offset + alignment - 1) & ~(alignment - 1)

def Bytes2Val (bytes):
    return reduce(lambda x,y: (x<<8)|y,  bytes[::-1] )

def Val2Bytes (value, blen):
    return [(value>>(i*8) & 0xff) for i in range(blen)]

def OutputStruct (obj, indent = 0, plen = 0):
    if indent:
        body = ''
    else:
        body = ('  ' * indent + '<%s>:\n') % obj.__class__.__name__

    if plen == 0:
        plen = sizeof(obj)

    max_key_len = 26
    pstr = ('  ' * (indent + 1) + '{0:<%d} = {1}\n') % max_key_len

    for field in obj._fields_:
        key = field[0]
        val = getattr(obj, key)
        rep = ''
        if not isinstance(val, c_uint24) and isinstance(val, Structure):
            body += pstr.format(key, val.__class__.__name__)
            body += OutputStruct (val, indent + 1)
            plen -= sizeof(val)
        else:
            if type(val) is str:
                rep = "0x%X ('%s')" % (Bytes2Val(bytearray(val)), val)
            elif type(val) in (int, long):
                rep = '0x%X' % val
            elif isinstance(val, c_uint24):
                rep = '0x%X' % val.get_value()
            elif 'c_ubyte_Array' in str(type(val)):
                if sizeof(val) == 16:
                    rep = str(uuid.UUID(bytes = str(bytearray(val)))).upper()
                else:
                    res = ['0x%02X'%i for i in bytearray(val)]
                    rep = '[%s]' % (','.join(res))
            else:
                rep = str(val)
            plen -= sizeof(field[1])
            body += pstr.format(key, rep)
        if plen <= 0:
            break
    return body

class Section:
    def __init__(self, offset, secdata):
        self.SecHdr   = EFI_COMMON_SECTION_HEADER.from_buffer (secdata, 0)
        self.SecData  = secdata[0:int(self.SecHdr.Size)]
        self.Offset   = offset

class FirmwareFile:
    def __init__(self, offset, filedata):
        self.FfsHdr   = EFI_FFS_FILE_HEADER.from_buffer (filedata, 0)
        self.FfsData  = filedata[0:int(self.FfsHdr.Size)]
        self.Offset   = offset
        self.SecList  = []

    def ParseFfs(self):
        ffssize = len(self.FfsData)
        offset  = sizeof(self.FfsHdr)
        if self.FfsHdr.Name != '\xff' * 16:
            while offset < ffssize:
                sechdr = EFI_COMMON_SECTION_HEADER.from_buffer (self.FfsData, offset)
                sec = Section (offset, self.FfsData[offset:offset + int(sechdr.Size)])
                self.SecList.append(sec)
                offset += int(sechdr.Size)
                offset  = AlignPtr(offset, 4)

class FirmwareVolume:
    def __init__(self, offset, fvdata):
        self.FvHdr    = EFI_FIRMWARE_VOLUME_HEADER.from_buffer (fvdata, 0)
        self.FvData   = fvdata[0 : self.FvHdr.FvLength]
        self.Offset   = offset
        if self.FvHdr.ExtHeaderOffset > 0:
            self.FvExtHdr = EFI_FIRMWARE_VOLUME_EXT_HEADER.from_buffer (self.FvData, self.FvHdr.ExtHeaderOffset)
        else:
            self.FvExtHdr = None
        self.FfsList  = []

    def ParseFv(self):
        fvsize = len(self.FvData)
        if self.FvExtHdr:
            offset = self.FvHdr.ExtHeaderOffset + self.FvExtHdr.ExtHeaderSize
        else:
            offset = self.FvHdr.HeaderLength
        offset = AlignPtr(offset)
        while offset < fvsize:
            ffshdr = EFI_FFS_FILE_HEADER.from_buffer (self.FvData, offset)
            if (ffshdr.Name == '\xff' * 16) and (int(ffshdr.Size) == 0xFFFFFF):
                offset = fvsize
            else:
                ffs = FirmwareFile (offset, self.FvData[offset:offset + int(ffshdr.Size)])
                ffs.ParseFfs()
                self.FfsList.append(ffs)
                offset += int(ffshdr.Size)
                offset = AlignPtr(offset)

class FspImage:
    def __init__(self, offset, fih, fihoff, patch):
        self.Fih       = fih
        self.FihOffset = fihoff
        self.Offset    = offset
        self.FvIdxList = []
        self.Type      = "XTMSXXXXOXXXXXXX"[(fih.ComponentAttribute >> 12) & 0x0F]
        self.PatchList = patch
        self.PatchList.append(fihoff + 0x1C)

    def AppendFv(self, FvIdx):
        self.FvIdxList.append(FvIdx)

    def Patch(self, delta, fdbin):
        count   = 0
        applied = 0
        for idx, patch in enumerate(self.PatchList):
            ptype = (patch>>24) & 0x0F
            if ptype not in [0x00, 0x0F]:
                raise Exception('ERROR: Invalid patch type %d !' % ptype)
            if patch & 0x80000000:
                patch = self.Fih.ImageSize - (0x1000000 - (patch & 0xFFFFFF))
            else:
                patch = patch & 0xFFFFFF
            if (patch < self.Fih.ImageSize) and (patch + sizeof(c_uint32) <= self.Fih.ImageSize):
                offset = patch + self.Offset
                value  = Bytes2Val(fdbin[offset:offset+sizeof(c_uint32)])
                value += delta
                fdbin[offset:offset+sizeof(c_uint32)] = Val2Bytes(value, sizeof(c_uint32))
                applied += 1
            count += 1
        # Don't count the FSP base address patch entry appended at the end
        if count != 0:
            count   -= 1
            applied -= 1
        return (count, applied)

class FirmwareDevice:
    def __init__(self, offset, fdfile):
        self.FvList  = []
        self.FspList = []
        self.FdFile = fdfile
        self.Offset = 0
        hfsp = open (self.FdFile, 'rb')
        self.FdData = bytearray(hfsp.read())
        hfsp.close()

    def ParseFd(self):
        offset = 0
        fdsize = len(self.FdData)
        self.FvList  = []
        while offset < fdsize:
            fvh = EFI_FIRMWARE_VOLUME_HEADER.from_buffer (self.FdData, offset)
            if '_FVH' != fvh.Signature:
                raise Exception("ERROR: Invalid FV header !")
            fv = FirmwareVolume (offset, self.FdData[offset:offset + fvh.FvLength])
            fv.ParseFv ()
            self.FvList.append(fv)
            offset += fv.FvHdr.FvLength

    def CheckFsp (self):
        if len(self.FspList) == 0:
            return

        fih = None
        for fsp in self.FspList:
            if fsp.Fih.HeaderRevision < 3:
                raise Exception("ERROR: FSP 1.x is not supported by this tool !")
            if not fih:
                fih = fsp.Fih
            else:
                newfih = fsp.Fih
                if (newfih.ImageId != fih.ImageId) or (newfih.ImageRevision != fih.ImageRevision):
                    raise Exception("ERROR: Inconsistent FSP ImageId or ImageRevision detected !")

    def ParseFsp(self):
        flen = 0
        for idx, fv in enumerate(self.FvList):
            # Check if this FV contains FSP header
            if flen == 0:
                if len(fv.FfsList) == 0:
                    continue
                ffs = fv.FfsList[0]
                if len(ffs.SecList) == 0:
                    continue
                sec = ffs.SecList[0]
                if sec.SecHdr.Type != EFI_SECTION_TYPE.RAW:
                    continue
                fihoffset = ffs.Offset + sec.Offset + sizeof(sec.SecHdr)
                fspoffset = fv.Offset
                offset    = fspoffset + fihoffset
                fih = FSP_INFORMATION_HEADER.from_buffer (self.FdData, offset)
                if 'FSPH' != fih.Signature:
                    continue

                offset += fih.HeaderLength
                offset = AlignPtr(offset, 4)
                plist  = []
                while True:
                    fch = FSP_COMMON_HEADER.from_buffer (self.FdData, offset)
                    if 'FSPP' != fch.Signature:
                        offset += fch.HeaderLength
                        offset = AlignPtr(offset, 4)
                    else:
                        fspp = FSP_PATCH_TABLE.from_buffer (self.FdData, offset)
                        offset += sizeof(fspp)
                        pdata  = (c_uint32 * fspp.PatchEntryNum).from_buffer(self.FdData, offset)
                        plist  = list(pdata)
                        break

                fsp  = FspImage (fspoffset, fih, fihoffset, plist)
                fsp.AppendFv (idx)
                self.FspList.append(fsp)
                flen = fsp.Fih.ImageSize - fv.FvHdr.FvLength
            else:
                fsp.AppendFv (idx)
                flen -= fv.FvHdr.FvLength
                if flen < 0:
                    raise Exception("ERROR: Incorrect FV size in image !")
        self.CheckFsp ()

class TeImage:
    def __init__(self, offset, tedata):
        self.Offset = offset
        self.TeHdr     = EFI_TE_IMAGE_HEADER.from_buffer (tedata, 0)
        self.TeData    = tedata
        self.RelocList = []

    def ParseReloc(self):
        rsize   = self.TeHdr.DataDirectoryBaseReloc.Size
        roffset   = sizeof(self.TeHdr) - self.TeHdr.StrippedSize + self.TeHdr.DataDirectoryBaseReloc.VirtualAddress
        alignment = 4
        offset = roffset
        while offset < roffset + rsize:
            offset = AlignPtr(offset, 4)
            blkhdr = PE_RELOC_BLOCK_HEADER.from_buffer(self.TeData, offset)
            offset += sizeof(blkhdr)
            # Read relocation type,offset pairs
            rlen  = blkhdr.BlockSize - sizeof(PE_RELOC_BLOCK_HEADER)
            rnum  = rlen/sizeof(c_uint16)
            rdata = (c_uint16 * rnum).from_buffer(self.TeData, offset)
            for each in rdata:
                roff  = each & 0xfff
                rtype = each >> 12
                if rtype == 0: # IMAGE_REL_BASED.ABSOLUTE:
                    continue
                if rtype != 3: # IMAGE_REL_BASED_HIGHLOW
                    raise Exception("ERROR: Unsupported relocation type %d!" % rtype)
                # Calculate the offset of the relocation
                aoff = sizeof(self.TeHdr) - self.TeHdr.StrippedSize + blkhdr.PageRVA + roff
                self.RelocList.append((rtype, aoff))
            offset += sizeof(rdata)

    def Rebase(self, delta, fdbin):
        count = 0
        if delta == 0:
            return count

        for (rtype, roff) in self.RelocList:
            if rtype == 0x03: # HIGHLOW
                offset = roff + self.Offset
                value  = Bytes2Val(fdbin[offset:offset+sizeof(c_uint32)])
                value += delta
                fdbin[offset:offset+sizeof(c_uint32)] = Val2Bytes(value, sizeof(c_uint32))
                count += 1
            else:
                raise Exception('ERROR: Unknown relocation type %d !' % rtype)

        tehdr = self.TeHdr
        tehdr.ImageBase += delta
        offset = self.Offset
        fdbin[offset:offset+sizeof(tehdr)] = bytearray(tehdr)

        return count

def ShowFspInfo (fspfile):
    fd = FirmwareDevice(0, fspfile)
    fd.ParseFd  ()
    fd.ParseFsp ()

    print ("\nFound the following %d Firmware Volumes in FSP binary:" % (len(fd.FvList)))
    for idx, fv in enumerate(fd.FvList):
        name = fv.FvExtHdr.FvName
        if not name:
            name = '\xff' * 16
        else:
            name = str(bytearray(name))
        guid = uuid.UUID(bytes = name)
        print ("FV%d:" % idx)
        print ("  GUID   : %s" % str(guid).upper())
        print ("  Offset : 0x%08X" %  fv.Offset)
        print ("  Length : 0x%08X" % fv.FvHdr.FvLength)
    print ("\n")

    for fsp in fd.FspList:
        fvlist = map(lambda x : 'FV%d' % x, fsp.FvIdxList)
        print ("FSP_%s contains %s" % (fsp.Type, ','.join(fvlist)))
        print ("%s" % (OutputStruct(fsp.Fih, 0, fsp.Fih.HeaderLength)))

def GenFspHdr (fspfile, outdir, hfile):
    fd = FirmwareDevice(0, fspfile)
    fd.ParseFd  ()
    fd.ParseFsp ()

    if not hfile:
        hfile = os.path.splitext(os.path.basename(fspfile))[0] + '.h'
    fspname, ext = os.path.splitext(os.path.basename(hfile))
    filename = os.path.join(outdir, fspname + ext)
    hfsp   = open(filename, 'w')
    hfsp.write ('%s\n\n' % CopyRightHeaderFile)

    firstfv = True
    for fsp in fd.FspList:
        fih = fsp.Fih
        if firstfv:
            hfsp.write("#define  FSP_IMAGE_ID    0x%016X    /* '%s' */\n" % (Bytes2Val(bytearray(fih.ImageId)), fih.ImageId))
            hfsp.write("#define  FSP_IMAGE_REV   0x%08X \n\n" % fih.ImageRevision)
            firstfv = False
        fv = fd.FvList[fsp.FvIdxList[0]]
        hfsp.write ('#define  FSP%s_BASE       0x%08X\n'   % (fsp.Type, fih.ImageBase))
        hfsp.write ('#define  FSP%s_OFFSET     0x%08X\n'   % (fsp.Type, fv.Offset))
        hfsp.write ('#define  FSP%s_LENGTH     0x%08X\n\n' % (fsp.Type, fih.ImageSize))

    hfsp.close()

def SplitFspBin (fspfile, outdir, nametemplate):
    fd = FirmwareDevice(0, fspfile)
    fd.ParseFd  ()
    fd.ParseFsp ()

    for fsp in fd.FspList:
        ftype = fsp.Type
        if not nametemplate:
            nametemplate = fspfile
        fspname, ext = os.path.splitext(os.path.basename(nametemplate))
        filename = os.path.join(outdir, fspname + '_' + fsp.Type + ext)
        hfsp = open(filename, 'wb')
        print ("Ceate FSP component file '%s'" % filename)
        for fvidx in fsp.FvIdxList:
            fv = fd.FvList[fvidx]
            hfsp.write(fv.FvData)
        hfsp.close()

def RebaseFspBin (FspBinary, FspComponent, FspBase, OutputDir, OutputFile):
    fd = FirmwareDevice(0, FspBinary)
    fd.ParseFd  ()
    fd.ParseFsp ()

    numcomp  = len(FspComponent)
    baselist = FspBase
    if numcomp != len(baselist):
        print "ERROR: Required number of base does not match number of FSP component !"
        return

    newfspbin = fd.FdData[:]

    for idx, fspcomp in enumerate(FspComponent):

        found = False
        for fsp in fd.FspList:
            ftype = fsp.Type.lower()
            if ftype == fspcomp:
                found = True
                break

        if not found:
            print "ERROR: Could not find FSP_%c component to rebase !" % fspcomp.upper()
            return

        fspbase = baselist[idx]
        if fspbase.startswith('0x'):
            newbase = int(fspbase, 16)
        else:
            newbase = int(fspbase)
        oldbase = fsp.Fih.ImageBase
        delta = newbase - oldbase
        print "Rebase FSP-%c from 0x%08X to 0x%08X:" % (ftype.upper(),oldbase,newbase)

        telist = []
        for fvidx in fsp.FvIdxList:
            fv = fd.FvList[fvidx]
            for ffs in fv.FfsList:
                for sec in ffs.SecList:
                    if sec.SecHdr.Type == EFI_SECTION_TYPE.TE:   # TE
                        offset = fd.Offset + fv.Offset + ffs.Offset + sec.Offset + sizeof(sec.SecHdr)
                        telist.append ((offset, len(sec.SecData) - sizeof(sec.SecHdr)))
                    elif sec.SecHdr.Type == EFI_SECTION_TYPE.PE32: # PE
                        raise Exception("ERROR: PE32 Section is not supported !")

        fcount  = 0
        tecount = 0
        for (teoffset, telen) in telist:
            tehdr = EFI_TE_IMAGE_HEADER.from_buffer (fd.FdData, teoffset)
            if 'VZ' != tehdr.Signature:
                raise Exception("ERROR: Invalid TE header !")
            te = TeImage(teoffset, fd.FdData[teoffset:teoffset + telen])
            te.ParseReloc()
            tecount += te.Rebase(delta, newfspbin)
            fcount  += 1
        print "  Patched %d entries in %d TE images." % (tecount, fcount)

        (count, applied) = fsp.Patch(delta, newfspbin)
        print "  Patched %d entries using FSP patch table." % applied
        if count != applied:
            print "  %d invalid entries are ignored !" % (count - applied)

    if OutputFile == '':
        filename = os.path.basename(FspBinary)
        base, ext  = os.path.splitext(filename)
        OutputFile = base + "_%08X" % newbase + ext

    fspname, ext = os.path.splitext(os.path.basename(OutputFile))
    filename = os.path.join(OutputDir, fspname + ext)
    fd = open(filename, "wb")
    fd.write(newfspbin)
    fd.close()

def main ():
    parser     = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(title='commands')

    parser_rebase  = subparsers.add_parser('rebase',  help='rebase a FSP into a new base address')
    parser_rebase.set_defaults(which='rebase')
    parser_rebase.add_argument('-f',  '--fspbin' , dest='FspBinary',  type=str, help='FSP binary file path', required = True)
    parser_rebase.add_argument('-c',  '--fspcomp', choices=['t','m','s','o'],  nargs='+', dest='FspComponent', type=str, help='FSP component to rebase', default = "['t']", required = True)
    parser_rebase.add_argument('-b',  '--newbase', dest='FspBase', nargs='+', type=str, help='Rebased FSP binary file name', default = '', required = True)
    parser_rebase.add_argument('-o',  '--outdir' , dest='OutputDir',  type=str, help='Output directory path', default = '.')
    parser_rebase.add_argument('-n',  '--outfile', dest='OutputFile', type=str, help='Rebased FSP binary file name', default = '')

    parser_split  = subparsers.add_parser('split',  help='split a FSP into multiple components')
    parser_split.set_defaults(which='split')
    parser_split.add_argument('-f',  '--fspbin' , dest='FspBinary', type=str, help='FSP binary file path', required = True)
    parser_split.add_argument('-o',  '--outdir' , dest='OutputDir', type=str, help='Output directory path',   default = '.')
    parser_split.add_argument('-n',  '--nametpl', dest='NameTemplate', type=str, help='Output name template', default = '')

    parser_genhdr = subparsers.add_parser('genhdr',  help='generate a header file for FSP binary')
    parser_genhdr.set_defaults(which='genhdr')
    parser_genhdr.add_argument('-f',  '--fspbin' , dest='FspBinary', type=str, help='FSP binary file path', required = True)
    parser_genhdr.add_argument('-o',  '--outdir' , dest='OutputDir', type=str, help='Output directory path',   default = '.')
    parser_genhdr.add_argument('-n',  '--hfile',   dest='HFileName', type=str, help='Output header file name', default = '')

    parser_info = subparsers.add_parser('info',  help='display FSP information')
    parser_info.set_defaults(which='info')
    parser_info.add_argument('-f',  '--fspbin' , dest='FspBinary', type=str, help='FSP binary file path', required = True)

    args = parser.parse_args()
    if args.which in ['rebase', 'split', 'genhdr', 'info']:
        if not os.path.exists(args.FspBinary):
            raise Exception ("ERROR: Could not locate FSP binary file '%s' !" % args.FspBinary)
        if hasattr(args, 'OutputDir') and not os.path.exists(args.OutputDir):
            raise Exception ("ERROR: Invalid output directory '%s' !" % args.OutputDir)

    if args.which == 'rebase':
        RebaseFspBin (args.FspBinary, args.FspComponent, args.FspBase, args.OutputDir, args.OutputFile)
    elif args.which == 'split':
        SplitFspBin (args.FspBinary, args.OutputDir, args.NameTemplate)
    elif args.which == 'genhdr':
        GenFspHdr (args.FspBinary, args.OutputDir, args.HFileName)
    elif args.which == 'info':
        ShowFspInfo (args.FspBinary)
    else:
        pass

    return 0

if __name__ == '__main__':
    sys.exit(main())
