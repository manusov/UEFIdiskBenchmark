del UefiDiskBenchmark.efi, UefiDiskBenchmark.tmp
set INCLUDE=C:\Work\Fasm_Editor_Debugger\WinFasm171\INCLUDE
C:\Work\Fasm_Editor_Debugger\WinFasm171\FASM.EXE Debug3\UefiDiskBenchmark.asm UefiDiskBenchmark.tmp
UEFImark_EBC_v024\source\Utilites\EbcPatch32 UefiDiskBenchmark.tmp UefiDiskBenchmark.efi
comp UefiDiskBenchmark.tmp UefiDiskBenchmark.efi
del UefiDiskBenchmark.tmp
rem pause
