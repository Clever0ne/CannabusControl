

#include "cannabus_slave_session.h"

namespace cannabus
{

    /**************************************************************************************************
    Описание:  Воркер для слейва каннабуса
    Аргументы: Время
    Возврат:   -
    Замечания: -
    **************************************************************************************************/
    void SlaveSession::work( uint32_t curTime )
    {
        UMBA_ASSERT( m_isInited );

        auto call = []( callback::VoidCallback c ){ if(c)c(); };

        switch( m_state )
        {
            case States::WAIT_FOR_REQUEST:
            {

                auto result = m_can->tryToReceive( m_req );
                if( !result )
                {
                    if( curTime - m_lastCallTime <= m_request_timeout_max )
                    {
                        return;
                    }
                    if( ! m_isConnected )
                    {
                        return;
                    }

                    call( m_onConnectionLost );

                    m_isConnected = false;
                }
                else
                {
                    // не для нас пакет, игнорим его
                    if( ( isSlaveAddressValid() == false ) && ( isMsgFromMaster() ) )
                    {
                        return;
                    }

                    m_lastCallTime = curTime;
                    process();

                    call( m_onMessageReceived );

                    //если не бродкаст - шлём ответ
                    if( ( isAnswerNeeded() == true ) && ( isMsgFromMaster() == true ) )
                    {
                        m_state = States::SEND_ANSWER;
                    }

                    // а если связь была потеряна, то находим её
                    if( m_isConnected == false )
                    {
                        m_isConnected = true;

                        call( m_onConnectionRestored );
                    }
                }
                return;
            }
            case States::SEND_ANSWER:
            {
                // когда можно будет посылать, тогда пошлем
                if( m_can->isReadyToTransmit() )
                {
                    // отправка представляет собой просто опускание сообщения в буфер, так что это не должно блокировать работу
                    auto sendState = m_can->transmitMessage( m_ans );

                    if( sendState != can::ReturnState::OK )
                    {
                        UMBA_ASSERT_FAIL();
                    }

                    m_state = States::WAIT_FOR_REQUEST;
                }
                return;
            }
            default:
            {
                UMBA_ASSERT_FAIL();
            }

        }

    }


    /**************************************************************************************************
    Описание:  Отправка высокоприоритетного сообщения чтения серии
    Аргументы: Массив номеров регистров
    Возврат:   -
    Замечания: -
    **************************************************************************************************/
    void SlaveSession::sendHighPrioMessageSeries( const umba::ArrayView<uint8_t> regNum )
    {
        auto cnt = regNum.size();

        can::CanMessage highPrioMsg;

        for( uint8_t i = 0; i < cnt ; i++ )
        {
            highPrioMsg.data[2 * i] = regNum[i];
            highPrioMsg.data[2 * i + 1] = m_table->getRegVal( regNum[i] );
        }
        
        //размер в 2 раза больше количества регистров
        highPrioMsg.length = cnt * 2;

        highPrioMsg.id = makeId( m_slaveAdr, IdFCode::READ_REGS_SERIES, IdMsgTypes::HIGH_PRIO_SLAVE );
        
        //посылать до тех пор, пока не смогу
        sendAnswerForcedly( highPrioMsg );
    }

    /**************************************************************************************************
    Описание:  Отправка высокоприоритетного сообщения чтения диапазона
    Аргументы: Массив номеров регистров
    Возврат:   -
    Замечания: -
    **************************************************************************************************/
    void SlaveSession::sendHighPrioMessageRange( uint8_t regNumBegin, uint8_t regNumEnd )
    {
        auto len = regNumEnd - regNumBegin + 1;

        UMBA_ASSERT( regNumBegin <= regNumEnd );
        UMBA_ASSERT( len <= max_regs_in_range );

        can::CanMessage highPrioMsg;

        highPrioMsg.data[0] = regNumBegin;
        highPrioMsg.data[1] = regNumEnd;

        for( uint8_t i = 0; i < len ; i++ )
        {
            highPrioMsg.data[i + 2] = m_table->getRegVal( regNumBegin + i );
        }

        highPrioMsg.length = len + 2;

        highPrioMsg.id = makeId( m_slaveAdr, IdFCode::READ_REGS_RANGE, IdMsgTypes::HIGH_PRIO_SLAVE );

        //посылать до тех пор, пока не смогу
        sendAnswerForcedly( highPrioMsg );
    }


