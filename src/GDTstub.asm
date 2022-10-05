[bits 64]

global gdt_load

gdtr DW 0 ; For limit storage
     DQ 0 ; For base storage
 
gdt_load:
   MOV   [gdtr], DI
   MOV   [gdtr+2], RSI
   LGDT  [gdtr]
   RET