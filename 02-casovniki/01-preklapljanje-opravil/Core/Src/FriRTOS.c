#include "FriRTOS.h"
#include <stdint.h>


uint32_t stackRegion[NTASKS * TASK_STACK_SIZE];
TCB_Type TCB[NTASKS];

volatile int current_task = -1;




/*
	The function initializes a task. Initializin a task involves:

	1. populating the initial HW stack frame for the task. HW stack frame initially resides
	   at the bottom of the task's stack. The HW stack frame is populated as follows:
	   PSR = 0x01000000 - this is the default reset value in the state register
	   PC = address of the task
	   LR = the address of the delete_task() function (if needed). In our case tasks never finish so LR=0xFFFFFFFF (reset value)
	   r12, r3-r0 = 0x00000000 - we may also pass the arguments into the tas via r0-r3, but this is not
	   	the case in our simple RTOS
	3. making room for the initial uninitialized SW stack frame.
	4. setting the task's stack pointer in the TCB to point at the top of the task's SW stack frame

	After initialization a new task is ready to be executed for the first time when task switch occurs and
	the task is selected for execution.

 	Pa3cio Bulic, 25.10.2023
*/

void TaskInit(TCB_Type* pTCB){
	HWSF_Type* pHWStackFrame;
	SWSF_Type* pSWStackFrame;


	// Set pointers to HWSF and SWSF:
	pHWStackFrame = (HWSF_Type*)((void*)pTCB->sp - sizeof(HWSF_Type));
	pSWStackFrame = (SWSF_Type*)((void*)pHWStackFrame - sizeof(SWSF_Type));


	// populate HW Stack Frame
	// The values in the general purpose registers are
	// chosen to make debugging easier
	pHWStackFrame->r0 	= 0;                          
	pHWStackFrame->r1	= 0x01010101;               
	pHWStackFrame->r2 	= 0x02020202;
	pHWStackFrame->r3 	= 0x03030303;
	pHWStackFrame->r12 	= 0x0C0C0C0C;
	pHWStackFrame->lr 	= 0xFFFFFFFF;	// (reset val - task never exits)
	pHWStackFrame->pc 	= (unsigned int) (pTCB->pTaskFunction);
	pHWStackFrame->psr 	= 0x01000000; 	// Set T bit (bit 24) in EPSR. The Cortex-M4 processor only
	 	 	 	 	 	 	 	 	 	// supports execution of instructions in Thumb state.
	 	 	 	 	 	 	 	 	 	// Attempting to execute instructions when the T bit is 0 (Debug state)
										// 		results in a fault.
	// napolni SW Stack Frame
	pSWStackFrame->r4	= 0x04040404;
	pSWStackFrame->r5	= 0x05050505;
	pSWStackFrame->r6	= 0x06060606;
	pSWStackFrame->r7	= 0x07070707;
	pSWStackFrame->r8	= 0x08080808;
	pSWStackFrame->r9	= 0x09090909;
	pSWStackFrame->r10	= 0x0a0a0a0a;
	pSWStackFrame->r11	= 0x0b0b0b0b;

	// Set task's stack pointer in the TCB to point at the top of the task's SW stack frame
	pTCB->sp 			= (unsigned int*) pSWStackFrame;
}




/*
	The function initializes the scheduler which will be started in by SVC.
	Our scheduler is very simple and has fixed number of tasks that are created
	at the start of scheduler.
	The tasks never stop and the number of the running task never changes.

 	Pa3cio Bulic, 25.10.2023
*/

void InitSchedulerSVC(uint32_t* pStackRegion, TCB_Type pTCB[], void (*TaskFunctions[])()){
	uint32_t* pTaskStackBase;

	// 1. create all tasks:  (inicializiraj TCB vsakega opravila)
	for(int i=0; i<NTASKS; i++){
		pTaskStackBase = pStackRegion + (i+1)*TASK_STACK_SIZE;
		TaskCreate(&pTCB[i], pTaskStackBase, TaskFunctions[i]);
	}


	// 2. initialize all tasks
	//    The main() and will be first interrupted by SVC.
	//    Task0 will be entered from SVC Handler
	for(int i=0; i<NTASKS; i++){
		TaskInit(&pTCB[i]);  // inicializiraj sklad opravila, da bo pripravljen za zagon
	}

	// set PSP to Task0.SP:
	__set_PSP((unsigned int)pTCB[0].sp);

}



/*
	The function creates a new task. Creating a new task involves:

	1. making space for the task's stack. Each task gets 1kB for the stack
	2. sets the tasks stack pointer in the task table to point at the top of the SW stack frame

	After this steps a new task is ready to be initialized.

 	Pa3cio Bulic, 25.10.2023
*/

