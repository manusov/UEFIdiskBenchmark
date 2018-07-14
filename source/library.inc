;===========================================================================;
;=        UEFI Disk Benchmark. Use reads by EFI_BLOCK_IO_PROTOCOL.         =;
;=              Library of helpers subroutines, module 3 of 5.             =;
;=                        (C) IC Book Labs, 2018                           =;
;=                                                                         =;
;= Code compatible and can be integrated into UEFImark EBC Edition project =;
;=      See UEFImark EBC Edition project sources for detail comments       =;
;===========================================================================;

;--- Check input status and get pressed key, without wait key -----------;
; Use Simple Text Input Protocol.                                        ;
; This procedure return also if no keys pressed, check only.             ;
; Key code cleared from buffer, unlike BIOS INT 16h function 1.          ;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;                                                                        ;
; OUTPUT:  R7 = UEFI Status, 64-bit or sign-extended 32 to 64 bit        ;
;          R2 = EFI_KEY, valid if Status=No errors                       ;
;          R2 bits [63-32] = 0, cleared                                  ;
;------------------------------------------------------------------------;

Input_Check_Key:
		MOVNW		R2,@R1,0,_EFI_Table	; R3=SysTable
		MOVNW		R2,@R2,3,24		; R3=ConIn
		PUSHN		R2		; Storage for key code
		PUSHN		R0 	    	; Parameter #2 for protocol
		PUSHN		R2 	    	; Parameter #1 for protocol
		CALL32EXA	@R2,1,0
		POPN		R2		; Remove parameter #1
		POPN		R2		; Remove parameter #2
		POPN		R2		; Read key code
		EXTNDD64	R7,R7
		RET

;--- Check input status and get pressed key, with wait key --------------;
; Use Simple Text Input Protocol.                                        ;
; This procedure not return if no keys pressed, wait for key press.      ;
; Key code cleared from buffer, like BIOS INT 16h function 0.            ;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;                                                                        ;
; OUTPUT:  Condition Flag = Status:                                      ;
;          0 = No errors, key ready, 1 = Error (reserved)                ;
;          R7 = UEFI Status                                              ;
;          R2 = EFI_KEY, valid if Status=No errors                       ;
;          R2 bits [63-32] = 0, cleared                                  ;
;------------------------------------------------------------------------;

Input_Wait_Key:
IWK_0:		CALL32		Input_Check_Key
		CMPI32WEQ	R7,0			; 0 if key ready
		JMP8CC		IWK_0			; Wait if no keys
		RET

;--- Select string from strings sequence --------------------------------;
; INPUT:   R7 = Selector, number of strings to skip                      ;
;          R6 = Selector Limit, skip move if AL>AH                       ;
;          R2 = Source Buffer, pointer to nul-terminated strings pool    ;
;               Both memory pointers is flat 64-bit                      ;
;                                                                        ;
; OUTPUT:  R2 = Pointer to selected string                               ;
;------------------------------------------------------------------------;
String_Selector:
;--- Check limit ---
		CMP32ULTE	R7,R6
		JMP8CC		SS_0		; Go if Selector > Limit
;--- Skip N strings, N=Selector ---
		PUSH64		R6
		PUSH64		R7
SS_3:		CMPI32WEQ	R7,0		; Located here for support R7=0
		JMP8CS		SS_2		; Exit cycle of nul-terminated strings
SS_1:		MOVB		R6,@R2		; Get current char
		MOVSNW		R2,R2,1		; R2+1, modify pointer
		CMPI32WEQ	R6,0
		JMP8CC		SS_1		; Go search cycle if char not zero
		MOVSNW		R7,R7,-1	; R7-1, decrement selector
		JMP8		SS_3
SS_2:		POP64		R6
		POP64		R7
;--- Exit ---
SS_0:		RET

;--- Select string from strings sequence and copy it to destination -----;
; ASCII edition                                                          ;
; INPUT:   R1 = Global variables pool base address, use as base          ;
;          R7 = Selector, number of strings to skip                      ;
;          R6 = Selector Limit, skip move if R7>R6                       ;
;          R2 = Source Buffer, pointer to nul-terminated strings pool    ;
;          R3 = Pointer to destination string                            ;
;               Both memory pointers is flat 64-bit                      ;
;                                                                        ;
; OUTPUT:  R2 = Modified by read source string                           ;
;          R3 = Modified by write destination string                     ;
;------------------------------------------------------------------------;

Copy_Selected_String:
		CALL32		String_Selector
; No RET, continue in the next subroutine

;--- Copy ASCII string --------------------------------------------------;
;                                                                        ;
; INPUT:   R2 = Pointer to nul-terminated source string, absolute 64     ;
;          R3 = Pointer destination string, absolute 64                  ;
;          Source and destination strings is ASCII (1 byte per char)     ;
;                                                                        ;
; OUTPUT:  R2 = Modified by read source string                           ;
;          R3 = Modified by write destination string                     ;
;------------------------------------------------------------------------;

