#pragma once

#include "cannabus_common.h"
#include "i_cannabus_reg_table.h"
#include "can/i_can.h"
#include "cannabus_request_creator.h"
#include <utility>
#include "umba_array/umba_array.h"
#include "callbacks/callbacks.h"
#include <algorithm>

namespace cannabus
{

    class Slave
    {

    public:


        using UnexpectedMsgHandler = callback::Callback< void (const ::can::CanMessage &) >;
        using DeviceSpecificMsgHandler = callback::Callback< void (::cannabus::IdFCode, const ::can::CanMessage &) >;

        Slave() = default;
        // копировать запрещено
        Slave( const Slave & rhs) = delete;
        Slave & operator=( Slave & s) = delete;

        virtual ~Slave()
        {}

        // что именно с чем синхронизировать - определит потомок
        virtual void synchronize() = 0;

        // таблицу тоже создает потомок
        virtual ICannabusRegTable * getSlaveTable() = 0;

        UnexpectedMsgHandler processUnexpectedMsg = nullptr;

        void init( uint8_t devAddr, uint32_t roUpdateInterval );


        void processAnswer( const can::CanMessage & answer );

        bool tryGetRequest(can::CanMessage & req, uint32_t curTime);

        bool trySendRequestDirectly( const can::CanMessage & msg,
                                     IdFCode fcode,
                                     Priority priority = Priority::NORMAL );

        void setConnectionState(bool state);

        virtual bool isConnected(void) const
        {
            return m_isConnected;
        }

        virtual uint32_t getConnectionFailures(void) const
        {
            return m_connectionFailuresCount;
        }

        uint8_t getAddress(void) const
        {
            return m_slaveAdr;
        }

        DeviceSpecificMsgHandler deviceSpecificFunction = nullptr;

    protected:


        void readRegsRange( const can::CanMessage & answer );
        void readRegsSeries( const can::CanMessage & answer );

        RequestCreator m_reqCreator = RequestCreator();

        uint8_t m_slaveAdr = 0;

        uint32_t m_roUpdateInterval = 500;
        uint32_t m_lastRoUpdateTime = 0;

        bool m_isConnected = false;

        uint16_t m_roPart = 0;

        uint32_t m_connectionFailuresCount = 0;

        uint8_t m_roBeginReg = 0xFF;
        uint8_t m_roEndreg = 0xFF;

        // поля для отправки сообщений без очереди через отдельный метод
        can::CanMessage m_outOfTurnMessage = can::CanMessage();
        bool m_outOfTurnFlag = false;

    private:

        bool tryGetRwRequest(can::CanMessage & req, uint32_t curTime);
        bool tryGetRoRequest(can::CanMessage & req, uint32_t curTime);


        auto getNextRegNumToWrite( uint8_t regNum, uint8_t length ) -> std::pair<uint8_t, bool>
        {
            if( regNum > getSlaveTable()->getRwMaxRegNum() )
            {
                return std::make_pair( 0, false );
            }
            for( uint8_t i = regNum; i <= getSlaveTable()->getRwMaxRegNum(); i += getSlaveTable()->getRegLength(i) )
            {
                // если регистр нужной длины, то надо проверить, обновились ли все его части
                if( ( getSlaveTable()->isRegChanged( i ) ) &&
                    ( getSlaveTable()->getRegLength(i) == length ) )
                {

                    return std::make_pair( i, true );
                }
            }

            return std::make_pair( 0, false );
        }


        // поиск первого ожидающего записи
        auto getFirstReg( uint8_t regNum ) -> std::pair<uint8_t, uint8_t>
        {
            for( uint8_t i = regNum; i <= getSlaveTable()->getRwMaxRegNum(); i++ )
            {
                if( getSlaveTable()->isRegChanged( i ) )
                {
                    return std::make_pair( i, getSlaveTable()->getRegLength(i) );
                }
            }

            return std::make_pair( 0, 0 );
        }
    };

} // namespace cannabus
