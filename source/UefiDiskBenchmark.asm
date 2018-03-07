;===========================================================================;
;=         UEFI Disk Benchmark. Use reads by EFI_BLOCK_IO_PROTOCOL.        =;
;=                       Main module, module 1 of 5.                       =;
;=                         (C) IC Book Labs, 2016                          =;
;=                                                                         =;
;= Code compatible and can be integrated into UEFImark EBC Edition project =;
;=      See UEFImark EBC Edition project sources for detail comments       =;
;===========================================================================;

;
; See UEFI Specification 2.6 page 650.
;
; Parameters:
; Software enumeration:
; DWORD  ID
;
; From protocol interface structure:
;
; QWORD  Revision  
;
; From EFI_BLOCK_IO_MEDIA structure:
;
; DWORD         MediaID           Media ID
; BYTE BOOLEAN  RemovableMedia    RM
; BYTE BOOLEAN  MediaPresent      MP
; BYTE BOOLEAN  LogicalPartition  LP
; BYTE BOOLEAN  ReadOnly          RO
; BYTE BOOLEAN  WriteCaching      WC
; DWORD         BlockSize         Block
; DWORD         IoAlign           Align
; QWORD         LastBlock         Size, calculated Block*(LastBlock+1)
; --- rev.2 additions ---
; QWORD         LowestAlignedLBA
; DWORD         LogicalBlocksPerPhysicalBlock
; --- rev.3 additions ---
; DWORD         OptinalTransferLengthGranularity
; ---
;
; Devices list design example, 80 columns:
;
; #         Revision  Media     RM MP LP RO WC  Block     Align     Size
; -----------------------------------------------------------------------------
; 01234567  01234567  01234567  0  0  0  0  0   01234567  01234567  01234567
;

;---------------------------------------------------------------------------;
;                              GLOBAL MACRO.                                ;
;---------------------------------------------------------------------------;

include 'ebcmacro.inc'	 	; Macro for assembling EBC instructions

;---------------------------------------------------------------------------;
;                       CODE SECTION DEFINITIONS.                           ;
;---------------------------------------------------------------------------;

format pe64 dll efi
entry main
section '.text' code executable readable
main:

;---------------------------------------------------------------------------;
;                  CODE SECTION \ MAIN EBC ROUTINE.                         ;
;---------------------------------------------------------------------------;

;---(01)--- Store non-volatile registers -----------------------------------;

		PUSH64		R1	; R1-R3 must be preserved across calls
		PUSH64		R2
		PUSH64		R3

;---(02)--- Setup common variables -----------------------------------------;


		MOVRELW 	R1,Global_Data_Pool - Anchor_IP
Anchor_IP:	MOVNW		R2,@R0, 0, 16+24		; R2=Handle
		MOVNW		R3,@R0, 1, 16+24		; R3=Table
		MOVQW		@R1,0,_EFI_Handle,R2		; Save Handle
		MOVQW		@R1,0,_EFI_Table,R3		; Save Table

;---(03)--- First message strings ------------------------------------------;

		MOVIQW		R2,0Fh				; Color=Bold White
		CALL32		Output_Attribute		; Set color
		MOVIQW		R2,_Msg_CRLF
		CALL32		String_Write			; Skip 1 string
		MOVIQW		R2,_String_Version
		CALL32		String_Write			; Write Program Version
		MOVIQW		R2,0Ah				; Color=Bold Green
		CALL32		Output_Attribute		; Set color
		MOVIQW		R2,_Program_Vendor_Data_1
		CALL32		String_Write			; Write Vendor Name
		MOVIQW		R2,07h				; Color=White
		CALL32		Output_Attribute		; Set color
		MOVIQW		R2,_Msg_Starting
		CALL32		String_Write			; Write "Starting..."

;---(04)--- Detect CPU Architectural Protocol (CAP) ------------------------;

		MOVIQW		R2,_GuidCap
		CALL32		Locate_Protocol		; Detect protocol
		JMP32CS		Error_Point		; Go if error: CAP not detected, R7=Status
		MOVQW		@R1,0,_CAP_Protocol,R2	; Save CAP interface pointer
		CALL32		Read_CAP_Timer		; Read CAP timer current value
		JMP32CS		Error_Point		; Go if error: CAP timer read error, R7=Status
		MOVQW		@R1,0,_CAP_Period,R6	; Save CAP period