Copy_String:
		PUSH64		R4
CS_2:		MOVB		R4,@R2
		MOVSNW		R2,R2,1		; R2+1
		CMPI64WEQ	R4,0
		JMP8CS		CS_0		; Go if end of string
		CMPI64WEQ	R4,0Ah
		JMP8CS		CS_1		; Go if char=Line Feed
		CMPI64WEQ	R4,0Dh
		JMP8CS		CS_1		; Go if char=Carriage Return
		CMPI64WLTE	R4,' '
		JMP8CS		CS_1		; Go if legal char
		CMPI64WLTE	R4,'}'
		JMP8CS		CS_1		; Go if legal char
		MOVIQW		R4,'.'
CS_1:		MOVB		@R3,R4
		MOVSNW		R3,R3,1		; R3+1
		JMP8		CS_2		; Cycle for all string
CS_0:		MOVB		@R3,R4		; Write terminator=0
		POP64		R4
		RET

;--- Convert Femtoseconds (fs) to MHz and write as ASCII ----------------;
; String not terminated, for flexibility, string can be not a last,      ;
; terminator byte 00h must be added at [R2] by caller, if required.      ;
;                                                                        ;
; INPUT:   R2 = Pointer to destination string                            ;
;          R3 = Number (femtoseconds), valid bits 0-63                   ;
;                                                                        ;
; OUTPUT:  R2 = Modified pointer by string write, current position       ;
;------------------------------------------------------------------------;

String_Fs_MHz:
;--- Save registers ---
		PUSH64		R3
		PUSH64		R4
		PUSH64		R5
;--- Verify input, prepare parameters ---
		XOR64		R4,R4			; R4=Template=0=None
		MOVQ		R5,R3
		CMP64EQ		R5,R4			; Here R3=Period (fs), R4=0
		JMP8CS		SFM_1			; Go if error: divisor=0
		MOVIQQ		R3,1000000000000000	; R3=Femtoseconds per Second
;--- Calculate and write integer part of Frequency, MHz ---
		DIVU64		R3,R5			; R5=Result frequency, Hz
		PUSH64		R3			; Save Result frequency, Hz
		MOVIQD		R5,1000000		; R5=Hz per MHz
		DIVU64		R3,R5			; R3=Result, MHz (integer)
		CALL32		String_Decimal32	; Note here required R4=0
;--- Write decimal point ---
		MOVIBW		@R2,'.'			; Write decimal point
		MOVSNW		R2,R2,1			; R2+1, advance string pointer
;--- Calculate and write float part of Frequency, MHz ---
		POP64		R3			; Restore to R3=Result, Hz
		MODU64		R3,R5			; R3=Remainder after divide by 1000000
		MOVIQD		R5,100000
		DIVU64		R3,R5			; R3=First digit of float part
SFM_2:		CALL32		String_Decimal32	; Note here required R4=0
;--- Restore registers and exit ---
SFM_0:		POP64		R5
		POP64		R4
		POP64		R3
		RET
;--- Handling errors: time value zero ---
SFM_1:		MOVIBW		@R2,'?'			; Write "?" char
		MOVSNW		R2,R2,1			; R2+1, advance string pointer		
		JMP8		SFM_0

;--- Convert Femtoseconds (fs) to Hz and write as ASCII -----------------;
; Can be used for visual any operations per second,                      ;
; if input = femtoseconds per measured operation.                        ;
;                                                                        ;
; String not terminated, for flexibility, string can be not a last,      ;
; terminator byte 00h must be added at [R2] by caller, if required.      ;
;                                                                        ;
; INPUT:   R2 = Pointer to destination string                            ;
;          R3 = Number (femtoseconds), valid bits 0-63                   ;
;                                                                        ;
; OUTPUT:  R2 = Modified pointer by string write, current position       ;
;------------------------------------------------------------------------;

String_Fs_Hz:
;--- Save registers ---
		PUSH64		R3
		PUSH64		R4
		PUSH64		R5
;--- Verify input, prepare parameters ---
		XOR64		R4,R4			; R4=Template=0=None
		MOVQ		R5,R3
		CMP64EQ		R5,R4			; Here R3=Period (fs), R4=0
		JMP8CS		SFM_1			; Go if error: divisor=0
		MOVIQQ		R3,10000000000000000	; R3=Femtoseconds per Second * 10
;--- Calculate and write integer part of Frequency, Hz ---
		DIVU64		R3,R5			; R5=Result frequency, Hz/10
		PUSH64		R3			; Save Result frequency, Hz/10
		MOVIQW		R5,10			; R5=Divisor for units Hz/10
		DIVU64		R3,R5			; R3=Result, Hz (integer)
		CALL32		String_Decimal32	; Note here required R4=0
