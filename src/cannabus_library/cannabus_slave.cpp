#include "cannabus_slave.h"

namespace cannabus
{

    void Slave::init( uint8_t devAddr, uint32_t roUpdateInterval )
    {
        m_reqCreator.init(devAddr);

        m_slaveAdr = devAddr;
        m_roUpdateInterval = roUpdateInterval;
    }


    void Slave::processAnswer( const can::CanMessage & answer )
    {
        auto address = getAddressFromId( answer.id );
        UMBA_ASSERT( address == m_slaveAdr );

        auto msgType = getMsgTypeFromId( answer.id );
        UMBA_ASSERT( ( msgType == IdMsgTypes::SLAVE ) || ( msgType == IdMsgTypes::HIGH_PRIO_SLAVE ) );

        auto ansFcode = getFCodeFromId( answer.id );

        switch ( ansFcode )
        {
            //стандартные
            case IdFCode::WRITE_REGS_RANGE: // запись ничего в таблицу не пишет
                break;
            case IdFCode::WRITE_REGS_SERIES: // запись ничего в таблицу не пишет
                break;
            case IdFCode::READ_REGS_RANGE:
                readRegsRange( answer );
                break;
            case IdFCode::READ_REGS_SERIES:
                readRegsSeries( answer );
                break;
            //специальные
            case IdFCode::DEVICE_SPECIFIC1:
            case IdFCode::DEVICE_SPECIFIC2:
            case IdFCode::DEVICE_SPECIFIC3:
            case IdFCode::DEVICE_SPECIFIC4:
                deviceSpecificFunction( ansFcode, answer );
                break;

            default:
                UMBA_ASSERT_FAIL();
        }

    }


    bool Slave::tryGetRequest(can::CanMessage & req, uint32_t curTime)
    {
        // сначала rw проверяем
        if( tryGetRwRequest(req, curTime) )
        {
            return true;
        }
        if( tryGetRoRequest(req, curTime) )
        {
            return true;
        }
        return false;
    }


    bool Slave::trySendRequestDirectly( const can::CanMessage & msg,
                                          IdFCode fcode,
                                          Priority priority )
    {
        if( m_outOfTurnFlag )
        {
            return false;
        }

        m_outOfTurnMessage = msg;

        // генерю id для своего адреса
        auto msgType = ( priority == Priority::HIGH ) ? IdMsgTypes::HIGH_PRIO_MASTER : IdMsgTypes::MASTER;

        m_outOfTurnMessage.id = makeId( m_slaveAdr, fcode, msgType );

        m_outOfTurnFlag = true;

        return true;
    }

    void Slave::setConnectionState(bool state)
    {
        m_isConnected = state;

        if( state == false )
        {
            m_connectionFailuresCount++;
        }
    }

    /**************************************************************************************************
    Описание:  Функция чтения диапазона регистров
    Аргументы: Нет
    Возврат:   Верный ответ или нет
    Замечания:

        Структура запроса:
        ---------------------------
        | StartRegAdr | EndRegAdr |
        ---------------------------
               0           1

        Структура ответа:
        -------------------------------------------------
        | StartRegAdr | EndRegAdr | Data0 | ... | DataN |
        -------------------------------------------------
               0           1          2            2+N

    **************************************************************************************************/
    void Slave::readRegsRange( const can::CanMessage & answer )
    {
        uint8_t regAdrStart = answer.data[0];
        uint8_t regAdrEnd = answer.data[1];
        uint8_t regsTotal = regAdrEnd - regAdrStart + 1;

        for( uint8_t i = 0; i < regsTotal; i++ )
        {
            getSlaveTable()->setRegVal( regAdrStart + i, answer.data[2 + i] );
        }
    }

    /**************************************************************************************************
    Описание:  Функция чтения серии регистров
    Аргументы: Нет
    Возврат:   Верный ответ или нет
    Замечания:

        Структура запроса:
        -----------------------------
        | RegAddr0 | ... | RegAddrN |
        -----------------------------
             0                N

        Структура ответа:
        ----------------------------------------------
        |  RegAddr0 | Data0 | ... | RegAddrN | DataN |
        ----------------------------------------------
              0         1            N*2       N*2+1

    **************************************************************************************************/
    void Slave::readRegsSeries( const can::CanMessage & answer )
    {
        for( uint8_t i = 0; i < answer.length; i += 2 )
        {
            getSlaveTable()->setRegVal( answer.data[i], answer.data[i + 1] );
        }
    }



