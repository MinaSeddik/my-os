;; Stage 1 bootloader for Firebird O.S
;;  FAT32 version

;; To do:
;;  Figure real first data sector

org 0x7C00
bits 16

BS_jmpBoot:
	jmp start
	nop

;; FAT fields, labeled here for convenience
BS_OEMName:
	dd 16843009
	dd 16843009
BPB_BytsPerSec:
	dw 257
BPB_SecPerClus:
	db 1
BPB_ResvdSecCnt:
	dw 257
BPB_NumFATs:
	db 1
BPB_RootEntCnt:
	dw 257
BPB_TotSec16:
	dw 257
BPB_Media:
	db 1
BPB_FATSz16:
	dw 257
BPB_SecPerTrk:
	dw 257
BPB_NumHeads:
	dw 257
BPB_HiddSec:
	dd 16843009
BPB_TotSec32:
	dd 16843009
BPB_FATSz32:
	dd 16843009
BPB_ExtFlags:
	;; Bits 0-3 = Active FAT, 7 = !FAT mirroring
	dw 257
BPB_FSVer:
	dw 257
BPB_RootClus:
	dd 16843009
BPB_FSInfo:
	dw 257
BPB_BkBootSec:
	dw 257
BPB_Reserved:
	dd 16843009
	dd 16843009
	dd 16843009
BS_DrvNum:
	db 1
BS_Reseved1:
	db 1
BS_BootSig:
	db 1
BS_VolID:
	dd 16843009
BS_VolLab:
	dd 16843009
	dd 16843009
	dw 257
	db 1
BS_FilSysType:
	dd 16843009
	dd 16843009

start:
	cli
	;; Set CS, DS, ES, & SS to a known value
	;; I used eax instead of just ax because I want to have the upper word cleared later
	xor eax, eax
	jmp 0:loadCS
loadCS:
	mov ds, ax
	mov es, ax
	mov ss, ax
	
	;; set up the stack ( a temporary 512-byte stack )
	mov sp, 0x8000
	mov bp, sp
	
	;; Save the boot drive
	;; BootDrive = dl
	push dx
	
	;; calculate the first data sector
	;; FirstDataSector = BPB_NumFATs * BPB_FATSz32 + BPB_ResvdSecCnt
	mov al, [BPB_NumFATs]
	mov ebx, [BPB_FATSz32]
	mul ebx
	xor ebx, ebx
	mov bx, [BPB_ResvdSecCnt]
	add eax, ebx
	;; eax = first data sector, for now let's just assume it's relative to MBR
	push eax
	
	;; now let's get the location of the first FAT
	;; FATsector = BPB_ResvdSecCnt
	xor eax, eax
	mov ax, [BPB_ResvdSecCnt]
	push eax
	
	;; BytsPerCluster = BPB_BytsPerSec * BPB_SecPerClus
	mov ax, [BPB_BytsPerSec]
	mov bx, [BPB_SecPerClus]
	mul bx
	push eax
	
	;; FATClusterMask = 0x0FFFFFFF
	mov eax, 0x0FFFFFFF
	push eax
	
	;; FATEoFMask = FATClusterMask & 0xFFFFFFF8
	and al, 0xF8
	push eax
	
	;; CurrentCluster = BPB_RootClus
	mov eax, [BPB_RootClus]
	push eax
	
	;; Stack is now as follows:
BootDrive 		equ BP- 2
FirstDataSector	equ BP- 6
FATsector			equ BP-10
BytsPerCluster		equ BP-14
FATClusterMask		equ BP-18
FATEoFMask		equ BP-22
CurrentCluster		equ BP-26
	
	;; Get the first root directory cluster
	mov eax, [CurrentCluster]
	mov di, bp
	call readCluster
	
	;; parse first cluster to see if it has what we want
nextDirCluster:
	;; Set bx to the end of the cluster
	mov bx, [BytsPerCluster]
	add bx, bp
	;; ax = 0x8000 - sizeof(FAT_DIR_entry)
	;; this simplifies the upcoming loop a bit
	mov ax, 0x7FE0

findloop:
	;; move to next entry
	add ax, 32
	;; check if we're at the end of the cluster
	cmp ax, bx
	;; if so, handle it
	jz notFound
	
	;; else let's check the entry
	mov si, ax
	mov di, fileName
	mov cx, 11
	;; compare names
	repe cmpsb
	;; if not the same, try next entry
	jnz findloop
	
	;; file found!
	;; +9 instead of +20 because SI is already 11 bytes into the entry
	mov ax, [si+9]
	;; push the low word
	push ax
	mov ax, [si+15]
	;; now push the high word
	push ax
	;; and pop the double word
	pop eax
	;; eax = cluster of file
	mov di, bp
	call readCluster
loadFileLoop:
	;; if we're already at the EoF, this won't read anything
	call readNextCluster
	;; it will, however, set the carry flag to indicate that we're at the EoF
	;; so if( !cf )
	;;  keep loading clusters
	jnc loadFileLoop
	;; else
	;;  jump to the second stage
	jmp bp