void TaskCreate(TCB_Type* pTCB,
	uint32_t* pTaskStackBase,   // kje je sklad tega opravila
	void (*TaskFunction)()){        // kje je koda, ki implementira opravilo

	pTCB->sp 			= (unsigned int*) pTaskStackBase;
	pTCB->pTaskFunction = TaskFunction;
}


/*
	The function switches the context from the current task to the next task.
	It saves the current task's stack pointer and sets the stack pointer to the next task.

*/	
int ContextSwitch(int current_task, TCB_Type pTCB[]){

	volatile int new_task;


	pTCB[current_task].sp = (unsigned int*) __get_PSP();

	// select next task in round-robin fashion
	new_task = current_task + 1;
	if (new_task == NTASKS) new_task = 0;


	__set_PSP((unsigned int)pTCB[new_task].sp);


	return new_task;
}



// Opravila - se ne smejo nikoli končati in nikoli poklicati - 
// Opravilo bo FriRTOS-u vidno le kot zapis 8 registrov na skladu,
// ki jih bo obnovil, ko bo želel zagnati posamezno opravilo

void Task0 (){

    // Opravilo se ne sme nikoli zaključiti
    while(1){
        // fukcionalnost opravila
        __NOP();
        __NOP();
        __NOP();
    }
}

void Task1 (){

    // Opravilo se ne sme nikoli zaključiti
    while(1){
        // fukcionalnost opravila
        __NOP();
        __NOP();
        __NOP();
    }
}

void Task2 (){

    // Opravilo se ne sme nikoli zaključiti
    while(1){
        // fukcionalnost opravila
        __NOP();
        __NOP();
        __NOP();
    }
}

void Task3 (){

    // Opravilo se ne sme nikoli zaključiti
    while(1){
        // fukcionalnost opravila
        __NOP();
        __NOP();
        __NOP();
    }
}


#if FRIRTOS_NAKED_HANDLERS
/*
 * ============================================================================
 *  NAKED IZVEDBI SVC_Handler IN PendSV_Handler
 * ============================================================================
 *
 *  ZAKAJ "NAKED"?
 *  --------------
 *  Za navadno C funkcijo prevajalnik doda PROLOG (na začetku) in EPILOG
 *  (na koncu). V prologu na sklad shrani registre, ki jih bo v funkciji
 *  uporabljal (npr. push {r4, r7, lr}), in morda nastavi r7 kot kazalec
 *  okvirja (frame pointer). V epilogu jih obnovi (pop {r4, r7, pc}).
 *  To zahteva klicna konvencija AAPCS: funkcija mora ob vrnitvi pustiti
 *  registre r4-r11 NESPREMENJENE.
 *
 *  Preklop konteksta pa mora to pravilo namerno PREKRŠITI: ob izhodu iz
 *  handlerja morajo biti v r4-r11 registri DRUGEGA opravila. Če bi bil
 *  handler navadna C funkcija, bi epilog naložene registre novega opravila
 *  prepisal s starimi vrednostmi s sklada (pri -Og se to zgodi z r4), pri
 *  -O0 pa bi prevajalnik po obnovi r4-r11 uporabil pokvarjen r7 kot
 *  kazalec okvirja (HardFault).
 *
 *  __attribute__((naked)) prevajalniku naroči, naj NE generira ne prologa
 *  ne epiloga. Funkcija vsebuje samo naše ukaze, zato so registri točno
 *  takšni, kot jih pustimo mi. Cena: vse, tudi vrnitev (bx lr), moramo
 *  napisati sami, v telesu pa je dovoljen samo osnovni asm, torej brez
 *  C-operandov ("=r"(x), "r"(y)) in brez C-spremenljivk.
 *
 *  DOSTOP DO GLOBALNIH SPREMENLJIVK IZ ASEMBLERJA
 *  ----------------------------------------------
 *  Ker C-operandov ne smemo uporabiti, naslov globalne spremenljivke (TCB,
 *  current_task) naložimo neposredno po imenu simbola, z dvema ukazoma:
 *
 *      movw r0, #:lower16:TCB    @ r0[15:0]  = spodnjih 16 bitov naslova TCB
 *      movt r0, #:upper16:TCB    @ r0[31:16] = zgornjih 16 bitov naslova TCB
 *
 *  Naslova ob prevajanju še ne poznamo: dopolni ju povezovalnik (linker).
 *  Alternativa "ldr r0, =TCB" bi naslov prebrala iz literalnega bazena
 *  (literal pool) v pomnilniku; movw/movt ga zapišeta kar v ukaz.
 *
 *  SKLAD OPRAVILA (kot ga pripravi TaskInit in kot ga uporabljata handlerja)
 *  -----------------------------------------------------------------------
 *  Vsako opravilo ima svoj sklad v stackRegion. Ko opravilo ne teče, je
 *  na vrhu njegovega sklada shranjen celoten kontekst (16 besed):
 *
 *        višji naslovi
 *      +----------------+  <- dno sklada opravila (pTaskStackBase)
 *      |  xPSR          |  +60  \
 *      |  PC            |  +56   |
 *      |  LR            |  +52   |  HW okvir (HW stack frame):
 *      |  R12           |  +48   |  shrani ga STROJNA OPREMA ob vstopu
 *      |  R3            |  +44   |  v izjemo in ga obnovi ob izstopu
 *      |  R2            |  +40   |  iz izjeme (exception return).
 *      |  R1            |  +36   |
 *      |  R0            |  +32  /   <- sem kaže PSP med izvajanjem handlerja
 *      |  R11           |  +28  \
 *      |  R10           |  +24   |
 *      |  R9            |  +20   |  SW okvir (SW stack frame):
 *      |  R8            |  +16   |  shranimo/obnovimo ga MI
 *      |  R7            |  +12   |  (stmdb / ldmia spodaj).
 *      |  R6            |  +8    |
 *      |  R5            |  +4    |
 *      |  R4            |  +0   /   <- TCB[i].sp (shranjen kazalec sklada)
 *      +----------------+
 *        nižji naslovi              (sklad raste proti nižjim naslovom)
 *
 *  ZAKAJ HANDLERJA LAHKO PROSTO UPORABLJATA r0-r3 IN r12?
 *  ------------------------------------------------------
 *  Vrednosti teh registrov opravila je strojna oprema že shranila v
 *  HW okvir na PSP, zato jih v handlerju lahko prepišemo. Ob izstopu iz
 *  izjeme jih strojna oprema obnovi iz HW okvirja.
 *  Registrov r4-r11 strojna oprema NE shrani, zato je to naša naloga.
 *
 *  OMEJITEV: opravila ne smejo uporabljati FPU. Shranjujemo samo r4-r11,
 *  ne pa s16-s31, predpostavljamo pa tudi osnovni HW okvir z 8 besedami
 *  (opravilo, ki uporablja FPU, ima razširjen okvir s 26 besedami in
 *  EXC_RETURN 0xFFFFFFED).
 * ============================================================================
 */