    /**************************************************************************************************
    Описание:  Отправка высокоприоритетного сообщения device-specific
    Аргументы: Массив номеров регистров
    Возврат:   -
    Замечания: -
    **************************************************************************************************/
    void SlaveSession::sendHighPrioMessageDeviceSpecific( IdFCode fcode, umba::ArrayView<uint8_t> data )
    {
        auto cnt = data.size();
        UMBA_ASSERT( cnt <= max_regs_in_specific );
        UMBA_ASSERT( (uint32_t)fcode >= (uint32_t)IdFCode::DEVICE_SPECIFIC1 );
        UMBA_ASSERT( (uint32_t)fcode < (uint32_t)IdFCode::DEVICE_SPECIFIC4 );

        can::CanMessage highPrioMsg;

        std::copy( data.begin(), data.end(), highPrioMsg.data );

        //размер равен длине пачки байт
        highPrioMsg.length = cnt;

        highPrioMsg.id = makeId( m_slaveAdr, fcode, IdMsgTypes::HIGH_PRIO_SLAVE );

        //посылать до тех пор, пока не смогу
        sendAnswerForcedly( highPrioMsg );
    }


    /**************************************************************************************************
    Описание:  Заполнить can-фильтры для этого слейва
    Аргументы: -
    Возврат:   -
    Замечания: -
    **************************************************************************************************/
    void SlaveSession::fillFilters()
    {
        // бродкаст, уникаст, свой адрес - минимум три фильтра надо
        static constexpr auto min_buffers_to_slave = 3;
        UMBA_ASSERT( m_can->getFilterCapacity() >= min_buffers_to_slave );

        // заполняю один буфер на бродкаст, один на уникаст и остальные на свой адрес
        can::CanFilter filterParams;
        filterParams.filter = makeAddressFilter( IdAddresses::BROADCAST );
        // только сообщения мастера допускаются
        filterParams.mask = (uint32_t)IdAddresses::MASK | (1 << (uint32_t)IdOffsets::MSG_TYPE);

        static constexpr auto broadcast_filter_num      = 0u;
        static constexpr auto unicast_filter_num        = 1u;
        static constexpr auto self_address_filter_num   = 2u;

        m_can->addFilter( filterParams, broadcast_filter_num );

        filterParams.filter = makeAddressFilter( IdAddresses::DIRECT_ACCESS );

        m_can->addFilter( filterParams, unicast_filter_num );

        filterParams.filter = m_slaveAdr << (uint32_t)IdOffsets::ADDRESS;

        for( auto i = self_address_filter_num; i < m_can->getFilterCapacity(); i++ )
        {
            m_can->addFilter( filterParams, i );
        }
    }