;--- Write decimal point ---
		MOVIBW		@R2,'.'			; Write decimal point
		MOVSNW		R2,R2,1			; R2+1, advance string pointer
;--- Calculate and write float part of Frequency, MHz ---
		POP64		R3			; Restore to R3=Result, Hz/10
		MODU64		R3,R5			; R3=Remainder after divide by 10
;--- Go to write number ---
		JMP8		SFM_2

;--- Write horizontal line ----------------------------------------------;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
; OUTPUT:  None                                                          ;
;------------------------------------------------------------------------;
Horizontal_Line:
;--- Save registers ---
		PUSH64		R2
		PUSH64		R3
		PUSH64		R4
		PUSH64		R5
		PUSH64		R6
		PUSH64		R7
;--- Built line string ---
		MOVSNW		R2,R1,200
		MOVIQW		R3,'-'
		MOVSNW		R5,R2,79
BVD_0:		MOVB		@R2,R3
		MOVSNW		R2,R2,1
		CMP64UGTE	R2,R5
		JMP8CC		BVD_0
		XOR64		R3,R3
		MOVB		@R2,R3
;--- Visual line string ---
		MOVIQW		R2,200
		CALL32		String_Write		; Write line at table up
;--- Restore registers and exit ---
		POP64		R7
		POP64		R6
		POP64		R5
		POP64		R4
		POP64		R3
		POP64		R2
		RET

;--- Output 8-bit boolean value as ASCII text string --------------------;
;                                                                        ;
; INPUT:   R2 = Pointer to destination string, flat 64                   ;
;          R3 = Number, valid bits 0-7                                  ;
;          R4 = Template size, chars, 0 means no template                ;
;                                                                        ;
; OUTPUT:  R2 = Modified pointer by string write, current position       ;
;------------------------------------------------------------------------;
String_Boolean:
		PUSH64		R3
		PUSH64		R4
		MOVIQW		R4,0FFh
		AND64		R3,R4
		CMPI64WEQ	R3,0
		JMP8CS		SBL_0
		MOVIQW		R3,1
SBL_0:		XOR64		R4,R4
		CALL32		String_Decimal32
		POP64		R4
		POP64		R3
		RET

;--- Output 32-bit unsigned decimal number as ASCII text string ---------;
;                                                                        ;
; INPUT:   R2 = Pointer to destination string, flat 64                   ;
;          R3 = Number, valid bits 0-31                                  ;
;          R4 = Template size, chars, 0 means no template                ;
;                                                                        ;
; OUTPUT:  R2 = Modified pointer by string write, current position       ;
;------------------------------------------------------------------------;

String_Decimal32:
;--- Save registers ---
		PUSH64		R3
		PUSH64		R4
		PUSH64		R5
		PUSH64		R6
;--- Prepare start parameters ---
		EXTNDD64	R3,R3
		MOVIQQ		R5,1000000000
;--- Generate current digit ---
SD_0:		PUSH64		R3
		DIVU64		R3,R5
		MOVIQW		R6,0Fh
		AND64		R3,R6
		CMPI64WGTE	R4,10
		JMP8CS		SD_1
		CMPI64WEQ	R5,1
		JMP8CS		SD_1
		CMPI64WEQ	R3,0
		JMP8CS		SD_2
;--- Write ASCII char ---
SD_1:		MOVSNW		R3,R3,30h
		MOVB		@R2,R3
		MOVSNW		R2,R2,1		; R2+1
		MOVIQW		R4,11
SD_2:		POP64		R3
;--- Generate modulus for next digit ---
		MODU64		R3,R5
;--- Generate divisor for next digit ---
		MOVIQW		R6,10
		DIVU64		R5,R6
;--- Cycle for entire number ---
		MOVSNW		R4,R4,1		; R4+1
		CMPI64WEQ	R5,0
		JMP8CC		SD_0
;--- Restore registers and exit ---
		POP64		R6
		POP64		R5
		POP64		R4
		POP64		R3
		RET

;--- Output 64-bit hex number as ASCII text string ----------------------;
;                                                                        ;
; INPUT:   R2 = Pointer to string                                        ;
;          R3 = Number, valid bits 0-63                                  ;
;                                                                        ;
; OUTPUT:  R2 = Modified pointer by string write, current position       ;
;------------------------------------------------------------------------;

String_Hex64:
		PUSH64		R3
		PUSH64		R4
		MOVIQW		R4,32		; Shift count
		ASHR64		R3,R4
		CALL32		String_Hex32
		POP64		R4
		POP64		R3
; No RET, continue in the next subroutine

