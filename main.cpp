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
#include <random>

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
    std::mutex protegeUrna;  //Semaforo para proteger escritas e leituras no vector votos
    std::vector<long> votos; //Vector que guarda os votos registrados nesta urna

public:
    Urna()
    {
        votos = std::vector<long>(N_CANDIDATOS); //Inicializa vector de votos
    }

    void vota(int candidato)
    {
        std::unique_lock<std::mutex> ul(protegeUrna); //Trava operacoes nesta urna

        votos.at(candidato)++; //Registra voto para um candidato nesta urna
        TOTAL++;               //Atualiza a quantidade de votos geral

        ul.unlock(); //Libera operacoes nesta urna
    }

    long apuraUrna(int candidato)
    {
        std::unique_lock<std::mutex> ul(protegeUrna); //Trava operacoes nesta urna
        return votos[candidato];                      //Retorna quantos votos o candidato tem registrados nesta urna
    }
};

class Cidade
{
private:
    std::vector<Urna> urnas;    //Vector que guarda cada urna desta cidade
    std::atomic<int> populacao; //Quantidade de habitantes desta cidade
    std::mutex mut;
    std::random_device rd;                      //Dispositivo gerador aleatorio
    std::mt19937 gen;                           //Gerador aleatorio
    std::discrete_distribution<int> tendencias; //Distribuicao discreta que gera votos aleatorios com base nas tendencias desta cidade

public:
    Cidade()
    {
        urnas = std::vector<Urna>(URNAS_CIDADE); //Inicializa vetor de cidades

        gen = std::mt19937(rd());
        std::uniform_int_distribution<int> popProb(1000, 30000);
        populacao = popProb(gen); //Define o tamanho da populacao desta cidade

        std::uniform_int_distribution<int> dis(1, 9);
        tendencias = std::discrete_distribution<int>{(double)dis(gen), //Gera as tendencias para esta cidade
                                                     (double)dis(gen),
                                                     (double)dis(gen),
                                                     (double)dis(gen),
                                                     (double)dis(gen),
                                                     (double)dis(gen),
                                                     (double)dis(gen)};
    }
    void votacao(std::mutex &protegeEscrita)
    {
        std::vector<std::thread> threadPorUrna;
        std::atomic<int> votos; //Contador de votos realizados

        for (Urna &urna : urnas) //Itera para cada urna da cidade
        {
            threadPorUrna.push_back(std::thread([](Urna &urna, std::atomic<int> &populacao, std::atomic<int> &votos,
                                                   std::mt19937 &gen, std::discrete_distribution<int> &prob, std::mutex &mut) {
                
                while (votos < populacao) //Enquanto toda a populacao ainda nao votou
                {
                    std::unique_lock<std::mutex> ul (mut);
                    urna.vota(prob(gen)); //Vota
                    votos++;              //Registra um novo voto
                    ul.unlock();
                }
            },
                                                std::ref(urna),
                                                std::ref(populacao),
                                                std::ref(votos),
                                                std::ref(gen),
                                                std::ref(tendencias),
                                                std::ref(protegeEscrita)));
        }

        for (auto &threadLancada : threadPorUrna) //Para cada thread lançada e armazenada em threadPoUrna
        {
            threadLancada.join(); //Aguarda a thread finalizar para proseguir
        }
    }

