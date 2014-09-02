#include <module.h>
#include <kernel/resource.h>
#include <system/system.h>
#include <base/base.h>
#include <base/network.h>
#include "ipv6.h"

static struct base_network_protocol protocol;

void init()
{

    ipv6_initprotocol(&protocol);
    base_network_registerprotocol(&protocol);

}

void destroy()
{

}

