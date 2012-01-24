#ifndef MYTH_CONTEXT_H_
#define MYTH_CONTEXT_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "myth_config.h"

typedef void (*void_func_t)(void);

//Attributes of functions called after context switch
#ifdef __i386__
#define MYTH_CTX_CALLBACK static __attribute__((used,noinline,regparm(0)))
#define USE_AVOID_OPTIMIZE
#elif __x86_64__
#define MYTH_CTX_CALLBACK static __attribute__((used,noinline,sysv_abi))
#define USE_AVOID_OPTIMIZE
//#define MYTH_CTX_CALLBACK static __attribute((used,noinline))
#else
#error
#endif

//Execution context
typedef struct myth_context
{
#if defined (__i386__)
	uint32_t esp;
#elif defined(__x86_64__)
	uint64_t rsp;
#else
#error "This architecture is not supported"
#endif
}myth_context,*myth_context_t;

static inline void myth_make_context_empty(myth_context_t ctx,void *stack);
static inline void myth_make_context_voidcall(myth_context_t ctx,void_func_t func,void *stack,size_t stacksize);

void myth_swap_context_s(myth_context_t switch_from,myth_context_t switch_to);
void myth_swap_context_withcall_s(myth_context_t switch_from,myth_context_t switch_to,void(*func)(void*,void*,void*),void *arg1,void *arg2,void *arg3);
void myth_get_context_s(myth_context_t ctx);
void myth_set_context_s(myth_context_t ctx);
void myth_set_context_withcall_s(myth_context_t switch_to,void(*func)(void*,void*,void*),void *arg1,void *arg2,void *arg3);

#if defined MYTH_COLLECT_LOG && defined MYTH_COLLECT_CONTEXT_SWITCH
//static inline void myth_log_add_context_switch(struct myth_running_env *env,struct myth_thread *th);
#define  myth_context_switch_hook(ctx) \
{ \
	struct myth_running_env* env=myth_get_current_env(); \
	myth_thread_t th=myth_context_to_thread(env,ctx); \
	myth_log_add_context_switch(env,th);\
}
#else
#define myth_context_switch_hook(ctx)
#endif

#if defined MYTH_INLINE_CONTEXT
#define myth_set_context(ctx) {myth_context_switch_hook(ctx);myth_set_context_i(ctx);}
#define myth_swap_context(from,to) {myth_context_switch_hook(to);myth_swap_context_i(from,to);}
#define myth_swap_context_withcall(from,to,fn,a1,a2,a3) {myth_context_switch_hook(to);myth_swap_context_withcall_i(from,to,fn,a1,a2,a3);}
#define myth_set_context_withcall(ctx,fn,a1,a2,a3) {myth_context_switch_hook(ctx);myth_set_context_withcall_i(ctx,fn,a1,a2,a3);}

//#define myth_swap_context(from,to) {myth_context_switch_hook(to);myth_swap_context_s(from,to);}
//#define myth_swap_context_withcall(from,to,fn,a1,a2,a3) {myth_context_switch_hook(to);myth_swap_context_withcall_s(from,to,fn,a1,a2,a3);}
//#define myth_set_context_withcall(ctx,fn,a1,a2,a3) {myth_context_switch_hook(ctx);myth_set_context_withcall_s(ctx,fn,a1,a2,a3);}
#else
#define myth_set_context(ctx) {myth_context_switch_hook(ctx);myth_set_context_s(ctx);}
#define myth_swap_context(from,to) {myth_context_switch_hook(to);myth_swap_context_s(from,to);}
#define myth_swap_context_withcall(from,to,fn,a1,a2,a3) {myth_context_switch_hook(to);myth_swap_context_withcall_s(from,to,fn,a1,a2,a3);}
#define myth_set_context_withcall(ctx,fn,a1,a2,a3) {myth_context_switch_hook(ctx);myth_set_context_withcall_s(ctx,fn,a1,a2,a3);}
#endif

