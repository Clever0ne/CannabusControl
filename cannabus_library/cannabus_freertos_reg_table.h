#pragma once

#include "project_config.h"

#ifdef USE_FREERTOS

#include "cannabus_reg_table.h"


namespace cannabus
{
    template <uint8_t TRoRegMin, uint8_t TRoRegMax,
              uint8_t TRwRegMin, uint8_t TRwRegMax>
    class CannabusFreertosRegTable : public CannabusRegTable < TRoRegMin, TRoRegMax, TRwRegMin, TRwRegMax >
	{
        typedef CannabusRegTable < TRoRegMin, TRoRegMax, TRwRegMin, TRwRegMax > Parent;
    
    public:
        CannabusFreertosRegTable(void) : m_rwRegMutexes()
        {
            m_ownerMutex = osSemaphoreCreateMutex();
        }
        
        bool operator ==( const CannabusFreertosRegTable & that ) const
        {
            //просто сравнение по адресам
            return (&that == this);
        }
        
        void setRegRwSemaphore(uint8_t regNum, SemaphoreHandle_t semaphore)
        {
            // rw-регистр
            if( regNum >= TRwRegMin && regNum <= TRwRegMax )
            {
                uint8_t realNum = regNum - TRwRegMin;

                ICannabusRegTable::Lock lock(this);

                if( m_rwRegMutexes[ realNum ] != NULL )
                {
                    // но там уже есть семафор
                    UMBA_ASSERT_FAIL();
                }

                m_rwRegMutexes[ realNum ] = semaphore;
            }
            // ro-регистр
            else if(regNum >= TRoRegMin && regNum <= TRoRegMax )
            {
                // на РО-регистры нельзя вешать семафор
                UMBA_ASSERT_FAIL();
            }
            else
            {
                // некорректный номер регистра
                UMBA_ASSERT_FAIL();
            }


        }
        
        // просто записать значение регистра
        virtual void setRegVal(uint8_t regNum, uint8_t val)
        {
            Parent::setRegVal(regNum, val);

            giveRegSemaphor(regNum);
        }
        
        // сдвоенные регистры и двухбайтные значения
        virtual void setReg16Val(uint8_t lowRegNum, uint16_t val)
        {
            Parent::setReg16Val(lowRegNum, val);

            {
                ICannabusRegTable::Lock lock(this);

                giveRegSemaphor(lowRegNum);
                giveRegSemaphor(lowRegNum+1);
            }
        }

        // счетверенные регистры и четырехбайтные значения
        virtual void setReg32Val(uint8_t lowRegNum, uint32_t val)
        {
            Parent::setReg32Val(lowRegNum, val);

            {
                ICannabusRegTable::Lock lock(this);

                giveRegSemaphor(lowRegNum);
                giveRegSemaphor(lowRegNum+1);
                giveRegSemaphor(lowRegNum+2);
                giveRegSemaphor(lowRegNum+2);
            }
        }
        
        // локи для всей таблицы
        virtual bool isTableLocked(void)
        {
            // пока не реализовано. Непонятно, зачем вам это?
            UMBA_ASSERT_FAIL();

            return true;
        }

        virtual void lockTable(void)
        {
            if( IS_IN_ISR() )
            {
                // чет это не очень хорошая идея, как по мне
                UMBA_ASSERT_FAIL();
            }
            else
            {
                BaseType_t result = osSemaphoreTake( m_ownerMutex, owner_delay_max / portTICK_PERIOD_MS );

                // ассерт означает, что кто-то не отдал лок на таблицу
                UMBA_ASSERT( result == pdTRUE );
            }
        }

        virtual void unlockTable(void)
        {
            osSemaphoreGive( m_ownerMutex );
        }

    private:

        static const uint32_t owner_delay_max = 1000;

        void giveRegSemaphor(uint8_t regNum)
        {
            // ro-регистр
            if( regNum >= TRoRegMin && regNum <= TRoRegMax )
            {
                return;
            }
            
            uint8_t realNum = regNum - TRwRegMin;
        
            if( m_rwRegMutexes[ realNum ] != NULL )
            {
                // почему в вызовах нужны касты к SemaphoreHandle_t - я без понятия
                // но без кастов Кейл ругается. Ему не нравится, что размер массива - параметр шаблона.
                if( IS_IN_ISR() )
                {                    
                    osSemaphoreGiveFromISR( (SemaphoreHandle_t)m_rwRegMutexes[realNum], (BaseType_t *)NULL );
                }
                else
                {
                    osSemaphoreGive( (SemaphoreHandle_t)m_rwRegMutexes[realNum] );
                }
            }
        }
        
        SemaphoreHandle_t m_rwRegMutexes[ TRwRegMax - TRwRegMin + 1 ];

        SemaphoreHandle_t m_ownerMutex;

    };
}
#endif