;---(05)--- Allocate firmware memory, 32 MB --------------------------------;

		XOR64		R2,R2			; R2 = EFI Memory Allocate Type = 0
		MOVQW		@R1,0,_True_Size,R2	; Pre-blank size for conditional release
		MOVIQW		R3,4			; R3 = EFI Memory Type = BS DATA = 4
		MOVIQW		R4,8192+1		; R4 = Num. of contiguous 4KB pages = 8129+1 (32MB) + align
		XOR64		R5,R5			; R5 = Limit, here not used = 0
		CALL32		EFI_Allocate_Pages	; Try memory allocate
		JMP32CS		Error_Point		; Go if error, CS condition if error

		MOVQW		@R1,0,_True_Base,R5
		MOVQW		@R1,0,_True_Size,R4

		MOVSNW		R4,R4,-1		; Minus 1 page, for alignment reserve
		MOVIQW		R7,12
		SHL64		R4,R7			; R4 = Size, Pages->Bytes
		MOVIQW		R7,0Fh			; Mask for check align 16
		MOVQ		R6,R7
		AND64		R6,R5
		CMP64EQ		R6,R2			; Here R2=0
		JMP32CS		Memory_Aligned		; Go if already aligned

		MOVSNW		R6,R7,1			; R6 = R7+1   = 0000000000000010h
		NOT64		R7,R7			; R7 = NOT R7 = FFFFFFFFFFFFFFF0h
		AND64		R5,R7			; R5 Align 16, clear bits [3-0]
		ADD64		R5,R6			; R5 + 16
Memory_Aligned:	
		MOVQW		@R1,0,_Buffer_Base,R5
		MOVQW		@R1,0,_Buffer_Size,R4

;---(06)--- Detect handles, supports BlockIO protocol ----------------------;

		MOVIQW		R2,_GuidBlockIO
		MOVIQW		R3,_BlockIO_Handles
		MOVIQW		R4,BlockIO_Handles_Size
		CALL32		Locate_Handles			; Result handles list R2=Limit, R3=Base 
		JMP32CS		Error_Point			; Go if error: protocol not detected, R7=Status

;---(07)--- Scan drives, open Block IO protocols for detected drives -------;

		MOVQ		R4,R2
		MOVSNW		R5,R1,_BlockIO_Interfaces
		MOVQ		R6,R5				; R6 = Copy pointer to interfaces pointers array
Scan_Drives:	
		MOVIQW		R2,_GuidBlockIO
		CALL32		Open_Protocol
		JMP32CS		Error_Point	; Go if error
		MOVNW		@R5,R2		; Store protocol interface pointer, natural width
		MOVINW		R2,1,0		; R2 = Natural width, 4 for IA32, 8 for x64
		ADD64		R3,R2		; Source address + Natural width
		ADD64		R5,R2		; Destination address + Natural width
		CMP64UGTE	R3,R4		; Compare with R4 = Limit
		JMP8CC		Scan_Drives	; Cycle for all detected drives
		MOVQW		@R1,0,_Handles_Limit,R4	; Save handles list limit

;---(08)--- Visual initialization results, memory occupy, CAP parameters ---;

;--- Save registers ---
		PUSH64		R4
		PUSH64		R6
;--- CAP ---
		MOVSNW		R2,R1,_Parm_CAP		; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_CAP_Period	; R3 = Femptoseconds
		CALL32		String_Fs_MHz
;--- Buffer base address ---
		MOVQ		R3,R2			; R3 = Destination
		MOVSNW		R2,R1,_Parm_Base	; R2 = Source
		CALL32		Copy_String
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Buffer_Base	; R3 = Buffer base address after memory allocation
		CALL32		String_Hex64
;--- Buffer size ---
		MOVQ		R3,R2			; R3 = Destination
		MOVSNW		R2,R1,_Parm_Size	; R2 = Source
		CALL32		Copy_String
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Buffer_Size	; R3 = Buffer size after memory allocation
		XOR64		R4,R4			; R4 = 0 and template for decimal print
		SHR64		R3,R4,10
		CALL32		String_Decimal32
;--- Last part, units = MB, CR, LF --
		MOVQ		R3,R2			; R3 = Destination
		MOVSNW		R2,R1,_Parm_Units	; R2 = Source
		CALL32		Copy_String
;--- Terminator ---
		MOVIBW		@R3,0			; Store terminator
;--- Initialization parameters string done ---
		MOVIQW		R2,200			; R2 = 200 , use input as R1-relative offset
		CALL32		String_Write		; Visual this prepared string
;--- Restore registers ---
		POP64		R6
		POP64		R4

;---(09)--- Built and visual drives table header ---------------------------;
; R4, R6 must be not changed at this step

	;	MOVIQW		R2,0Fh			; Color = Bold White
	;	CALL32		Output_Attribute	; Set color
		MOVIQW		R2,_Result_Table
		CALL32		String_Write		; Write parameters at table up
		CALL32		Horizontal_Line

