#include "thread.h"
#include "main_class.h"

__BEGIN_API

// Declaração inicial do uid das threads
int unsigned Thread::thread_count = 0;
// Declaração inicial do ponteiro que aponta para thread
// que está rodando
Thread* Thread::_running = 0;
Thread::Ready_Queue Thread::_ready;
Thread::Sus_Queue Thread::_suspension;
Thread Thread::_dispatcher;
Thread Thread::_main;
CPU::Context Thread::_main_context;


void Thread::thread_exit(int exit_code) {
    //Implementação da destruição da thread
    db<Thread>(TRC) << "Método thread_exit iniciou execução\n";
    //Correções - Solução do professor
    this->_state = FINISHING;
    //thread_exit chama o yield
    Thread::yield();
    
    //
    // delete this->_context;// Implementar no Destrutor da classe
    // Thread::thread_count --; // Implementar no dispatcher
    
}

int Thread::join() {

}

void Thread::suspend() {
    db<Thread>(TRC) << "Thread iniciou suspensão\n";

    this->_state = State::SUSPENDED;

    _suspension.insert(&this->_link);



}

void Thread::resume(){

    
    this->_state = State::READY;

    _suspension.remove(&this->_link);


}

/*
* Retorna o id de uma thread em específico
*/
int Thread::id() {
    return this->_id;
};

void Thread::init(void (*main)(void *)) {
    db<Thread>(TRC) << "Método Thread::init iniciou execução\n";

    //Criação de adição de casting para as funções, além de adicionar uma string ao final para obedecer a chamada da função
    // Criação das threads para que vazamento de memória não ocorra
    new (&_main) Thread(main, (void *) "main");
    new (&_dispatcher) Thread((void (*) (void*)) &Thread::dispatcher, (void*) NULL);

    // Como inserir ele na fila usando o _link? 
    _ready.insert(&_dispatcher._link);

    //Fazer assim dá ruim e pode dar vazamento de memória
    // Thread* main_thread = new Thread((void(*)(char*)) main, (char*) "Thread Main");
    // Thread* dispatcher_thread = new Thread((void(*)(char*))Thread::dispatcher, (char*) "Thread Dispatcher");

    _running = &_main;

    _main._state = State::RUNNING;
    new (&_main_context) CPU::Context();

    // Troca de CONTEXTO, criação de um contexto vazio para realizar a troca
    //Context* mock_context = new CPU::Context();
    CPU::switch_context(&_main_context, _main._context);
};



void Thread::dispatcher() {
    //Utilizar o contador de threads dentro do dispatcher
    db<Thread>(TRC) << "Thread dispatcher iniciou execução\n";
    //TODO:Ajustes de sintaxe/
    //Correções -> Checar enquanto thread_count for maior que 2
    while (thread_count>2){ //Enquanto ouverem Threads de usuário//
        //Errado - 
        //Ready_Queue::Element* next_element = Thread::_ready.head(); 
        
        Ready_Queue::Element* next_element = _ready.head();
        int id = (*next_element->object()).id();
        db<Thread>(TRC) << "ID que a Dispatcher selecionou:" << id << "\n";

        //Iterar sobre a lista até encontrar uma thread que n seja a dispatcher
        // while (next_element->object()->_id != 1) {
        //     // Talvez n seja o melhor jeito, revisar outros métodos de list
        //     Ready_Queue::Element* next_element = next_element->next();
        // }

        Thread* next_thread = next_element->object(); // Pegar o objeto Thread de dentro do elemento
            

        //Se thread está em estado finishing temos que removê-la da lista e diminuir o count
        //Se não, iremos recolocar a thread dispatcher na fila de prontos e começar a executar a próxima thread
        //Como a próxima Thread está running, tiramos ela da fila de prontos / ela será recolocada em yield
        if (next_thread->_state == State::FINISHING){
            //Não atualizar timestamp
            // Dispatcher não pode escolher thread com estado finishing, remove da lista
            Thread::_ready.remove_head();
            thread_count --;
        } else {
            //Atualizar dispatcher
            _dispatcher._state = State::READY; //Estado da next_thread alterado para ready
            _ready.insert(&_dispatcher._link); //Insere dispatcher no Ready_Queue
        
            //Atualizar thread que irá executar
            next_thread->_state = State::RUNNING;
            _running = next_thread; //Definir a next_thread como a thread rodando 
            // Executar o switch_context da thread
            _ready.remove(&next_thread->_link);
            switch_context((&_dispatcher), next_thread);
        }
    };
    db<Thread>(TRC) << "Thread dispatcher está terminando\n";
    Thread::_dispatcher._state = State::FINISHING; //Dispatcher em finishing
    Thread::_ready.remove(&Thread::_dispatcher._link); //Remover dispatcher da fila
    Thread::switch_context(&Thread::_dispatcher, &Thread::_main); //Troca de contexto para main

}

Thread::~Thread() {
    int id = this->id();
    db<Thread>(TRC) << "Desconstrutor da thread:" << id <<"\n";
    if (this->_context) {
        delete this->_context;
    }
}

__END_API