#include <lib/memory.h>
#include <kernel/log.h>
#include <arch/x86/mboot.h>
#include <arch/x86/vbe.h>

void mboot_init(struct mboot_info *info)
{

    log_string("Multiboot information\n");
    log_string("=====================\n");

    if (info->flags & MBOOT_FLAG_LOADER)
    {

        log_string("Name: ");
        log_string((char *)info->name);
        log_string("\n");


    }

    log_string("\n");

    if (info->flags & MBOOT_FLAG_MEMORY)
    {

        log_string("Memory\n");
        log_string("------\n");

        log_string("Lower memory: ");
        log_num(info->memoryLower, 10);
        log_string("KiB\n");

        log_string("Upper memory: ");
        log_num(info->memoryUpper, 10);
        log_string("KiB\n");

        log_string("\n");

    }

    if (info->flags & MBOOT_FLAG_DEVICE)
    {

        log_string("Boot device\n");
        log_string("-----------\n");

        unsigned int deviceNumber = info->device >> 24;

        log_string("Id: 0x");
        log_num(deviceNumber, 16);
        log_string("\n");

        switch (deviceNumber)
        {

            case 0xE0:

                log_string("Type: CD\n");

                break;

            case 0x00:

                log_string("Type: Floppy Disk\n");

                break;

            case 0x80:

                log_string("Type: Hard Drive\n");

                break;

            default:

                log_string("Type: Unknown\n");

                break;

        }

        log_string("\n");

    }

    log_string("Kernel\n");
    log_string("------\n");

    if (info->flags & MBOOT_FLAG_CMDLINE)
    {

        log_string("Arguments: ");
        log_string((char *)info->cmdline);
        log_string("\n");

    }

    if (info->flags & MBOOT_FLAG_AOUT)
    {

        log_string("Format: AOUT\n");

    }

    if (info->flags & MBOOT_FLAG_ELF)
    {

        log_string("Format: ELF\n");

    }

    log_string("\n");

    if (info->flags & MBOOT_FLAG_MODULES)
    {

        log_string("Modules\n");
        log_string("-------\n");

        //struct mboot_module *module = (struct mboot_module *)info->modulesAddresses;

        //void *args[] = {(char *)module->name, &module->base, &module->length, &module->reserved};
        //log_message(LOG_TYPE_INFO, "Modules: %s Base: 0x%x Length: 0x%x Reserved:%d\n", args);

        log_string("\n");
    }

    if (info->flags & MBOOT_FLAG_MMAP)
    {

        log_string("Memory map\n");
        log_string("----------\n");

        struct mboot_mmap *mmap = (struct mboot_mmap *)info->mmapAddress;
/*
        log_message(LOG_TYPE_INFO, "Memory map\n", 0);

        while (mmap < info->mmapAddress + info->mmapLength)
        {

            void *args[] = {&mmap->baseHigh, &mmap->baseLow};

            if (mmap->type == 1)
                log_message(LOG_TYPE_INFO, "Available: BaseHigh: 0x%x BaseLow: 0x%x\n", args);
            else
                log_message(LOG_TYPE_INFO, "Reserved: BaseHigh: 0x%x BaseLow: 0x%x\n", args);

            mmap = (struct mboot_mmap *)((unsigned int)mmap + mmap->size + sizeof (unsigned int));

        }
*/

        log_string("\n");

    }

    if (info->flags & MBOOT_FLAG_VBE)
    {

        log_string("VESA BIOS Extension\n");
        log_string("-------------------\n");

        log_string("Controller info address: 0x");
        log_num(info->vbeControllerInfo, 16);
        log_string("\n");

        if (info->vbeControllerInfo)
        {

            struct vbe_controller_info *controller = (struct vbe_controller_info *)info->vbeControllerInfo;

            log_string("Signature: 0x");
            log_num(controller->version, 16);
            log_string("\n");

            char text[5];
        
            memory_set(text, 0, 5);
            memory_copy(text, controller->signature, 4);

            log_string(text);

        }

        log_string("Mode info address: 0x");
        log_num(info->vbeModeInfo, 16);
        log_string("\n");

        log_string("Mode: 0x");
        log_num(info->vbeMode, 16);
        log_string("\n");

        log_string("Interface Segment: 0x");
        log_num(info->vbeInterfaceSegment, 16);
        log_string("\n");

        log_string("Interface Offset: 0x");
        log_num(info->vbeInterfaceOffset, 16);
        log_string("\n");

        log_string("Interface Length: 0x");
        log_num(info->vbeInterfaceLength, 16);
        log_string("\n");

        log_string("\n");

    }

}