;---(10)--- Built and visual drives list with brief info -------------------;
; At start, interfaces list parameters: R4=Limit, R6=Base

		MOVSNW		R5,R1,_BlockIO_Handles	; R5 = Here for count only
		XOR64		R3,R3			; R3 = Items counter
List_Drives:
		PUSH64		R7
		PUSH64		R6
		PUSH64		R5
		PUSH64		R4
		PUSH64		R3
		PUSH64		R2

		MOVSNW		R2,R1,200		; R2 = R1 + 200
		MOVIWW		@R2,0A0Dh		; Store CR, LF
		MOVSNW		R2,R2,2			; R2 = Pointer + 2

		MOVQ		R4,R2			; R4 = Local base for string blank
		MOVSNW		R5,R4,79		; R5 = Local limit for string blank
Fill_String:	MOVIBW		@R4,' '
		MOVSNW		R4,R4,1			; R3 = Local pointer + 1
		CMP64UGTE	R4,R5			; R4 = Local pointer limit compare
		JMP8CC		Fill_String		; Cycle for pre-blank string
		MOVIQW		@R4,0
;--- Numeration, # ---		
		MOVQ		R5,R2			; Store base, outputs can be variable size
		MOVSNW		R2,R2,1			; R2 = Pointer + 1 , left space before drive ID
		XOR64		R4,R4			; R4 = Template size, 0 means no template
		CALL32		String_Decimal32	; Store number: Drive ID , R2 = Pointer modified
;--- Revision ---					; Before next load, R6 = Pointer to array of pointers
		MOVNW		R6,@R6			; R6 = Pointer to interface block for current drive
		MOVSNW		R2,R5,6			; R2 = Position independent from previous outputs
		MOVWW		R3,@R6,0,2		; Get major version word from protocol block
		CMPI64WUGTE	R3,10
		JMP8CS		Skip_1
		CALL32		String_Decimal32	; Print major version
		JMP8		Skip_2
Skip_1:		MOVIBW		@R2,'?'			; Print "?" if major version invalid
		MOVSNW		R2,R2,1
Skip_2:		MOVIBW		@R2,'.'			; Point between major and minor version numbers
		MOVSNW		R2,R2,1
		MOVW		R3,@R6			; Get minor version word from protocol block
		CMPI64WUGTE	R3,1000
		JMP8CS		Skip_3
		CALL32		String_Decimal32	; Print minor version
		JMP8		Skip_4
Skip_3:		MOVIBW		@R2,'?'			; Print "?" if minor version invalid
Skip_4:
;--- Media ID ---
		MOVNW		R6,@R6,0,8		; R6 = Pointer to EFI_BLOCK_IO_MEDIA structure, n=0, c=8
		MOVSNW		R2,R5,16		; R2 = Position independent from previous outputs
		MOVD		R3,@R6			; R3 = Read MediaID from EFI_BLOCK_IO_MEDIA structure
		CALL32		String_Decimal32	; Print Media ID
;--- Boolean attribute RM ---
		MOVSNW		R2,R5,26		; R2 = Position independent from previous outputs
		MOVQW		R3,@R6,0,4		; R3 = Boolean flags, valid low 5 bytes at QWORD
		XOR64		R7,R7			; R7 = Flags counter
Boolean_Values:	CALL32		String_Boolean		; Print boolean attribute, RemovableMedia
		MOVSNW		R2,R2,2			; R2 = String pointer, skip 2 spaces
		SHR64		R3,R4,8			; R3 = Flags, because R4=0, shift by 8 bits (R4+8=8)
		MOVSNW		R7,R7,1			; R7 = Counter + 1
		CMPI64WUGTE	R7,5
		JMP8CC		Boolean_Values		; Cycle for 5 flags
;--- Block size ---
		XOR64		R7,R7			; R7 = Pre-blank block size, used at next steps
		MOVSNW		R2,R5,42		; R2 = Position independent from previous outputs
		MOVDW		R3,@R6,0,12		; R3 = Data, parameter value
		CMPI64DUGTE	R3,65536+1
		JMP8CS		Skip_10
		CALL32		String_Decimal32	; Print Block size
		MOVQ		R7,R3			; R7 = Block size, used at next steps
		JMP8		Skip_11
Skip_10:	MOVIBW		@R2,'?'
Skip_11:
;--- Memory buffer alignment ---
		MOVSNW		R2,R5,52		; R2 = Position independent from previous outputs
		MOVDW		R3,@R6,0,16		; R3 = Data, parameter value
		CMPI64DUGTE	R3,4096+1
		JMP8CS		Skip_20
		CALL32		String_Decimal32	; Print Buffer alignment
		JMP8		Skip_21
