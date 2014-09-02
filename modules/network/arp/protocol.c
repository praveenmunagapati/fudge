#include <module.h>
#include <kernel/resource.h>
#include <system/system.h>
#include <base/base.h>
#include <base/network.h>
#include "arp.h"

static unsigned int protocol_read(struct base_network_interface *interface, unsigned int offset, unsigned int count, void *buffer)
{

    return 0;

}

static unsigned int protocol_write(struct base_network_interface *interface, unsigned int offset, unsigned int count, void *buffer)
{

    return 0;

}

void arp_initprotocol(struct base_network_protocol *protocol)
{

    base_network_initprotocol(protocol, "arp", protocol_read, protocol_write);

}

