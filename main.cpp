#include<iostream>
#include<vector>
#include<thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <atomic>
#include<algorithm>

#include <stdlib.h>
#include <time.h>
//OLHA O EXEMPLO NA MAIN
//COMPILA COM "c++ -pthread"
#define N_CANDIDATOS 7
#define URNAS_CIDADE 5
#define CIDADE_REINO 20
#define N_REINOS 7

std::atomic<long> TOTAL;
class Urna{
    private:
    std::mutex mut;
    std::condition_variable cv;


    public:
    std::vector<long> votos;
    Urna()
    {
        votos = std::vector<long> (N_CANDIDATOS);
    }
    void vota(int candidato)
    {
        std::unique_lock<std::mutex> ul(mut);
        std::cout<<"\nCheguei aqui\n";
        votos[candidato]++;
        ul.unlock();
        cv.notify_one();
    }
    void vota()
    {
        std::unique_lock<std::mutex> ul(mut);
        int candidato =rand()%N_CANDIDATOS;
        votos.at(candidato)++;
        TOTAL++;
        std::cout<<"\nTOTAL de votos: "<<TOTAL;
        ul.unlock();
        cv.notify_one();
    }
    std::vector<long> apuraUrna()
    {
        std::unique_lock<std::mutex> ul(mut);
        return votos;
    }
    long apuraUrna(int candidato)
    {
        std::unique_lock<std::mutex> ul(mut);
        return votos[candidato];
    }

};

class Cidade{
    private:
    std::vector<long> votosPorCidade;
    std::vector<Urna> urnas;
    std::vector<int> urnasLivres;
    std::mutex mut;
    std::condition_variable cv;
    std::atomic<int> populacao;


    public:

    Cidade(){
        urnas = std::vector<Urna> (URNAS_CIDADE);
        votosPorCidade = std::vector<long>(N_CANDIDATOS);
        populacao = (rand()%29001) + 1000;
        for(int i=0;i<URNAS_CIDADE;i++)
        {
            urnasLivres.push_back(i);
        }
    }
    void votacao(){
        std::vector<std::thread> votPorHabitante;
        //std::cout<<"\nvec tam"<<urnasLivres.size();
        for (int i=0;i<populacao;i++)
        {
            votPorHabitante.push_back(std::thread([] (std::vector<Urna>& urnas, std::mutex& mut,
            std::vector<int> urnasLivres, std::condition_variable& cv, std::atomic<int>& populacao){
                std::unique_lock<std::mutex> ul (mut);
                ul.unlock();
                //std::cout<<"\nComecou a Thread: "<<std::this_thread::get_id()<<" ->Cidade.votacao()\n";
                while(urnasLivres.empty())
                    cv.wait(ul);
                //std::cout<<"\nPassei do while\n";
                //std::cout<<"\nLivres.size() = "<<urnasLivres.size();
                //std::cout<<"\nUrnas.size() = "<<urnas.size();
                ul.lock();
                int ind = urnasLivres.back();
                //std::cout<<"\nPeguei o indice "<<ind;
                urnasLivres.pop_back();
                urnas.at(ind).vota();
                urnasLivres.push_back(ind);
                ul.unlock();
                cv.notify_one();
                //std::cout<<"\nTerminou a Thread: "<<std::this_thread::get_id()<<" ->Cidade.votacao()\n";
            }, std::ref(urnas), std::ref(mut), std::ref(urnasLivres), std::ref(cv), std::ref(populacao)));
        }
        //mut.unlock();
        //cv.notify_one();
        for(auto& threadLancada:votPorHabitante)
        {
            threadLancada.join();
        }
    }
    void apura(){
        std::vector<std::thread> APURA;
        for(int i =0;i<URNAS_CIDADE;i++)
        {
            APURA.push_back(std::thread ([] (std::vector<long>& v, std::vector<Urna>& u, int n, std::mutex& mut,
            std::condition_variable& cv) {
                std::unique_lock<std::mutex> lk (mut);
                //std::cout<<"\nComecou a Thread: "<<std::this_thread::get_id()<<" ->cidade.apura()\n";
                for(int i=0;i<N_CANDIDATOS;i++)
                    v[i] += u[n].apuraUrna(i);
                //std::cout<<"\nTerminou a Thread: "<<std::this_thread::get_id()<<" ->cidade.apura()\n";
                lk.unlock();
                cv.notify_one();
            },std::ref(votosPorCidade), std::ref(urnas), i, std::ref(mut), std::ref(cv)));
        }
        for(auto& x:APURA)
        {
            x.join();
        }
    }

