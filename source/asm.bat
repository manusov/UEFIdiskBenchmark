del UefiDiskBenchmark.efi, UefiDiskBenchmark.tmp
set INCLUDE=C:\Work\Fasm_Editor_Debugger\WinFasm171\INCLUDE
C:\Work\Fasm_Editor_Debugger\WinFasm171\FASM.EXE UefiDiskBenchmark.asm UefiDiskBenchmark.tmp
E:\v029\source\Utilites\EbcPatch32 UefiDiskBenchmark.tmp UefiDiskBenchmark.efi
comp UefiDiskBenchmark.tmp UefiDiskBenchmark.efi
del UefiDiskBenchmark.tmp
copy UefiDiskBenchmark.efi E:\v011\executable
move UefiDiskBenchmark.efi D:\u.efi
rem pause