/*
 * ----------------------------------------------------------------------------
 *  SVC_Handler: zagon prvega opravila (Task0)
 * ----------------------------------------------------------------------------
 *  Kako pridemo sem:
 *    main() je pripravil opravila (InitSchedulerSVC), z __set_CONTROL(3)
 *    preklopil na PSP in neprivilegiran način, nato izvedel "svc 0".
 *    Ukaz SVC sproži izjemo SVCall. Strojna oprema ob vstopu:
 *      - shrani HW okvir main-a (r0-r3, r12, lr, pc, xPSR) na PSP,
 *        ker je main tekel s PSP,
 *      - preklopi v Handler mode (vedno privilegiran, sklad MSP),
 *      - v LR zapiše posebno vrednost EXC_RETURN (glej spodaj).
 *
 *  Kaj naredimo:
 *    V main() se nikoli ne vrnemo, zato njegovega konteksta ne shranimo.
 *    HW okvir, ki ga je strojna oprema shranila na PSP, preprosto zavržemo
 *    tako, da PSP nastavimo na sklad Task0. Nato "ponaredimo" vrnitev iz
 *    izjeme: strojna oprema bo mislila, da se vrača v prekinjeno kodo,
 *    v resnici pa bo iz sklada Task0 naložila PC = Task0 in začela izvajati
 *    Task0.
 * ----------------------------------------------------------------------------
 */