//Suffix for PLT
#ifdef COMPILED_AS_PIC
#define FUNC_SUFFIX "@PLT"
#define GOTPCREL_SUFFIX "@GOTPCREL"
#else
#define FUNC_SUFFIX ""
#define GOTPCREL_SUFFIX ""
#endif

static inline void myth_make_context_empty(myth_context_t ctx,void *stack)
{
#if defined(__i386__)
	//Get stack tail
	uint32_t stack_tail=((uint32_t)stack);
	//Align
	stack_tail&=0xFFFFFFF0;
	//Set stack pointer
	ctx->esp=stack_tail;
#elif defined(__x86_64__)
	//Get stack tail
	uint64_t stack_tail=((uint64_t)stack);
	//Align
	stack_tail&=0xFFFFFFFFFFFFFFF0;
	//Set stack pointer
	ctx->rsp=stack_tail;
#else
#error "This architecture is not supported"
#endif
}

//Make a context for executing "void foo(void)"
static inline void myth_make_context_voidcall(myth_context_t ctx,void_func_t func,void *stack,size_t stacksize)
{
#if defined(__i386__)
	//Get stack tail
	uint32_t stack_tail=((uint32_t)stack)+stacksize;
	stack_tail-=4;
	uint32_t *dest_addr;
	//Align
	stack_tail&=0xFFFFFFF0;
	dest_addr=(uint32_t*)stack_tail;
	//Set stack pointer
	ctx->esp=stack_tail;
	//Set retuen address
	*dest_addr=(uint32_t)func;
#elif defined(__x86_64__)
	//Get stack tail
	uint64_t stack_tail=((uint64_t)stack)+stacksize;
	stack_tail-=8;
	uint64_t *dest_addr;
	//Align
	stack_tail&=0xFFFFFFFFFFFFFFF0;
	dest_addr=(uint64_t*)stack_tail;
	//Set stack pointer
	ctx->rsp=stack_tail;
	//Set retuen address
	*dest_addr=(uint64_t)func;
#else
#error "This architecture is not supported"
#endif
}

#if defined(__i386__)

#ifdef MYTH_INLINE_PUSH_CALLEE_SAVED
#define PUSH_CALLEE_SAVED() \
	"push %%ebp\n" \
	"push %%ebx\n" \
	"push %%edi\n" \
	"push %%esi\n"
#define POP_CALLEE_SAVED() \
	"pop %%esi\n" \
	"pop %%edi\n" \
	"pop %%ebx\n" \
	"pop %%ebp\n"
#define REG_BARRIER() \
	asm volatile("":::"%eax","%ecx","%edx","memory");
#else
#define PUSH_CALLEE_SAVED() \
	"push %%ebp\n" \
	"push %%ebx\n"
#define POP_CALLEE_SAVED() \
	"pop %%ebx\n" \
	"pop %%ebp\n"
#define REG_BARRIER() \
	asm volatile("":::"%eax","%ecx","%edx","%esi","%edi","memory");
#endif

#ifdef USE_JUMP_INSN_A
#define MY_RET_A "pop %%eax\njmp *%%eax\n"
#else
#define MY_RET_A "ret\n"
#endif
#ifdef USE_JUMP_INSN_B
#define MY_RET_B "pop %%eax\njmp *%%eax\n"
#else
#define MY_RET_B "ret\n"
#endif

//Context switching functions (inlined)
#define myth_swap_context_i(switch_from,switch_to) \
	{asm volatile(\
		PUSH_CALLEE_SAVED() \
		"push $1f\n"\
		"mov %%esp,%0\n"\
		"mov %1,%%esp\n"\
		MY_RET_A \
		"1:\n"\
		POP_CALLEE_SAVED() \
		:"=m"(*(switch_from)):"g"(*(switch_to)));\
	REG_BARRIER();}

