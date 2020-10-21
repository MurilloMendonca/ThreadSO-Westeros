#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <iomanip>
#include <chrono>

#include <unistd.h>
#include <stdlib.h>
#include <time.h>
//OLHA O EXEMPLO NA MAIN
//COMPILA COM "c++ -pthread"
#define N_CANDIDATOS 7
#define URNAS_CIDADE 5
#define CIDADE_REINO 20
#define N_REINOS 7

using namespace std::chrono_literals;

std::atomic<long> TOTAL;
std::atomic<long> URNAS;
std::atomic<long> VOTOS;
class Urna
{
private:
    std::mutex mut;
    std::condition_variable cv;

public:
    std::vector<long> votos;
    Urna()
    {
        votos = std::vector<long>(N_CANDIDATOS);
    }
    void vota(int candidato)
    {
        std::unique_lock<std::mutex> ul(mut);
        std::cout << "\nCheguei aqui\n";
        votos[candidato]++;
        ul.unlock();
        cv.notify_one();
    }
    void vota()
    {
        std::unique_lock<std::mutex> ul(mut);
        int candidato = rand() % N_CANDIDATOS;
        votos.at(candidato)++;
        TOTAL++;
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

class Cidade
{
private:
    std::vector<long> votosDaCidade;
    std::vector<Urna> urnas;
    std::mutex mut;
    std::condition_variable cv;
    std::atomic<int> populacao;

public:
    Cidade()
    {
        urnas = std::vector<Urna>(URNAS_CIDADE);
        votosDaCidade = std::vector<long>(N_CANDIDATOS);
        populacao =(rand() % 29001) + 1000;
    }
    void votacao()
    {
        std::vector<std::thread> threadPorUrna;
        std::atomic<int> votos;
        for (Urna &urna : urnas)
        {
            threadPorUrna.push_back(std::thread([](Urna &urna, std::atomic<int> &populacao, std::atomic<int> &votos) {
                while (votos < populacao)
                {
                    votos++;
                    urna.vota();
                }
            },
                                                std::ref(urna), std::ref(populacao), std::ref(votos)));
        }
        //mut.unlock();
        //cv.notify_one();
        for (auto &threadLancada : threadPorUrna)
        {
            threadLancada.join();
        }
    }
    void apura(std::vector<long> &votosGeral, std::vector<long> &votosPorReino, std::vector<long> &votosPorCidade, std::condition_variable &urnaApurada, std::mutex& protegeEscrita)
    {


        std::vector<std::thread> APURA;
        auto f = [](
                    std::vector<long> &votosGeral,
                    std::vector<long> &votosPorCidade,
                    std::vector<long> &votosPorReino,
                    Urna &urna,
                    std::condition_variable &urnaApurada,
                    std::mutex &protegeEscrita
                    )
                    {
                        std::unique_lock<std::mutex> lk(protegeEscrita);



                        for (int i = 0; i < N_CANDIDATOS; i++)
                        {
                            int temp = urna.apuraUrna(i);
                            votosGeral.at(i) +=temp;
                            votosPorCidade.at(i) += temp;
                            votosPorReino.at(i) += temp;
                            VOTOS += temp;
                        }
                        URNAS++;


                        lk.unlock();
                        urnaApurada.notify_one();

                    };
        for (Urna &urna : urnas)
        {
            APURA.push_back(std::thread(f,
                                        std::ref(votosGeral),
                                        std::ref(votosPorCidade),
                                        std::ref(votosPorReino),
                                        std::ref(urna),
                                        std::ref(urnaApurada),
                                        std::ref(protegeEscrita)
                                        )
                                    );
                        int tempo = rand() % 119 + 2;
                        auto start = std::chrono::high_resolution_clock::now();
                        std::this_thread::sleep_for(std::chrono::seconds(tempo));
                        auto end = std::chrono::high_resolution_clock::now();
        }
        for (auto &x : APURA)
        {
            x.join();
        }
    }

    void mostra()
    {
        std::unique_lock<std::mutex> lk(mut);
        while (votosDaCidade.empty())
            cv.wait(lk);
        for (int i = 0; i < N_CANDIDATOS; i++)
            std::cout << "Candidato " << i << ": " << votosDaCidade[i] << " votos." << std::endl;
        std::cout << std::endl;
    }
};

class Reino
{
private:
    std::mutex mut;
    std::condition_variable cv;
    std::vector<Cidade> cidades;

public:
    Reino()
    {
        cidades = std::vector<Cidade>(20);
    }

    void apura(std::vector<long> &votosGeral, std::vector<long> &votosPorReino, std::vector<std::vector<long>> &votosPorCidade, std::condition_variable &urnaApurada, std::mutex& protegeEscrita)
    {
        std::vector<std::thread> votPorCidades;
        for (int cidade = 0; cidade < CIDADE_REINO; cidade++)
        {
            votPorCidades.push_back(std::thread([](Cidade &cidade, std::vector<long> &votosGeral,
                                                   std::vector<long> &votosPorReino,
                                                   std::vector<long> &votosPorCidade,
                                                   std::condition_variable &urnaApurada,
                                                   std::mutex& protegeEscrita) {
                cidade.apura(std::ref(votosGeral), std::ref(votosPorReino), std::ref(votosPorCidade), std::ref(urnaApurada), std::ref(protegeEscrita));
            },
                                                std::ref(cidades.at(cidade)),
                                                std::ref(votosGeral),
                                                std::ref(votosPorReino),
                                                std::ref(votosPorCidade.at(cidade)),
                                                std::ref(urnaApurada),
                                                std::ref(protegeEscrita)));
        }
        for (auto &threadLancada : votPorCidades)
        {
            threadLancada.join();
        }
    }
    void votacao()
    {
        std::vector<std::thread> votPorReino;
        for (int i = 0; i < CIDADE_REINO; i++)
        {
            votPorReino.push_back(std::thread([](Cidade &cidade) {
                cidade.votacao();
            },
                                              std::ref(cidades[i])));
        }
        for (auto &threadLancada : votPorReino)
        {
            threadLancada.join();
        }
    }
};

class Eleicao
{
    std::vector<std::vector<std::vector<long>>> votosPorCidade;
    std::vector<std::vector<long>> votosPorReino;
    std::vector<long> votosGeral;
    std::vector<std::vector<int>> maisVotadoPorCidade;
    std::vector<int> maisVotadoPorReino;
    std::vector<Reino> reinos;
    std::mutex impressaoMut;
    std::mutex mut;
    std::mutex protegeEscrita;
    std::condition_variable urnaApurada;
    std::condition_variable valoresAtualizados;
    std::atomic<long> apuradas;

public:
    Eleicao()
    {
        votosGeral = std::vector<long>(7);
        votosPorReino = std::vector<std::vector<long>>(7);
        for (int i = 0; i < N_REINOS; i++)
        {
            votosPorReino[i] = std::vector<long>(7);
            votosPorCidade.emplace_back();
            maisVotadoPorReino.push_back(-1);
            maisVotadoPorCidade.emplace_back();
            for (int j = 0; j < CIDADE_REINO; j++)
            {
                maisVotadoPorCidade[i].push_back(-1);
                votosPorCidade[i].emplace_back();
                for (int c = 0; c < N_CANDIDATOS; c++)
                    votosPorCidade[i][j].push_back(0);
            }
        }
        reinos = std::vector<Reino>(7);
    }

    void votacao()
    {
        std::vector<std::thread> votPorReino;
        for (int i = 0; i < N_REINOS; i++)
        {
            votPorReino.push_back(std::thread([](Reino &reino) {
                reino.votacao();
            },
                                              std::ref(reinos[i])));
        }
        for (auto &threadLancada : votPorReino)
        {
            threadLancada.join();
        }
    }

    void atualizaMaisVotados()
    {
        std::unique_lock<std::mutex> ul (protegeEscrita);
        urnaApurada.wait(ul);
            for (int reino = 0; reino < N_REINOS; reino++)
            {
                maisVotadoPorReino[reino] = std::find(votosPorReino[reino].begin(),
                                                       votosPorReino[reino].end(),
                                                       *std::max_element(votosPorReino[reino].begin(),
                                                       votosPorReino[reino].end()))
                                                       - votosPorReino[reino].begin();

                for (int cidade = 0; cidade < CIDADE_REINO; cidade++)
                {
                    maisVotadoPorCidade[reino][cidade] = std::find(votosPorCidade[reino][cidade].begin(),
                                                                    votosPorCidade[reino][cidade].end(),
                                                                    *std::max_element(votosPorCidade[reino][cidade].begin(), votosPorCidade[reino][cidade].end()))
                                                            -votosPorCidade[reino][cidade].begin();
                }
            }
            //valoresAtualizados.notify_one();

    }
    void apura()
    {
        std::vector<std::thread> votPorReino;
        for (int reino = 0; reino < N_REINOS; reino++)
        {
            votPorReino.push_back(std::thread([](Reino &reino, std::vector<long>& votosGeral,
                                                 std::vector<long> &votosPorReino,
                                                 std::vector<std::vector<long>> &votosPorCidade,
                                                 std::condition_variable &urnaApurada,
                                                 std::mutex& protegeEscrita) {
                reino.apura(std::ref(votosGeral), std::ref(votosPorReino), std::ref(votosPorCidade), std::ref(urnaApurada), std::ref(protegeEscrita));
            },
                                              std::ref(reinos.at(reino)),
                                              std::ref(votosGeral),
                                              std::ref(votosPorReino[reino]),
                                              std::ref(votosPorCidade.at(reino)),
                                              std::ref(urnaApurada),
                                              std::ref(protegeEscrita)));
        }
        for (auto &threadLancada : votPorReino)
        {
            threadLancada.join();
        }
    }

    void mostraParcial()
    {
        while (URNAS < URNAS_CIDADE * CIDADE_REINO * N_REINOS)
        {
            atualizaMaisVotados();
            system("clear");
            std::cout << "\nTOTAL de votos: " << TOTAL<<"\tUrnas apuradas: " << URNAS<<"\tVotos apurados: "<<VOTOS;
            std::cout <<std::endl<< std::setw(75) << "Classificacao Geral" << std::endl;
            for (int i = 0; i < 7; i++)
            {
                std::cout << std::setw(60) << "Candidato: " << i << std::setw(20) << votosGeral.at(i) << std::endl;
            }

            std::cout << std::endl
                      << "Reino 1: " << maisVotadoPorReino[0] << std::setw(23)
                      << "Reino 2: " << maisVotadoPorReino[1] << std::setw(23)
                      << "Reino 3: " << maisVotadoPorReino[2] << std::setw(23)
                      << "Reino 4: " << maisVotadoPorReino[3] << std::setw(23)
                      << "Reino 5: " << maisVotadoPorReino[4] << std::setw(23)
                      << "Reino 6: " << maisVotadoPorReino[5] << std::setw(23)
                      << "Reino 7: " << maisVotadoPorReino[6] << std::endl;

            for (int i = 0; i < 20; i++)
            {
                std::cout << "Cidade " << std::setw(2) << i << ": " << std::setw(5) << maisVotadoPorCidade[0].at(i) << std::setw(14)
                          << "Cidade " << std::setw(2) << i << ": " << std::setw(5) << maisVotadoPorCidade[1].at(i) << std::setw(14)
                          << "Cidade " << std::setw(2) << i << ": " << std::setw(5) << maisVotadoPorCidade[2].at(i) << std::setw(14)
                          << "Cidade " << std::setw(2) << i << ": " << std::setw(5) << maisVotadoPorCidade[3].at(i) << std::setw(14)
                          << "Cidade " << std::setw(2) << i << ": " << std::setw(5) << maisVotadoPorCidade[4].at(i) << std::setw(14)
                          << "Cidade " << std::setw(2) << i << ": " << std::setw(5) << maisVotadoPorCidade[5].at(i) << std::setw(14)
                          << "Cidade " << std::setw(2) << i << ": " << std::setw(5) << maisVotadoPorCidade[6].at(i) << std::endl;
            }
        }
    }
};
int main()
{
    srand(time(NULL));
    Eleicao a;
    std::thread vota ([](Eleicao& a){
        a.votacao();
    }, std::ref(a));
    vota.join();
    std::cout << "VOTACAO ENCERRADA";
    //sleep(5);
    std::thread apura ([](Eleicao& a){
        a.apura();
    }, std::ref(a));
    /*std::thread att ([](Eleicao& a){
        a.atualizaMaisVotados();
    }, std::ref(a));*/
    std::thread mostra ([](Eleicao& a){
        a.mostraParcial();
    }, std::ref(a));
    apura.join();
    //att.join();
    mostra.join();
    //a.apura();
    //a.mostraGeral();
    //a.mostraParcial();
    return 0;
}
