class Eleicao
{
private:
    std::vector<std::vector<std::vector<long>>> votosPorCidade; //Vector que guarda os votos de cada candidato por cidade [reino] [cidade] [candidato]
    std::vector<std::vector<long>> votosPorReino;               //Vector que guarda os votos de cada candidato por reino [reino] [candidato]
    std::vector<long> votosGeral;                               //Vector que guarda a quantidade de votos de cada candidato [considato]
    std::vector<std::vector<char>> maisVotadoPorCidade;         //Vector que guarda os condidatos mais votados por cidade [reino] [cidade]
    std::vector<char> maisVotadoPorReino;                       //Vector que guarda os condidatos mais votados por reino [reino]
    std::vector<Reino> reinos;                                  //Vector de reinos
    std::mutex printMut;                                    //Leitores
    std::mutex protegeEscrita;                                  //Semaforo para escritas nos vectors
    std::mutex lock_cont; 
    std::condition_variable cond_escr;                        //avisa que uma urna foi apurada
    std::condition_variable cond_leit;                      //avisa que os leitores podem ser ativados
    std::atomic<long> apuradas;                                 //Contador de urnas apuradas
    int leitores;
    int escritores;
    int leitoresMax;

public:
    Eleicao(int Nleitores)
    {
        leitoresMax=Nleitores;
        leitores =0;
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

    void mostraParcial()
    {
        while(true){
        std::unique_lock<std::mutex> ul(lock_cont);    //Trava o semaforo para ler os vetores
        while(escritores>0)
            cond_leit.wait(ul);
        leitores++;
        ul.unlock();

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
        printMut.lock();
        system("clear");                                                     //Limpa tela
            std::cout << "Imprimindo da thread: " << std::this_thread::get_id(); //Mostra qual thread esta imprimindo
            std::cout << "\tNumero de leitores ativos: " << leitores;
            std::cout << "\tNumero de escritores ativos: " << escritores;
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
        printMut.unlock();

        ul.lock();
        leitores--;
        if(leitores==0) cond_escr.notify_all();
        ul.unlock();
        }
    }
    void apura()
    {
        std::vector<std::thread> votPorReino;
        cond_escr.notify_one();
        for (int reino = 0; reino < N_REINOS; reino++)
        {
            votPorReino.push_back(std::thread([](Reino &reino,
                                                 std::vector<long> &votosGeral,
                                                 std::vector<long> &votosPorReino,
                                                 std::vector<std::vector<long>> &votosPorCidade,
                                                 std::condition_variable &cond_escr,
                                                 std::condition_variable &cond_leit,
                                                 int &leitores,
                                                 int &escritores,
                                                 std::mutex &lock_cont) {
                //Chama a apuracao de cada reino
                reino.apura(std::ref(votosGeral), std::ref(votosPorReino), std::ref(votosPorCidade), std::ref(cond_escr), std::ref(cond_leit),  std::ref(leitores), std::ref(escritores), std::ref(lock_cont));
            },
                                              std::ref(reinos.at(reino)),
                                              std::ref(votosGeral),
                                              std::ref(votosPorReino[reino]),
                                              std::ref(votosPorCidade.at(reino)),
                                              std::ref(cond_escr),
                                              std::ref(cond_leit),
                                              std::ref(leitores),
                                              std::ref(escritores),
                                              std::ref(lock_cont)
                                              ));
        }
        for (auto &threadLancada : votPorReino) //Para cada thread lançada e armazenada em apuraPoUrna
        {
            threadLancada.join(); //Aguarda a thread finalizar para proseguir
        }
        cond_escr.notify_all(); //Notifica para avisar todas as outras threads de que a apuracao foi finalizada
    }

};