    /**************************************************************************************************
    Описание:  Пытается собрать rw запрос
    Аргументы: Ссылка на пакет с запросом и время
    Возврат:   Удалось/нет
    Замечания:
    **************************************************************************************************/
    bool Slave::tryGetRwRequest(can::CanMessage & req, uint32_t)
    {
        /// без очереди сообщение улетает первым
        if( m_outOfTurnFlag )
        {
            req = m_outOfTurnMessage;
            m_outOfTurnFlag = false;
            return true;
        }

        // логика такая - какой первый регистр нашли, такого типа пакет и собираем
        umba::Array<RequestCreator::Register, max_regs_in_series> series;
        uint32_t registerLength = 0;


        auto fillReg = [this]( RequestCreator::Register & reg, uint8_t regNum )
            {
                reg.num = regNum;
                reg.val = getSlaveTable()->getRegVal( regNum );
            };

        auto firstSingleReg = getFirstReg( getSlaveTable()->getRwMinRegNum() );

        // ничего не найдено
        if( firstSingleReg.second == 0 )
        {
            return false;
        }

        // если первый оказался однобайтным, то ищу ему однобайтных коллег для пакетирования
        if( firstSingleReg.second == 1 )
        {
            fillReg( series[0], firstSingleReg.first );

            // сброс флага обновления
            getSlaveTable()->checkRegUpdate( firstSingleReg.first );

            registerLength = 1;

            while( registerLength < max_regs_in_series )
            {
                auto prevRegNum = series[ registerLength - 1 ].num;

                auto regNum = getNextRegNumToWrite( prevRegNum + 1, 1 );

                // больше нечего слать
                if( regNum.second == false )
                {
                    break;
                }

                fillReg( series[registerLength], regNum.first );

                // сброс флага обновления
                getSlaveTable()->checkRegUpdate( regNum.first );

                registerLength++;
            }
            // на этот момент пакет собран

            m_reqCreator.createWriteSeries( req, umba::ArrayView<const RequestCreator::Register>( series.data(), registerLength ) );
            return true;
        }

        // если первый оказался двухбайтным, то ищу ему двухбайтных коллег для пакетирования
        if( firstSingleReg.second == 2 )
        {
            fillReg( series[0], firstSingleReg.first );
            fillReg( series[1], firstSingleReg.first + 1 );

            // сброс флага обновления
            getSlaveTable()->checkRegUpdate( firstSingleReg.first );
            getSlaveTable()->checkRegUpdate( firstSingleReg.first + 1 );

            registerLength = 2;

            while( registerLength < max_regs_in_series - 1 )
            {
                auto prevRegNum = series[ registerLength - 1 ].num;

                auto regNum = getNextRegNumToWrite( prevRegNum, 2 );

                // больше нечего слать
                if( regNum.second == false )
                {
                    break;
                }


                fillReg( series[registerLength], regNum.first );
                fillReg( series[registerLength + 1], regNum.first + 1 );

                // сброс флага обновления
                getSlaveTable()->checkRegUpdate( regNum.first );
                getSlaveTable()->checkRegUpdate( regNum.first + 1 );

                registerLength += 2;
            }
            // на этот момент пакет собран

            m_reqCreator.createWriteSeries( req, umba::ArrayView<const RequestCreator::Register>( series.data(), registerLength ) );
            return true;
        }


        // четырехбайтный улетает сразу
        fillReg( series[0], firstSingleReg.first );
        fillReg( series[1], firstSingleReg.first + 1 );
        fillReg( series[2], firstSingleReg.first + 2);
        fillReg( series[3], firstSingleReg.first + 3 );
        registerLength = 4;

        // сброс флага обновления
        getSlaveTable()->checkRegUpdate( firstSingleReg.first + 0 );
        getSlaveTable()->checkRegUpdate( firstSingleReg.first + 1 );
        getSlaveTable()->checkRegUpdate( firstSingleReg.first + 2 );
        getSlaveTable()->checkRegUpdate( firstSingleReg.first + 3 );

        m_reqCreator.createWriteSeries( req, umba::ArrayView<const RequestCreator::Register>( series.data(), registerLength ) );

        return true;
    }

    /**************************************************************************************************
    Описание:  Пытается собрать ro запрос
    Аргументы: Ссылка на пакет с запросом и время
    Возврат:   Удалось/нет
    Замечания:
    **************************************************************************************************/
    bool Slave::tryGetRoRequest(can::CanMessage & req, uint32_t curTime)
    {
        if(curTime - m_lastRoUpdateTime < m_roUpdateInterval)
        {
            return false;
        }

        m_lastRoUpdateTime = curTime;

        // нужно обновить все регистры
        // они могут не влезть в один пакет миллиганджубуса, поэтому их нужно разбить на диапазоны

        // эти переменные разные для разных объектов этого класса
        // поэтому их  нельзя делать static
        const uint16_t min = getSlaveTable()->getRoMinRegNum();
        const uint16_t max = getSlaveTable()->getRoMaxRegNum();

        // типа шагаем диапазонами максимальной длины по всей таблице
        if( m_roEndreg >= max )
            m_roBeginReg = (uint8_t)min;
        else
            m_roBeginReg = m_roEndreg + 1;

        m_roEndreg = m_roBeginReg + max_regs_in_range - 1;

        // uint16 вроде не должен переполниться
        if( m_roEndreg > max)
            m_roEndreg = (uint8_t)max;

        for(uint16_t i=m_roBeginReg; i <= m_roEndreg; i++)
        {
            uint8_t len = getSlaveTable()->getRegLength( (uint8_t)i );

            if( len == 1)
                continue;

            // многобайтный регистр должен помещаться в диапазон целиком
            if( m_roEndreg >= i + len - 1 )
                continue;

            // или не входить туда вообще
            m_roEndreg = i-1;
            break;
        }

        UMBA_ASSERT( m_roBeginReg >= min && m_roBeginReg <= max );
        UMBA_ASSERT( m_roEndreg >= min && m_roEndreg <= max );

        m_reqCreator.createReadRange(req, m_roBeginReg, m_roEndreg);

        return true;
    }


} // namespace cannabus
