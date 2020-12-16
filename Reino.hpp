class Reino
{
private:
    std::vector<Cidade> cidades; //Vector que guarda cada cidade deste reino

public:
    Reino()
    {
        cidades = std::vector<Cidade>(20); //Inicializa o vetor de cidades
    }

    void apura(std::vector<long> &votosGeral, std::vector<long> &votosPorReino, std::vector<std::vector<long>> &votosPorCidade, std::condition_variable &cond_escr,
                                                 std::condition_variable &cond_leit,
                                                 int &leitores,
                                                 int &escritores,
                                                 std::mutex &lock_cont)
    {
        std::vector<std::thread> votPorCidades;
        for (int cidade = 0; cidade < CIDADE_REINO; cidade++) //Para cada cidade em um reino
        {
            votPorCidades.push_back(std::thread([](Cidade &cidade, std::vector<long> &votosGeral,
                                                   std::vector<long> &votosPorReino,
                                                   std::vector<long> &votosPorCidade,
                                                   std::condition_variable &cond_escr,
                                                   std::condition_variable &cond_leit,
                                                   int &leitores,
                                                   int &escritores,
                                                   std::mutex &lock_cont) {
                //Apura votos em cada cidade
                cidade.apura(std::ref(votosGeral), std::ref(votosPorReino), std::ref(votosPorCidade), std::ref(cond_escr), std::ref(cond_leit),  std::ref(leitores), std::ref(escritores), std::ref(lock_cont));
            },
                                                std::ref(cidades.at(cidade)),
                                                std::ref(votosGeral),
                                                std::ref(votosPorReino),
                                                std::ref(votosPorCidade.at(cidade)),
                                                std::ref(cond_escr),
                                                std::ref(cond_leit),
                                                std::ref(leitores),
                                                std::ref(escritores),
                                                std::ref(lock_cont)));
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
