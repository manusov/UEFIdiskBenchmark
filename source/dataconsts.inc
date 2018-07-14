;===========================================================================;
;=        UEFI Disk Benchmark. Use reads by EFI_BLOCK_IO_PROTOCOL.         =;
;=                 Pre-initialized constants, module 4 of 5.               =;
;=                        (C) IC Book Labs, 2018                           =;
;=                                                                         =;
;= Code compatible and can be integrated into UEFImark EBC Edition project =;
;=      See UEFImark EBC Edition project sources for detail comments       =;
;===========================================================================;

; For compact coding, use 16-bit offsets relative Global_Data_Pool
; instead full 64-bit flat addresses. This method also eliminate relocation
; elements with some bugs in the old IA32 EFI implementations.

;---------- Equations for data access --------------------------------------;

_String_Version		= String_Version - Global_Data_Pool
_String_Copyright	= String_Copyright - Global_Data_Pool
_Program_Vendor_Data_1	= Program_Vendor_Data_1 - Global_Data_Pool
_Program_Vendor_Data	= Program_Vendor_Data - Global_Data_Pool

_Msg_Starting		= Msg_Starting - Global_Data_Pool
_Msg_Exit		= Msg_Exit - Global_Data_Pool
_Msg_CRLF_2		= Msg_CRLF_2 - Global_Data_Pool
_Msg_CRLF		= Msg_CRLF - Global_Data_Pool
_Msg_Error		= Msg_Error - Global_Data_Pool
_Msg_Press		= Msg_Press - Global_Data_Pool
_Msg_OK			= Msg_OK - Global_Data_Pool
_Msg_FAILED		= Msg_FAILED - Global_Data_Pool
_Msg_Addend		= Msg_Addend - Global_Data_Pool

_Parm_CAP		= Parm_CAP - Global_Data_Pool
_Parm_Base		= Parm_Base - Global_Data_Pool
_Parm_Size		= Parm_Size - Global_Data_Pool
_Parm_Units		= Parm_Units - Global_Data_Pool

_Parm_Media_ID		= Parm_Media_ID - Global_Data_Pool
_Parm_Drive_Bytes	= Parm_Drive_Bytes - Global_Data_Pool
_Parm_Drive_Blocks	= Parm_Drive_Blocks - Global_Data_Pool
_Parm_Drive_Block	= Parm_Drive_Block - Global_Data_Pool
_Parm_Full_Bytes	= Parm_Full_Bytes - Global_Data_Pool
_Parm_Full_Blocks	= Parm_Full_Blocks - Global_Data_Pool
_Parm_Tail_Bytes	= Parm_Tail_Bytes - Global_Data_Pool
_Parm_Tail_Blocks	= Parm_Tail_Blocks - Global_Data_Pool
_Parm_Full_Count	= Parm_Full_Count - Global_Data_Pool
_CR_Current_LBA		= CR_Current_LBA - Global_Data_Pool
_Parm_Current_LBA	= Parm_Current_LBA - Global_Data_Pool
_Parm_CAP_Sum		= Parm_CAP_Sum - Global_Data_Pool
_Parm_MBPS		= Parm_MBPS - Global_Data_Pool

_Result_Table		= Result_Table - Global_Data_Pool
_Select_Drive		= Select_Drive - Global_Data_Pool
_Show_Drive		= Show_Drive - Global_Data_Pool
_Last_Drive		= Last_Drive - Global_Data_Pool
_New_Drive		= New_Drive - Global_Data_Pool

_GuidBlockIO		= GuidBlockIO - Global_Data_Pool
_GuidCap		= GuidCap - Global_Data_Pool

_EFI_Errors_Names	= EFI_Errors_Names - Global_Data_Pool
_EFI_Warnings_Names	= EFI_Warnings_Names - Global_Data_Pool


;---------- Data -----------------------------------------------------------;

; This 2 strings must be sequental, without separation by 0.
String_Version		DB  'Block IO protocol benchmark v0.11. '  ; DEBUG FOR CAP=2.4 GHZ FIXED.
String_Copyright	DB  '(C) IC Book Labs.',0

; This string X position must be conform with previous string length.
Program_Vendor_Data_1	DB  0Dh,0Ah                                    ; '  '
Program_Vendor_Data	DB  'http://uefi.composter.com.ua/' , 0

Msg_Starting	        DB  0Dh,0Ah,'Starting...',0Dh,0Ah,0