;--- Output 32-bit hex number as ASCII text string ----------------------;
;                                                                        ;
; INPUT:   R2 = Pointer to string                                        ;
;          R3 = Number, valid bits 0-31                                  ;
;                                                                        ;
; OUTPUT:  R2 = Modified pointer by string write, current position       ;
;------------------------------------------------------------------------;

String_Hex32:
		PUSH64		R3
		PUSH64		R4
		MOVIQW		R4,16		; Shift count
		ASHR64		R3,R4
		CALL32		String_Hex16
		POP64		R4
		POP64		R3
; No RET, continue in the next subroutine

;--- Output 16-bit hex number as ASCII text string ----------------------;
;                                                                        ;
; INPUT:   R2 = Pointer to string                                        ;
;          R3 = Number, valid bits 0-15                                  ;
;                                                                        ;
; OUTPUT:  R2 = Modified pointer by string write, current position       ;
;------------------------------------------------------------------------;

String_Hex16:
		PUSH64		R3
		PUSH64		R4
		MOVIQW		R4,08		; Shift count
		ASHR64		R3,R4
		CALL32		String_Hex8
		POP64		R4
		POP64		R3
; No RET, continue in the next subroutine

;--- Output 8-bit hex number as ASCII text string -----------------------;
;                                                                        ;
; INPUT:   R2 = Pointer to string                                        ;
;          R3 = Number, valid bits 0-7                                   ;
;                                                                        ;
; OUTPUT:  R2 = Modified pointer by string write, current position       ;
;------------------------------------------------------------------------;

String_Hex8:
		PUSH64		R3
		PUSH64		R4
		MOVIQW		R4,04		; Shift count
		ASHR64		R3,R4
		CALL32		String_Hex4
		POP64		R4
		POP64		R3
; No RET, continue in the next subroutine

;--- Output 4-bit hex number as ASCII text string -----------------------;
;                                                                        ;
; INPUT:   R2 = Pointer to string                                        ;
;          R3 = Number, valid bits 0-3                                   ;
;                                                                        ;
; OUTPUT:  R2 = Modified pointer by string write, current position       ;
;------------------------------------------------------------------------;

String_Hex4:
		PUSH64		R3
		PUSH64		R4
		MOVIQW		R4,0Fh		; Select bits [3-0]
		AND64		R3,R4
		MOVIQW		R4,'0'		; 0-9 convert to 30h-39h
		CMPI32WLTE	R3,9
		JMP8CS		SH4_0
		MOVIQW		R4,'A'-10	; A-F convert to 41h-46h
SH4_0:		ADD64		R3,R4
		MOVB		@R2,R3		; Write ASCII char
		MOVSNW		R2,R2,1		; String pointer+1
		POP64		R4
		POP64		R3
		RET

;--- Output text string -------------------------------------------------;
; Use Simple Text Output Protocol, string must be in UNICODE16.          ;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;          R2 = Pointer to string, flat 64                               ;
;                                                                        ;
; OUTPUT:  R7 = UEFI Status                                              ;
;------------------------------------------------------------------------;

Output_String:
		PUSH64		R3
		PUSH64		R2		; R2 for alignment only
		MOVNW		R3,@R1,0,_EFI_Table	; R3=SysTable
		MOVNW		R3,@R3,5,24		; R3=ConOut
;--- Start built frame ---
		PUSHN		R2 	    	; Parameter #2 for protocol
		PUSHN		R3 	    	; Parameter #1 for protocol
;--- Here R0 must be align 16 ---
; OK for x64 (N=8), need correct for IA32 (N=4) ---
;---
		CALL32EXA	@R3,1,0
;--- Externally used exit point ---
OS_0:		POPN		R3		; Remove parameter #1
		POPN		R2		; Remove parameter #2
		POP64		R2		; R2 for alignment only
		POP64		R3
		RET

;--- Set video attribute for next string writes -------------------------;
; Use Simple Text Output Protocol.                                       ;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;          R2 = Video attribute                                          ;
;                                                                        ;
; OUTPUT:  R7 = UEFI Status                                              ;
;------------------------------------------------------------------------;

Output_Attribute:
		PUSH64		R3
		PUSH64		R2		; R2 for Align 16 only
		MOVNW		R3,@R1,0,_EFI_Table	; R3=SysTable
		MOVNW		R3,@R3,5,24		; R3=ConOut
;--- Start built frame ---
		PUSHN		R2 	    	; Parameter #2 for protocol
		PUSHN		R3 	    	; Parameter #1 for protocol
;--- Here R0 must be align 16 ---
; OK for x64 (N=8), need correct for IA32 (N=4) ---
;---
		CALL32EXA	@R3,5,0
		JMP8		OS_0

