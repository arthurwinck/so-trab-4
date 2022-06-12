#ifndef traits_h
#define traits_h

// UPDATE: Adicionar se quer ou não e quais o debugging são os níveis para cada classe do sistema
//Não alterar as 3 declarações abaixo

#define __BEGIN_API             namespace SOLUTION {
#define __END_API               }
#define __USING_API            using namespace SOLUTION;

__BEGIN_API

class CPU; //declaração das classes criadas nos trabalhos devem ser colocadas aqui
class Debug; //declaração do Debug para que possamos colocar os atributos
class System; // declaração das classes
class Thread; //...
class Lists;

//declaração da classe Traits
template<typename T>
struct Traits {
};
// UPDATE: Fazer isso para todas as classes do sistema
// Declaração para decidir se queremos ver o debug para cada classe
template<> struct Traits<System> : public Traits<void> {
    static const bool debugged = true;
};

template<> struct Traits<CPU> : public Traits<void> {
    static const int STACK_SIZE = 64000;
    static const bool debugged = false;
};

template<> struct Traits<Thread> : public Traits<void> {
    static const bool debugged = false;
};

template<> struct Traits<Lists> : public Traits<void> {
    static const bool debugged = false;
};


template<> struct Traits<Debug>: public Traits<void>
{
    static const bool error   = true;
    static const bool warning = true;
    static const bool info    = true;
    static const bool trace   = true;
};



__END_API

#endif
