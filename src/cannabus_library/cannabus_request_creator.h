#pragma once

#include <stdint.h>

#include "project_config.h"
#include "cannabus_common.h"
#include "can/i_can.h"
#include "umba_array/umba_array.h"

namespace cannabus
{

    class RequestCreator
    {

    public:

        void init( uint8_t deviceAdr )
        {
            UMBA_ASSERT( deviceAdr <= (uint32_t)IdAddresses::MAX_PERMITTED_ADDRESS );

            m_deviceAddress = deviceAdr;
        }

        struct Register
        {
            uint8_t num = 0;
            uint8_t val = 0;
        };

        void createWriteRange( can::CanMessage & request,
                               uint8_t regNumBegin,
                               uint8_t regNumEnd,
                               umba::ArrayView<const uint8_t> values,
                               bool isHighPrio = false );

        void createWriteSeries( can::CanMessage & request, umba::ArrayView<const Register> series, bool isHighPrio = false );

        void createReadRange( can::CanMessage & request, uint8_t regNumBegin, uint8_t regNumEnd, bool isHighPrio = false );

        void createReadSeries( can::CanMessage & request, umba::ArrayView<const Register> series, bool isHighPrio = false );

        void createDeviceSpecific( can::CanMessage & request, umba::ArrayView<const uint8_t> data, IdFCode fcode, bool isHighPrio = false );

    protected:

        static IdMsgTypes msgTypeFromPriority( bool isHighPrio )
        {
            return isHighPrio ? IdMsgTypes::HIGH_PRIO_MASTER : IdMsgTypes::MASTER;
        }

        uint8_t m_deviceAddress = 0;

    };

}