;--- Output text string -------------------------------------------------;
; Use conversion ASCII->UNICODE16 and Simple Text Output Protocol,       ;
; string must be in ASCII.                                               ;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address, use buffer on it     ;
;          R2 = Relative pointer to string, signed offset relative R1    ;
;                                                                        ;
; OUTPUT:  R7 = UEFI Status                                              ;
;------------------------------------------------------------------------;

String_Write:
		PUSH64		R2
		PUSH64		R3
		ADD64		R2,R1			; R2=Source, add R1 to relative offset
		MOVQ		R3,R1			; R3=Destination, use scratch pad at R1
		CALL32		Convert_String_1	; Convert ASCII-UNICODE16
		MOVQ		R2,R1
;--- Here R0 must be align 16 ---
		CALL32		Output_String		; Write UNICODE16
		POP64		R3
		POP64		R2
		RET

;--- Convert string from ASCII to UNICODE16 -----------------------------;
;                                                                        ;
; INPUT:   R2 = Pointer to nul-terminated source string, flat 64         ;
;          R3 = Pointer destination string, flat 64                      ;
;          Source string is ASCII (1 byte per char)                      ;
;          Destination string is UNICODE (2 bytes per char)              ;
;                                                                        ;
; OUTPUT:  R2 = Modified by read source string                           ;
;          R3 = Modified by write destination string                     ;
;------------------------------------------------------------------------;

Convert_String_1:
		PUSH64		R4
CS1_2:		MOVB		R4,@R2
		CMPI64WEQ	R4,0
		JMP8CS		CS1_0		; Go if end of string
		CMPI64WEQ	R4,0Ah
		JMP8CS		CS1_1		; Go if char=Line Feed
		CMPI64WEQ	R4,0Dh
		JMP8CS		CS1_1		; Go if char=Carriage Return
		CMPI64WLTE	R4,' '
		JMP8CS		CS1_1		; Go if legal char
		CMPI64WLTE	R4,'}'
		JMP8CS		CS1_1		; Go if legal char
		MOVIQW		R4,'.'
CS1_1:		MOVW		@R3,R4
		MOVSNW		R2,R2,1
		MOVSNW		R3,R3,2
		JMP8		CS1_2		; Cycle for all string
CS1_0:		MOVW		@R3,R4		; Write terminator=0
		POP64		R4
		RET

;--- Get CPU Architectural Protocol timer period and current value ------;
; INPUT   : R1 = Global variables pool base address                      ;
;                                                                        ;
; OUTPUT  : Condition Flag (CF) = Status:                                ;
;            0(CC) = No errors, 1(CS) = Error detected                   ;
;           R7 = UEFI Status, unified for 32 and 64                      ;
;           R6 = CAP timer period, femtoseconds                          ;
;           R5 = CAP timer value, clocks                                 ;
;           R2-R4 can be corrupted, because save-restore operations      ;
;                 minimized for better timings.                          ;
;------------------------------------------------------------------------;

Read_CAP_Timer:
;--- Assign CAP protocol pointer ---
		MOVQW		R2,@R1,0,_CAP_Protocol	; Save CAP interface pointer
;--- Assign timer index=0 ---
		XOR64		R6,R6
;--- Prepare scratch pads in the stack ---
		XOR64		R7,R7		; R7=0 for blank storages
		PUSH64		R7		; Storage for Timer Value
		PUSH64		R7		; Storage for Timer Period
;--- Built function frame frame ---
		MOVQ		R7,R0		; R7=Address of Storage for Timer Period
		PUSHN		R7		; Parm#4=Pointer to Timer Period
		MOVSNW		R7,R7,8 	; R7=Address of Storage for Timer Value
		PUSHN		R7		; Parm#3=Pointer to Timer Value
		PUSHN		R6		; Parm#2=Timer index
		PUSHN		R2		; Parm#1=Protocol pointer
;--- Call function ---
		CALL32EXA	@R2,6,0 	; CapTable.GetTimerValue
;--- Remove function stack frame ---
		POPN		R2		; Parm#1, restore
		POPN		R6		; Parm#2, restore
		POPN		R3		; Parm#3, remove only
		POPN		R3		; Parm#4, remove only
;--- Read and remove output parameters ---
		POP64		R6		; R6=Timer Period
		POP64		R5		; R5=Timer Value
;--- Check status ---
		MOVSNW		R7,R7		; Make unify for 32 and 64
		CMPI64WUGTE	R7,1		; CF=1 if R7>0
;--- Return ---
		RET

;--- Allocate memory pages from the UEFI firmware -----------------------;
; Use 64-bit flat memory addressing even if IA32.                        ;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;          R2 = EFI Memory Allocate Type                                 ;
;          R3 = EFI Memory Type                                          ;
;          R4 = Number of contiguous 4KB pages                           ;
;          R5 = Address Value or Limit, used only for                    ;
;               AllocateAddress, AllocateMaxAddress                      ;
;                                                                        ;
; OUTPUT:  Condition flag: 0(CC)=Successfully, 1(CS)=Failed              ;
;          R7 = UEFI Status                                              ;
;          R5 = Base address, if allocate OK                             ;
;------------------------------------------------------------------------;

