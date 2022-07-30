// SPDX-Licence-Identifier: GPL-3.0-or-later

#include "mos/kernel.h"
#include "mos/x86/drivers/port.h"
#include "mos/x86/x86_platform.h"

// End-of-interrupt command code
#define PIC_EOI 0x20

const char *x86_exception_names[EXCEPTION_COUNT] = {
    "Divide-By-Zero Error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved",
};

static void isr_handle_irq(x86_stack_frame *frame);
static void isr_handle_exception(x86_stack_frame *frame);

void x86_handle_interrupt(u32 esp)
{
    x86_stack_frame *stack = (x86_stack_frame *) esp;

    if (stack->interrupt_number < IRQ_BASE)
    {
        isr_handle_exception(stack);
        return;
    }
    else if (stack->interrupt_number < IRQ_SYSCALL)
    {
        isr_handle_irq(stack);
    }
    else if (stack->interrupt_number == IRQ_SYSCALL)
    {
        printk("Syscall.");
    }
    else
    {
        printk("Unknown interrupt.");
    }
}

static void isr_handle_exception(x86_stack_frame *stack)
{
    MOS_ASSERT(stack->interrupt_number < EXCEPTION_COUNT);
    pr_warn("x86 exception: %s", x86_exception_names[stack->interrupt_number]);

    // Faults: These can be corrected and the program may continue as if nothing happened.
    // Traps:  Traps are reported immediately after the execution of the trapping instruction.
    // Aborts: Some severe unrecoverable error.
    switch ((x86_exception_enum_t) stack->err_code)
    {
        case EXCEPTION_DIVIDE_ERROR:
        case EXCEPTION_DEBUG:
        case EXCEPTION_NMI:
        case EXCEPTION_OVERFLOW:
        case EXCEPTION_BOUND_RANGE_EXCEEDED:
        case EXCEPTION_INVALID_OPCODE:
        case EXCEPTION_DEVICE_NOT_AVAILABLE:
        case EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN:
        case EXCEPTION_INVALID_TSS:
        case EXCEPTION_SEGMENT_NOT_PRESENT:
        case EXCEPTION_STACK_SEGMENT_FAULT:
        case EXCEPTION_GENERAL_PROTECTION_FAULT:
        case EXCEPTION_PAGE_FAULT:
        case EXCEPTION_FPU_ERROR:
        case EXCEPTION_ALIGNMENT_CHECK:
        case EXCEPTION_SIMD_ERROR:
        case EXCEPTION_VIRTUALIZATION_EXCEPTION:
        case EXCEPTION_CONTROL_PROTECTION_EXCEPTION:
        case EXCEPTION_HYPERVISOR_EXCEPTION:
        case EXCEPTION_VMM_COMMUNICATION_EXCEPTION:
        case EXCEPTION_SECURITY_EXCEPTION:
        {
            mos_warn("Fault Exception %d", stack->interrupt_number);
            break;
        }

        case EXCEPTION_BREAKPOINT:
        {
            mos_warn("Breakpoint not handled.");
            return;
        }

        case EXCEPTION_DOUBLE_FAULT:
        case EXCEPTION_MACHINE_CHECK:
        {
            mos_panic("Fatal x86 Exception:\n"
                      "  %s (%d, by interrupt %d)\n"
                      "General Purpose Registers:\n"
                      "  EAX: 0x%08x EBX: 0x%08x ECX: 0x%08x EDX: 0x%08x\n"
                      "  ESI: 0x%08x EDI: 0x%08x EBP: 0x%08x ESP: 0x%08x\n"
                      "  EIP: 0x%08x\n"
                      "Segment Registers:\n"
                      "  DS:  0x%08x ES:  0x%08x FS:  0x%08x GS:  0x%08x\n"
                      "  CS:  0x%08x\n"
                      "EFLAGS: 0x%08x",                               //
                      x86_exception_names[stack->err_code],           //
                      stack->err_code,                                //
                      stack->interrupt_number,                        //
                      stack->eax, stack->ebx, stack->ecx, stack->edx, //
                      stack->esi, stack->edi, stack->ebp, stack->esp, //
                      stack->eip,                                     //
                      stack->ds, stack->es, stack->fs, stack->gs,     //
                      stack->cs,                                      //
                      stack->eflags                                   //
            );
        }
        default: mos_panic("Unknown exception.");
    }
}

static void isr_handle_irq(x86_stack_frame *frame)
{
    int irq = frame->interrupt_number - IRQ_BASE;

    if (irq == 7 || irq == 15)
    {
        // these irqs may be fake ones, test it
        uint8_t pic = (irq < 8) ? PIC1 : PIC2;
        port_outb(pic + 3, 0x03);
        if ((port_inb(pic) & 0x80) != 0)
            goto irq_handeled;
    }

    pr_debug("IRQ: %d", irq);

irq_handeled:
    if (irq >= 8)
        port_outb(PIC2_COMMAND, PIC_EOI);
    port_outb(PIC1_COMMAND, PIC_EOI);
}