Skip_20:	MOVIBW		@R2,'?'
Skip_21:
;--- Drive size by last block ---
		MOVSNW		R2,R5,62		; R2 = Position independent from previous outputs
		MOVQW		R3,@R6,0,24		; R3 = Last block, element LAST_LBA, note aligned by 8 bytes
		ADD64		R3,R4,1			; R3 = Number of blocks, because R4=0, this add is +1
		MULU64		R3,R7			; R3 = Number of blocks * Block size = Drive size, bytes
		SHR64		R3,R4,20		; R3 = Convert Bytes to MB, because R4=0, this shift is SHR 20
		CALL32		String_Decimal32	; Print Drive size, units = megabytes
		MOVIDD		@R2,' MB '		; Print space and units 'MB'
;--- Drive string done ---
		MOVIQW		R2,200			; R2 = 200 , use input as R1-relative offset
		CALL32		String_Write		; Visual this prepared string
;--- Cycle ---		
		POP64		R2
		POP64		R3
		POP64		R4
		POP64		R5
		POP64		R6
		MOVINW		R7,1,0		; R7 = Natural width, 4 for IA32, 8 for x64
		ADD64		R5,R7		; Service count address + Natural width
		ADD64		R6,R7		; Source address + Natural width
		CMP64UGTE	R5,R4		; Compare with R4 = Limit
		POP64		R7
		MOVSNW		R3,R3,1		; R3+1
		JMP32CC		List_Drives	; Cycle for all detected drives
;--- Separate line ---
		MOVIQW		R2,_Msg_CRLF
		CALL32		String_Write		; Skip 1 string
		CALL32		Horizontal_Line

		MOVDW		@R1,0,_Number_of_Drives,R3

;---(11)--- Wait for user press key, drive selection -----------------------;
; R3 = Actual as number of drives
							
		MOVSNW		R2,R1,_Last_Drive	; R2 = Pointer, absolute 64
		MOVSNW		R3,R3,-1		; R3 = Value, 0-based last drive = number of drives - 1
		XOR64		R4,R4			; R4 = Template
		CALL32		String_Decimal32

		MOVIQW		R2,0Eh			; Color = Bold Yellow
		CALL32		Output_Attribute	; Set color
		MOVIQW		R2,_Select_Drive
		CALL32		String_Write		; Write drive selection string
Wait_Right_Key:
		CALL32		Input_Wait_Key		; Wait for key, return when press

		MOVIQD		R4,0000FFFFh
		AND64		R4,R2
		CMPI64WEQ	R4,0017h		; ESCAPE = 17h
		JMP32CS		Benchmark_Skip
		CMPI64WEQ	R4,0
		JMP8CC		Wait_Right_Key
		MOVIQW		R4,16
		SHR64		R2,R4
		CMPI64WULTE	R2,030h - 1		; "0" = 30h
		JMP8CS		Wait_Right_Key
		CMPI64WUGTE	R2,039h + 1		; "9" = 39h
		JMP8CS		Wait_Right_Key

	;--- ENCODING BUG ---
	; *	MOVBW		@R1,0,_New_Drive,R2
		MOVSNW		R4,R1,_New_Drive
		MOVBW		@R4,0,0,R2
	;---

		MOVIQW		R4,0Fh
		AND64		R2,R4
		CMP64ULTE	R2,R3
		JMP8CC		Wait_Right_Key

		MOVINW		R4,1,0				; R4 = Natural width
		MULU64		R2,R4				; R2 = Scaled index by natural width
		MOVQW		@R1,0,_Selected_Drive,R2	; Store scaled index
		
		MOVIQW		R2,_Show_Drive
		CALL32		String_Write		; Write drive selection string
		MOVIQW		R2,_Msg_CRLF_2
		CALL32		String_Write		; Skip 2 strings

;---(12)--- Interpreting drive selection -----------------------------------;

		MOVSNW		R2,R1,_BlockIO_Interfaces	; R2 = Pointer to interface pointers list
		ADD64		R2,@R1,0,_Selected_Drive	; add offset by user selection in the drive list
		MOVNW		R2,@R2				; R2 = Pointer to interface block for selected drive

;---(13)--- Built parameters list with detail info about selected drive ----;

		; Reserved for future use, 
		; include additional parameters visual, 4KB sectors
		; can add this step to visual start conditions step 

;---(14)--- Wait for user press key ----------------------------------------;

		; Reserved for previous step support

