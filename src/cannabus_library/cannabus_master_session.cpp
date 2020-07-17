#include "cannabus_master_session.h"

namespace cannabus
{

    void MasterSession :: init( can::ICan & can,
                                OnAnswerReceived onAnswerReceived,
                                OnUnexpectedMsgReceived onUnexpectedReceived,
                                callback::VoidCallback onConnectionFailure,
                                OnAnswerReceived onIrrelevantAnswerReceived )
    {
        m_can = &can;

        // порт наш навечно
        //UMBA_ASSERT( ! m_can->isLocked() );
        m_can->lock();

        // для тестов
        m_state = SessionStates::WAITING_REQUEST;

        m_onAnswerReceived = onAnswerReceived;
        m_onUnexpectedReceived = onUnexpectedReceived;
        m_onConnectionFailure = onConnectionFailure;
        m_onIrrelevantAnswer = onIrrelevantAnswerReceived;
    }

    void MasterSession :: sendRequest( can::CanMessage & request ) //-V2009
    {
        // проверка на соответствие запросов формальным требованиям спецификации cannabus plus
        UMBA_ASSERT( isRequestValid( request ) );

        // нельзя посылать бродкаст и уникаст через этот метод
        UMBA_ASSERT( getAddressFromId( request.id ) != (uint32_t)IdAddresses::BROADCAST );
        UMBA_ASSERT( getAddressFromId( request.id ) != (uint32_t)IdAddresses::DIRECT_ACCESS );

        m_isRequestPending = true;
        // реквест копируется
        m_requestBasic = request;
    }


    void MasterSession :: work( uint32_t curTime )
    {
        UMBA_ASSERT(m_can);

        m_curCallTime = curTime;

        auto receiveHighPrio = [this] ()-> void
        {
            if( m_can->tryToReceive( m_unexpectedSlaveMsg ) == false )
            {
                return;
            }
            if( isMsgHighPrio( m_unexpectedSlaveMsg ) )
            {
                m_onUnexpectedReceived( m_unexpectedSlaveMsg, getAddressFromId( m_unexpectedSlaveMsg.id ) );
            }
            else
            {
                m_onIrrelevantAnswer( m_unexpectedSlaveMsg );
            }
        };

        // может придти высокоприоритетное сообщение
        switch( m_state )
        {
            case SessionStates::WAITING_ANSWER:
            case SessionStates::WAITING_ANSWER_UNICAST:
                break;

            default:
                receiveHighPrio();
                break;
        }

        switch( m_state )
        {

        case SessionStates::WAITING_REQUEST:

            // а еще у мастера может висеть неотправленный бродкаст или уникаст
            if( m_isBroadcastPending )
            {
                m_actualRequestToSend = &m_requestBroadcast;
                m_isBroadcastPending = false;
                m_state = SessionStates::SENDING_REQUEST;
                break;
            }
            if( m_isDirectPending )
            {
                m_actualRequestToSend = &m_requestDirect;
                m_isDirectPending = false;
                m_state = SessionStates::SENDING_REQUEST;
                break;
            }
            if( m_isRequestPending )
            {
                m_actualRequestToSend = &m_requestBasic;
                m_isRequestPending = false;
                m_state = SessionStates::SENDING_REQUEST;
                break;
            }

            break;


        case SessionStates::SENDING_REQUEST:
        {

            if( m_can->isReadyToTransmit() == false )
            {
                break;
            }
            auto sendState = m_can->transmitMessage( *m_actualRequestToSend );

            // если не отправилось, то следующим разом попробуем еще
            if( sendState != can::ReturnState::OK )
            {
                break;
            }

            // если direct, то надо ждать ответов по-особенному
            if( getAddressFromId( m_actualRequestToSend->id ) == (uint32_t)IdAddresses::DIRECT_ACCESS )
            {
                m_state = SessionStates::WAITING_ANSWER_UNICAST;
                m_lastRequestTime = m_curCallTime;
                break;
            }
            // если бродкаст, то ответов ждать не надо
            if( getAddressFromId( m_actualRequestToSend->id ) == (uint32_t)IdAddresses::BROADCAST )
            {
                m_state = SessionStates::WAITING_REQUEST;
                break;
            }

            m_lastRequestTime = m_curCallTime;
            m_state = SessionStates::WAITING_ANSWER;

            break;
        }

        case SessionStates::WAITING_ANSWER:

            if( m_can->tryToReceive( m_answer ) == false )
            {
                // не слишком ли долго мы ждем
                if( m_curCallTime - m_lastRequestTime < m_answer_timeout_max )
                {
                    break;
                }

                // повторы не помогают - связь потеряна
                if( m_repeatsCount >= m_repeats_max )
                {
                    m_repeatsCount = 0;

                    m_onConnectionFailure();

                    // дальше можно слать новое сообщение
                    m_state = SessionStates::WAITING_REQUEST;

                    break;
                }

                // попробуем повторить текущее сообщение еще раз
                m_repeatsCount++;

                m_state = SessionStates::SENDING_REQUEST;


                // если ничего не пришло, то ничего и делать не надо)
                break;
            }
            // проверяем, то ли это или не то

            
            // может быть, пришло высокоприоритетное сообщение вместо того, что мы ожидали?
            if( isMsgHighPrio( m_answer) )
            {
                m_onUnexpectedReceived( m_answer, getAddressFromId( m_answer.id ) );

                // ждать мы при этом не перестаем, нормальный ответ все еще должен придти
                break;
            }
            
            // что-то не то
            if( ! isAnswerRelevant( m_answer, m_requestBasic ) )
            {
                m_onIrrelevantAnswer(m_answer);
                break;
            }
            // то, что надо
            else
            {
                m_onAnswerReceived(m_answer);
            }


            m_repeatsCount = 0;
            m_state = SessionStates::WAITING_REQUEST;

            break;

        case SessionStates::WAITING_ANSWER_UNICAST:


            if( m_can->tryToReceive( m_answer ) == false )
            {
                // не слишком ли долго мы ждем
                if( m_curCallTime - m_lastRequestTime >= m_unicast_timeout_max )
                {
                    // дальше можно слать новое сообщение, таймаут уникаста вышел
                    m_state = SessionStates::WAITING_REQUEST;

                    break;
                }

                break;
            }
            // проверяем, то ли это или не то

            // может быть, пришло высокоприоритетное сообщение вместо того, что мы ожидали?
            if( isMsgHighPrio( m_answer ) )
            {
                m_onUnexpectedReceived( m_answer, getAddressFromId( m_answer.id ) );

                // ждать мы при этом не перестаем, нормальный ответ все еще должен придти
                break;
            }

            // что-то не то
            if( ! isAnswerRelevant( m_answer, m_requestDirect ) )
            {
                m_onIrrelevantAnswer(m_answer);
                break;
            }
            // то, что надо
            else
            {
                // ответы на прямое обращение приходят через unexpected, чтобы синхронизатор не догадался
                m_onUnexpectedReceived( m_answer, getAddressFromId( m_answer.id ) );
            }

            break;



        default:
PRAGMA_SUPPRESS_STATEMENT_UNREACHABLE_BEGIN
            UMBA_ASSERT_FAIL();
            break;
PRAGMA_END
        }
    }
    bool MasterSession::tryToSendBroadcast(   const can::CanMessage & msg,
                                              IdFCode fcode,
                                              Priority priority )
    {
        // если уже ждем отправку, то больше не можем принять
        if( m_isBroadcastPending )
        {
            return false;
        }

        m_requestBroadcast.length = msg.length;
        std::copy( msg.data, msg.data + msg.length, m_requestBroadcast.data );
        m_requestBroadcast.frameFormat = can::FrameFormat::STANDART;
        m_requestBroadcast.type = can::MsgType::DATA;

        // генерю id для бродкаста
        if( priority == cannabus::Priority::HIGH )
        {
            m_requestBroadcast.id = makeId( IdAddresses::BROADCAST, fcode, IdMsgTypes::HIGH_PRIO_MASTER );
        }
        else
        {
            m_requestBroadcast.id = makeId( IdAddresses::BROADCAST, fcode, IdMsgTypes::MASTER );
        }

        m_isBroadcastPending = true;

        return true;
    }

