#include "Stm0_Scheduler.h"
#include "Bsp.h"
#include "IfxPort.h"
#include "IfxStm.h"


/*********************************************************************************************************************/
/*-------------------------------------------------Macros------------------------------------------------------------*/
/*********************************************************************************************************************/
#define BASE_TICK_IN_MS                      1
#define ISR_PRIORITY_STM0               40
#define STM0                            (&MODULE_STM0)

#define STM0_MAX_TIMERS                  15

/*********************************************************************************************************************/
/*--------------------------------------------------Structure Types--------------------------------------------------*/
/*********************************************************************************************************************/
typedef struct {
    uint32_t tickCount;       // Next time to run (relative to g_swTickCount)
    uint32_t tickDuration;     // Task interval in software ticks (e.g., 1 tick = 1 ms)
    void (*callback)(void);     // Task function pointer
    bool running;               // Used by monitor to detect deadline overrun
    bool active;               // Used by monitor to detect deadline overrun
} Stm0SoftTimer;

Stm0SoftTimer g_Stm0SoftTimer[STM0_MAX_TIMERS];


/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
IfxStm_CompareConfig g_Stm0CompareConfig;                 /* STM configuration structure                      */
Ifx_TickTime g_Stm0TickTime;                                   /* Variable to store the number of ticks to wait    */

/*********************************************************************************************************************/
/*-------------------------------------------------Private variables--------------------------------------------------*/
/*********************************************************************************************************************/
static uint8_t timerId;



/*********************************************************************************************************************/
/*-------------------------------------Interrupt/Tick Handler Implementations----------------------------------------*/
/*********************************************************************************************************************/
/* Macro to define Interrupt Service Routine.
 * This macro makes following definitions:
 * 1) Define linker section as .intvec_tc<vector number>_<interrupt priority>.
 * 2) define compiler specific attribute for the interrupt functions.
 * 3) define the Interrupt service routine as ISR function.
 *
 * IFX_INTERRUPT(isr, vectabNum, priority)
 *  - isr: Name of the ISR function.
 *  - vectabNum: Vector table number.
 *  - priority: Interrupt priority. Refer Usage of Interrupt Macro for more details.
 */
IFX_INTERRUPT(isrStm0_TickHandler, 0, ISR_PRIORITY_STM0);

void isrStm0_TickHandler(void){

    /* Scheduler for next tickBase interval at g_Stm0TickTime */
    IfxStm_increaseCompare(STM0, g_Stm0CompareConfig.comparator, g_Stm0TickTime);



    for (int i = 0; i < STM0_MAX_TIMERS; i++) {

        if (!g_Stm0SoftTimer[i].active) continue;
        if (!g_Stm0SoftTimer[i].running) continue;

        g_Stm0SoftTimer[i].tickCount++;


        if (g_Stm0SoftTimer[i].tickCount >= g_Stm0SoftTimer[i].tickDuration) {

            g_Stm0SoftTimer[i].callback();
            g_Stm0SoftTimer[i].tickCount = 0;

        }
    }
}

/*********************************************************************************************************************/
/*---------------------------------------------Scheduler Implementations---------------------------------------------*/
/*********************************************************************************************************************/

/* Function to initialize the STM */
void initStm0_TickComparator(void)
{
    IfxStm_initCompareConfig(&g_Stm0CompareConfig);           /* Initialize the configuration structure with default values   */

    g_Stm0CompareConfig.triggerPriority = ISR_PRIORITY_STM0;   /* Set the priority of the interrupt                            */
    g_Stm0CompareConfig.typeOfService = IfxSrc_Tos_cpu0;      /* Set the service provider for the interrupts                  */
    g_Stm0CompareConfig.ticks = g_Stm0TickTime;              /* Set the number of ticks after which the timer triggers an interrupt for the first time */

    /* Compare hardware &MODULE_STM0 with g_Stm0CompareConfig */
    IfxStm_initCompare(STM0, &g_Stm0CompareConfig);            /* Initialize the STM with the user configuration*/
}

/* Function to initialize all the peripherals and variables used */
void initStm0_Scheduler(void)
{
    /* Initialize time constant */
    g_Stm0TickTime = IfxStm_getTicksFromMilliseconds(BSP_DEFAULT_TIMER, BASE_TICK_IN_MS);

    initStm0_TickComparator();                                      /* Configure the STM module                                     */
}


/*********************************************************************************************************************/
/*-----------------------------------------Timer Control Implementations---------------------------------------------*/
/*********************************************************************************************************************/

/**
 * @brief Registers a new task with the scheduler.
 * @param callback Function to execute when the task triggers.
 * @param duration_ms Task interval in milliseconds.
 * @return true if task added successfully, false if no slots available.
 */
void Stm0_NewTimer(void (*callback)(void), uint8_t timerId, uint32_t tickDuration) {

        if (!g_Stm0SoftTimer[timerId].active) {
           g_Stm0SoftTimer[timerId].tickCount = 0;
           g_Stm0SoftTimer[timerId].tickDuration = tickDuration;
           g_Stm0SoftTimer[timerId].callback = callback;
           g_Stm0SoftTimer[timerId].active = true;   // Mark task as existing
           g_Stm0SoftTimer[timerId].running = false; // Task is created in stopped state

        }

}

/**
 * @brief Deletes a task from the scheduler.
 * @param callback Function pointer of the task to delete.
 */
void Stm0_DeleteTimer(uint8_t timerId) {

    if (g_Stm0SoftTimer[timerId].active) {
       g_Stm0SoftTimer[timerId].tickCount = 0;
       g_Stm0SoftTimer[timerId].tickDuration = 0;
       g_Stm0SoftTimer[timerId].callback = NULL;
       g_Stm0SoftTimer[timerId].active = false;   // Mark task as existing
       g_Stm0SoftTimer[timerId].running = false; // Task is created in stopped state

    }

}

/**
 * @brief Starts a task that was previously stopped.
 * @param callback Function pointer of the task to start.
 */
void Stm0_StartTimer(uint8_t timerId) {

    if (g_Stm0SoftTimer[timerId].active &&g_Stm0SoftTimer[timerId].tickDuration > 0) {

       g_Stm0SoftTimer[timerId].tickCount = 0;
       g_Stm0SoftTimer[timerId].running = true; // Task is created in stopped state

    }

}


/**
 * @brief Stops a running task.
 * @param callback Function pointer of the task to stop.
 */
void  Stm0_StopTimer(uint8_t timerId) {

    if (g_Stm0SoftTimer[timerId].active &&g_Stm0SoftTimer[timerId].tickDuration > 0) {

       g_Stm0SoftTimer[timerId].tickCount = 0;
       g_Stm0SoftTimer[timerId].running = false; // Task is created in stopped state

    }

}