EFI_Allocate_Pages:
;--- Built parameter storage, addressed by Parm#4 ---
		PUSH64		R5		; Address field for Parm#4 (QWORD)
;--- Built function stack frame ---
		PUSHN		R0		; Parm#4=Pointer to address
		PUSHN		R4		; Parm#3=Number of Pages
		PUSHN		R3		; Parm#2=Memory Type
		PUSHN		R2		; Parm#1=Allocate Type
;--- Read pointer and handler call ---
		MOVNW		R7,@R1,0,_EFI_Table	; R7=SysTable
		MOVNW		R7,@R7,9,24		; R7=BootServices
		CALL32EXA	@R7,2,24		; Header=24, Element=2
;--- Remove stack frame ---
		POPN		R2		; Remove Parm#1
		POPN		R3		; Remove Parm#2
		POPN		R4		; Remove Parm#3
		POPN		R5		; Remove Parm#4
;--- Get data from parameter storage, addressed by Parm#4 ---
		POP64		R5		; Allocated base address
;--- Convert 32/64-bit results to 64-bit, analyse status ---
		MOVNW		R5,R5		; Make address 64-bit
EAP_0:		MOVSNW		R7,R7		; Determine D[63-32]
		CMPI64WUGTE	R7,1		; Check status, set CS if R7>0
;--- Return, here Condition Flag=F(R7)  ---
		RET

;--- Free memory pages from the UEFI firmware ---------------------------;
; Use 64-bit flat memory addressing even if IA32.                        ;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;          R2 = Base Address of memory block                             ;
;          R3 = Number of contiguous 4KB pages                           ;
;                                                                        ;
; OUTPUT:  Condition flag: 0(CC)=Successfully, 1(CS)=Failed              ;
;          R7 = UEFI Status                                              ;
;------------------------------------------------------------------------;

EFI_Free_Pages:
;--- Built function stack frame ---
		PUSHN		R3		; Parm#2=Number of Pages
		PUSH64		R2		; Parm#1=Base address of block (QWORD)
;--- Read pointer and handler call ---
		MOVNW		R7,@R1,0,_EFI_Table	; R7=SysTable
		MOVNW		R7,@R7,9,24		; R7=BootServices
		CALL32EXA	@R7,3,24		; Header=24, Element=3
;--- Remove stack frame ---
		POP64		R2		; Remove Parm#1
		POPN		R3		; Remove Parm#2
;--- Go to set Condition Flag=F(R7) and return ---
		JMP8		EAP_0

;--- Locate EFI protocol by GUID ----------------------------------------;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;          R2 = Pointer to Protocol 128-bit GUID,                        ;
;               signed 16-bit offset relative R1                         ;
;                                                                        ;
; OUTPUT:  Condition Flag = Status:                                      ;
;          0(CC) = No errors, R2 valid, 1(CS) = Error                    ;
;          R7 = UEFI Status                                              ;
;          R2 = Protocol pointer if operation successfully               ;
;------------------------------------------------------------------------;
Locate_Protocol:
;--- Calculate absolute pointer ---
		ADD64		R2,R1
;--- Save registers ---
		PUSH64		R3
		PUSH64		R4
		PUSH64		R5
		PUSH64		R6
;--- Built function stack frame ---
		XOR64		R3,R3
		PUSHN		R3		; Storage for output
		PUSHN		R0		; Parm#3 = Output pointer
		PUSHN		R3		; Parm#2 = Registration
		PUSHN		R2		; Parm#1 = Pointer to GUID
;--- Read pointer and handler call ---
		MOVNW		R3,@R1,0,_EFI_Table	; R3 = SysTable
		MOVNW		R3,@R3,9,24		; R3 = BootServices
		CALL32EXA	@R3,37,24		; Entry #37 = Locate Protocol
;--- Remove stack frame ---
		POPN		R2		; Parm#1
		POPN		R3		; Parm#2
		POPN		R3		; Parm#3
		POPN		R2		; Protocol pointer
;--- Check status and result ---
		MOVSNW		R7,R7
		CMPI64WUGTE	R7,1		; Check status
		JMP8CS		LP_0
		CMPI64WEQ	R2,0		; Check protocol pointer
;--- Restore registers and return ---
LP_0:		POP64		R6
		POP64		R5
		POP64		R4
		POP64		R3
		RET