    bool MasterSession::tryToSendDirectMessage( const can::CanMessage & msg,
                                                IdFCode fcode,
                                                Priority priority )
    {
        // если уже ждем отправку, то больше не можем принять
        if( m_isDirectPending )
        {
            return false;
        }

        m_requestDirect.length = msg.length;
        std::copy( msg.data, msg.data + msg.length, m_requestDirect.data );
        m_requestDirect.frameFormat = can::FrameFormat::STANDART;
        m_requestDirect.type = can::MsgType::DATA;

        // генерю id для бродкаста
        if( priority == cannabus::Priority::HIGH )
        {
            m_requestDirect.id = makeId( IdAddresses::DIRECT_ACCESS, fcode, IdMsgTypes::HIGH_PRIO_MASTER );
        }
        else
        {
            m_requestDirect.id = makeId( IdAddresses::DIRECT_ACCESS, fcode, IdMsgTypes::MASTER );
        }

        m_isDirectPending = true;

        return true;
   }

    // заполнить can фильтры
    void MasterSession::fillFilters()
    {
        // ответы от любых адресов принимаются, было бы куда класть
        static constexpr auto min_buffers_to_master = 1;
        UMBA_ASSERT( m_can->getFilterCapacity() >= min_buffers_to_master );

        can::CanFilter filterParams;
        filterParams.filter = 1 << (uint32_t)IdOffsets::MSG_TYPE; // сообщение должно быть только от слейва
        filterParams.mask = 1 << (uint32_t)IdOffsets::MSG_TYPE;


        for( decltype( m_can->getFilterCapacity() ) i = 0; i < m_can->getFilterCapacity(); i++ )
        {
            m_can->addFilter( filterParams, i );
        }
    }

    // высокоприоритетные сообщения могут приходить без запросов, сами по себе
    bool MasterSession::isMsgHighPrio( const can::CanMessage & msg ) const
    {
        return getMsgTypeFromId( msg.id ) == IdMsgTypes::HIGH_PRIO_SLAVE;
    }

