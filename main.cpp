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
    std::atomic<int> popNaoVotante;


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
        //popNaoVotante = std::copy(populacao);
        for (int i=0;i<URNAS_CIDADE;i++)
        {
            votPorHabitante.push_back(std::thread([] (Urna& urna, std::atomic<int>& populacao){
                while(populacao>0)
                {
                    populacao--;
                    urna.vota();
                }
            }, std::ref(urnas.at(i)), std::ref(populacao)));
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
                for(int i=0;i<N_CANDIDATOS;i++)
                    v[i] += u[n].apuraUrna(i);
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
    void mostra()
    {
        for(auto& cidade:cidades){
            cidade.apura();
            cidade.mostra();
        }
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
        votosPorCidade= std::vector<std::vector<long>>  (140);
        reinos = std::vector<Reino>  (7);
    }

    void votacao(){
        std::vector<std::thread> votPorReino;
        for(int i=0;i<N_REINOS;i++)
        {
            votPorReino.push_back(std::thread([] (Reino& reino){
                reino.votacao();
            }, std::ref(reinos[i])));
        }
        for(auto& threadLancada:votPorReino)
        {
            threadLancada.join();
        }
    }

    void mostraGeral(){
        for(auto& reino: reinos)
            reino.mostra();
    }

    void apura(){
        /*td::vector<std::thread> votPorReino;
        for(Reino& reino:reinos)
        {
            votPorReino.push_back(std::thread([] (Reino& reino){
                reino.apura();
            }, std::ref(reino)));
        }
        for(auto& threadLancada:votPorReino)
        {
            threadLancada.join();
        }*/
    }
    auto getReinos()
    {
        return &reinos;
    }
    void mostraParcial(int NUM_THREADS)
    {

        //Espera-se que o Drucco faça
    }


};
int main(){
    srand(time(NULL));
    Eleicao a;
    a.votacao();
    std::cout<<"VOTACAO ENCERRADA";
    a.mostraGeral();

    return 0;
}