;---(15)--- Reset device ---------------------------------------------------;
; R2 = Protocol pointer for selected device, from poiners list
; Call function EFI_BLOCK_RESET means EFI_BLOCK_IO_PROTOCOL.Reset()

		MOVIQW		R7,1
		PUSHN		R7		; Parm#2 = Extended verification flag
		PUSHN		R2		; Parm#1 = Protocol pointer
		CALL32EXA	@R2,1,8		; n=1, c=8, skip one natural width pointer and one 64-bit QWORD
		POPN		R6		; Remove parameter #2
		POPN		R6		; Remove parameter #1
		MOVSNW		R7,R7
		CMPI64WUGTE	R7,1		; Check status, error if status > 0
		JMP32CS		Error_Point	; Go if error: drive reset failed		

;---(16)--- Initializing variables for benchmarks --------------------------;
; Optimization required, some parameters calculation duplicated at this step

		MOVNW		R3,@R2,0,8	; R3 = Pointer to EFI_BLOCK_IO_MEDIA

;--- Media ID ---
		MOVD		R4,@R3
		MOVDW		@R1,0,_Media_ID,R4
;--- Number of blocks (sectors) per drive ---
		MOVQW		R4,@R3,0,24		; R4 = Last block, element LAST_LBA, note aligned by 8 bytes
		MOVIQW		R5,1
		ADD64		R4,R5			; R4 = Number of blocks
		MOVQW		@R1,0,_Drive_Blocks,R4
;--- Size of one block (sector) at drive, bytes ---
		MOVDW		R5,@R3,0,12		; R5 = Block size, bytes
		MOVQW		@R1,0,_Drive_Block,R5
;--- Size of drive, bytes ---
		MULU64		R4,R5			; R4 = Number of blocks * Block size = Drive size, bytes
		MOVQW		@R1,0,_Drive_Bytes,R4
;--- Number of bytes per full-sized part ---
		MOVQW		R6,@R1,0,_Buffer_Size	; R6 = Bytes per one full-sized iteration
		MOVQW		@R1,0,_Full_Bytes,R6
;--- Number of blocks (sectors) per full-sized part ---
		MOVQ		R7,R6
		CMPI64WEQ	R5,0
		JMP32CS		Error_Point_FF		; Go error if divisor=0, prevent exception
		DIVU64		R7,R5			; R7 = Bytes per one full-sized iteration / Block size, bytes
		MOVQW		@R1,0,_Full_Blocks,R7	; R7 = Blocks per one full-sized iteration
;--- Number of iterations with full-sized parts ---
		MOVQ		R7,R4			; R7 = Bytes per drive
		CMPI64WEQ	R6,0			; R6 = Bytes per one full-sized iteration
		JMP32CS		Error_Point_FF		; Go error if divisor=0, prevent exception
		DIVU64		R7,R6			; R7 = Drive size / Iteration size
		MOVQW		@R1,0,_Full_Count,R7	; R7 = Number of iterations
;--- Number of bytes per tail ---
		MODU64		R4,R6			; R4 = MOD ( Drive size / Iteration size )
		MOVQW		@R1,0,_Tail_Bytes,R4	; R4 = Tail size, bytes
;--- Number of blocks per tail ---
		DIVU64		R4,R5			; R4 = Tail size, bytes / Block size, bytes
		MOVQW		@R1,0,_Tail_Blocks,R4	; R4 = Tail size, blocks
;--- Current LBA ---
		MOVIQW		@R1,0,_Current_LBA,0
;--- Accumulator for add Delta-CAP per Block IO protocol calls ---
		MOVIQW		@R1,0,_CAP_Sum,0

;---(17)--- Visual start conditions: block size, block count, tail size ----;
; Optimizing required, make subroutines for repeated fragments

		MOVIQW		R2,07h			; Color = Normal white
		CALL32		Output_Attribute	; Set color
		XOR64		R4,R4			; R4 =0 , template mode for decimal print
