pragma once

#include "project_config.h"
#include "Cannabus_cpp/cannabus_slave_session.h"
#include "freertos/i_freertos_task.h"


namespace cannabus
{    
    class FreertosSlaveSession : public IFreeRtosTask
    {
        public:

            FreertosSlaveSession( SlaveSession & slave, uint32_t timeout = 1000) :
                m_slave( slave ),
                m_timeoutTime( timeout )
            {
            }
            
            void setTimeout( uint32_t timeout )
            {
                m_timeoutTime = timeout;
                if( m_timeoutTime == 0 )
                {
                    m_timeoutTime = 0xFFFFFFFF;
                }
            }
            
        private:

            virtual void run( void )
            {
                while(OS_IS_RUNNING)
                {
                    m_slave.work( osTaskGetTickCount() );

                    m_slave.setTimeout( m_timeoutTime );
                }
            }
            
            SlaveSession & m_slave;
            uint32_t m_timeoutTime = 0;
    };

} // namespace

