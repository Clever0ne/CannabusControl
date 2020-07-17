#pragma once

#include "reg_tables/not_type_safe_reg_table.h"

namespace cannabus
{
    template < uint8_t TRoRegMin, uint8_t TRoRegMax,
               uint8_t TRwRegMin, uint8_t TRwRegMax>
    using CannabusRegTable = ::regs::NotTypeSafeRegTable<TRoRegMin, TRoRegMax, TRwRegMin, TRwRegMax>;
}