__attribute__((naked)) void SVC_Handler(void)
{
  __asm volatile(
    /* 1. r0 = TCB[0].sp
     *    TCB[0] je prvi element tabele TCB, sp pa je prvo polje strukture
     *    TCB_Type, zato je naslov TCB[0].sp kar naslov simbola TCB.
     *    TCB[0].sp kaže na R4 v SW okvirju Task0 (glej sliko zgoraj).
     */
    "movw  r0, #:lower16:TCB       \n"  // r0 = &TCB (spodnjih 16 bitov)
    "movt  r0, #:upper16:TCB       \n"  // r0 = &TCB (zgornjih 16 bitov)
    "ldr   r0, [r0]                \n"  // r0 = TCB[0].sp

    /* 2. Obnovi SW okvir Task0 (r4-r11) s sklada Task0.
     *    LDMIA = LoaD Multiple, Increment After:
     *      r4 = [r0], r5 = [r0+4], ..., r11 = [r0+28]
     *    '!' pomeni write-back: na koncu r0 = r0 + 8*4 = r0 + 32.
     *    r0 zdaj kaže na R0 v HW okvirju Task0.
     */
    "ldmia r0!, {r4-r11}           \n"

    /* 3. PSP = začetek HW okvirja Task0.
     *    Ob izstopu iz izjeme bo strojna oprema HW okvir pobrala s PSP.
     *    S tem zavržemo tudi HW okvir main-a, ki ga je shranil ukaz SVC.
     */
    "msr   psp, r0                 \n"

    /* 4. current_task = 0 (Task0 je zdaj trenutno opravilo).
     *    r2 = 0 bomo uporabili še v koraku 5.
     */
    "movw  r1, #:lower16:current_task \n"  // r1 = &current_task
    "movt  r1, #:upper16:current_task \n"
    "movs  r2, #0                  \n"     // r2 = 0
    "str   r2, [r1]                \n"     // current_task = 0

    /* 5. BASEPRI = 0: omogoči vse prekinitve.
     *    V main() smo z BASEPRI = 0x10 maskirali vse izjeme s prioriteto
     *    1 ali nižjo (večja številka = nižja prioriteta). Tako med
     *    inicializacijo nista mogla priti SysTick (1) in PendSV (15).
     *    Ko opravilo začne teči, ju dovolimo: SysTick bo odslej periodično
     *    sprožal PendSV in s tem preklop konteksta.
     */
    "msr   basepri, r2             \n"

    /* 6. LR = EXC_RETURN = 0xFFFFFFFD
     *    "mvn lr, #2" zapiše v LR bitno negacijo števila 2:
     *      ~0x00000002 = 0xFFFFFFFD
     *    Ko handler izvede "bx lr" z LR = 0xFFFFFFxx, procesor tega NE
     *    razume kot skok na naslov, ampak kot vrnitev iz izjeme. Spodnji
     *    biti povedo, kam in kako:
     *      bit 4 = 1  ->  osnovni HW okvir (8 besed, brez FPU registrov)
     *      bit 3 = 1  ->  vrnitev v Thread mode
     *      bit 2 = 1  ->  HW okvir poberi s PSP (ne z MSP)
     *    Ob vstopu v SVC je bila ta vrednost v LR že tako nastavljena,
     *    vendar jo zapišemo eksplicitno, da ne bomo odvisni od tega, ali
     *    je main pred tem uporabil FPU (potem bi bila 0xFFFFFFED).
     */
    "mvn   lr, #2                  \n"

    /* 7. Vrnitev iz izjeme. Strojna oprema s PSP pobere HW okvir Task0:
     *      r0, r1, r2, r3, r12, lr, pc (= naslov Task0), xPSR (bit T = 1)
     *    in nadaljuje v Thread mode na PC = Task0, s skladom PSP.
     *    CONTROL.nPRIV = 1 ostane, zato Task0 teče neprivilegirano.
     */
    "bx    lr                      \n"
  );
}


/*
 * ----------------------------------------------------------------------------
 *  PendSV_Handler: preklop konteksta
 * ----------------------------------------------------------------------------
 *  Kako pridemo sem:
 *    SysTick_Handler v registru ICSR postavi bit PENDSVSET in s tem
 *    "naroči" izjemo PendSV. Ker ima PendSV najnižjo prioriteto (15),
 *    se izvede šele, ko ni aktivna nobena druga prekinitev, torej ko
 *    se vračamo v opravilo. Tako preklop konteksta nikoli ne prekine
 *    druge prekinitvene rutine.
 *
 *    Ob vstopu v PendSV je strojna oprema že:
 *      - shranila HW okvir trenutnega opravila (r0-r3, r12, lr, pc, xPSR)
 *        na njegov sklad (PSP),
 *      - preklopila na sklad MSP,
 *      - v LR zapisala EXC_RETURN (0xFFFFFFFD: Thread mode, PSP).
 *
 *  Kaj naredimo (trije koraki):
 *    A. Shranimo SW okvir (r4-r11) trenutnega opravila na njegov sklad.
 *       Zdaj je na skladu celoten kontekst (slika zgoraj).
 *    B. Pokličemo ContextSwitch(current_task, TCB), ki:
 *         - shrani PSP v TCB[current_task].sp,
 *         - izbere naslednje opravilo (round-robin),
 *         - nastavi PSP na TCB[novo].sp,
 *         - vrne številko novega opravila.
 *    C. S sklada novega opravila obnovimo njegov SW okvir (r4-r11) in se
 *       vrnemo iz izjeme. Strojna oprema obnovi še HW okvir novega
 *       opravila in novo opravilo nadaljuje tam, kjer je bilo prekinjeno.
 * ----------------------------------------------------------------------------
 */