;--- Messages strings ---

Msg_Exit	DB  'Exit to UEFI.',0Dh,0Ah,0Ah,0
Msg_CRLF_2	DB  0Dh,0Ah
Msg_CRLF	DB  0Dh,0Ah,0
Msg_Error	DB  0Dh,0Ah,'ERROR: ',0
Msg_Press	DB  0Dh,0Ah,'Press any key...',0
Msg_OK		DB  'OK',0
Msg_FAILED	DB  0Dh,0Ah, 'FAILED',0
Msg_Addend	DB  ', status = ',0

;--- Application initialization parameters strings ---
; Note optimizing required, "=" can be supported by message writer

Parm_CAP	DB  'CAP timer='         ,0
Parm_Base	DB  ' MHz, Buffer base=' ,0
Parm_Size	DB  'h, size='           ,0
Parm_Units	DB  ' KB',0Dh,0Ah        ,0

;--- Drive test initialization parameters strings ---
; Note optimizing required, " = " can be supported by message writer

Parm_Media_ID		DB  'Drive media ID = '                   ,0
Parm_Drive_Bytes	DB  'Total bytes per drive = '            ,0
Parm_Drive_Blocks	DB  'Total blocks (sectors) per drive = ' ,0
Parm_Drive_Block	DB  'Bytes per one block (sector) = '     ,0
Parm_Full_Bytes		DB  'Bytes per full sized operation = '   ,0
Parm_Full_Blocks	DB  'Blocks per full sized operation = '  ,0
Parm_Tail_Bytes		DB  'Bytes per tail = '                   ,0
Parm_Tail_Blocks	DB  'Blocks per tail = '                  ,0
Parm_Full_Count		DB  'Number of full sized operations = '  ,0
CR_Current_LBA		DB  0Dh
Parm_Current_LBA	DB  'Current LBA = '                      ,0
Parm_CAP_Sum		DB  'CAP timer accumulator = '            ,0
Parm_MBPS		DB  0Dh,0Ah
			DB  'MBPS = '                             ,0

;--- Scan result string ---

Result_Table	DB  0Dh,0Ah
		DB  ' #    Revision  Media     RM MP LP RO WC  Block     Align     Size'
		DB  0Dh,0Ah,0

;--- Drive selection, last number = f(scan results) ---

Select_Drive	DB  0Ah
Show_Drive      DB  0Dh,'Select item 0...'
Last_Drive	DB  '0 : '
New_Drive       DB  9 DUP (0)   ; DB  '  ',0

;--- GUID macro ---

MACRO GUID x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11
{
DD x1
DW x2, x3
DB x4, x5, x6, x7, x8, x9, x10, x11
}

;--- GUIDs ---

;--- Block IO: 0964E5B21-6459-11D2-8E39-00A0C969723B ---
GuidBlockIO:
GUID  0964E5B21h,06459h,011D2h,08Eh,039h,000h,0A0h,0C9h,069h,072h,03Bh

;--- CPU Architectural: 26BACCB1-6F42-11D4-BCE7-0080C73C8881 ---
GuidCap:
GUID  026BACCB1h,06F42h,011D4h,0BCh,0E7h,000h,080h,0C7h,03Ch,088h,081h

;---------- Equations for errors and warnings codes ------------------------;

;--- Success status ---

EFI_SUCCESS			= 0

;--- EFI error codes, status high bit=1 ---

