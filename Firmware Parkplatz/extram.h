#ifndef _EXTRAM_H_
#define _EXTRAM_H_

/*
For this to work:
-	Put a write protected file into the /dep subdirectory with the single line "HEX_FLASH_FLAGS += -R .extram" in it
	(this file is then included in the makefile and makes sure that the new section is not included in the hex file because
	the makefile would then become to large for the flash)
-	Copy the linker script from program files\winavr\avr\lib\ldscript\... (avrxmega7.x in this case) and add the following at the
	end but before the debug info:
  	.extram : { *(.extram) }
	(To make the linker join all .extram-sections from all files into one output section which can then be relocated by AVR studio with the memory definition)
-	Add the following linker script parameter to the AVR studio dialog
	-Wl,-script=avrxmega7.x
	to make it use the new linker script.
-	Define memory area ".extram" in project properties, starting with XRAM-addr 0x4000 (0x800000-offset added automatically)
-	Use the EXTRAM keyword after every variable declaration that's supposed to reside in the external memory.
*/

//#define EXTRAM

#define EXTRAM __attribute__ ((section (".extram")))


#endif