    /**************************************************************************************************
    Описание:  Заполнить can-фильтры для высокоприоритеток от другого слейва
    Аргументы: -
    Возврат:   -
    Замечания: -
    **************************************************************************************************/
    void SlaveSession::addFilterToAnotherSlave( uint8_t anotherSlaveAddress )
    {
        // слейв должен быть с валидным адресом
        UMBA_ASSERT( anotherSlaveAddress <= (uint32_t)IdAddresses::MAX_SLAVE_ADDRESS );
        UMBA_ASSERT( anotherSlaveAddress >= (uint32_t)IdAddresses::MIN_SLAVE_ADDRESS );
        // должен быть обработчик сообщений от слейвов, чтобы уметь их принимать
        UMBA_ASSERT( m_anotherSlaveMsgHandler );

        // бродкаст, уникаст, свой адрес - минимум три фильтра надо для собственных нужд
        static constexpr auto min_buffers_to_self = 3u;
        // Номер фильтра отсчитывается от конца
        auto filterNumber = m_can->getFilterCapacity() - m_anotherSlaveNumber - 1;

        UMBA_ASSERT( filterNumber >= min_buffers_to_self );

        m_anotherSlaveNumber++;

        // заполняю один буфер на бродкаст, один на уникаст и остальные на свой адрес
        can::CanFilter filterParams;
        filterParams.filter = makeId( anotherSlaveAddress, ( IdFCode )0, IdMsgTypes::HIGH_PRIO_SLAVE );
        filterParams.mask = (uint32_t)IdAddresses::MASK | (uint32_t)IdMsgTypes::MASK;

        m_can->addFilter( filterParams, filterNumber );
    }
    /**************************************************************************************************
    Описание:  Обработка принятого запроса
    Аргументы: Указатель на запрос, указатель на ответ
    Возврат:   -
    Замечания: -
    **************************************************************************************************/
    void SlaveSession::process()
    {
        auto fCode = getFCodeFromId( m_req.id );

        {
            // проверка на высокоприоритетку от другого слейва
            auto msgType = getMsgTypeFromId( m_req.id );
            if( msgType == IdMsgTypes::HIGH_PRIO_SLAVE )
            {
                auto address = getAddressFromId( m_req.id );
                m_anotherSlaveMsgHandler( fCode, address, umba::ArrayView<uint8_t>(m_req.data, m_req.length) );

                return;
            }
        }

        //если что-то пойдёт не так, то надо будет отвечать сообщением с длиной 0
        m_ans.length = 0;

        auto isRequestValid = false;

        switch ( fCode )
        {
            //стандартные
            case IdFCode::WRITE_REGS_RANGE:
                isRequestValid = writeRegsRange();
                break;
            case IdFCode::WRITE_REGS_SERIES:
                isRequestValid = writeRegsSeries();
                break;
            case IdFCode::READ_REGS_RANGE:
                isRequestValid = readRegsRange();
                break;
            case IdFCode::READ_REGS_SERIES:
                isRequestValid = readRegsSeries();
                break;
            //специальные
            case IdFCode::DEVICE_SPECIFIC1:
            case IdFCode::DEVICE_SPECIFIC2:
            case IdFCode::DEVICE_SPECIFIC3:
            case IdFCode::DEVICE_SPECIFIC4:
                isRequestValid = handleDeviceSpecific( fCode );
                break;

            default:
                UMBA_ASSERT_FAIL();
        }

        if( !isRequestValid )
        {
            // длина должна быть точно нулевой, так как запрос был невалидным
            m_ans.length = 0;
        }

        //вытащил адрес из запроса
        uint8_t addr = getAddressFromId( m_req.id );

        //прямое обращение
        if( addr == (uint32_t)IdAddresses::DIRECT_ACCESS )
        {
            addr = m_slaveAdr;
        }

        //заполнил поле id ответа
        m_ans.id = makeId( addr, fCode, IdMsgTypes::SLAVE );
    }