;--- Media ID ---
		MOVSNW		R2,R1,_Parm_Media_ID	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVDW		R3,@R1,0,_Media_ID	; R3 = Value for visual
		CALL32		String_Decimal32	; Built string with value
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Size of drive, bytes ---
		MOVSNW		R2,R1,_Parm_Drive_Bytes	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Drive_Bytes	; R3 = Value for visual
		SHR64		R3,R4,20		; Because R4=0, shifts R3 by 20, bytes to megabytes
		CALL32		String_Decimal32	; Built string with value
		MOVIDD		@R2,' MB '
		MOVSNW		R2,R2,4
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Number of blocks (sectors) per drive ---
		MOVSNW		R2,R1,_Parm_Drive_Blocks	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Drive_Blocks	; R3 = Value for visual
		CALL32		String_Decimal32	; Built string with value
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Size of one block (sector) at drive, bytes ---
		MOVSNW		R2,R1,_Parm_Drive_Block	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Drive_Block	; R3 = Value for visual
		CALL32		String_Decimal32	; Built string with value
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Number of bytes per full-sized part ---
		MOVSNW		R2,R1,_Parm_Full_Bytes	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Full_Bytes	; R3 = Value for visual
		SHR64		R3,R4,10		; Because R4=0, shifts R3 by 10, bytes to kilobytes
		CALL32		String_Decimal32	; Built string with value
		MOVIDD		@R2,' KB '
		MOVSNW		R2,R2,4
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Number of blocks (sectors) per full-sized part ---
		MOVSNW		R2,R1,_Parm_Full_Blocks	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Full_Blocks	; R3 = Value for visual
		CALL32		String_Decimal32	; Built string with value
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Number of bytes per tail ---
		MOVSNW		R2,R1,_Parm_Tail_Bytes	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Tail_Bytes	; R3 = Value for visual
		SHR64		R3,R4,10		; Because R4=0, shifts R3 by 10, bytes to kilobytes
		CALL32		String_Decimal32	; Built string with value
		MOVIDD		@R2,' KB '
		MOVSNW		R2,R2,4
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Number of blocks per tail ---
		MOVSNW		R2,R1,_Parm_Tail_Blocks	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Tail_Blocks	; R3 = Value for visual
		CALL32		String_Decimal32	; Built string with value
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Number of iterations with full-sized parts ---
		MOVSNW		R2,R1,_Parm_Full_Count	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Full_Count	; R3 = Value for visual
		CALL32		String_Decimal32	; Built string with value
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Current LBA ---
		MOVSNW		R2,R1,_Parm_Current_LBA	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Current_LBA	; R3 = Value for visual
		CALL32		String_Decimal32	; Built string with value
		MOVIDW		@R2,00000A0Dh		; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string
;--- Accumulator for add Delta-CAP per Block IO protocol calls ---
		MOVSNW		R2,R1,_Parm_CAP_Sum	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_CAP_Sum	; R3 = Value for visual
		CALL32		String_Hex64		; Built string with value
		MOVIDD		@R2,000A0D00h+'h'	; Store CR, LF, Terminator
		MOVIQW		R2,200
		CALL32		String_Write		; Visual this prepared string

;---(18)--- Execute main part of test, interruptable -----------------------;

		MOVIQW		R2,0Ah			; Color = Bold Green
		CALL32		Output_Attribute	; Set color
		MOVIQW		R2,_Msg_CRLF
		CALL32		String_Write		; Skip 1 string
		MOVIQW		R7,1			; R7 = Interruption flag, non-zero means no interrupts

Benchmark_Cycle:

;--- Visual progress ---
		PUSH64		R7			; Store interruption flag
		MOVSNW		R2,R1,_CR_Current_LBA	; R2 = Source
		MOVSNW		R3,R1,200		; R3 = Destination
		CALL32		Copy_String		; Copy ASCII string
		MOVQ		R2,R3			; R2 = Destination
		MOVQW		R3,@R1,0,_Current_LBA	; R3 = Value for visual
		XOR64		R4,R4			; R4 = Print template
		CALL32		String_Decimal32	; Built string with value
		MOVIBW		@R2,0			; Store terminator byte to string
		MOVIQW		R2,200			; R2 = Offset from data pool
		CALL32		String_Write		; Visual this prepared string
		POP64		R7			; Restore interruption flag
;--- Check user input, check flag after previous iteration ---
		CMPI32WEQ	R7,0			; 0 if key ready
		JMP8CS		Benchmark_Done		; Go if break by user input
;--- Get start CAP, store ---
		CALL32		Read_CAP_Timer
		JMP8CS		Error_Point		; Go if error: read timer failed
		MOVQW		@R1,0,_CAP_Temp,R5	; CAP_Temp = Timer at Start