#define myth_swap_context_withcall_i(switch_from,switch_to,f,arg1,arg2,arg3) \
	{asm volatile(\
		PUSH_CALLEE_SAVED() \
		"push $1f\n"\
		"mov %%esp,%0\n"\
		"mov %1,%%esp\n"\
		"pushl $0\n"\
		"push %4\npush %3\npush %2\n"\
		"call " #f "\n"\
		"lea 16(%%esp),%%esp\n"\
		MY_RET_A \
		"1:\n"\
		POP_CALLEE_SAVED() \
		:"=m"(*(switch_from)):"g"(*(switch_to)),"r"(arg1),"r"(arg2),"r"(arg3));\
	REG_BARRIER();}

#define myth_set_context_i(switch_to) \
	{asm volatile(\
		"mov %0,%%esp\n"\
		MY_RET_B \
		::"g"(*(switch_to)));\
	myth_unreachable();\
	}

#define myth_set_context_withcall_i(switch_to,f,arg1,arg2,arg3) \
	{asm volatile(\
		"mov %0,%%esp\n"\
		"push $0\n"\
		"push %3\npush %2\npush %1\n"\
		"call " #f "\n"\
		"lea 16(%%esp),%%esp\n"\
		MY_RET_B \
		::"g"(*(switch_to)),"r"(arg1),"r"(arg2),"r"(arg3));\
	myth_unreachable();\
	}

#elif defined(__x86_64__)

#ifdef COMPILED_AS_PIC
#define PUSH_LABEL(label,reg) \
	"leaq "label"(%%rip)," reg "\n"\
	"push " reg "\n"
#define DUMMYREG_ARG ,R_DUMMY(0LL)
#else
#define PUSH_LABEL(label,reg) \
	"pushq $" label "\n"
#define DUMMYREG_ARG
#endif

#if 0 && defined MYTH_INLINE_PUSH_CALLEE_SAVED
#define PUSH_CALLEE_SAVED() \
	"push %%rbp\n" \
	"push %%rbx\n" \
	"push %%r12\n" \
	"push %%r13\n" \
	"push %%r14\n" \
	"push %%r15\n"
#define POP_CALLEE_SAVED() \
	"pop %%r15\n" \
	"pop %%r14\n" \
	"pop %%r13\n" \
	"pop %%r12\n" \
	"pop %%rbx\n" \
	"pop %%rbp\n"
#define DECLARE_DUMMY_VARIABLES int d0,d1,d2,d3,\
	d4,d5,d6,d7,\
	d8;
#define DUMMY_VARIABLE_CONSTRAINTS "=r"(d0),"=r"(d1),"=r"(d2),"=r"(d3),\
		"=r"(d4),"=r"(d5),"=r"(d6),"=r"(d7),\
		"=r"(d8)
#define C0 "%9"
#define C1 "%10"
#define C2 "%11"
#define C3 "%12"
#define C4 "%13"
#define C5 "%14"
#else
#if 0
#define PUSH_CALLEE_SAVED() "push %%rbp\n"
#define POP_CALLEE_SAVED() "pop %%rbp\n"
#define DECLARE_DUMMY_VARIABLES int d0,d1,d2,d3,\
	d4,d5,d6,d7,\
	d8,d9,dA,dB,\
	dC,dD;
#define DUMMY_VARIABLE_CONSTRAINTS "=&r"(d0),"=&r"(d1),"=&r"(d2),"=&r"(d3),\
		"=&r"(d4),"=&r"(d5),"=&r"(d6),"=&r"(d7),\
		"=&r"(d8),"=&a"(d9),"=&d"(dA),"=&D"(dB),\
		"=&S"(dC),"=&r"(dD)
#define C0 "%14"
#define C1 "%15"
#define C2 "%16"
#define C3 "%17"
#define C4 "%18"
#define C5 "%19"
#define R_ARG0 "0"
#define R_ARG1 "1"
#define R_ARG2 "2"
#define R_A "9"
#define R_D "10"
#define R_DI "11"
#define R_SI "12"
#define R_DUMMY "13"
#else
#define PUSH_CALLEE_SAVED()
#define POP_CALLEE_SAVED()
#define DECLARE_DUMMY_VARIABLES int d0,d1,d2,d3,\
	d4,d5,d6,d7,\
	d8,d9,dA,dB,\
	dC,dD,dE;