    /**************************************************************************************************
    Описание:  Функция записи диапазона регистров
    Аргументы: Нет
    Возврат:   Нет
    Замечания:

        Структура обрабатываемого запроса:
        -------------------------------------------------
        | StartRegAdr | EndRegAdr | Data0 | ... | DataN |
        -------------------------------------------------
               0            1         2            2+N

        Структура подготавливаемого ответа:
        ---------------------------
        | StartRegAdr | EndRegAdr |
        ---------------------------
              0             1

    **************************************************************************************************/
    bool SlaveSession::writeRegsRange()
    {
        uint8_t regAdrStart = m_req.data[0];
        uint8_t regAdrEnd = m_req.data[1];
        uint8_t regsTotal = regAdrEnd - regAdrStart + 1;

        if( m_req.length != regsTotal + 2 )
            return false;

        if( regAdrEnd < regAdrStart )
            return false;

        if( regsTotal > max_regs_in_range )
            return false;

        //проверяем, что регистры RW
        for( uint8_t i = 0; i < regsTotal; i++ )
        {            
            if( !m_table->isRegNumRw( regAdrStart + i ) )
                return false;
        }

        for( uint8_t i = 0; i < regsTotal; i++ )
        {
            m_table->setRegVal( regAdrStart + i, m_req.data[2 + i] );
        }
        m_ans.data[0] = regAdrStart;
        m_ans.data[1] = regAdrEnd;
        m_ans.length = 2;

        return true;
    }

    /**************************************************************************************************
    Описание:  Функция записи серии регистров
    Аргументы: Нет
    Возврат:   Нет
    Замечания:

        Структура обрабатываемого запроса:
        --------------------------------------------
        |  RegAdr0 | Data0 | ... | RegAdrN | DataN |
        --------------------------------------------
             0         1             N*2     N*2+1

        Структура подготавливаемого ответа:
        ---------------------------
        | RegAdr0 | ... | RegAdrN |
        ---------------------------
             1               N

    **************************************************************************************************/
    bool SlaveSession::writeRegsSeries()
    {
        if( m_req.length % 2 != 0 )
            return false;

        for( uint8_t i = 0; i < m_req.length; i += 2 )
        {
            if( !m_table->isRegNumRw( m_req.data[i] ) )
                return false;
        }

        uint8_t cnt = 0;
        for( uint8_t i = 0; i < m_req.length; i += 2 )
        {
            m_table->setRegVal( m_req.data[i], m_req.data[i + 1] );
            m_ans.data[cnt] = m_req.data[i];
            cnt++;
        }
        m_ans.length = cnt;

        return true;
    }


    /**************************************************************************************************
    Описание:  Функция чтения диапазона регистров
    Аргументы: Нет
    Возврат:   Нет
    Замечания:

        Структура обрабатываемого запроса:
        ---------------------------
        | StartRegAdr | EndRegAdr |
        ---------------------------
               0           1

        Структура подготавливаемого ответа:
        -------------------------------------------------
        | StartRegAdr | EndRegAdr | Data0 | ... | DataN |
        -------------------------------------------------
               0           1          2            2+N

    **************************************************************************************************/
    bool SlaveSession::readRegsRange()
    {
        uint8_t regAdrStart = m_req.data[0];
        uint8_t regAdrEnd = m_req.data[1];
        uint8_t regsTotal = regAdrEnd - regAdrStart + 1;

        if( m_req.length != 2 )
            return false;

        if( regAdrEnd < regAdrStart )
            return false;

        if( regsTotal > max_regs_in_range )
            return false;


        for( uint8_t i = 0; i < regsTotal; i++ )
        {
            if( !m_table->isRegNumValid( regAdrStart + i ) )
                return false;
        }

        m_ans.data[0] = regAdrStart;
        m_ans.data[1] = regAdrEnd;

        for( uint8_t i = 0; i < regsTotal; i++ )
        {
            m_ans.data[2 + i] = m_table->getRegVal( regAdrStart + i );
        }

        m_ans.length = 2 + regsTotal;

        return true;
    }

