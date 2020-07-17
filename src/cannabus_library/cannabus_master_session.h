#pragma once

#include "project_config.h"
#include "cannabus_common.h"
#include "can/i_can.h"
#include "callbacks/callbacks.h"

namespace cannabus
{
    class MasterSession
    {

    public:

        explicit MasterSession( uint32_t answer_timeout_max,
                                uint32_t request_sending_timeout_max = 10,
                                uint32_t repeats_max = 3,
                                uint32_t unicast_timeout_max = 1000 ):
            m_answer_timeout_max( answer_timeout_max ),
            m_request_sending_timeout_max( request_sending_timeout_max ),
            m_repeats_max( repeats_max ),
            m_unicast_timeout_max( unicast_timeout_max )
        { }

        // копировать запрещено
        MasterSession( const MasterSession & rhs ) = delete;
        MasterSession & operator=( MasterSession & s) = delete;

        using OnAnswerReceived = callback::Callback<void ( const can::CanMessage &)>;
        using OnUnexpectedMsgReceived = callback::Callback<void ( const can::CanMessage &, uint32_t slaveAdr )>;


        void init(can::ICan & can,
                  OnAnswerReceived onAnswerReceived,
                  OnUnexpectedMsgReceived onUnexpectedReceived,
                  callback::VoidCallback onConnectionFailure,
                  OnAnswerReceived onIrrelevantAnswerReceived = OnAnswerReceived() );

        void sendRequest( can::CanMessage & request );

        void work(uint32_t curTime);

        bool tryToSendBroadcast( const can::CanMessage & msg,
                                 IdFCode fcode,
                                 Priority priority = Priority::NORMAL );

        bool tryToSendDirectMessage( const can::CanMessage & msg,
                                     IdFCode fcode,
                                     Priority priority = Priority::NORMAL );

        void fillFilters();

    protected:

        STRONG_ENUM(SessionStates, WAITING_REQUEST,
                                   SENDING_REQUEST,
                                   WAITING_FOR_SENDING_COMPLETE,
                                   WAITING_ANSWER,
                                   WAITING_ANSWER_UNICAST );



        bool isAnswerRelevant( const can::CanMessage & answer, const can::CanMessage & request ) const;

        bool isAnswerAdressValid( const uint8_t ansAdr, const uint8_t reqAdr ) const;

        bool isMsgHighPrio( const can::CanMessage & msg ) const;

        bool isWriteRegsRangeValid( const can::CanMessage & answer, const can::CanMessage & request ) const;
        bool isWriteRegsSeriesValid( const can::CanMessage & answer, const can::CanMessage & request ) const;
        bool isReadRegsRangeValid( const can::CanMessage & answer, const can::CanMessage & request ) const;
        bool isReadRegsSeriesValid( const can::CanMessage & answer, const can::CanMessage & request ) const;

        static bool isRequestValid( const can::CanMessage & request )
        {
            if( (uint32_t)getAddressFromId( request.id ) > (uint32_t)IdAddresses::MAX_PERMITTED_ADDRESS )
                return false;

            if( getAddressFromId( request.id ) != (uint32_t)IdAddresses::BROADCAST )
            {
                return true;
            }
            //  бродкасты через свои методы идут
            return false;
        }


        uint32_t m_curCallTime = 0;

        can::ICan * m_can = nullptr;

        can::CanMessage m_requestBasic = {};
        can::CanMessage m_requestBroadcast = {};
        can::CanMessage m_requestDirect = {};
        can::CanMessage * m_actualRequestToSend = nullptr;

        can::CanMessage m_answer = {};

        bool m_isRequestPending = false;
        bool m_isBroadcastPending = false;
        bool m_isDirectPending = false;

        can::CanMessage m_unexpectedSlaveMsg = {};

        uint32_t m_lastRequestTime = 0;

        const uint32_t m_answer_timeout_max = 0;
        const uint32_t m_request_sending_timeout_max = 0;

        uint8_t m_repeats_max = 3;
        const uint32_t m_unicast_timeout_max = 0;

        uint8_t m_repeatsCount = 0;

        SessionStates m_state = SessionStates::WAITING_REQUEST;

        callback::VoidCallback m_onConnectionFailure = {};
        OnAnswerReceived m_onAnswerReceived = {};
        OnAnswerReceived m_onIrrelevantAnswer = {};

        OnUnexpectedMsgReceived m_onUnexpectedReceived = {};

    };

} // namespace cannabus