    bool MasterSession :: isAnswerAdressValid( const uint8_t ansAdr, const uint8_t reqAdr ) const
    {
        if( reqAdr == (uint32_t)IdAddresses::DIRECT_ACCESS )
        {
            return true;
        }

        // на броадкаст не должно быть ответа
        if( reqAdr == (uint32_t)IdAddresses::BROADCAST )
        {
            UMBA_ASSERT_FAIL();
        }

        if( reqAdr == ansAdr )
            return true;
        else
            return false;
    }

    bool MasterSession :: isAnswerRelevant( const can::CanMessage & answer, const can::CanMessage & request ) const
    {
        // адрес должен быть правильным
        if( ! isAnswerAdressValid( getAddressFromId( answer.id ), getAddressFromId( request.id ) ) )
            return false;

        // ф-код должен совпадать
        auto reqFcode = getFCodeFromId( request.id );
        auto ansFcode = getFCodeFromId( answer.id );

        if( reqFcode != ansFcode )
            return false;

        // ни разу не валидный ответ об ошибке
        if( answer.length == 0 )
        {
PRAGMA_SUPPRESS_STATEMENT_UNREACHABLE_BEGIN
            UMBA_ASSERT_FAIL();
            return false;
PRAGMA_END
        }

        bool result = false;

        switch ( reqFcode )
        {
            //стандартные
            case IdFCode::WRITE_REGS_RANGE:
                result = isWriteRegsRangeValid( answer, request );
                break;
            case IdFCode::WRITE_REGS_SERIES:
                result = isWriteRegsSeriesValid( answer, request );
                break;
            case IdFCode::READ_REGS_RANGE:
                result = isReadRegsRangeValid( answer, request );
                break;
            case IdFCode::READ_REGS_SERIES:
                result = isReadRegsSeriesValid( answer, request );
                break;
            //специальные функции могут содержать любые данные, поэтому они всегда валидны, если номер верный
            case IdFCode::DEVICE_SPECIFIC1:
            case IdFCode::DEVICE_SPECIFIC2:
            case IdFCode::DEVICE_SPECIFIC3:
            case IdFCode::DEVICE_SPECIFIC4:
                result = true;
                break;

            default:
                UMBA_ASSERT_FAIL();
        }

        return result;
    }

    /**************************************************************************************************
    Описание:  Функция записи диапазона регистров
    Аргументы: Нет
    Возврат:   Верный ответ или нет
    Замечания:

        Структура запроса:
        -------------------------------------------------
        | StartRegAdr | EndRegAdr | Data0 | ... | DataN |
        -------------------------------------------------
               0            1         2            2+N

        Структура ответа:
        ---------------------------
        | StartRegAdr | EndRegAdr |
        ---------------------------
              0             1

    **************************************************************************************************/
    bool MasterSession::isWriteRegsRangeValid( const can::CanMessage & answer, const can::CanMessage & request ) const
    {
        //если длина не подходят, то ответ кривой
        if( answer.length != 2 )
            return false;

        if( request.data[0] != answer.data[0] )
            return false;

        if( request.data[1] != answer.data[1] )
            return false;

        return true;
    }

    /**************************************************************************************************
    Описание:  Функция записи серии регистров
    Аргументы: Нет
    Возврат:   Верный ответ или нет
    Замечания:

        Структура запроса:
        --------------------------------------------
        |  RegAdr0 | Data0 | ... | RegAdrN | DataN |
        --------------------------------------------
             0         1             N*2     N*2+1

        Структура ответа:
        ---------------------------
        | RegAdr0 | ... | RegAdrN |
        ---------------------------
             1               N

    **************************************************************************************************/
    bool MasterSession::isWriteRegsSeriesValid( const can::CanMessage & answer, const can::CanMessage & request ) const
    {
        //если длины не подходят, то ответ кривой
        if( request.length  != ( answer.length * 2 ) )
            return false;

        //если номера регистров разные, то ответ кривой
        for( uint8_t i = 0; i < request.length / 2; i++ )
        {
            if( request.data[i * 2] != answer.data[i] )
                return false;
        }

        return true;
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
    bool MasterSession::isReadRegsRangeValid( const can::CanMessage & answer, const can::CanMessage & request ) const
    {
        int16_t delta = request.data[1] - request.data[0] + 1;

        // в ответе должен быть начало диапазона, конец диапазона + данные
        if( answer.length != ( request.length + delta ) )
            return false;

        if( request.data[0] != answer.data[0] )
            return false;

        if( request.data[1] != answer.data[1] )
            return false;

        return true;
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
    bool MasterSession::isReadRegsSeriesValid( const can::CanMessage & answer, const can::CanMessage & request ) const
    {
        //если длины не подходят, то ответ кривой
        if( ( request.length * 2 )  != answer.length )
            return false;

        //если номера регистров разные, то ответ кривой
        for( uint8_t i = 0; i < request.length; i++ )
        {
            if( request.data[i] != answer.data[i * 2] )
                return false;
        }

        return true;
    }

} // namespace cannabus
