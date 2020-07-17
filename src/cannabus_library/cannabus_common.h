#pragma once

#include <stdint.h>
//#include "project_config.h"
//#include "umba_array/umba_array.h"
//#include "can/i_can.h"

namespace cannabus
{
    //параметры каннабуса общие
    enum class Priority{ NORMAL, HIGH };


    // Структура ID сообщения CANNABUS PLUS:
    //  ----------------------------------------------------------------------------------------------
    //  |  Бит 10  |  Бит 9  | Бит 8 | Бит 7 | Бит 6 | Бит 5 | Бит 4 | Бит 3 | Бит 2 | Бит 1 | Бит 0 |
    //  ----------------------------------------------------------------------------------------------
    //  | Код типа сообщения |                   Адрес slave-узла                    |  Код функции  |
    //  ----------------------------------------------------------------------------------------------

    // Смещения полей в ID сообщения
    enum class IdOffsets{ FCODE    = 0,
                          ADDRESS  = 3,
                          MSG_TYPE = 9 };


    enum class IdAddresses{ BROADCAST = 0,
                            MIN_SLAVE_ADDRESS = 1,
                            MAX_SLAVE_ADDRESS = 60,
                            DIRECT_ACCESS = 61,
                            MAX_PERMITTED_ADDRESS = 61,
                            MASK = 0x3F << static_cast<uint32_t>( IdOffsets::ADDRESS ) };

    enum class IdFCode{ WRITE_REGS_RANGE  = 0,
                        WRITE_REGS_SERIES = 1,
                        READ_REGS_RANGE   = 2,
                        READ_REGS_SERIES  = 3,
                        DEVICE_SPECIFIC1 = 4,
                        DEVICE_SPECIFIC2 = 5,
                        DEVICE_SPECIFIC3 = 6,
                        DEVICE_SPECIFIC4 = 7,
                        MASK = 0x07 << static_cast<uint32_t>( IdOffsets::FCODE ) };

    enum class IdMsgTypes { HIGH_PRIO_MASTER = 0,
                            HIGH_PRIO_SLAVE  = 1,
                            MASTER           = 2,
                            SLAVE            = 3,
                            MASK = 0x03 << static_cast<uint32_t>( IdOffsets::MSG_TYPE ) };

    // функции для получения куска ID из полей енумов
    constexpr uint32_t makeAddressFilter( IdAddresses address )
    {
        return static_cast<uint32_t>( address ) <<  static_cast<uint32_t>( IdOffsets::ADDRESS );
    }

    constexpr uint32_t makeFCodeFilter( IdFCode fCode )
    {
        return static_cast<uint32_t>( fCode ) <<  static_cast<uint32_t>( IdOffsets::FCODE );
    }

    constexpr uint32_t makeMsgTypeFilter( IdMsgTypes msgType )
    {
        return static_cast<uint32_t>( msgType ) <<  static_cast<uint32_t>( IdOffsets::MSG_TYPE );
    }


    // функции генерации ID
    constexpr uint32_t makeId( IdAddresses address, IdFCode fCode, IdMsgTypes msgType )
    {
        return makeAddressFilter( address ) | makeFCodeFilter( fCode ) | makeMsgTypeFilter( msgType );
    }

    constexpr uint32_t makeId( uint32_t address, IdFCode fCode, IdMsgTypes msgType )
    {
        return ( address << (uint32_t)IdOffsets::ADDRESS ) | makeFCodeFilter( fCode ) | makeMsgTypeFilter( msgType );
    }

    // функции извлечения данных из ID
    constexpr uint32_t getAddressFromId( uint32_t id )
    {
        return ( ( id & (uint32_t)( IdAddresses::MASK ) ) >> ( uint32_t )IdOffsets::ADDRESS );
    }

    constexpr IdFCode getFCodeFromId( uint32_t id )
    {
        return (IdFCode)( ( id & (uint32_t)( IdFCode::MASK ) ) >> ( uint32_t )IdOffsets::FCODE );
    }

    constexpr IdMsgTypes getMsgTypeFromId( uint32_t id )
    {
        return (IdMsgTypes)( ( id & (uint32_t)( IdMsgTypes::MASK ) ) >> ( uint32_t )IdOffsets::MSG_TYPE );
    }

    // Максимальные количества регистров для функций
    static const uint8_t max_regs_in_range = 6;
    static const uint8_t max_regs_in_series = 4;
    static const uint8_t max_regs_in_specific = 8;

    static constexpr uint8_t device_specific_functions_num = (uint32_t)IdFCode::DEVICE_SPECIFIC4 -
                                                             (uint32_t)IdFCode::DEVICE_SPECIFIC1 + 1;

}

