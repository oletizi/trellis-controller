/* Linker script for SAMD51J19A */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00004000, LENGTH = 512K - 0x4000  /* Leave space for bootloader */
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 192K
}

/* Stack size */
_stack_size = 0x2000;  /* 8KB stack */

SECTIONS
{
    /* Interrupt vectors */
    .vectors :
    {
        KEEP(*(.vectors))
    } > FLASH
    
    /* Program code */
    .text :
    {
        *(.text*)
        *(.rodata*)
        
        /* Support C++ global constructors */
        . = ALIGN(4);
        KEEP(*(.init))
        KEEP(*(.fini))
        
        /* Global constructor array */
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
        
        . = ALIGN(4);
        _etext = .;
    } > FLASH
    
    /* Initialized data */
    .data : AT(_etext)
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM
    
    /* Uninitialized data */
    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
    
    /* Stack */
    .stack :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + _stack_size;
        . = ALIGN(8);
        _estack = .;
    } > RAM
    
    /* Set data initialization source */
    _sidata = LOADADDR(.data);
}