;--- Built (locate) handles list by EFI protocol GUID -------------------;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;          R2 = Pointer to Protocol 128-bit GUID,                        ;
;               R2 is signed 16-bit offset relative R1                   ;
;          R3 = Pointer to output buffer, handles list                   ;
;               R3 is signed 16-bit offset relative R1                   ;
;          R4 = Output buffer length maximum limit, bytes                ;
;                                                                        ;
; OUTPUT:  Condition Flag = Status:                                      ;
;          0(CC) = No errors, R2 valid, 1(CS) = Error                    ;
;          R7 = UEFI Status                                              ;
;          R2 = Absolute limit of handles buffer, if no errors           ;
;          R3 = Absolute address of handles buffer, if no errors         ;
;------------------------------------------------------------------------;
Locate_Handles:
;--- Calculate absolute pointers ---
		ADD64		R2,R1
		ADD64		R3,R1
;--- Save registers ---
		PUSH64		R4		; Note R0 = Stack pointer
		PUSH64		R5		; Note R0-R3 is non-volatile by callee
		PUSH64		R6		; Note R4-R7 is volatile by callee
;--- Built function stack frame ---
		XOR64		R5,R5
		PUSH64		R4		; Storage for output = buffer size
		MOVQ		R6,R0		; R6 = Pointer to temporary output variable
		PUSHN		R3		; Parm #5 = Pointer to buffer
		PUSHN		R6		; Parm #4 = Pointer to buffer size
		PUSHN		R5		; Parm #3 = Search Key, not used = 0
		PUSHN		R2		; Parm #2 = Pointer to GUID
		PUSHN		R5,2		; Parm #1 = Search Type, by Protocol = 2
;--- Read pointer and handler call ---
		MOVNW		R7,@R1,0,_EFI_Table	; R7 = SysTable
		MOVNW		R7,@R7,9,24		; R7 = BootServices
		CALL32EXA	@R7,19,24		; Entry #19 = Locate Handle (handles list)
;--- Remove stack frame ---
		POPN		R2		; Remove Parm#1
		POPN		R2		; Remove Parm#2
		POPN		R2		; Remove Parm#3
		POPN		R2		; Remove Parm#4
		POPN		R2		; Remove Parm#5
		POP64		R2		; Pointer to updated parameter = buffer size
;--- Calculate buffer limit ---
		ADD64		R2,R3		; R2 = Limit = Size + Base
;--- Check status and result ---
LH_1:		MOVSNW		R7,R7
		CMPI64WUGTE	R7,1		; Check status, error if status > 0
		JMP8CS		LH_0
		CMPI64WEQ	R2,0		; Check buffer size, error if buffer size = 0
;--- Restore registers and return ---
LH_0:		POP64		R6
		POP64		R5
		POP64		R4
		RET

;--- Open protocol (get interface) by EFI protocol handle ---------------;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;          R2 = Pointer to Protocol 128-bit GUID,                        ;
;               R2 is signed 16-bit offset relative R1                   ;
;          R3 = Absolute pointer to protocol handle                      ;
;                                                                        ;
; OUTPUT:  Condition Flag = Status:                                      ;
;          0(CC) = No errors, R2 valid, 1(CS) = Error                    ;
;          R7 = UEFI Status                                              ;
;          R2 = Protocol interface pointer, if no errors                 ;
;------------------------------------------------------------------------;
Open_Protocol:
;--- Calculate absolute pointers ---
		ADD64		R2,R1
;--- Save registers ---
		PUSH64		R4		; Note R0 = Stack pointer
		PUSH64		R5		; Note R0-R3 is non-volatile by callee
		PUSH64		R6		; Note R4-R7 is volatile by callee
;--- Built function stack frame ---
		XOR64		R4,R4
		PUSH64		R4			; Storage for output data
		MOVQ		R5,R0
		PUSHN		R4,1			; Parm #6 = EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
		PUSHN		R4			; Parm #5 = Controller handle = 0, not used here
		PUSHN		@R1,0,_EFI_Handle	; Parm #4 = Application Module Handle
		PUSHN		R5			; Parm #3 = Pointer to Pointer to Interface
		PUSHN		R2			; Parm #2 = Pointer to GUID
		PUSHN		@R3			; Parm #1 = Handle
;--- Read pointer and handler call ---
		MOVNW		R7,@R1,0,_EFI_Table	; R7 = SysTable
		MOVNW		R7,@R7,9,24		; R7 = BootServices
		CALL32EXA	@R7,32,24		; #32 = OpenProtocol
;--- Remove stack frame ---
		POPN		R2		; Remove Parm#1
		POPN		R2		; Remove Parm#2
		POPN		R2		; Remove Parm#3
		POPN		R2		; Remove Parm#4
		POPN		R2		; Remove Parm#5
		POPN		R2		; Remove Parm#6
		POP64		R2		; R2 = Protocol interface pointer
;--- Go to check status and result, restore registers, exit ---
		JMP8		LH_1

