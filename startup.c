//*****************************************************************************
//
// startup_gcc.c - Startup code for use with GNU tools.
//
// 
// This is part of revision 9453 of the EK-LM3S6965 Firmware Package.
//
//*****************************************************************************
//*****************************************************************************
//
// Forward declaration of the default fault handlers.
//
//*****************************************************************************
void ResetISR(void);
//
//
extern void NmiSR(void);
extern void FaultISR(void);
extern void IntDefaultHandler(void);
extern void uart0_isr(void);
extern void uart1_isr(void);
extern void uart2_isr(void);
extern void systick_isr(void);
extern void mpu_isr(void);
extern void bus_isr(void);
extern void usage_isr(void);
extern void systick_isr(void);
//
//
extern int main(void);
//
//
//
#define sz_STACK 128
//
static unsigned long stack[sz_STACK];
int get_stack_top(void){ return (int)(stack + sizeof(stack)); }
int get_stack_bottom(void){ return (int)stack; }
//
//
// The vector table.  Note that the proper constructs must be placed on this to
// ensure that it ends up at physical address 0x0000.0000.
//
//
// I don't like the g_pfnFoobar naming convention
// How about vtable
//
//

__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) =
{
    (void (*)(void))((unsigned long)stack + sizeof(stack)),
                                            // The initial stack pointer
    ResetISR,                               // The reset handler
    NmiSR,                                  // The NMI handler
    FaultISR,                               // The hard fault handler
    mpu_isr, //IntDefaultHandler,                      // The MPU fault handler
    bus_isr, //IntDefaultHandler,                      // The bus fault handler
    usage_isr, //IntDefaultHandler,                      // The usage fault handler
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    0,                                      // Reserved
    IntDefaultHandler,                      // SVCall handler
    IntDefaultHandler,                      // Debug monitor handler
    0,                                      // Reserved
    IntDefaultHandler,                      // The PendSV handler
    systick_isr, //IntDefaultHandler,                      // The SysTick handler
    IntDefaultHandler,                      // GPIO Port A
    IntDefaultHandler,                      // GPIO Port B
    IntDefaultHandler,                      // GPIO Port C
    IntDefaultHandler,                      // GPIO Port D
    IntDefaultHandler,                      // GPIO Port E
    uart0_isr, //IntDefaultHandler,                      // UART0 Rx and Tx
    uart1_isr, //IntDefaultHandler,                      // UART1 Rx and Tx
    IntDefaultHandler,                      // SSI0 Rx and Tx
    IntDefaultHandler,                      // I2C0 Master and Slave
    IntDefaultHandler,                      // PWM Fault
    IntDefaultHandler,                      // PWM Generator 0
    IntDefaultHandler,                      // PWM Generator 1
    IntDefaultHandler,                      // PWM Generator 2
    IntDefaultHandler,                      // Quadrature Encoder 0
    IntDefaultHandler,                      // ADC Sequence 0
    IntDefaultHandler,                      // ADC Sequence 1
    IntDefaultHandler,                      // ADC Sequence 2
    IntDefaultHandler,                      // ADC Sequence 3
    IntDefaultHandler,                      // Watchdog timer
    IntDefaultHandler,                      // Timer 0 subtimer A
    IntDefaultHandler,                      // Timer 0 subtimer B
    IntDefaultHandler,                      // Timer 1 subtimer A
    IntDefaultHandler,                      // Timer 1 subtimer B
    IntDefaultHandler,                      // Timer 2 subtimer A
    IntDefaultHandler,                      // Timer 2 subtimer B
    IntDefaultHandler,                      // Analog Comparator 0
    IntDefaultHandler,                      // Analog Comparator 1
    IntDefaultHandler,                      // Analog Comparator 2
    IntDefaultHandler,                      // System Control (PLL, OSC, BO)
    IntDefaultHandler,                      // FLASH Control
    IntDefaultHandler,                      // GPIO Port F
    IntDefaultHandler,                      // GPIO Port G
    IntDefaultHandler,                      // GPIO Port H
    uart2_isr, //IntDefaultHandler,                      // UART2 Rx and Tx
    IntDefaultHandler,                      // SSI1 Rx and Tx
    IntDefaultHandler,                      // Timer 3 subtimer A
    IntDefaultHandler,                      // Timer 3 subtimer B
    IntDefaultHandler,                      // I2C1 Master and Slave
    IntDefaultHandler,                      // Quadrature Encoder 1
    IntDefaultHandler,                      // CAN0
    IntDefaultHandler,                      // CAN1
    IntDefaultHandler,                      // CAN2
    IntDefaultHandler,                      // Ethernet
    IntDefaultHandler                       // Hibernate
};

//*****************************************************************************
//
// The following are constructs created by the linker, indicating where the
// the "data" and "bss" segments reside in memory.  The initializers for the
// for the "data" segment resides immediately following the "text" segment.
//
//*****************************************************************************
extern unsigned long _etext;
extern unsigned long _data;
extern unsigned long _edata;
extern unsigned long _bss;
extern unsigned long _ebss;

//extern unsigned long _end;

//extern unsigned long _xxx;

//*****************************************************************************
//
// This is the code that gets called when the processor first starts execution
// following a reset event.  Only the absolutely necessary set is performed,
// after which the application supplied entry() routine is called.  Any fancy
// actions (such as making decisions based on the reset cause register, and
// resetting the bits in that register) are left solely in the hands of the
// application.
//
//*****************************************************************************

void ResetISR(void){
  unsigned long *src, *dest;

  // Copy the data segment initializers from flash to SRAM.
  src = &_etext;

  for(dest=&_data; dest<&_edata;){
    *dest++ = *src++;
  }


  // Zero fill the bss segment.
  __asm("            ldr     r0, =_bss          \n"
	"            ldr     r1, =_ebss         \n"
	"            mov     r2, #0             \n"
	"            .thumb_func                \n"
	"zero_loop:                             \n"
	"            cmp     r0, r1             \n"
	"            it      lt                 \n"
	"            strlt   r2, [r0], #4       \n"
	"            blt     zero_loop            "
  );

  main();

}


