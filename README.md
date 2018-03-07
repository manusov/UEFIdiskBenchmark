# UefiDiskBenchmark
Mass storage benchmark, based on EFI_BLOCK_IO_PROTOCOL
EFI Byte Code (EBC) application.

"source" directory contains FASM sources.

"executable" directory contains UEFI EBC application.

Output parameters and flags:

"#" Device number in the list

"Revision" Revision of UEFI API EFI_BLOCK_IO_PROTOCOL.

"Media" Media ID

"RM" Removable Media flag, for example CD or USB flash

"MP" Media Present

"LP" Logical Partition

"RO" Read Only

"WC" Write Cache

"Block" Block size, bytes

"Align" Required alignment for memory buffer, bytes

"Size" Available size of mass storage device

Known bug:
maximum 10 devices supported, include aliases.