    void apura(std::vector<long> &votosGeral, std::vector<long> &votosPorReino, std::vector<long> &votosPorCidade, std::condition_variable &urnaApurada, std::mutex &protegeEscrita)
    {

        std::vector<std::thread> apuraPorUrna;

        auto apuracao = [](
                            std::vector<long> &votosGeral,
                            std::vector<long> &votosPorCidade,
                            std::vector<long> &votosPorReino,
                            Urna &urna,
                            std::condition_variable &urnaApurada,
                            std::mutex &protegeEscrita) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dis(2, 120);

            int tempo = dis(gen);                                     //define o tempo aleatorio de apuracao
            std::this_thread::sleep_for(std::chrono::seconds(tempo)); //espera o tempo de apuracao

            for (int candidato = 0; candidato < N_CANDIDATOS; candidato++) //para cada candidato
            {
                std::unique_lock<std::mutex> lk(protegeEscrita); //Trava o acesso para escrever no vetores
                int temp = urna.apuraUrna(candidato);            //votos para o candidato nesta urna
                votosGeral.at(candidato) += temp;                //Contabiliza a nivel geral para o candidato
                votosPorCidade.at(candidato) += temp;            //Contabiliza a nivel de cidade para o candidato
                votosPorReino.at(candidato) += temp;             //Contabiliza a nivel de reino para o candidato
                VOTOS += temp;                                   //Contabiliza a quantidade total de votos apurados
                lk.unlock();                                     //Destrava o acesso
            }
            URNAS++; //Contabiliza a quantidade total de urnas apurados

            urnaApurada.notify_all(); //Notifica que uma urna foi apurada
                                        //Para escritas mais legiveis na tela pode-se usar .notify_one()
                                        //Nao foi usado neste caso pois apos apurar cada urna deve-se avisar todas as
                                        //telas leitoras
        };

        for (Urna &urna : urnas) //Para cada urna
        {

            apuraPorUrna.push_back(std::thread(apuracao, //lanca a apuracao
                                               std::ref(votosGeral),
                                               std::ref(votosPorCidade),
                                               std::ref(votosPorReino),
                                               std::ref(urna),
                                               std::ref(urnaApurada),
                                               std::ref(protegeEscrita)));
        }

        for (auto &threadLancada : apuraPorUrna) //Para cada thread lançada e armazenada em apuraPoUrna
        {
            threadLancada.join(); //Aguarda a thread finalizar para proseguir
        }
    }
    long getPopulacao()
    {
        return populacao;
    }
};

class Reino
{
private:
    std::vector<Cidade> cidades; //Vector que guarda cada cidade deste reino

public:
    Reino()
    {
        cidades = std::vector<Cidade>(20); //Inicializa o vetor de cidades
    }

    void apura(std::vector<long> &votosGeral, std::vector<long> &votosPorReino, std::vector<std::vector<long>> &votosPorCidade, std::condition_variable &urnaApurada, std::mutex &protegeEscrita)
    {
        std::vector<std::thread> votPorCidades;
        for (int cidade = 0; cidade < CIDADE_REINO; cidade++) //Para cada cidade em um reino
        {
            votPorCidades.push_back(std::thread([](Cidade &cidade, std::vector<long> &votosGeral,
                                                   std::vector<long> &votosPorReino,
                                                   std::vector<long> &votosPorCidade,
                                                   std::condition_variable &urnaApurada,
                                                   std::mutex &protegeEscrita) {
                //Apura votos em cada cidade
                cidade.apura(std::ref(votosGeral), std::ref(votosPorReino), std::ref(votosPorCidade), std::ref(urnaApurada), std::ref(protegeEscrita));
            },
                                                std::ref(cidades.at(cidade)), std::ref(votosGeral), std::ref(votosPorReino), std::ref(votosPorCidade.at(cidade)), std::ref(urnaApurada), std::ref(protegeEscrita)));
        }

        for (auto &threadLancada : votPorCidades) //Para cada thread lançada e armazenada em apuraPoUrna
        {
            threadLancada.join(); //Aguarda a thread finalizar para proseguir
        }
    }

    void votacao(std::mutex &protegeEscrita)
    {
        std::vector<std::thread> votPorReino;
        for (int i = 0; i < CIDADE_REINO; i++) //Para cada cidade em um reino
        {
            votPorReino.push_back(std::thread([](Cidade &cidade, std::mutex &protegeEscrita) {
                cidade.votacao(std::ref(protegeEscrita)); //realiza votacao na cidade
            },
                                              std::ref(cidades.at(i)), std::ref(protegeEscrita)));
        }
        for (auto &threadLancada : votPorReino) //Para cada thread lançada e armazenada em apuraPoUrna
        {
            threadLancada.join(); //Aguarda a thread finalizar para proseguir
        }
    }
    long getPopulacao()
    {
        long pop = 0;
        for (Cidade &cid : cidades)
            pop += cid.getPopulacao();
        return pop;
    }
};