    Urna* getUrna(int urna){
        return &urnas[urna];
    }
    void mostra(){
        std::unique_lock<std::mutex> lk (mut);
        while (votosPorCidade.empty())
            cv.wait(lk);
        //std::cout<<"\nComecou a Thread: "<<std::this_thread::get_id()<<" ->cidade.mostra()\n";
        for(int i=0;i<N_CANDIDATOS;i++)
            std::cout<<"Candidato "<<i<<": "<<votosPorCidade[i]<<" votos."<<std::endl;
        std::cout<<std::endl;
        //std::cout<<"\nTerminou a Thread: "<<std::this_thread::get_id()<<" ->cidade.mostra()\n";
    }
};

class Reino{
    private:
    std::mutex mut;
    std::condition_variable cv;
    std::vector<Cidade> cidades;

    public:
    Reino(){
        cidades = std::vector<Cidade>(20);
    }
    void votacao(){
        std::vector<std::thread> votPorReino;
        for(int i=0;i<CIDADE_REINO;i++)
        {
            votPorReino.push_back(std::thread([] (Cidade& cidade){
                //std::cout<<"\nComecou a Thread: "<<std::this_thread::get_id()<<" ->reino.votacao()\n";
                cidade.votacao();
                //std::cout<<"\nTerminou a Thread: "<<std::this_thread::get_id()<<" ->reino.votacao()\n";
                //politica de acesso
            }, std::ref(cidades[i])));
        }
        for(auto& threadLancada:votPorReino)
        {
            threadLancada.join();
        }
    }
};

class Eleicao{
    std::vector<std::vector<long>> votosPorCidade;
    std::vector<std::vector<long>> votosPorReino;
    std::vector<long> votosGeral;
    std::vector<Reino> reinos;
    std::mutex impressaoMut;
    std::mutex mut;

    public:
    Eleicao(){
        votosGeral = std::vector<long> (7);
        votosPorReino = std::vector<std::vector<long>>  (7);
        votosPorCidade=std::vector<std::vector<long>>  (140);
        reinos = std::vector<Reino>  (7);
    }

    void votacao(){
        std::vector<std::thread> votPorReino;
        for(int i=0;i<N_REINOS;i++)
        {
            votPorReino.push_back(std::thread([] (Reino& reino){
                //politica de acesso
                //std::cout<<"\nComecou a Thread: "<<std::this_thread::get_id()<<" ->Eleicao.votacao()\n";
                reino.votacao();
                //std::cout<<"\nTerminou a Thread: "<<std::this_thread::get_id()<<" ->Eleicao.votacao()\n";
                //politica de acesso
            }, std::ref(reinos[i])));
        }
        for(auto& threadLancada:votPorReino)
        {
            threadLancada.join();
        }
    }

    void apuraGeral(){}

    void mostraParcial(int NUM_THREADS)
    {

        //Espera-se que o Drucco faça
    }


};
int main(){
    std::vector<std::thread> vec;
    std::vector<Cidade> v(5);
    srand(time(NULL));/*
    for(int cidade =0;cidade<5;cidade++)
    {
        for(int urna =0;urna<URNAS_CIDADE;urna++)
        {
            for(int pessoa=0;pessoa<5;pessoa++)
            {
                vec.push_back(std::thread([] (std::vector<Cidade>& v, int cidade, int urna) {
                    v[cidade].getUrna(urna)->vota(rand()%7);
                },std::ref(v), cidade, urna ));
            }
        }
    }
    for(auto& x:vec)
    {
        x.join();
    }*/
    Eleicao a;
    a.votacao();
    std::cout<<"VOTACAO ENCERRADA";
    vec.clear();
    for(int i=0;i<5;i++)
    {
        vec.push_back(std::thread([] (std::vector<Cidade>& v, int i) {
            v[i].apura();
            v[i].mostra();
        }, std::ref(v), i));
    }
    for(auto& x:vec)
    {
        x.join();
    }
    return 0;
}

