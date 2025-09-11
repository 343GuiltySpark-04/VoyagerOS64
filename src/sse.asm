[bits 64]


global save_floats
global restore_floats


segment .text

save_floats:
    fxsave [float_save_area]

restore_floats:
    fxrstor [float_save_area]



segment .data

align 16

float_save_area: TIMES 512 db 0
