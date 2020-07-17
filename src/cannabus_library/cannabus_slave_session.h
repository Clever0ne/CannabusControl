
#pragma once

#include "project_config.h"
#include "callbacks/callbacks.h"
#include "can/i_can.h"
#include "i_cannabus_reg_table.h"
#include "cannabus_common.h"
#include <algorithm>
#include "umba_array/umba_array.h"

namespace cannabus
{    
    class SlaveSession
    {
        public:

            SlaveSession() = default;
            SlaveSession( const SlaveSession & ) = delete;
            void operator=( const SlaveSession & ) = delete;

            using VoidCallback = callback::VoidCallback;
            using NullCallback = callback::NullCallback;

            void init( can::ICan & can,
                      uint8_t slaveAdr,
                      ICannabusRegTable & table,
                      VoidCallback lostLinkHandler = NullCallback(),
                      VoidCallback restoreLinkHandler = NullCallback(),
                      uint32_t lostLinkTimeout = 1000,
                      VoidCallback messageRecievedHandler = NullCallback() )
            {
                m_request_timeout_max = lostLinkTimeout;
                m_can = &can;
                m_slaveAdr = slaveAdr;
                m_table = &table;
                m_onConnectionLost = lostLinkHandler;
                m_onConnectionRestored = restoreLinkHandler;
                m_onMessageReceived = messageRecievedHandler;

                m_can->lock();
                for( auto & handlers : m_deviceSpecificHandlers )
                {
                    handlers = callback::NullCallback();
                }


                UMBA_ASSERT( m_can->isInited() == true );

                m_isInited = true;
            }


            void work(uint32_t curTime);
            
            
            void sendHighPrioMessageSeries( umba::ArrayView<uint8_t> regNum );
            void sendHighPrioMessageRange( uint8_t regNumBegin, uint8_t regNumEnd );
            void sendHighPrioMessageDeviceSpecific( IdFCode fcode, umba::ArrayView<uint8_t> data );

            void setTimeout( uint32_t lostLinkTimeout )
            {
                m_request_timeout_max = lostLinkTimeout;
            }
            
            // на вход принимает принятый запрос и ссылку на массив с ответом, который надо сделать
            // возвращает длину ответа
            using DeviceSpecificHandler = callback::Callback< uint8_t ( umba::ArrayView<uint8_t> request, umba::ArrayView<uint8_t> answer ) >;

            // метод для задания колбека на обработку приема и отправки device-specific сообщений
            void setDeviceSpecific( IdFCode fcode, DeviceSpecificHandler deviceSpecificHandler )
            {
                UMBA_ASSERT( (uint32_t)fcode >= (uint32_t)IdFCode::DEVICE_SPECIFIC1 );
                UMBA_ASSERT( (uint32_t)fcode <= (uint32_t)IdFCode::DEVICE_SPECIFIC4 );

                m_deviceSpecificHandlers[(uint32_t)fcode - (uint32_t)IdFCode::DEVICE_SPECIFIC1] = deviceSpecificHandler;
            }

            // функции для настройки кану фильтров для работы с cannabus
           void fillFilters();

           using HighPrioSlaveCallback = callback::Callback< void ( IdFCode fcode,
                   uint8_t address, umba::ArrayView<const uint8_t> data ) >;

           // для возможности чтения высокоприоритетных сообщений от других слейвов
           void addFilterToAnotherSlave( uint8_t anotherSlaveAddress );

           void setAnotherSlaveMessageHandler( HighPrioSlaveCallback handler )
           {
               m_anotherSlaveMsgHandler = handler;
           }

        private:

            enum class States{WAIT_FOR_REQUEST, SEND_ANSWER};

            void process();
            bool writeRegsRange();
            bool writeRegsSeries();
            bool readRegsRange();
            bool readRegsSeries();
            void generateNack();

            bool handleDeviceSpecific( IdFCode fcode );

            bool isAnswerNeeded() const;
            bool isMsgFromMaster() const;

            bool isSlaveAddressValid() const;

            void sendAnswerForcedly( const can::CanMessage & forcedMsg ) const
            {
                can::ReturnState result = can::ReturnState::ERROR;

                while( result != can::ReturnState::OK )
                {
                    result = m_can->transmitMessage( forcedMsg );
                }
            }

            States m_state = States::WAIT_FOR_REQUEST;

            uint32_t m_request_timeout_max = 0;

            can::ICan * m_can = nullptr;
            uint8_t m_slaveAdr = 0;

            cannabus::ICannabusRegTable * m_table = nullptr;

            bool m_isRequestComposed = false;

            bool m_isConnected = false;
            uint32_t m_lastRequestTimestamp = 0;
            uint32_t m_lastCallTime = 0;

            VoidCallback m_onConnectionLost = NullCallback();
            VoidCallback m_onConnectionRestored = NullCallback();
            VoidCallback m_onMessageReceived = NullCallback();
            
            can::CanMessage m_req = {};
            can::CanMessage m_ans = {};

            umba::Array< DeviceSpecificHandler, device_specific_functions_num > m_deviceSpecificHandlers = {};

            bool m_isInited = false;

            uint32_t m_anotherSlaveNumber = 0;

            HighPrioSlaveCallback m_anotherSlaveMsgHandler{};
    };

} // namespace

