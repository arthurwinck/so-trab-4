#ifndef cpu_h
#define cpu_h

#include <ucontext.h>
#include <iostream>
#include "traits.h"

__BEGIN_API

class CPU
{
    public:

        class Context
        {
        private:
            static const unsigned int STACK_SIZE = Traits<CPU>::STACK_SIZE;
        public:
            Context() { _stack = 0; }

            template<typename ... Tn>
            Context(void (* func)(Tn ...), Tn ... an);

            ~Context();

            void save();
            void load();
        private:            
            char *_stack;
        public:
            ucontext_t _context;
        };
    public:

        static int switch_context(Context *from, Context *to) {
            //implementação do método
            //salva o primeiro contexto e restaura o segundo
            if (from && to) {
                swapcontext(&from->_context, &to->_context);
                return 0;
            } else {
                return -1;
            }
        }
};

//Construtor da classe usando variadic templates
template<typename ... Tn>
CPU::Context::Context(void (* func)(Tn ...), Tn ... an) {
    //Criação de um contexto nulo
    ucontext_t u_context;
    save();
    _context.uc_link = 0;
    //Alocação da memória para a stack

    // Temos que usar new, mas não temos um tipo, então alocamos um char com tamanho da stack
    _stack = new char[STACK_SIZE];
    _context.uc_stack.ss_sp = _stack;

    _context.uc_stack.ss_size=STACK_SIZE;
    //Flags da stack
    _context.uc_stack.ss_flags=0;
    // Criação e alocação do novo contexto

    makecontext(&_context, (void(*)(void))func, sizeof...(Tn),an...);
    // Alocação da função passada como parâmetro como função a ser executada pelo contexto, passada junto do número de parâmetros dessa função (cabeça da lista)
    // e quais são esses parâmetros (cauda da lista)
}


__END_API

#endif