class Eleicao
{
private:
    std::vector<std::vector<std::vector<long>>> votosPorCidade; //Vector que guarda os votos de cada candidato por cidade [reino] [cidade] [candidato]
    std::vector<std::vector<long>> votosPorReino;               //Vector que guarda os votos de cada candidato por reino [reino] [candidato]
    std::vector<long> votosGeral;                               //Vector que guarda a quantidade de votos de cada candidato [considato]
    std::vector<std::vector<char>> maisVotadoPorCidade;         //Vector que guarda os condidatos mais votados por cidade [reino] [cidade]
    std::vector<char> maisVotadoPorReino;                       //Vector que guarda os condidatos mais votados por reino [reino]
    std::vector<Reino> reinos;                                  //Vector de reinos
    std::mutex impressaoMut;                                    //Semaforo para escritas na tela
    std::mutex protegeEscrita;                                  //Semaforo para escritas nos vectors
    std::condition_variable urnaApurada;                        //avisa que uma urna foi apurada
    std::atomic<long> apuradas;                                 //Contador de urnas apuradas
    long populacaoTotal;

public:
    Eleicao()
    {
        votosGeral = std::vector<long>(N_CANDIDATOS); //Inicializa os vectors
        votosPorReino = std::vector<std::vector<long>>(N_REINOS);
        for (int i = 0; i < N_REINOS; i++)
        {
            votosPorReino[i] = std::vector<long>(N_CANDIDATOS);
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
        reinos = std::vector<Reino>(N_REINOS);
    }

    void votacao()
    {
        std::vector<std::thread> votPorReino; //Vector de threads
        for (int i = 0; i < N_REINOS; i++)    //Para cada reino
        {
            votPorReino.push_back(std::thread([](Reino &reino, std::mutex &protegeEscrita) {
                reino.votacao(std::ref(protegeEscrita)); //realiza votacao em um reino
            },
                                              std::ref(reinos.at(i)), std::ref(protegeEscrita)));
        }
        for (auto &threadLancada : votPorReino) //Para cada thread lançada e armazenada em apuraPoUrna
        {
            threadLancada.join(); //Aguarda a thread finalizar para proseguir
        }
    }

    void atualizaMaisVotados()
    {
        std::unique_lock<std::mutex> ul(protegeEscrita); //Trava o semaforo para ler os vetores
        urnaApurada.wait(ul);                            //Espera notificacao de urna apurada

        for (int reino = 0; reino < N_REINOS; reino++) //Para cada reino
        {
            bool zeros = std::all_of(votosPorReino[reino].begin(),
                                     votosPorReino[reino].end(),
                                     [](long i) { return i == 0; });
            if (zeros)
                maisVotadoPorReino[reino] = '*';
            else
                maisVotadoPorReino[reino] =48 + std::max_element(votosPorReino[reino].begin(), //Pega o maior elemento deste vector
                                                             votosPorReino[reino].end()) -
                                            votosPorReino[reino].begin();

            for (int cidade = 0; cidade < CIDADE_REINO; cidade++) //Para cada cidade
            {
                bool zeros = std::all_of(votosPorCidade[reino][cidade].begin(),
                                         votosPorCidade[reino][cidade].end(),
                                         [](long i) { return i == 0; });
                if (zeros)
                    maisVotadoPorCidade[reino][cidade] ='*';
                else
                    maisVotadoPorCidade[reino][cidade] = 48 + std::max_element(votosPorCidade[reino][cidade].begin(),
                                                                          votosPorCidade[reino][cidade].end()) -
                                                         votosPorCidade[reino][cidade].begin();
            }
        }
    }
    void apura()
    {
        std::vector<std::thread> votPorReino;
        for (int reino = 0; reino < N_REINOS; reino++)
        {
            votPorReino.push_back(std::thread([](Reino &reino, std::vector<long> &votosGeral,
                                                 std::vector<long> &votosPorReino,
                                                 std::vector<std::vector<long>> &votosPorCidade,
                                                 std::condition_variable &urnaApurada,
                                                 std::mutex &protegeEscrita) {
                //Chama a apuracao de cada reino
                reino.apura(std::ref(votosGeral), std::ref(votosPorReino), std::ref(votosPorCidade), std::ref(urnaApurada), std::ref(protegeEscrita));
            },
                                              std::ref(reinos.at(reino)), std::ref(votosGeral), std::ref(votosPorReino[reino]), std::ref(votosPorCidade.at(reino)), std::ref(urnaApurada), std::ref(protegeEscrita)));
        }
        for (auto &threadLancada : votPorReino) //Para cada thread lançada e armazenada em apuraPoUrna
        {
            threadLancada.join(); //Aguarda a thread finalizar para proseguir
        }
        urnaApurada.notify_all(); //Notifica para avisar todas as outras threads de que a apuracao foi finalizada
    }
    void mostraParcial()
    {
        while (URNAS < URNAS_CIDADE * CIDADE_REINO * N_REINOS) //Enquanto faltam urnas para apurar
        {

            atualizaMaisVotados();                         //Atualiza os candidatos mais votados
            std::unique_lock<std::mutex> ul(impressaoMut); //Bloqueia outras threads de impressao

            system("clear");                                                     //Limpa tela
            std::cout << "Imprimindo da thread: " << std::this_thread::get_id(); //Mostra qual thread esta imprimindo

            float urnaPercent = 100 * URNAS / (URNAS_CIDADE * CIDADE_REINO * N_REINOS); //Calcula a porcentagem de urnas já apuradas
            //BARRA DE CARREGAMENTO
            std::cout << "\n[";
            for (int i = 0; i < 100; i++)
            {
                if (urnaPercent > i)
                    std::cout << "#";
                else
                    std::cout << ".";
            }
            std::cout << "]";
            std::cout << "\tVotos apurados: " << 100 * VOTOS / TOTAL << "%"
                      << "\tUrnas apuradas: " << urnaPercent << "%";

            //MOSTRA CLASSIFICACAO GERAL
            std::cout << std::endl
                      << std::setw(75) << "Classificacao Geral" << std::endl;
            for (int i = 0; i < N_CANDIDATOS; i++) //Para cada candidato
            {
                std::cout << std::setw(60) << "Candidato: " << i << std::setw(20) << votosGeral.at(i) << " -> " << 100.0 * votosGeral.at(i) / VOTOS << "%" << std::endl;
            }

            //MOSTRA MAIS VOTADO EM CADA REINO
            std::cout << std::endl;
            for (int i = 0; i < N_REINOS; i++)
            {
                std::cout << "Reino " << i << ": " << maisVotadoPorReino[i] << std::setw(19);
            }
            std::cout << std::endl
                      << std::setw(0);

            //MOSTRA MAIS VOTADO EM CADA CIDADE
            for (int i = 0; i < CIDADE_REINO; i++) //Para cada cidade
            {
                for (int j = 0; j < N_REINOS; j++) //Para cada reino
                {
                    std::cout << "Cidade " << std::setw(2) << i << ": " << std::setw(5) << maisVotadoPorCidade[j].at(i) << std::setw(14);
                }
                std::cout << std::endl
                          << std::setw(0);
            }

            ul.unlock(); //Desbloqueia o semaforo
        }
    }
};
int main()
{
    Eleicao westeros;                               //Cria eleicao
    std::thread vota([](Eleicao &a) {               //Lanca a thread de votacao
        a.votacao();
    },
                     std::ref(westeros));
    vota.join();                                    //Espera a votacao terminar

    std::cout << "\nVOTACAO ENCERRADA\n A APURACAO SE INICIARA EM BREVE";

    std::thread apura([](Eleicao &a) {              //Lanca a thread de apuracao da eleicao
        a.apura();
    },
                      std::ref(westeros));


    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(100, 500);

    int numMonitores = dis(gen);                        //Sorteia o numeros de threads de leitura             

    std::vector<std::thread> monitores;                 //Cria vector de threads leitoras

    for (int monitor = 0; monitor < numMonitores; monitor++)    //Para cada monitor
    {
        monitores.push_back(std::thread([](Eleicao &a) {
            a.mostraParcial();
        },
                                        std::ref(westeros)));
    }
    apura.join();                                       //Aguarda a apuracao terminar
    for (auto &monitor : monitores)                     //Para cada monitor
        monitor.join();                                 //Aguarda o monitor terminar

    return 0;
}