;--- Close protocol, update handles database ----------------------------;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;          R2 = Pointer to Protocol 128-bit GUID,                        ;
;               R2 is signed 16-bit offset relative R1                   ;
;          R3 = Absolute pointer to protocol handle                      ;
;                                                                        ;
; OUTPUT:  Condition Flag = Status:                                      ;
;          0(CC) = No errors, R2 valid, 1(CS) = Error                    ;
;          R7 = UEFI Status                                              ;
;          R2 = Reserved                                                 ;
;------------------------------------------------------------------------;
Close_Protocol:
;--- Calculate absolute pointers ---
		ADD64		R2,R1
;--- Save registers ---
		PUSH64		R4		; Note R0 = Stack pointer
		PUSH64		R5		; Note R0-R3 is non-volatile by callee
		PUSH64		R6		; Note R4-R7 is volatile by callee
;--- Built function stack frame ---
		XOR64		R4,R4
		PUSHN		R4			; Parm #4 = Controller handle = 0, not used here
		PUSHN		@R1,0,_EFI_Handle	; Parm #3 = Application Module Handle
		PUSHN		R2			; Parm #2 = Pointer to GUID
		PUSHN		@R3			; Parm #1 = Handle
;--- Read pointer and handler call ---
		MOVNW		R7,@R1,0,_EFI_Table	; R7 = SysTable
		MOVNW		R7,@R7,9,24		; R7 = BootServices
		CALL32EXA	@R7,33,24		; #33 = OpenProtocol
;--- Remove stack frame ---
		POPN		R2		; Remove Parm#1
		POPN		R2		; Remove Parm#2
		POPN		R2		; Remove Parm#3
		POPN		R2		; Remove Parm#4
;--- Check status ---
		MOVSNW		R7,R7
		CMPI64WUGTE	R7,1		; Check status, error if status > 0
;--- Go to restore registers and return ---
		JMP8		LH_0


;--- Write error or warning string by UEFI status code ------------------;
; Built string only, caller must visual by String_Write.                 ;
; String not terminated, caller must terminate it (for flexibility).     ;
;                                                                        ;
; INPUT:   R1 = Global variables pool base address                       ;
;          R2 = Pointer to destination string, flat 64                   ;
;          R7 = UEFI Status for decoding as text string,                 ;
;               converted to 64-bit                                      ;
;                                                                        ;
; OUTPUT:  R2 = Modified by write destination string                     ;
;          ASCII text string at [input R2]                               ;
;------------------------------------------------------------------------;

Strings_Errors:
;--- Save registers ---
		PUSH64		R3
		PUSH64		R4
;--- Reload destination pointer ---
		MOVQ		R3,R2		; R3=Destination
;--- Select error, warning or unsupported code, mask R7 ---
		MOVIQQ		R2,8000000000000000h
		AND64		R2,R7
		CMPI64WEQ	R2,0		; Set Condition Flag = F(R7.63)
		MOVIQW		R2,00FFh
		AND64		R7,R2		; Mask bits R7.[7-0]
		JMP8CS		SE_3		; If bit R7.63=0, go WARNING mode
		MOVSNW		R4,R1,_EFI_Errors_Names
		CMPI32WULTE	R7,NUMBER_OF_EFI_ERRORS
		JMP8CC		SE_1		; Go skip if wrong ERROR number
		JMP8		SE_2		; Otherwise go execute
SE_3:		MOVSNW		R4,R1,_EFI_Warnings_Names
		CMPI32WULTE	R7,NUMBER_OF_EFI_WARNINGS
		JMP8CC		SE_1		; Go skip if wrong WARNING number
;--- Built left constant part ", status = " ---
SE_2:		MOVSNW		R2,R1,_Msg_Addend
		CALL32		Copy_String	; R2=Source, R3=Destination
;--- Cycle for select message, count nul-terminated strings ---
		MOVQ		R2,R4		; R2=Pointer
		CMPI32WEQ	R7,0		; Check status=0
		JMP8CS		SE_4		; Skip scan if argument=0
SE_0:		MOVB		R4,@R2		; Load current char to R4
		MOVSNW		R2,R2,1		; R2+1, increment chars pointer
		CMPI32WEQ	R4,0		; Check char is nul
		JMP8CC		SE_0		; Cycle if char not nul
		MOVSNW		R7,R7,-1	; R7-1, decrement string number
		CMPI32WEQ	R7,0		; Check count is done
		JMP8CC		SE_0		; Cycle if count not done
;--- Built right variable part: error name ---
SE_4:		CALL32		Copy_String	; R2=Source, R3=Destination
;--- Set output parameter ---
SE_1:		MOVQ		R2,R3		; R2=Modified destination pointer
;--- Restore registers and return ---
		POP64		R4
		POP64		R3
		RET