NUMBER_OF_EFI_ERRORS		= 32
EFIERR				= 8000000000000000h	; This value for 64-bit interpreting
EFI_LOAD_ERROR			= EFIERR or 1
EFI_INVALID_PARAMETER		= EFIERR or 2
EFI_UNSUPPORTED 		= EFIERR or 3
EFI_BAD_BUFFER_SIZE		= EFIERR or 4
EFI_BUFFER_TOO_SMALL		= EFIERR or 5
EFI_NOT_READY			= EFIERR or 6
EFI_DEVICE_ERROR		= EFIERR or 7
EFI_WRITE_PROTECTED		= EFIERR or 8
EFI_OUT_OF_RESOURCES		= EFIERR or 9
EFI_VOLUME_CORRUPTED		= EFIERR or 10
EFI_VOLUME_FULL 		= EFIERR or 11
EFI_NO_MEDIA			= EFIERR or 12
EFI_MEDIA_CHANGED		= EFIERR or 13
EFI_NOT_FOUND			= EFIERR or 14
EFI_ACCESS_DENIED		= EFIERR or 15
EFI_NO_RESPONSE 		= EFIERR or 16
EFI_NO_MAPPING			= EFIERR or 17
EFI_TIMEOUT			= EFIERR or 18
EFI_NOT_STARTED 		= EFIERR or 19
EFI_ALREADY_STARTED		= EFIERR or 20
EFI_ABORTED			= EFIERR or 21
EFI_ICMP_ERROR			= EFIERR or 22
EFI_TFTP_ERROR			= EFIERR or 23
EFI_PROTOCOL_ERROR		= EFIERR or 24
EFI_INCOMPATIBLE_VERSION	= EFIERR or 25
EFI_SECURITY_VIOLATION		= EFIERR or 26
EFI_CRC_ERROR			= EFIERR or 27
EFI_END_OF_MEDIA		= EFIERR or 28
EFI_END_OF_FILE			= EFIERR or 31
EFI_INVALID_LANGUAGE		= EFIERR or 32

;--- EFI warning codes, status high bit=0 ---

NUMBER_OF_EFI_WARNINGS		= 4
EFI_WARN_UNKNOWN_GLYPH		= 1
EFI_WARN_DELETE_FAILURE		= 2
EFI_WARN_WRITE_FAILURE		= 3
EFI_WARN_BUFFER_TOO_SMALL	= 4

;---------- Strings --------------------------------------------------------;

;--- EFI error codes, status high bit=1 ---

EFI_Errors_Names:
DB  '?'				, 0	; 0=Invalid error code
DB  'EFI_LOAD_ERROR'		, 0     ; 1 
DB  'EFI_INVALID_PARAMETER'	, 0     ; 2 
DB  'EFI_UNSUPPORTED'		, 0     ; 3 
DB  'EFI_BAD_BUFFER_SIZE'	, 0     ; 4 
DB  'EFI_BUFFER_TOO_SMALL'	, 0     ; 5 
DB  'EFI_NOT_READY'		, 0     ; 6 
DB  'EFI_DEVICE_ERROR'		, 0     ; 7 
DB  'EFI_WRITE_PROTECTED'	, 0     ; 8 
DB  'EFI_OUT_OF_RESOURCES'	, 0     ; 9 
DB  'EFI_VOLUME_CORRUPTED'	, 0     ; 10
DB  'EFI_VOLUME_FULL'		, 0     ; 11
DB  'EFI_NO_MEDIA'		, 0     ; 12
DB  'EFI_MEDIA_CHANGED'		, 0     ; 13
DB  'EFI_NOT_FOUND'		, 0     ; 14
DB  'EFI_ACCESS_DENIED'		, 0     ; 15
DB  'EFI_NO_RESPONSE'		, 0     ; 16
DB  'EFI_NO_MAPPING'		, 0     ; 17
DB  'EFI_TIMEOUT'		, 0     ; 18
DB  'EFI_NOT_STARTED'		, 0     ; 19
DB  'EFI_ALREADY_STARTED'	, 0     ; 20
DB  'EFI_ABORTED'		, 0     ; 21
DB  'EFI_ICMP_ERROR'		, 0     ; 22
DB  'EFI_TFTP_ERROR'		, 0     ; 23
DB  'EFI_PROTOCOL_ERROR'	, 0     ; 24
DB  'EFI_INCOMPATIBLE_VERSION'	, 0     ; 25
DB  'EFI_SECURITY_VIOLATION'	, 0     ; 26
DB  'EFI_CRC_ERROR'		, 0     ; 27
DB  'EFI_END_OF_MEDIA'		, 0     ; 28
DB  '?'				, 0	; 29
DB  '?'				, 0	; 30
DB  'EFI_END_OF_FILE'		, 0     ; 31
DB  'EFI_INVALID_LANGUAGE'	, 0     ; 32

;--- EFI warning codes, status high bit=0 ---

EFI_Warnings_Names:
DB  '?'				, 0	; 0=Invalid warning code
DB  'EFI_WARN_UNKNOWN_GLYPH'	, 0     ; 1
DB  'EFI_WARN_DELETE_FAILURE'	, 0	; 2
DB  'EFI_WARN_WRITE_FAILURE'	, 0     ; 3
DB  'EFI_WARN_BUFFER_TOO_SMALL'	, 0     ; 4




