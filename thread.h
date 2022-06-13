#ifndef thread_h
#define thread_h

#include "cpu.h"
#include "traits.h"
#include "debug.h"
#include "list.h"
#include <ctime> 
#include <chrono>

__BEGIN_API

class Thread
{
protected:
    typedef CPU::Context Context;
public:

    typedef Ordered_List<Thread> Ready_Queue;
    typedef Ordered_List<Thread> Sus_Queue; /*T4=Queue*/

    // Thread State
    enum State {
        RUNNING,
        READY,
        FINISHING,
        SUSPENDED
    };

    /*
     * Construtor vazio. Necessário para inicialização, mas sem importância para a execução das Threads.
     */ 
    Thread() { }

    /*
     * Cria uma Thread passando um ponteiro para a função a ser executada
     * e os parâmetros passados para a função, que podem variar.
     * Cria o contexto da Thread.
     * PS: devido ao template, este método deve ser implementado neste mesmo arquivo .h
     */ 
    template<typename ... Tn>
    Thread(void (* entry)(Tn ...), Tn ... an);

    /*
     * Retorna a Thread que está em execução.
     */ 
    static Thread * running() { return _running; }

    /*
     * Método para trocar o contexto entre duas thread, a anterior (prev)
     * e a próxima (next).
     * Deve encapsular a chamada para a troca de contexto realizada pela class CPU.
     * Valor de retorno é negativo se houve erro, ou zero.
     */ 
    static int switch_context(Thread * prev, Thread * next);
    /*
     * Termina a thread.
     * exit_code é o código de término devolvido pela tarefa (ignorar agora, vai ser usado mais tarde).
     * Quando a thread encerra, o controle deve retornar à main. 
     */  
    void thread_exit (int exit_code);

    /*
     * Retorna o ID da thread.
     */ 
    int id();

    /*
     * NOVO MÉTODO DESTE TRABALHO.
     * Daspachante (disptacher) de threads. 
     * Executa enquanto houverem threads do usuário.
     * Chama o escalonador para definir a próxima tarefa a ser executada.
     */

    int join();

    void suspend();

    void resume();

    static void dispatcher(); 

    /*
     * NOVO MÉTODO DESTE TRABALHO.
     * Realiza a inicialização da class Thread.
     * Cria as Threads main e dispatcher.
     */ 
    static void init(void (*main)(void *));


    /*
     * Devolve o processador para a thread dispatcher que irá escolher outra thread pronta
     * para ser executada.
     */
    static void yield();


    /*
     * Destrutor de uma thread. Realiza todo os procedimentos para manter a consistência da classe.
     */ 
    ~Thread();

    /*
     * Qualquer outro método que você achar necessário para a solução.
     */ 

private:
    int _id;
    Context * volatile _context;
    static Thread * _running;
    static unsigned int thread_count; 
    static Thread _main; 
    static CPU::Context _main_context;
    static Thread _dispatcher;
    static Ready_Queue _ready;
    static Sus_Queue _suspension; //Queue para elemntos suspensos
    Ready_Queue::Element _link;
    //Sus_Queue::Element _linksus adcionar element do queue/
    volatile State _state;
    volatile int _exit_code;
    Sus_Queue::Element _waiting_link;
    volatile int _waiting_bool;

    /*
     * Qualquer outro atributo que você achar necessário para a solução.
     */ 

};

template<typename ... Tn>
inline Thread::Thread(void (* entry)(Tn ...), Tn ... an) : _link(this, (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()))
{
    //IMPLEMENTAÇÃO DO CONSTRUTOR
    //UPDATE: Chamada do debugger
    db<Thread>(TRC) << "Thread::Thread(): criou thread\n";
    //Criação do Contexto...
    this->_context = new CPU::Context(entry, an...);
    //... Outras inicializações
    // Incremento o valor de id para gerar um novo id para a thread (Update para não usar getter)
    this->_id = Thread::thread_count ++;
    //Alterar status para ready
    this->_state = State::READY;

    //Essa thread não está em nenhum join no momento, setamos então id_waiting para -1
    this->_waiting_link = NULL;
    this->_waiting_bool = 0;

    // Preciso realizar a atribuição de new (?) e adicionar o elemento na fila
    // Inserir a thread na fila de prontos
    if (_id != 0 && _id != 1) { //Thread não é dispatcher e não é main
        Thread::_ready.insert(&this->_link); //Inserimos a dispatcher na thread::main
    }
        
    
}
__END_API

#endif
