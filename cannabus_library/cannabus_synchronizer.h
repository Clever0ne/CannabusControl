#pragma once

#include "project_config.h"
#include "cannabus_slave.h"
#include "can/i_can.h"

#include "reg_tables/synchronizer.h"

namespace cannabus
{
    template< size_t TSlavesMax >
    using Synchronizer = ::regs::Synchronizer< TSlavesMax, cannabus::Slave, can::CanMessage >;
    
}

