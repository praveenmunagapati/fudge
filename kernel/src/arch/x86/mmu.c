#include <memory.h>
#include <error.h>
#include <runtime.h>
#include <arch/x86/arch.h>
#include <arch/x86/cpu.h>
#include <arch/x86/idt.h>
#include <arch/x86/isr.h>
#include <arch/x86/mmu.h>

static void enable()
{

    cpu_set_cr0(cpu_get_cr0() | 0x80000000);

}

static void set_directory_table(struct mmu_directory *directory, unsigned int frame, struct mmu_table *table, unsigned int tflags)
{

    directory->tables[frame / MMU_TABLE_SLOTS] = (struct mmu_table *)((unsigned int)table | tflags);

}

static void set_table_page(struct mmu_table *table, unsigned int frame, unsigned int page, unsigned int pflags)
{

    table->pages[frame % MMU_PAGE_SLOTS] = (void *)(page | pflags);

}

void mmu_map_memory(struct mmu_directory *directory, struct mmu_table *table, unsigned int paddress, unsigned int vaddress, unsigned int size, unsigned int tflags, unsigned int pflags)
{

    unsigned int frame = vaddress / MMU_PAGE_SIZE;
    unsigned int i;

    memory_clear(table, sizeof (struct mmu_table));

    for (i = 0; i < size / MMU_PAGE_SIZE; i++)
        set_table_page(table, frame + i, paddress + i * MMU_PAGE_SIZE, pflags);

    set_directory_table(directory, frame, table, tflags);

}

void mmu_load_memory(struct mmu_directory *directory)
{

    cpu_set_cr3((unsigned int)directory);

}

void mmu_reload_memory()
{

    cpu_set_cr3(cpu_get_cr3());

}

static void handle_interrupt(struct isr_registers *registers)
{

    unsigned int address = cpu_get_cr2();

    error_register(1, address);
    error_register(2, registers->extra);

}

void mmu_setup_arch(unsigned short selector)
{

    struct mmu_directory *directory = (struct mmu_directory *)(0x00300000);
    struct mmu_table *rtable = (struct mmu_table *)(0x00310000);
    struct mmu_table *ktable = (struct mmu_table *)(0x00320000);

    idt_set_entry(0x0E, mmu_routine, selector, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_TYPE32INT);
    isr_set_routine(0x0E, handle_interrupt);

    mmu_map_memory(directory, ktable, ARCH_KERNEL_BASE, ARCH_KERNEL_BASE, ARCH_KERNEL_SIZE, MMU_TABLE_FLAG_PRESENT | MMU_TABLE_FLAG_WRITEABLE, MMU_PAGE_FLAG_PRESENT | MMU_PAGE_FLAG_WRITEABLE);
    mmu_map_memory(directory, rtable, RUNTIME_TASK_PADDRESS_BASE + RUNTIME_TASK_ADDRESS_SIZE, RUNTIME_TASK_VADDRESS_BASE, RUNTIME_TASK_ADDRESS_SIZE, MMU_TABLE_FLAG_PRESENT | MMU_TABLE_FLAG_WRITEABLE | MMU_TABLE_FLAG_USERMODE, MMU_PAGE_FLAG_PRESENT | MMU_PAGE_FLAG_WRITEABLE | MMU_PAGE_FLAG_USERMODE);
    mmu_load_memory(directory);

    enable();

}

