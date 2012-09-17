#include <memory.h>
#include <vfs.h>
#include <base/base.h>
#include <net/net.h>
#include <arch/x86/apic/apic.h>
#include <arch/x86/io/io.h>
#include <arch/x86/pci/pci.h>
#include <arch/x86/rtl8139/rtl8139.h>

static void poweron(struct rtl8139_driver *self)
{

    io_outb(self->io + RTL8139_REGISTER_CONFIG1, 0x00);

}

static void reset(struct rtl8139_driver *self)
{

    io_outb(self->io + RTL8139_REGISTER_CR, 0x10);

    while (io_inb(self->io + RTL8139_REGISTER_CR) & 0x10);

}

static void enable(struct rtl8139_driver *self)
{

    io_outb(self->io + RTL8139_REGISTER_CR, 0x0C);

}

static void setup_interrupts(struct rtl8139_driver *self, unsigned short flags)
{

    io_outw(self->io + RTL8139_REGISTER_IMR, flags);

}

static void setup_receiver(struct rtl8139_driver *self)
{

    io_outd(self->io + RTL8139_REGISTER_RBSTART, (unsigned int)self->rx);
    io_outd(self->io + RTL8139_REGISTER_RCR, 0x0000000F);

}

static void setup_transmitter(struct rtl8139_driver *self)
{

    io_outd(self->io + RTL8139_REGISTER_TSAD0, (unsigned int)self->tx0);
    io_outd(self->io + RTL8139_REGISTER_TSAD1, (unsigned int)self->tx1);
    io_outd(self->io + RTL8139_REGISTER_TSAD2, (unsigned int)self->tx2);
    io_outd(self->io + RTL8139_REGISTER_TSAD3, (unsigned int)self->tx3);

}

static void handle_irq(struct base_device *device)
{

    struct rtl8139_driver *driver = (struct rtl8139_driver *)device->driver;
    unsigned int status = io_inw(driver->io + RTL8139_REGISTER_ISR);

    if (status & RTL8139_ISR_FLAG_ROK)
        io_outw(driver->io + RTL8139_REGISTER_ISR, RTL8139_ISR_FLAG_ROK);

    if (status & RTL8139_ISR_FLAG_TOK)
        io_outw(driver->io + RTL8139_REGISTER_ISR, RTL8139_ISR_FLAG_TOK);

}

static void start(struct base_driver *self)
{

    struct rtl8139_driver *driver = (struct rtl8139_driver *)self;

    poweron(driver);
    reset(driver);
    setup_interrupts(driver, RTL8139_ISR_FLAG_ROK | RTL8139_ISR_FLAG_TOK);
    setup_receiver(driver);
    setup_transmitter(driver);
    enable(driver);

    driver->interface.mac[0] = io_inb(driver->io + RTL8139_REGISTER_IDR0);
    driver->interface.mac[1] = io_inb(driver->io + RTL8139_REGISTER_IDR1);
    driver->interface.mac[2] = io_inb(driver->io + RTL8139_REGISTER_IDR2);
    driver->interface.mac[3] = io_inb(driver->io + RTL8139_REGISTER_IDR3);
    driver->interface.mac[4] = io_inb(driver->io + RTL8139_REGISTER_IDR4);
    driver->interface.mac[5] = io_inb(driver->io + RTL8139_REGISTER_IDR5);

}

static void attach(struct base_device *device)
{

    struct pci_device *pciDevice = (struct pci_device *)device;
    struct rtl8139_driver *driver = (struct rtl8139_driver *)device->driver;
    unsigned int bar0 = pciDevice->config_ind(pciDevice, PCI_CONFIG_BAR0);
    unsigned int irq = pciDevice->config_inb(pciDevice, PCI_CONFIG_IRQ_LINE);

    driver->io = bar0 & ~1;

    apic_register_routine(irq, device, handle_irq);

}

static unsigned int check(struct base_driver *self, struct base_device *device)
{

    struct pci_device *pciDevice;

    if (device->type != PCI_DEVICE_TYPE)
        return 0;

    pciDevice = (struct pci_device *)device;

    return pciDevice->config_inw(pciDevice, PCI_CONFIG_VENDOR) == RTL8139_PCI_VENDOR && pciDevice->config_inw(pciDevice, PCI_CONFIG_DEVICE) == RTL8139_PCI_DEVICE;

}

static unsigned int read_data(struct net_interface *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct rtl8139_driver *driver = (struct rtl8139_driver *)self->driver;
    unsigned short current = io_inw(driver->io + RTL8139_REGISTER_CAPR) + 0x10;
    struct rtl8139_header *header = (struct rtl8139_header *)(driver->rx + current);

    memory_copy(buffer, (char *)driver->rx + current + 4, header->length);

    current += (header->length + 4 + 3) & ~3;

    io_outw(driver->io + RTL8139_REGISTER_CAPR, current - 0x10);

    return header->length;

}

static unsigned int write_data(struct net_interface *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct rtl8139_driver *driver = (struct rtl8139_driver *)self->driver;
    unsigned int status = (0x3F << 16) | (count & 0x1FFF);

    switch (driver->txp)
    {

        case 0:

            memory_copy(driver->tx0, buffer, count);
            io_outd(driver->io + RTL8139_REGISTER_TSD0, status);

            break;

        case 1:

            memory_copy(driver->tx1, buffer, count);
            io_outd(driver->io + RTL8139_REGISTER_TSD1, status);

            break;

        case 2:

            memory_copy(driver->tx2, buffer, count);
            io_outd(driver->io + RTL8139_REGISTER_TSD2, status);

            break;

        case 3:

            memory_copy(driver->tx3, buffer, count);
            io_outd(driver->io + RTL8139_REGISTER_TSD3, status);

            break;

    }

    driver->txp++;
    driver->txp %= 4;

    return count;

}

void rtl8139_driver_init(struct rtl8139_driver *driver)
{

    memory_clear(driver, sizeof (struct rtl8139_driver));

    base_driver_init(&driver->base, RTL8139_DRIVER_TYPE, "rtl8139", start, check, attach);

    driver->interface.read_data = read_data;
    driver->interface.write_data = write_data;

}

