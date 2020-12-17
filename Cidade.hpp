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

    void apura(std::vector<long> &votosGeral, std::vector<long> &votosPorReino, std::vector<long> &votosPorCidade,
                                                 std::mutex &protegeEscrita)
    {

        std::vector<std::thread> apuraPorUrna;

        auto apuracao = []( std::vector<long> &votosGeral,
                            std::vector<long> &votosPorCidade,
                            std::vector<long> &votosPorReino,
                            Urna &urna,
                            std::mutex &protegeEscrita) {
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dis(2, 120);
            
            int tempo = dis(gen);                                     //define o tempo aleatorio de apuracao
            std::this_thread::sleep_for(std::chrono::seconds(tempo)); //espera o tempo de apuracao
            
            protegeEscrita.lock();                      //Trava mutex de escrita
            //ESCREVE
            {
            for (int candidato = 0; candidato < N_CANDIDATOS; candidato++) //para cada candidato
            {
                
                int temp = urna.apuraUrna(candidato);            //votos para o candidato nesta urna
                votosGeral.at(candidato) += temp;                //Contabiliza a nivel geral para o candidato
                votosPorCidade.at(candidato) += temp;            //Contabiliza a nivel de cidade para o candidato
                votosPorReino.at(candidato) += temp;             //Contabiliza a nivel de reino para o candidato
                VOTOS += temp;                                 //Contabiliza a quantidade total de votos apurados       
                                          //Destrava o acesso
            }
            URNAS++; //Contabiliza a quantidade total de urnas apurados
            }
            protegeEscrita.unlock();                    //Libera mutex de escrita
            
            
            
        };

        for (Urna &urna : urnas) //Para cada urna
        {

            apuraPorUrna.push_back(std::thread(apuracao, //lanca a apuracao
                                               std::ref(votosGeral),
                                               std::ref(votosPorCidade),
                                               std::ref(votosPorReino),
                                               std::ref(urna),
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
