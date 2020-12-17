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
#include "Urna.hpp"
#include"Cidade.hpp"
#include "Reino.hpp"
#include "Eleicao.hpp"




int main()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(100, 500);

    int numMonitores = dis(gen); 
    Eleicao westeros(numMonitores);                               //Cria eleicao
    std::thread vota([](Eleicao &a) {               //Lanca a thread de votacao
        a.votacao();
    },
                     std::ref(westeros));
    vota.join();                                    //Espera a votacao terminar

    std::cout << "\nVOTACAO ENCERRADA\n A APURACAO SE INICIARA EM BREVE";
    std::cout.flush();

    std::thread apura([](Eleicao &a) {              //Lanca a thread de apuracao da eleicao
        a.apura();
    },
                      std::ref(westeros));

          
    std::vector<std::thread> monitores;  
    while(!westeros.finalizada){
         monitores.clear();          //Cria vector de threads leitoras
    
    for (int monitor = 0; monitor < numMonitores; monitor++)    //Para cada monitor
    {
        monitores.push_back(std::thread([](Eleicao &a) {
            a.mostraParcial();
        },
                                        std::ref(westeros)));
    }
                                     //Aguarda a apuracao terminar
    for (auto &monitor : monitores)                     //Para cada monitor
        monitor.join();                                 //Aguarda o monitor terminar
    }
    apura.join();
    return 0;
}
