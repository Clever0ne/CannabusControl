#include "cannabus_request_creator.h"
#include <algorithm>

namespace cannabus
{
    void RequestCreator :: createWriteRange( can::CanMessage & request,
                                             uint8_t regNumBegin,
                                             uint8_t regNumEnd,
                                             umba::ArrayView<const uint8_t> values, //-V813
                                             bool isHighPrio )
    {
        //идентификатор собираю
        request.id = makeId( m_deviceAddress, IdFCode::WRITE_REGS_RANGE, msgTypeFromPriority( isHighPrio ) );

        UMBA_ASSERT( regNumEnd >= regNumBegin );

        uint8_t regsNum = regNumEnd - regNumBegin + 1;
        // в сообщении может лежать 6 значений, а диапазон от начала до конца включительно
        UMBA_ASSERT( regsNum <= max_regs_in_range);

        request.length = regsNum + 2;
        request.data[0] = regNumBegin;
        request.data[1] = regNumEnd;

        for( uint8_t i = 0; i < regsNum; i++ )
        {
            request.data[2 + i] = values[i];
        }
    }

    void RequestCreator :: createWriteSeries( can::CanMessage & request,
                                              umba::ArrayView<const Register> series, //-V813
                                              bool isHighPrio )
    {
        //идентификатор собираю
        request.id = makeId( m_deviceAddress, IdFCode::WRITE_REGS_SERIES, msgTypeFromPriority( isHighPrio ) );

        // в cannabus сообщение влезает серия из четырех регистров максимум
        UMBA_ASSERT( series.size() <= max_regs_in_series );

        request.length = series.size() * 2;

        for( uint8_t i=0; i < series.size(); ++i )
        {
            request.data[i * 2] = series[i].num;
            request.data[i * 2 + 1] = series[i].val;
        }
    }

    void RequestCreator :: createReadRange( can::CanMessage & request,
                                            uint8_t regNumBegin,
                                            uint8_t regNumEnd,
                                            bool isHighPrio )
    {
        //идентификатор собираю
        request.id = makeId( m_deviceAddress, IdFCode::READ_REGS_RANGE, msgTypeFromPriority( isHighPrio ) );

        UMBA_ASSERT( regNumEnd >= regNumBegin );


        uint8_t regsNum = regNumEnd - regNumBegin + 1;

        // в сообщении может лежать 6 значений, а диапазон от начала до конца включительно
        UMBA_ASSERT( regsNum <= max_regs_in_range );

        request.length = 2;

        request.data[0] = regNumBegin;
        request.data[1] = regNumEnd;
    }

    void RequestCreator :: createReadSeries( can::CanMessage & request,
                                             umba::ArrayView<const Register> series, //-V813
                                             bool isHighPrio )
    {
        //идентификатор собираю
        request.id = makeId( m_deviceAddress, IdFCode::READ_REGS_SERIES, msgTypeFromPriority( isHighPrio ) );

        // в cannabus сообщение влезает серия из четырех регистров максимум
        UMBA_ASSERT( series.size() <= max_regs_in_series );

        request.length = series.size();

        for( uint8_t i=0; i < series.size(); i++ )
        {
            request.data[i] = series[i].num;
        }

    }

    void RequestCreator :: createDeviceSpecific( can::CanMessage & request,
                                                 umba::ArrayView<const uint8_t> data, //-V813
                                                 IdFCode fcode,
                                                 bool isHighPrio )
    {
        UMBA_ASSERT( data.size() <= max_regs_in_specific );
        UMBA_ASSERT( (uint32_t)fcode >= (uint32_t)IdFCode::DEVICE_SPECIFIC1 );
        UMBA_ASSERT( (uint32_t)fcode <= (uint32_t)IdFCode::DEVICE_SPECIFIC4 );

        request.id = makeId( m_deviceAddress, fcode, msgTypeFromPriority( isHighPrio ) );

        request.length = data.size();

        std::copy( data.begin(), data.end(), request.data );
    }


}
