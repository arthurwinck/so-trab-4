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
    
    //Retomar a execução da thread que chamou join
    if (this->_waiting_bool) {
        Thread* waiting_thread = Thread::_suspension.remove(&this->_waiting_link)->object();
        waiting_thread->resume(); 
        this->_exit_code = exit_code;       
        // Setar exit_code para id da thread
        // Mudar a thread para a thread que deu join
        // Tirar ela da lista de suspensas
    } 
    Thread::yield();
    
    //
    // delete this->_context;// Implementar no Destrutor da classe
    // Thread::thread_count --; // Implementar no dispatcher
    
}
// t1(f1)
// t2(f2)
// f1: t2->join()
// Quem executou o join foi a thread que estava rodando,
// Mas o objeto que executou foi da thread que vamos esperar
// Retornar o código de thread exit
int Thread::join() {
    //Setar id_waiting para a thread que está esperando essa thread finalizar
    this->_waiting_link = _running->_link;
    this->_waiting_bool = 1;
    this->_exit_code = this->id();
    _running->suspend();

    return this->_exit_code;
}

void Thread::suspend() {
    db<Thread>(TRC) << "Thread iniciou suspensão\n";
    if (this->_state != State::SUSPENDED) {
        this->_state = State::SUSPENDED;

        _suspension.insert(&this->_link);
    }
    Thread::yield();
}

void Thread::resume(){
    db<Thread>(TRC) << "Thread está resumindo\n";
    
    this->_state = State::READY;

    _suspension.remove(&this->_link);
    _ready.insert(&this->_link);
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

void Thread::yield() {
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
}

int Thread::switch_context(Thread * prev, Thread * next) {
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