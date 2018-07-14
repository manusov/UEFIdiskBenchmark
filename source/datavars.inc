;===========================================================================;
;=        UEFI Disk Benchmark. Use reads by EFI_BLOCK_IO_PROTOCOL.         =;
;=               Non pre-initialized variables, module 5 of 5.             =;
;=                        (C) IC Book Labs, 2018                           =;
;=                                                                         =;
;= Code compatible and can be integrated into UEFImark EBC Edition project =;
;=      See UEFImark EBC Edition project sources for detail comments       =;
;===========================================================================;

; For compact coding, use 16-bit offsets relative Global_Data_Pool
; instead full 64-bit flat addresses. This method also eliminate relocation
; elements with some bugs in the old IA32 EFI implementations.

;---------- Equations for data access --------------------------------------;

_EFI_Handle		= EFI_Handle - Global_Data_Pool
_EFI_Table		= EFI_Table - Global_Data_Pool
_Handles_Limit		= Handles_Limit - Global_Data_Pool
_CAP_Protocol		= CAP_Protocol - Global_Data_Pool
_CAP_Period		= CAP_Period - Global_Data_Pool
_True_Base		= True_Base - Global_Data_Pool
_True_Size		= True_Size - Global_Data_Pool
_Buffer_Base		= Buffer_Base - Global_Data_Pool
_Buffer_Size		= Buffer_Size - Global_Data_Pool
_Number_of_Drives	= Number_of_Drives - Global_Data_Pool
_Selected_Drive		= Selected_Drive - Global_Data_Pool

_Media_ID		= Media_ID - Global_Data_Pool
_Drive_Bytes		= Drive_Bytes - Global_Data_Pool
_Drive_Blocks		= Drive_Blocks - Global_Data_Pool
_Drive_Block		= Drive_Block - Global_Data_Pool
_Full_Bytes		= Full_Bytes - Global_Data_Pool
_Full_Blocks		= Full_Blocks - Global_Data_Pool
_Tail_Bytes		= Tail_Bytes - Global_Data_Pool
_Tail_Blocks		= Tail_Blocks - Global_Data_Pool
_Full_Count		= Full_Count - Global_Data_Pool
_Current_LBA		= Current_LBA - Global_Data_Pool
_CAP_Sum		= CAP_Sum - Global_Data_Pool
_CAP_Temp		= CAP_Temp - Global_Data_Pool

_BlockIO_Handles	= BlockIO_Handles - Global_Data_Pool
_BlockIO_Interfaces	= BlockIO_Interfaces - Global_Data_Pool

;---------- Data -----------------------------------------------------------;

;---------- Multifunctional buffer -----------------------------------------;
; Use fact: Scratch_Pad = Global_Data_Pool (pointers equal),
; this address loaded into R1 by first EBC instruction MOVRELW.

Scratch_Pad_Size	=   3072
Scratch_Pad		DB  Scratch_Pad_Size DUP (?)

;---------- Save parameters, received from UEFI firmware or UEFI shell -----;

EFI_Handle		DQ  ?		; This application handle
EFI_Table		DQ  ?		; System table address

;---------- Drives scan and other protocols control ------------------------;

Handles_Limit		DQ  ?		; Block IO handles list limit
CAP_Protocol		DQ  ?		; CPU Architectural Protocol (CAP)
CAP_Period		DQ  ?		; CAP timer clock period, femtoseconds
True_Base		DQ  ?		; File I/O buffer base address, before alignment, this for release
True_Size		DQ  ?		; File I/O buffer size, pages, before alignment, this for release
Buffer_Base		DQ  ?		; File I/O buffer base address
Buffer_Size		DQ  ?		; File I/O buffer size, bytes
Number_of_Drives	DD  ?		; Number of drives, note last drive number is this value -1
Selected_Drive		DQ  ?		; Drive, selected by user, address-scaled index in the drives list

;---------- Variables for benchmarks, for selected drive -------------------;

Media_ID		DD  ?		; Media ID dword from EFI_BLOCK_IO_MEDIA data structure
Drive_Bytes		DQ  ?		; Size of drive, bytes
Drive_Blocks		DQ  ?		; Number of blocks (sectors) per drive
Drive_Block		DQ  ?		; Size of one block (sector) at drive, bytes
Full_Bytes		DQ  ?		; Number of bytes per full-sized part
Full_Blocks		DQ  ?		; Number of blocks (sectors) per full-sized part
Tail_Bytes		DQ  ?		; Number of bytes per tail
Tail_Blocks		DQ  ?		; Number of blocks per tail
Full_Count		DQ  ?		; Number of iterations with full-sized parts
Current_LBA		DQ  ?		; Current LBA (sector number) for disk operaion
CAP_Sum			DQ  ?		; Accumulator for add Delta-CAP per Block IO protocol calls
CAP_Temp		DQ  ?		; Temporary storage for time point

;---------- List of handles, devices, supported Block IO protocol ----------;

BlockIO_Handles_Size	=   2048
BlockIO_Handles		DB  BlockIO_Handles_Size DUP (?)

;---------- List of interfaces, devices, supported Block IO protocol -------;

BlockIO_Interfaces_Size	=   2048
BlockIO_Interfaces	DB  BlockIO_Interfaces_Size DUP (?)