    /**************************************************************************************************
    Описание:  Функция чтения серии регистров
    Аргументы: Нет
    Возврат:   Нет
    Замечания:

        Структура обрабатываемого запроса:
        -----------------------------
        | RegAddr0 | ... | RegAddrN |
        -----------------------------
             0                N

        Структура подготавливаемого ответа:
        ----------------------------------------------
        |  RegAddr0 | Data0 | ... | RegAddrN | DataN |
        ----------------------------------------------
              0         1            N*2       N*2+1

    **************************************************************************************************/
    bool SlaveSession::readRegsSeries()
    {
        if( m_req.length > max_regs_in_series )
            return false;

        if( m_req.length <= 0 )
            return false;

        for( uint8_t i = 0; i < m_req.length; i++ )
        {
            if( !m_table->isRegNumValid( m_req.data[i] ) )
                return false;
        }

        uint8_t cnt = 0;
        for( uint8_t i = 0; i < m_req.length; i++ )
        {
            m_ans.data[cnt] = m_req.data[i];
            m_ans.data[cnt + 1] = m_table->getRegVal( m_req.data[i] );
            cnt += 2;
        }
        m_ans.length = cnt;

        return true;
    }


    /**************************************************************************************************
    Описание:  Обработчик device-specific функции
    Аргументы: Номер функции
    Возврат:   Успех\не успех
    Замечания:
    **************************************************************************************************/
    bool SlaveSession::handleDeviceSpecific( IdFCode fcode )
    {
        UMBA_ASSERT( (uint32_t)fcode >= (uint32_t)IdFCode::DEVICE_SPECIFIC1 );
        UMBA_ASSERT( (uint32_t)fcode <= (uint32_t)IdFCode::DEVICE_SPECIFIC4 );


        if( m_req.length > cannabus::max_regs_in_specific )
            return false;
        if( m_req.length == 0 )
            return false;

        // вызвали обработчик
        if( m_deviceSpecificHandlers[ (uint32_t)fcode - (uint32_t)IdFCode::DEVICE_SPECIFIC1 ] )
        {
            umba::ArrayView<uint8_t> req( m_req.data, m_req.length );
            umba::ArrayView<uint8_t> ans( m_ans.data, cannabus::max_regs_in_specific );
            
            auto length = m_deviceSpecificHandlers[ (uint32_t)fcode - (uint32_t)IdFCode::DEVICE_SPECIFIC1 ]( req, ans );

            m_ans.length = length;

            return true;
        }
        else
        {
            return false;
        }
    }

    /**************************************************************************************************
    Описание:  Создание ответа на ошибочный запрос
    Аргументы: Указатель на ответ
    Возврат:   -
    Замечания: -
    **************************************************************************************************/
    void SlaveSession::generateNack()
    {
        m_ans.length = 0;
    }

    /**************************************************************************************************
    Описание:  Проверка, нужен ли ответ на принятый запрос
    Аргументы: ID сообщения-запроса
    Возврат:   True, если придется отвечать
    Замечания: -
    **************************************************************************************************/
    bool SlaveSession::isAnswerNeeded() const
    {
        return ( getAddressFromId( m_req.id ) != (uint32_t)IdAddresses::BROADCAST );
    }

    /**************************************************************************************************
    Описание:  Проверка, от мастера ли запрос
    Аргументы: ID сообщения-запроса
    Возврат:   True, если придется отвечать
    Замечания: -
    **************************************************************************************************/
    bool SlaveSession::isMsgFromMaster() const
    {
        return ( ( getMsgTypeFromId( m_req.id ) == IdMsgTypes::HIGH_PRIO_MASTER ) |
                 ( getMsgTypeFromId( m_req.id ) == IdMsgTypes::MASTER ) );
    }



    /**************************************************************************************************
    Описание:  Проверка, наш ли это запрос
    Аргументы: ID сообщения-запроса
    Возврат:   True, если да
    Замечания: -
    **************************************************************************************************/
    bool SlaveSession::isSlaveAddressValid() const
    {
        auto addr = getAddressFromId( m_req.id );
        auto isAddressMine = ( addr == m_slaveAdr );
        auto isAddressBroadcast = ( addr == (uint32_t)IdAddresses::BROADCAST );
        auto isAddressDirect = ( addr == (uint32_t)IdAddresses::DIRECT_ACCESS );
        return isAddressMine || isAddressBroadcast || isAddressDirect;
    }
}