notFound:
	;; try reading the next cluster
	call readNextCluster
	jnc nextDirCluster
	;; if this was the last one
	;; show an error
	mov si, noFile
	call putStr
	;; and stop
	hlt
	
;; A few useful routines	
putStr:
	;; ah = useTeletype
	mov ah, 0x0E
	;; bh = use page 0, bl = useless in Bochs
	xor bx, bx
loopPut:
	;; lodsb == mov al, si \ inc si
	;; too bad it doesn't do "or al, al" also
	lodsb
	;; check if al == 0
	or al, al
	;; if so, we're done
	jz donePut
	;; otherwise, let's put it
	int 10h
	;; and go again for the next character
	jmp loopPut
donePut:
	ret
	
readError:
	;; whoops! there was an error reading the drive!
	mov si, readFail
	call putStr
	hlt

;;  readNextCluster :: ES:DI = dest
readNextCluster:
	;; Check if we're already at the EoF
	mov eax, [CurrentCluster]
	and eax, [FATClusterMask]
	cmp eax, [FATEoFMask]
	;; if so, bail
	jz eofLoad
	
	;; now, let's get the number of
	;; cluster pointers in a sector
	xor ebx, ebx
	mov bx, [BPB_BytsPerSec]
	shr bx, 2
	;; now that we have that, let's figure
	;; out the offset, in sectors, of the
	;; current cluster pointer
	xor edx, edx
	div eax, ebx
	;; now that we have those, let's save them
	push ebx
	push eax
	;; add in the location of the first FAT sector
	;; to get the location on disk
	add eax, [FATsector]
	;; convert to the BIOS's format
	call getCHS
	;; figure out where to put it
	mov bx, 0x7C00
	sub bx, [BPB_BytsPerSec]
	xor esi, esi
	;; save it for later
	mov si, bx
	mov ax, 0x0201
	mov dl, [BootDrive]
	;; and now let's read the sector
	int 13h
	jc readError
	pop eax
	pop ebx
	;; eax = how many sectors into the FAT to look
	;; ebx = clusterpointers per sector
	
	;; let's figure out the memory offset
	;; for the cluster pointer
	mul ebx
	mov ebx, [CurrentCluster]
	and ebx, [FATClusterMask]
	sub ebx, eax
	sal ebx, 2
	;; offset the pointer
	add esi, ebx
	;; load the next cluster
	mov eax, [esi]
	;; and set it to the new current cluster
	mov [CurrentCluster], eax
readCluster:
	;; Get the first sector of the cluster
	;; Sector = (cluster - 2) * BPB_SecPerClus + FirstDataSector
	and eax, [FATClusterMask]
	dec eax
	dec eax
	xor ebx, ebx
	mov bl, [BPB_SecPerClus]
	mul ebx
	add eax, [FirstDataSector]
	;; eax = first sector of cluster
	
	;; convert to CHS to load directory
	call getCHS
	
	;; fetch the cluster and put it where the user wanted
	mov bx, di
	mov ah, 0x02
	mov al, [BPB_SecPerClus]
	mov dl, [BootDrive]
	
	int 13h
	jc readError

	add di, [BytsPerCluster]
	
	;; clear the EoF flag and return
	clc
	ret
	
eofLoad:
	;; set the EoF flag and return
	stc
	ret

getCHS:
	;; Borrowed from http://en.wikipedia.org/wiki/CHS_conversion
	xor dx, dx
	mov bx, [BPB_SecPerTrk]
	div ax, bx
	inc dx
	mov cl, dl
	xor dx, dx
	mov bx, [BPB_NumHeads]
	div ax, bx
	mov ch, al
	mov dh, dl
	sal ah, 6	
	or ah, cl
	mov cl, ah
	ret
	
;; Some basic data

readFail:
	db "Couldn't read drive!", 0
noFile:
	db "Couldn't find /FBOOT.SYS!",0

fileName:
	db "FBOOT   SYS"

;; FAT layout:
; DIR_name:
;	 db "FILE    EXT"
; DIR_Attr:
;	 db 1	; ReadOnly = 0x01, Hidden = 0x02, System = 0x04, VolID = 0x08, Dir = 0x10, Arch = 0x20, LongName = 0x0F
; DIR_NTRes:
;	 db 1
; DIR_CrtTimeTenth:
;	 db 1
; DIR_CrtTime:
;	 dw 257
; DIR_CrtDate:
;	 dw 257
; DIR_LstAccDate:
;	 dw 257
; DIR_FstClusHI:
;	 dw 257
; WrtTime:
;	 dw 257
; WrtDate:
;	 dw 257
; FstClusLO:
;	 dw 257
; FileSize:
;	 dd 16843009

	times 510-($-$$) db 0
	db 0x55, 0xAA		;; Standard end of boot sector