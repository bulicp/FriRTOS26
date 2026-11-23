#ifndef FRIRTOS_H_
#define FRIRTOS_H_

#define NTASKS 4
#define TASK_STACK_SIZE 256

/*
 * Izbira izvedbe SVC_Handler in PendSV_Handler:
 *
 *   0 - C handlerja v stm32h7xx_it.c z inline makroma __SAVE_CONTEXT()
 *       in __RESTORE_CONTEXT(). Delujeta le "po sreči", odvisno od tega,
 *       katere registre r4-r11 prevajalnik uporabi v prologu/epilogu
 *       (-O0: r7 kot frame pointer -> HardFault v SVC; -Og: r4 v PendSV
 *       se prepiše z r4 prejšnjega opravila).
 *
 *   1 - naked handlerja v FriRTOS.c, v celoti v asemblerju. Prevajalnik
 *       ne doda prologa/epiloga, zato je kontekst r4-r11 pod našim
 *       nadzorom ne glede na nivo optimizacije.
 */
#define FRIRTOS_NAKED_HANDLERS 1

#include "main.h"

// Vsako opravilo bo nekje v RAM-u imelo eno podatkovno strukturo TCB (Task Control Block),
//  ki bo vsebovala za opravilo pomembne informacije, kot so npr. kazalec na sklad opravila 
//  ter naslov funkcije, ki implementira opravilo.
// Podatkovna struktura za Task Control Block:
typedef struct{
	unsigned int* sp;			// nazadnje videni SP opravila
	void (*pTaskFunction)(void);	// naslov funkcije, ki implementira opravilo
} TCB_Type;


typedef struct{
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r12;
	unsigned int lr;
	unsigned int pc;
	unsigned int psr;
} HWSF_Type;


typedef struct{
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
} SWSF_Type;

void TaskInit(TCB_Type* pTCB);
void InitSchedulerSVC(uint32_t* pStackRegion, TCB_Type pTCB[], void (*TaskFunctions[])());
void TaskCreate(TCB_Type* pTCB,
	uint32_t* pTaskStackBase,   // kje je sklad tega opravila
	void (*TaskFunction)());       // kje je koda, ki implementira opravilo
int ContextSwitch(int current_task, TCB_Type pTCB[]);

// Deklaracije opravil, ki jih bo FriRTOS izvajal. 
void Task0 ();
void Task1 ();
void Task2 ();
void Task3 ();






/*
	The inline function saves the context (r4-r11) into the software stack-frame on the process stack.
 	The hardware stack frame already contains the context saved by hardware (pc, lr, sp, r0-r3, r12, spr)

 	This function should be defined into the same translational module in which is use
 	as it is defined as static inline!

 	Pa3cio Bulic, 27.10.2023
*/
__attribute__((always_inline)) static inline void __SAVE_CONTEXT(void)
{
  unsigned int tmp;
  __asm__ volatile
  (
  " MRS   %0, psp                    \n"
  " STMFD %0!, {r4-r11}              \n"
  " MSR   psp, %0                    \n"
  : "=r" (tmp)
  );
}


/*
	The function restores the context (r4-r11) from the software stack-frame on the process stack.

 	This function should be defined into the same translational module in which is use
 	as it is defined as static inline!

 	Pa3cio Bulic, 27.10.2023
*/
__attribute__((always_inline)) static inline void __RESTORE_CONTEXT(void)
{
  unsigned int tmp;
  __asm__ volatile
  (
  " MRS   %0, psp                    \n"
  " LDMFD %0!, {r4-r11}              \n"
  " MSR   psp, %0                    \n"
  : "=r" (tmp)
  );
}

#endif /* FRIRTOS_H_ */
