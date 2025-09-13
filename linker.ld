/* x86_64 ELF kernel (PIE-friendly), higher-half link */
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

/* Program headers (PHDRs) so Limine maps with proper permissions */
PHDRS
{
    null    PT_NULL    FLAGS(0);                         /* Null segment */
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2));       /* X | R */
    rodata  PT_LOAD    FLAGS((1 << 2));                  /* R */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2));       /* W | R */
    dynamic PT_DYNAMIC FLAGS((1 << 1) | (1 << 2));       /* W | R (for PIE) */
}

/* Link at classic higher-half base; Limine will relocate as needed (PIE) */
SECTIONS
{
    . = 0xffffffff80000000;

    /* Whole-kernel bounds */
    __kernel_start = .;

    /* -------- TEXT -------- */
    . = ALIGN(0x1000);
    __text_start = .;
    .text : ALIGN(0x1000)
    {
        *(.text .text.*)
    } :text
    __text_end = .;

    /* -------- RODATA -------- */
    . = ALIGN(0x1000);
    __rodata_start = .;
    .rodata : ALIGN(0x1000)
    {
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
    } :rodata
    __rodata_end = .;

    /* -------- INIT/FINI ARRAYS -------- */
    . = ALIGN(0x1000);
    __init_array_start = .;
    .init_array : ALIGN(8)
    {
        KEEP(*(.init_array .init_array.*))
    } :rodata
    __init_array_end = .;

    __fini_array_start = .;
    .fini_array : ALIGN(8)
    {
        KEEP(*(.fini_array .fini_array.*))
    } :rodata
    __fini_array_end = .;

    /* -------- DATA -------- */
    . = ALIGN(0x1000);
    __data_start = .;
    .data : ALIGN(0x1000)
    {
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        *(.got .got.* .got.plt)
    } :data
    __data_end = .;

    /* -------- DYNAMIC (for PIE) -------- */
    .dynamic : ALIGN(0x10)
    {
        *(.dynamic)
    } :data :dynamic

    /* -------- BSS -------- */
    . = ALIGN(0x1000);
    __bss_start = .;
    .bss (NOLOAD) : ALIGN(0x1000)
    {
        *(COMMON)
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
    } :data
    __bss_end = .;

    __kernel_end = .;

    /* Old names kept alive for compatibility */
    text_start_addr   = __text_start;
    text_end_addr     = __text_end;
    rodata_start_addr = __rodata_start;
    rodata_end_addr   = __rodata_end;
    data_start_addr   = __data_start;
    data_end_addr     = __data_end;

    /DISCARD/ :
    {
        *(.comment)
        *(.note .note.*)
        *(.eh_frame .eh_frame.*)
        *(.eh_frame_hdr)
        *(.gnu_debuglink)
        *(.gnu.lto_*)
    }
}
