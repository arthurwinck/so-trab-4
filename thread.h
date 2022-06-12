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
    static int switch_context(Thread * prev, Thread * next) {
        db<Thread>(TRC) << "Trocando contexto Thread::switch_context()\n";
        if (prev != next) {
            //UPDATE: ORDEM ERRADA, primeiro se troca o _running depois executa switch_context (UPDATE: Fazemos isso em yield())
            // Se for feito do jeito inverso, quando chega em switch_context o código n executa mais 
            int result = CPU::switch_context(prev->_context, next->_context);
            
            // Se eu não conseguir realizar switch_context da CPU, aviso que deu ruim
            if (result) {
                return 0;
            } else {
                return -1;
            }

        } else {
            return -1;
        }
    }
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
    static void yield() {
        db<Thread>(TRC) << "Thread iniciou processo de yield\n";
        Thread * prev = Thread::_running;
        Thread * next = Thread::_ready.remove()->object();


        //Correções
        // Atualiza a prioridade de prev com timestamp se n for a main
        if(prev != &_main && prev != (&_dispatcher) && prev->_state != FINISHING) {
            prev->_state = READY;
            int now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            prev->_link.rank(now);
            db<Thread>(TRC) << "ID da Thread com timestamp atualizado:" << (*prev).id() << "\n";
        }

        if (prev != &_main) {
            _ready.insert(&prev->_link);
        }

        // Muda o ponteiro running para a próxima thread
        _running = next;

        // Muda o estado da próxima thread 
        next->_state = RUNNING;

        switch_context(prev, next);

        //----------

        // //Validando a thread que está rodando
        // if (prev->_state == State::FINISHING) {
        //     db<Thread>(TRC) << "Thread que estava rodando está terminando\n";

        //     //Thread está terminando, não vamos colocar ela na fila
        //     // TODO.... (?)
        // } else if (prev->id() == 0) {
        //     db<Thread>(TRC) << "[Thread Main] iniciou processo de yield\n";
        //     //Essa é a thread main, se ela deu yield quer dizer que nosso programa está terminando 
        //     // TODO.... (?)
        //     // Desalocar a memória de todos os atributos da classe Thread e afins
        // } else {
        //     db<Thread>(TRC) << "Thread será recolocada na fila\n";
        //     // Atualizar o estado da thread que terminou a execução            
        //     prev->_state = State::READY;
        //     int now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        //     prev->_link.rank(now);
        //     //Remover a próxima thread da fila para colocá-la em execução, mudando seu estado no método switch_context
        //     //Na fila temos o tipo ELEMENT, precisamos retirar esse ELEMENT e pegar a thread de dentro dele, usando o object()
        // }
        
        // // Busca o primeiro elemento da lista, a próxima thread que irá executar
        // //Ready_Queue::Element* next_element = _ready.head();
        // // Pegar a thread de dentro do elemento
        // //Thread* next = next_element->object();
        // //Atualizar o ponteiro _running para a thread que está executando
        // Thread::_running = &_dispatcher;
        // Thread::_dispatcher._state = State::RUNNING;
        // Thread::_ready.remove(&Thread::_dispatcher._link);
        // // Chamada de switch context para a thread que deu yield e a thread que irá executar
        // Thread::switch_context(prev, &_dispatcher);
    }


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
    // Preciso realizar a atribuição de new (?) e adicionar o elemento na fila
    // Inserir a thread na fila de prontos
    if (_id != 0 && _id != 1) { //Thread não é dispatcher e não é main
        Thread::_ready.insert(&this->_link); //Inserimos a dispatcher na thread::main
    }
        
    
}
__END_API

#endif