;--- Target operation, read device sectors ---
		MOVSNW		R2,R1,_BlockIO_Interfaces	; R2 = Pointer to interface pointers list
		ADD64		R2,@R1,0,_Selected_Drive	; add offset by user selection in the drive list
		MOVNW		R2,@R2				; R2 = Pointer to interface block for selected drive

		MOVNW		R3,@R1,0,_Buffer_Base
		PUSHN		R3			; Parm#5 = Buffer base address
		MOVNW		R3,@R1,0,_Full_Bytes
		PUSHN		R3			; Parm#4 = Buffer size bytes, implicit number of sectors
		MOVQW		R3,@R1,0,_Current_LBA
		PUSH64		R3			; Parm#3 = LBA, sector number
		MOVDW		R3,@R1,0,_Media_ID
		PUSHN		R3			; Parm#2 = Media ID
		PUSHN		R2			; Parm#1 = Protocol pointer
		CALL32EXA	@R2,2,8			; n=2, c=8, skip 2 natural width pointers and one 64-bit QWORD
		POPN		R6			; Remove parameter #5
		POPN		R6			; Remove parameter #4
		POP64		R6			; Remove parameter #3
		POPN		R6			; Remove parameter #2
		POPN		R6			; Remove parameter #1
		MOVSNW		R7,R7
		CMPI64WUGTE	R7,1			; Check status, error if status > 0
		JMP32CS		Error_Point		; Go if error: drive reset failed
;--- Get stop CAP, add to accumulator ---
		CALL32		Read_CAP_Timer
		JMP8CS		Error_Point		; Go if error: read timer failed
		SUB64		R5,@R1,0,_CAP_Temp	; Subtract, Delta CAP = ( Timer at Stop ) - ( Timer at Start )
		MOVSNW		R2,R1,_CAP_Sum		; R2 = Base address for variable: sum of Deltas
		ADD64		@R2,R5			; Add current delta to total sum
;--- Modify current LBA --
		MOVSNW		R2,R1,_Current_LBA
		ADD64		@R2,@R1,0,_Full_Blocks
;--- Cycle with tail support ---
		MOVIQW		R2,1			; R2 = Constant for decrement/compare
		MOVSNW		R3,R1,_Full_Count	; R3 = Pointer to cycle counter
		SUB64		@R3,R2			; Decrement cycle counter
		CMPI64WEQ	@R3,-1			; Detect cycle counter underflow after 1 extra-pass for tail
		JMP8CS		Benchmark_Done		; Go exit if tail already handled
		CMPI64WEQ	@R3,0	
		JMP8CC		No_Tail_Pass		; Go normal cycle if count > 0
		MOVQW		R4,@R1,0,_Tail_Bytes
		MOVQW		@R1,0,_Full_Bytes,R4	; Prepare variables for tail pass
		MOVQW		R4,@R1,0,_Tail_Blocks
		MOVQW		@R1,0,_Full_Blocks,R4
		CMPI64WEQ	R4,0
		JMP8CS		Benchmark_Done		; Go exit if tail=0, when R4=0
No_Tail_Pass:
;--- Check user input, set flag for use at next iteration ---
		CALL32		Input_Check_Key
	;	CMPI32WEQ	R7,0			; 0 if key ready
	;	JMP8CS		Benchmark_Done		; Go if break by user input
;--- Cycle for entire drive include tail ---
; R7 must be unchanged
		JMP8		Benchmark_Cycle		; Repeat, exit conditional in the cycle

Benchmark_Done:

;---(19)--- Visual results, total size, total time, MBPS -------------------;
; Future extensions required: min, max, average, median
; MBPS = F ( CAP units , CAP deltas sum , Data size )
; Data size = Current_LBA, note size added when exit,
; For optimize integer calculations, can use:
; Byter per Microsecond instead Megabytes per Second (but note 1000/1024 factor)
; Can be used subroutine String_Fs_Hz
; This subroutine can be used for visual any operations per second,
; if input = femtoseconds per measured operation


;********** DEBUG, FIX ORACLE VIRTUAL BOX CAP TIMER BUG **********

; MOVIQD @R1,0,_CAP_Period,417000  ; 417 PS MEANS APPROX. 2.4 GHZ

;*****************************************************************



;--- Calculation femtoseconds per one megabyte ---
		MOVQW		R2,@R1,0,_Current_LBA		; R2 = Total blocks executed
		MULU64		R2,@R1,0,_Drive_Block		; R2 = Total bytes executed
		MOVIQD		R3,1000000
		DIVU64		R2,R3				; R2 = Total decimal megabytes executed
		CMPI64WEQ	R2,0
		JMP8CS		Error_Point_FF			; Go error if divisor=0, prevent exception
		MOVQW		R7,@R1,0,_CAP_Sum		; R7 = Total clocks done
		DIVU64		R7,R2				; R7 = Clocks per one decimal megabyte
		MULU64		R7,@R1,0,_CAP_Period		; R7 = Total femtoseconds done