#define DUMMY_VARIABLE_CONSTRAINTS "=&r"(d0),"=&r"(d1),"=&r"(d2),"=&r"(d3),\
		"=&r"(d4),"=&r"(d5),"=&r"(d6),"=&r"(d7),\
		"=&r"(d8),"=&a"(d9),"=&d"(dA),"=&D"(dB),\
		"=&S"(dC),"=&r"(dD),"=&r"(dE)
#define C0 "%15"
#define C1 "%16"
#define C2 "%17"
#define C3 "%18"
#define C4 "%19"
#define C5 "%20"
#define R_ARG0 "0"
#define R_ARG1 "1"
#define R_ARG2 "2"
#define R_A "9"
#define R_D "10"
#define R_DI "11"
#define R_SI "12"
#define R_DUMMY "13"
#endif
#endif

#ifdef USE_JUMP_INSN_A
#define MY_RET_A "pop %%rax\njmp *%%rax\n"
#else
#define MY_RET_A "ret\n"
#endif
#ifdef USE_JUMP_INSN_B
#define MY_RET_B "pop %%rax\njmp *%%rax\n"
#else
#define MY_RET_B "ret\n"
#endif

//Context switching functions (inlined)
#define myth_swap_context_i(switch_from,switch_to) \
	{DECLARE_DUMMY_VARIABLES\
	asm volatile(\
		PUSH_CALLEE_SAVED() \
		PUSH_LABEL("1f",C2) \
		"mov %%rsp,("C0")\n"\
		"mov ("C1"),%%rsp\n"\
		MY_RET_A \
		"1:\n"\
		POP_CALLEE_SAVED() \
		:DUMMY_VARIABLE_CONSTRAINTS\
		:R_ARG0((void*)(switch_from)),R_ARG1((void*)(switch_to)) DUMMYREG_ARG\
		:"cc","memory");\
	/*REG_BARRIER();*/}

#define myth_swap_context_withcall_i(switch_from,switch_to,f,arg1,arg2,arg3) \
	{DECLARE_DUMMY_VARIABLES\
	asm volatile(\
		PUSH_CALLEE_SAVED() \
		PUSH_LABEL("1f",C5) \
		"mov %%rsp,("C0")\n"\
		"mov ("C1"),%%rsp\n"\
		"call " #f FUNC_SUFFIX "\n"\
		MY_RET_A \
		"1:\n"\
		POP_CALLEE_SAVED() \
		:DUMMY_VARIABLE_CONSTRAINTS\
		:R_ARG0((void*)(switch_from)),R_ARG1((void*)(switch_to)),R_DI((void*)arg1),R_SI((void*)arg2),R_D((void*)arg3) DUMMYREG_ARG\
		:"cc","memory");\
	/*REG_BARRIER();*/}

#define myth_set_context_i(switch_to) \
	{DECLARE_DUMMY_VARIABLES\
	asm volatile(\
		"mov ("C0"),%%rsp\n"\
		MY_RET_B \
		:DUMMY_VARIABLE_CONSTRAINTS\
		:R_ARG0((void*)(switch_to))\
		:"cc","memory");\
	myth_unreachable();\
	}

#define myth_set_context_withcall_i(switch_to,f,arg1,arg2,arg3) \
	{DECLARE_DUMMY_VARIABLES\
	asm volatile(\
		"mov ("C0"),%%rsp\n"\
		"call " #f FUNC_SUFFIX "\n"\
		MY_RET_B \
		:DUMMY_VARIABLE_CONSTRAINTS\
		:R_ARG0((void*)(switch_to)),R_DI(arg1),R_SI(arg2),R_D(arg3)\
		:"cc","memory");\
	myth_unreachable();\
	}

#else
#error "This architecture is not supported"
#endif

#endif /* MYTH_CONTEXT_H_ */
