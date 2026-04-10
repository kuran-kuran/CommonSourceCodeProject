Based on MAME 0.152.
Fixes in MAME 0.154 to 0.279 are applied.

Mr. cracyc's fix is applied:
https://github.com/mamedev/mame/commit/eeb81cdf30f80cd7ce797c4574b8534dd4cddc46

cycle_table_rm/pm are changed from dynamic array to static array.

convert char to _TCHAR in disassembler.

i386_limit_check is improved to consider data size.
