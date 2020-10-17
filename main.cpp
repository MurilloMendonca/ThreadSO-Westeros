#include<iostream>
#include<vector>
#include<thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <atomic>
//OLHA O EXEMPLO NA MAIN
//COMPILA COM "c++ -pthread"
#define N_CANDIDATOS 7
#define URNAS_CIDADE 5
#define CIDADE_REINO 20
#define N_REINOS 7
//IGNORA POR ENQUANTO
class Urna{
    private:
    std::mutex mut;
    std::condition_variable cv;
    std::vector<long> votos;

    public:

    Urna()
    {
        for(int i=0;i<N_CANDIDATOS;i++)
            votos.push_back(0);
    }
    void vota(int candidato)
    {
        std::unique_lock<std::mutex> ul(mut);
        votos[candidato]++;
        ul.unlock();
        cv.notify_all();
    }
    std::vector<long> apura()
    {
        mut.lock();
        return votos;
    }
    long apura(int candidato)
    {
        mut.lock();
        return votos[candidato];
    }
    

};
//IGNORA POR ENQUANTO
class Cidade{
    private:
    std::vector<Urna> urnasPorCidade;
    std::vector<long> votosPorCidade;
    std::mutex mut;
    std::condition_variable cv;

    public:
    /*Cidade()
    {
        for(int i=0;i<URNAS_CIDADE;i++)
        {
            urnasPorCidade.push_back(Urna ());
        }
    }*/

    void apura(){
        std::vector<std::thread> APURA;
        for(int i =0;i<URNAS_CIDADE;i++)
        {
            APURA.push_back(std::thread ([] (std::vector<long>& v, std::vector<Urna>& u, int n, std::mutex& mut) {
                mut.lock();
                for(int i=0;i<N_CANDIDATOS;i++)
                    v[i] += u[i].apura(n);
                mut.unlock();
            },std::ref(votosPorCidade), std::ref(urnasPorCidade), i, std::ref(mut)));
        }
    }

    void mostra(){

    }
};

int main(){
    //EXEMPLO PRODUTOR CONSUMIDOR
    std::vector<int> a {1,2,3,4,5,6,7};     //VECTOR DE "ENTRADA"
    std::vector<int> b;                     //VECTOR TEMP
    std::vector<std::thread> Prod;          //VECTOR DE THREADS PRODUTORAS
    std::vector<std::thread> Cons;          //VECTOR DE THREADS CONSUMIDORAS
    std::mutex mut;                         //MUTEX PRA TRAVAR ACESSOS
    std::condition_variable cv;             //SISTEMA DE NOTIFICAÇÃO
    for(int i=0; i<5;i++)
    {
        Prod.push_back(std::thread([](std::vector<int>& a, std::vector<int>& b,//AQUI EU LANÇO AS PRODUTORAS
         int i, std::mutex& mut, std::condition_variable& cv){
            std::unique_lock<std::mutex> lk (mut);//TRAVO ACESSO PARA FAZER A ESCRITA
            b.push_back(a[i] * 2*i);                    //OPERAÇÃO ALEATÓRIA -> "PRODUZ" E GUARDA NUM TEMP
            lk.unlock();                                //TRAVO ACESSO PARA OUTRAS THREADS
            cv.notify_one();                            //NOTIFICA ALGUEM QUE ESTEJA ESPERANDO
        }, std::ref(a), std::ref(b), i, std::ref(mut), std::ref(cv)));
        
        Cons.push_back(std::thread([]( std::vector<int>& b, //AQUI EU LANÇO AS CONSUMIDORAS
        std::mutex& mut, std::condition_variable& cv){
            std::unique_lock<std::mutex> lk (mut);//TRAVO O ACESSO PARA FAZER A LEITURA
            while (b.empty())                           //VERIFICO SE JÁ TEM ALGUEM NO VETOR 
                cv.wait(lk);                            //SE NÃO TEM, ESPERA UMA NOTIFICAÇÃO
            std::cout<<b.back()<<std::endl;             //PRINTA -> "CONSOME"
            b.pop_back();                               //RETIRA O CARA QUE JÁ FOI PRINTADO DO VECTOR
        }, std::ref(b), std::ref(mut), std::ref(cv)));
    }
        
    for(auto& x:Prod)
        x.join();       //ESPERO TODAS AS PRODUTORAS ACABAREM
    for(auto& x:Cons)
        x.join();       //ESPERO TODAS AS CONSUMIDORAS ACABAREM
    return 0;
}