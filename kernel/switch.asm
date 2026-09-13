[BITS 32]
GLOBAL context_switch

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