__attribute__((naked)) void PendSV_Handler(void)
{
  __asm volatile(
    /* ---- A. Shrani SW okvir trenutnega opravila --------------------------
     *    PSP trenutno kaže na R0 v HW okvirju. SW okvir zapišemo tik pod njega.
     *    STMDB = STore Multiple, Decrement Before:
     *      najprej r0 = r0 - 32, nato [r0] = r4, [r0+4] = r5, ..., [r0+28] = r11
     *    (r4 pride na najnižji naslov, točno kot na sliki zgoraj).
     *    STMDB je zrcalni ukaz ukazu LDMIA: kar STMDB shrani, LDMIA naloži
     *    v istem vrstnem redu.
     */
    "mrs   r0, psp                 \n"  // r0 = PSP trenutnega opravila
    "stmdb r0!, {r4-r11}           \n"  // shrani r4-r11, r0 -= 32
    "msr   psp, r0                 \n"  // PSP = vrh SW okvirja (to bo shranil ContextSwitch)

    /* ---- B. Pokliči ContextSwitch(current_task, TCB) ---------------------
     *    PUSH {r3, lr} na MSP:
     *      - LR vsebuje EXC_RETURN, ki ga potrebujemo za vrnitev iz izjeme.
     *        Ukaz BL bo LR prepisal s povratnim naslovom, zato ga shranimo.
     *      - r3 shranimo samo zato, da potisnemo DVE besedi (8 bajtov):
     *        AAPCS zahteva, da je sklad ob klicu C funkcije poravnan na
     *        8 bajtov. Vrednost r3 ni pomembna.
     *    Ta push gre na MSP (Handler mode), ne na sklad opravila.
     */
    "push  {r3, lr}                \n"

    /* Argumenti po AAPCS: 1. argument v r0, 2. argument v r1. */
    "movw  r2, #:lower16:current_task \n"  // r2 = &current_task
    "movt  r2, #:upper16:current_task \n"
    "ldr   r0, [r2]                \n"     // r0 = current_task  (1. argument)
    "movw  r1, #:lower16:TCB       \n"
    "movt  r1, #:upper16:TCB       \n"     // r1 = TCB           (2. argument)

    /* Klic C funkcije. ContextSwitch sme po AAPCS spremeniti r0-r3 in r12
     * (caller-saved), r4-r11 pa mora pustiti nespremenjene. Vrnjena
     * vrednost (številka novega opravila) je v r0. Po vrnitvi PSP že kaže
     * na SW okvir novega opravila.
     */
    "bl    ContextSwitch           \n"

    /* current_task = r0.
     * Naslov current_task naložimo ZNOVA, ker je ContextSwitch smel r2
     * prepisati (r2 je caller-saved).
     */
    "movw  r2, #:lower16:current_task \n"
    "movt  r2, #:upper16:current_task \n"
    "str   r0, [r2]                \n"     // current_task = novo opravilo

    "pop   {r3, lr}                \n"     // LR = EXC_RETURN, MSP spet poravnan kot ob vstopu

    /* ---- C. Obnovi SW okvir novega opravila in se vrni ---------------------
     *    PSP zdaj kaže na R4 v SW okvirju NOVEGA opravila.
     *    LDMIA: r4 = [r0], ..., r11 = [r0+28], nato r0 += 32
     *    -> r0 kaže na R0 v HW okvirju novega opravila.
     */
    "mrs   r0, psp                 \n"     // r0 = PSP novega opravila
    "ldmia r0!, {r4-r11}           \n"     // obnovi r4-r11 novega opravila
    "msr   psp, r0                 \n"     // PSP = začetek HW okvirja novega opravila

    /* Vrnitev iz izjeme (LR = EXC_RETURN = 0xFFFFFFFD).
     * Strojna oprema s PSP pobere HW okvir NOVEGA opravila (r0-r3, r12, lr,
     * pc, xPSR). Novo opravilo nadaljuje točno tam, kjer ga je ob zadnjem
     * preklopu prekinil SysTick, z vsemi registri, kot jih je takrat imelo.
     * Če opravilo teče prvič, je ta okvir pripravil TaskInit in PC = začetek
     * funkcije opravila.
     */
    "bx    lr                      \n"
  );
}
#endif /* FRIRTOS_NAKED_HANDLERS */