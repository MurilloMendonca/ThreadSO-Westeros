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
