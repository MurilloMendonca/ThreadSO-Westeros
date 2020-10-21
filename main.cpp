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
#include<random>



#define N_CANDIDATOS 7
#define URNAS_CIDADE 5
#define CIDADE_REINO 20
#define N_REINOS 7



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
    void vota()
    {
        std::unique_lock<std::mutex> ul(mut);           //Trava operacoes nesta urna


        std::random_device rd;                          //Gera numeros aleatorios com base em
        std::mt19937 gen(rd());                         //uma distribuicao uniforme de 
        std::uniform_int_distribution<int> dis(0, 6);   //probabilidade


        votos.at(dis(gen))++;                           //Registra voto para um candidato nesta urna
        TOTAL++;                                        //Atualiza a quantidade de votos geral

        ul.unlock();                                    //Libera operacoes nesta urna
    }

    long apuraUrna(int candidato)
    {
        std::unique_lock<std::mutex> ul(mut);           //Trava operacoes nesta urna
        return votos[candidato];                        //Retorna quantos votos o candidato tem registrados nesta urna
    }
};

class Cidade
{
private:
    std::vector<Urna> urnas;
    std::atomic<int> populacao;

public:
    Cidade()
    {
        urnas = std::vector<Urna>(URNAS_CIDADE);        //Inicializa vetor de cidades

        std::random_device rd;                          
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(1000, 30000);

        populacao = dis(gen);                           //Define o tamanho da populacao desta cidade
    }
    void votacao()
    {
        std::vector<std::thread> threadPorUrna;         
        std::atomic<int> votos;                         //Contador de votos realizados

        for (Urna &urna : urnas)                        //Itera para cada urna da cidade
        {
            threadPorUrna.push_back(std::thread([](Urna &urna, std::atomic<int> &populacao, std::atomic<int> &votos) {
                while (votos < populacao)               //Enquanto toda a populacao ainda nao votou
                {
                    votos++;                            //Registra um novo voto
                    urna.vota();                        //Vota
                }
            },
                                                std::ref(urna), std::ref(populacao), std::ref(votos)));
        }

        for (auto &threadLancada : threadPorUrna)       //Para cada thread lançada e armazenada em threadPoUrna
        {
            threadLancada.join();                       //Aguarda a thread finalizar para proseguir 
        }
    }


    void apura(std::vector<long> &votosGeral, std::vector<long> &votosPorReino, std::vector<long> &votosPorCidade, std::condition_variable &urnaApurada, std::mutex& protegeEscrita)
    {


        std::vector<std::thread> apuraPorUrna;

        auto apuracao = [](
                    std::vector<long> &votosGeral,
                    std::vector<long> &votosPorCidade,
                    std::vector<long> &votosPorReino,
                    Urna &urna,
                    std::condition_variable &urnaApurada,
                    std::mutex &protegeEscrita
                    )
                    {
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<int> dis(2, 120);

                        int tempo = dis(gen);                                       //define o tempo aleatorio de apuracao
                        std::this_thread::sleep_for(std::chrono::seconds(tempo));   //espera o tempo de apuracao
                        
                        for (int candidato = 0; candidato < N_CANDIDATOS; candidato++)                      //para cada candidato
                        {
                            std::unique_lock<std::mutex> lk(protegeEscrita);        //Trava o acesso para escrever no vetores
                            int temp = urna.apuraUrna(candidato);                   //votos para o candidato nesta urna
                            votosGeral.at(candidato) +=temp;                        //Contabiliza a nivel geral para o candidato
                            votosPorCidade.at(candidato) += temp;                   //Contabiliza a nivel de cidade para o candidato
                            votosPorReino.at(candidato) += temp;                    //Contabiliza a nivel de reino para o candidato
                            VOTOS += temp;                                          //Contabiliza a quantidade total de votos apurados
                            lk.unlock();                                            //Destrava o acesso
                        }
                        URNAS++;                                                    //Contabiliza a quantidade total de urnas apurados                           
                        
                        urnaApurada.notify_one();                                   //Notifica que uma urna foi apurada

                    };

        for (Urna &urna : urnas)                                                    //Para cada urna
        {
            
            apuraPorUrna.push_back(std::thread(apuracao,                            //lanca a apuracao
                                        std::ref(votosGeral),
                                        std::ref(votosPorCidade),
                                        std::ref(votosPorReino),
                                        std::ref(urna),
                                        std::ref(urnaApurada),
                                        std::ref(protegeEscrita)
                                        )
                                    );
        }

        for (auto &threadLancada : apuraPorUrna)                //Para cada thread lançada e armazenada em apuraPoUrna
        {
            threadLancada.join();                               //Aguarda a thread finalizar para proseguir 
        }
    }
};

class Reino
{
private:
    std::vector<Cidade> cidades;

public:
    Reino()
    {
        cidades = std::vector<Cidade>(20);                      //Inicializa o vetor de cidades
    }

    void apura(std::vector<long> &votosGeral, std::vector<long> &votosPorReino, std::vector<std::vector<long>> &votosPorCidade, std::condition_variable &urnaApurada, std::mutex& protegeEscrita)
    {
        std::vector<std::thread> votPorCidades;
        for (int cidade = 0; cidade < CIDADE_REINO; cidade++)   //Para cada cidade em um reino
        {
            votPorCidades.push_back(std::thread([](Cidade &cidade, std::vector<long> &votosGeral,
                                                   std::vector<long> &votosPorReino,
                                                   std::vector<long> &votosPorCidade,
                                                   std::condition_variable &urnaApurada,
                                                   std::mutex& protegeEscrita) {
                //Apura votos em cada cidade
                cidade.apura(std::ref(votosGeral), std::ref(votosPorReino), std::ref(votosPorCidade), std::ref(urnaApurada), std::ref(protegeEscrita));
            },
                                                std::ref(cidades.at(cidade)),
                                                std::ref(votosGeral),
                                                std::ref(votosPorReino),
                                                std::ref(votosPorCidade.at(cidade)),
                                                std::ref(urnaApurada),
                                                std::ref(protegeEscrita)));
        }

        for (auto &threadLancada : votPorCidades)               //Para cada thread lançada e armazenada em apuraPoUrna
        {
            threadLancada.join();                               //Aguarda a thread finalizar para proseguir 
        }
    }


    void votacao()
    {
        std::vector<std::thread> votPorReino;
        for (int i = 0; i < CIDADE_REINO; i++)                  //Para cada cidade em um reino
        {
            votPorReino.push_back(std::thread([](Cidade &cidade) {
                cidade.votacao();                               //realiza votacao na cidade
            },
                                              std::ref(cidades[i])));
        }
        for (auto &threadLancada : votPorReino)                 //Para cada thread lançada e armazenada em apuraPoUrna
        {
            threadLancada.join();                               //Aguarda a thread finalizar para proseguir 
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
    Eleicao westeros;
    std::cout << "VOTACAO INICIADA... aguarde";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::thread vota ([](Eleicao& a){
        a.votacao();
    }, std::ref(westeros));

    vota.join();

    std::cout << "\nVOTACAO ENCERRADA\n A APURACAO SE INICIARA EM BREVE";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::thread apura ([](Eleicao& a){
        a.apura();
    }, std::ref(westeros));
    std::thread mostra ([](Eleicao& a){
        a.mostraParcial();
    }, std::ref(westeros));

    apura.join();
    mostra.join();

    return 0;
}