;--- Print megabytes per second ---		
		MOVSNW		R2,R1,_Parm_MBPS		; R2 = Source
		MOVSNW		R3,R1,200			; R3 = Destination
		CALL32		Copy_String			; Copy ASCII string
		MOVQ		R2,R3				; R2 = Destination
		MOVQW		R3,R7				; R3 = Value for visual
		CALL32		String_Fs_Hz			; Built string with value
		MOVIBW		@R2,0				; Store 0=terminator
		MOVIQW		R2,200
		CALL32		String_Write			; Visual this prepared string

;---(20)--- Wait for user press key ----------------------------------------;

		MOVIQW		R2,0Eh			; Color = Bold Yellow
		CALL32		Output_Attribute	; Set color
		MOVIQW		R2,_Msg_Press
		CALL32		String_Write		; Write parameters at table up
		CALL32		Input_Wait_Key		; Wait for key, return when press
		MOVIQW		R2,_Msg_CRLF
		CALL32		String_Write		; Skip 1 string

;---(21)--- Release firmware memory ----------------------------------------;

Benchmark_Skip:

		MOVQW		R2,@R1,0,_True_Base
		MOVQW		R3,@R1,0,_True_Size
		CMPI64WEQ	R3,0
		JMP8CS		No_Memory
		CALL32		EFI_Free_Pages
		JMP8CS		Error_Point	; Go if error: memory release failed, R7=Status
No_Memory:

;---(22)--- Close protocols ------------------------------------------------;
		
		MOVSNW		R3,R1,_BlockIO_Handles	; R3 = Handles list base
		MOVQW		R4,@R1,0,_Handles_Limit	; R4 = Handles list limit
Close_Drives:
		MOVIQW		R2,_GuidBlockIO		; R2 = Pointer to GUID
		CALL32		Close_Protocol
		JMP8CS		Error_Point	; Go if error
		MOVINW		R2,1,0		; R2 = Natural width, 4 for IA32, 8 for x64
		ADD64		R3,R2		; Source address + Natural width
		CMP64UGTE	R3,R4		; Compare with R4 = Limit
		JMP8CC		Close_Drives	; Cycle for all detected drives
		
;---(23)--- Exit -----------------------------------------------------------;

Exit_Point:	POP64		R3
		POP64		R2
		POP64		R1
		XOR64		R7,R7
		RET

;---(24)--- Errors handling, output messages -------------------------------;

Error_Point_FF:
		MOVIQW		R7,0FFh
Error_Point:

		PUSH64		R7
		MOVIQW		R2,0Ch			; Color = Bold Red
		CALL32		Output_Attribute	; Set color
		MOVIQW		R2,_Msg_FAILED
		CALL32		String_Write		; Write parameters at table up
		POP64		R7

		MOVSNW		R2,R1,256		; R2 = Scratch pad, skip used by String_Write
		CALL32		Strings_Errors		; This receive R2=Absolute address
		MOVIBW		@R2,0			; 0=Terminator byte
		MOVIQW		R2,256			; R2 = Scratch pad, skip used by String_Write
		CALL32		String_Write		; This receive R2=Relative address

		MOVIQW		R2,_Msg_Press
		CALL32		String_Write		; Write parameters at table up
		CALL32		Input_Wait_Key		; Wait for key, return when press
		MOVIQW		R2,_Msg_CRLF
		CALL32		String_Write		; Skip 2 strings

		JMP8		Exit_Point

;---------------------------------------------------------------------------;
;           CODE SECTION \ EBC-EXECUTABLE SUBROUTINES LIBRARIES.            ;
;---------------------------------------------------------------------------;

include 'library.inc'				; Helpers subroutines

;---------------------------------------------------------------------------;
;          DATA IN THE CODE SECTION (Note EBC code is native data).         ;
;---------------------------------------------------------------------------;

include 'dataconsts.inc'			; Program and vendor strings

;---------- Base point for fields addressing -------------------------------;
; Negative offsets for constants, positive offsets for variables,
; use +/- regions instead + only to expand region

align 16
Global_Data_Pool:

; This file must be last to eliminate reserve space for non-predefined fields
include 'datavars.inc'				; Scratch pad and variables

;---------------------------------------------------------------------------;
;                            DATA SECTION.                                  ;
;---------------------------------------------------------------------------;

; Not used, data located in the code section, note EBC code is native data.
; This design for old IA32 EFI compatibility and use compact 16-bit offsets.
section '.data' data readable writeable
; This for prevent zero number of relocatable elements
Dummy_Label	DQ  Dummy_Label

;---------------------------------------------------------------------------;
;                     RELOCATION ELEMENTS SECTION.                          ;
;---------------------------------------------------------------------------;

; Not used, data located in the code section, note EBC code is native data.
; This design for old IA32 EFI compatibility and use compact 16-bit offsets.
section '.reloc' fixups data discardable
