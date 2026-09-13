[BITS 32]
GLOBAL context_switch
GLOBAL default_irq_handler

context_switch:
    pushad
    pushfd
    
    mov eax, [esp + 40]         
    mov [eax + 8], esp       

    mov edx, [esp + 44]         
    mov esp, [edx + 8]          

    popfd
    popad
    ret                         

default_irq_handler:
    pushad
    
    ; Send End Of Interrupt (EOI) command to PIC Master
    mov al, 0x20
    out 0x20, al
    
    popad
    iretd                  