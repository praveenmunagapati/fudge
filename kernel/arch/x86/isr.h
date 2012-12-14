#define ISR_ROUTINE_SLOTS               256

#define ISR_INDEX_DE                    0x00
#define ISR_INDEX_DB                    0x01
#define ISR_INDEX_NI                    0x02
#define ISR_INDEX_BP                    0x03
#define ISR_INDEX_OF                    0x04
#define ISR_INDEX_BR                    0x05
#define ISR_INDEX_UD                    0x06
#define ISR_INDEX_NM                    0x07
#define ISR_INDEX_DF                    0x08
#define ISR_INDEX_CO                    0x09
#define ISR_INDEX_TS                    0x0A
#define ISR_INDEX_NP                    0x0B
#define ISR_INDEX_SS                    0x0C
#define ISR_INDEX_GP                    0x0D
#define ISR_INDEX_PF                    0x0E
#define ISR_INDEX_MF                    0x10
#define ISR_INDEX_AC                    0x11
#define ISR_INDEX_MC                    0x12
#define ISR_INDEX_XM                    0x13

struct isr_general_registers
{

    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;

};

struct isr_interrupt_registers
{

    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
    unsigned int esp;
    unsigned int ss;

};

struct isr_registers
{

    unsigned int ds;
    struct isr_general_registers general;
    unsigned int index;
    unsigned int extra;
    struct isr_interrupt_registers interrupt;

};

void isr_routine();
void isr_usermode(unsigned int cs, unsigned int ds, unsigned int ip, unsigned int sp);
void isr_setup(unsigned short selector);