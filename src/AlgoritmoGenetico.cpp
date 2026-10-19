#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const int NUM_REINAS = 8;

typedef array<int, NUM_REINAS> Tablero;

class GeneradorAleatorio {
public:
    explicit GeneradorAleatorio(uint32_t semilla = 1) : motor(semilla) {}

    void sembrar(uint32_t semilla) { motor.seed(semilla); }

    int entero(int minimo, int maximo) {
        uint32_t cantidadValores = (uint32_t)(maximo - minimo + 1);
        uint32_t limiteAceptado = 0xFFFFFFFFu - (0xFFFFFFFFu % cantidadValores);
        uint32_t numeroGenerado;
        do {
            numeroGenerado = (uint32_t)motor();
        } while (numeroGenerado >= limiteAceptado);
        return minimo + (int)(numeroGenerado % cantidadValores);
    }

    double real() { return (uint32_t)motor() / 4294967296.0; }

    void permutacionAleatoria(Tablero& tablero) {
        for (int fila = 0; fila < NUM_REINAS; fila++) tablero[fila] = fila;
        for (int fila = NUM_REINAS - 1; fila > 0; fila--) swap(tablero[fila], tablero[entero(0, fila)]);
    }

private:
    mt19937 motor;
};

int contarConflictos(const Tablero& tablero) {
    int conflictos = 0;
    for (int filaA = 0; filaA < NUM_REINAS; filaA++)
        for (int filaB = filaA + 1; filaB < NUM_REINAS; filaB++) {
            bool mismaColumna = tablero[filaA] == tablero[filaB];
            bool mismaDiagonal = abs(tablero[filaA] - tablero[filaB]) == filaB - filaA;
            if (mismaColumna || mismaDiagonal) conflictos++;
        }
    return conflictos;
}

class FuncionObjetivo {
public:
    explicit FuncionObjetivo(long limiteEvaluaciones) : maximoEvaluaciones(limiteEvaluaciones), evaluacionesUsadas(0) {}

    int evaluar(const Tablero& tablero) {
        evaluacionesUsadas++;
        return contarConflictos(tablero);
    }

    long usadas() const { return evaluacionesUsadas; }
    bool agotada() const { return evaluacionesUsadas >= maximoEvaluaciones; }

private:
    long maximoEvaluaciones;
    long evaluacionesUsadas;
};

struct Resultado {
    bool encontroSolucion = false;
    int conflictos = 0;
    long evaluaciones = 0;
    Tablero mejorTablero{};
};

string configuracionTexto(const Tablero& tablero) {
    ostringstream texto;
    texto << "[";
    for (int fila = 0; fila < NUM_REINAS; fila++) texto << tablero[fila] << (fila < NUM_REINAS - 1 ? " " : "");
    texto << "]";
    return texto.str();
}

void imprimirTablero(const Tablero& tablero) {
    cout << "     ";
    for (int columna = 0; columna < NUM_REINAS; columna++) cout << columna << " ";
    cout << "\n   +" << string(2 * NUM_REINAS + 1, '-') << "+\n";
    for (int fila = 0; fila < NUM_REINAS; fila++) {
        cout << " " << fila << " | ";
        for (int columna = 0; columna < NUM_REINAS; columna++) cout << (tablero[fila] == columna ? "Q " : ". ");
        cout << "|\n";
    }
    cout << "   +" << string(2 * NUM_REINAS + 1, '-') << "+\n";
}

struct Opciones {
    uint32_t semillaInicial = 1;
    int numeroEjecuciones = 1;
    long maximoEvaluaciones = 10000;
};

template <class FuncionEjecutar>
void correrExperimento(const string& nombreAlgoritmo, const Opciones& opciones, FuncionEjecutar ejecutarAlgoritmo) {
    int ejecucionesExitosas = 0, ejecucionesFallidas = 0;
    long sumaEvaluacionesExitosas = 0, mejorEvaluaciones = -1, peorEvaluaciones = -1, sumaConflictosFallidas = 0;
    set<Tablero> solucionesDistintas;
    vector<Resultado> resultados;

    cout << "Algoritmo: " << nombreAlgoritmo << " | ejecuciones: " << opciones.numeroEjecuciones
         << " | presupuesto: " << opciones.maximoEvaluaciones << " evaluaciones\n\n";

    for (int ejecucion = 1; ejecucion <= opciones.numeroEjecuciones; ejecucion++) {
        uint32_t semilla = opciones.semillaInicial + (uint32_t)(ejecucion - 1);
        Resultado resultado = ejecutarAlgoritmo(semilla, opciones.maximoEvaluaciones);
        resultados.push_back(resultado);

        cout << "Ejecucion " << setw(2) << ejecucion << " | semilla " << setw(4) << semilla
             << " | solucion: " << (resultado.encontroSolucion ? "si" : "no") << " | conflictos: " << resultado.conflictos
             << " | evaluaciones: " << setw(6) << resultado.evaluaciones << " | "
             << configuracionTexto(resultado.mejorTablero) << "\n";
        if (resultado.encontroSolucion || opciones.numeroEjecuciones == 1) {
            imprimirTablero(resultado.mejorTablero);
            cout << "\n";
        }

        if (resultado.encontroSolucion) {
            ejecucionesExitosas++;
            sumaEvaluacionesExitosas += resultado.evaluaciones;
            if (mejorEvaluaciones < 0 || resultado.evaluaciones < mejorEvaluaciones) mejorEvaluaciones = resultado.evaluaciones;
            if (peorEvaluaciones < 0 || resultado.evaluaciones > peorEvaluaciones) peorEvaluaciones = resultado.evaluaciones;
            solucionesDistintas.insert(resultado.mejorTablero);
        } else {
            ejecucionesFallidas++;
            sumaConflictosFallidas += resultado.conflictos;
        }
    }

    if (opciones.numeroEjecuciones > 1) {
        cout << "\n" << left << setw(11) << "Ejecucion" << setw(11) << "Algoritmo" << setw(20)
             << "Encontro solucion" << setw(12) << "Conflictos" << "Evaluaciones\n";
        for (int indice = 0; indice < opciones.numeroEjecuciones; indice++)
            cout << setw(11) << indice + 1 << setw(11) << nombreAlgoritmo << setw(20)
                 << (resultados[indice].encontroSolucion ? "Si" : "No") << setw(12) << resultados[indice].conflictos
                 << resultados[indice].evaluaciones << "\n";
        cout << right;
    }

    cout << fixed << setprecision(2);
    cout << "\n===== Resumen " << nombreAlgoritmo << " =====\n";
    cout << "Ejecuciones exitosas: " << ejecucionesExitosas << " de " << opciones.numeroEjecuciones << "\n";
    cout << "Porcentaje de exito: " << 100.0 * ejecucionesExitosas / opciones.numeroEjecuciones << " %\n";
    if (ejecucionesExitosas > 0) {
        cout << "Promedio de evaluaciones (exitosas): " << (double)sumaEvaluacionesExitosas / ejecucionesExitosas << "\n";
        cout << "Mejor numero de evaluaciones: " << mejorEvaluaciones << "\n";
        cout << "Peor numero de evaluaciones: " << peorEvaluaciones << "\n";
        cout << "Configuraciones distintas encontradas: " << solucionesDistintas.size() << "\n";
    }
    if (ejecucionesFallidas > 0)
        cout << "Promedio de conflictos finales (no exitosas): " << (double)sumaConflictosFallidas / ejecucionesFallidas << "\n";
    else
        cout << "Promedio de conflictos finales (no exitosas): N/A (no hubo fallos)\n";
}

struct ParametrosAG {
    int tamanoPoblacion = 30;
    int numeroElites = 2;
    int tamanoTorneo = 3;
    double probabilidadCruza = 0.9;
    double probabilidadMutacion = 0.8;
};

struct Individuo {
    Tablero tablero{};
    int conflictos = 0;
};

class AlgoritmoGenetico {
public:
    explicit AlgoritmoGenetico(const ParametrosAG& parametrosIniciales = ParametrosAG()) : parametros(parametrosIniciales) {}

    Resultado ejecutar(uint32_t semilla, long maximoEvaluaciones) {
        generador.sembrar(semilla);
        FuncionObjetivo funcionObjetivo(maximoEvaluaciones);
        Individuo mejorIndividuo;
        mejorIndividuo.conflictos = 1 << 30;

        vector<Individuo> poblacion(parametros.tamanoPoblacion);
        for (Individuo& individuo : poblacion) {
            if (funcionObjetivo.agotada()) return construirResultado(mejorIndividuo, funcionObjetivo);
            generador.permutacionAleatoria(individuo.tablero);
            individuo.conflictos = funcionObjetivo.evaluar(individuo.tablero);
            if (individuo.conflictos < mejorIndividuo.conflictos) mejorIndividuo = individuo;
            if (mejorIndividuo.conflictos == 0) return construirResultado(mejorIndividuo, funcionObjetivo);
        }

        vector<Individuo> nuevaPoblacion;
        nuevaPoblacion.reserve(parametros.tamanoPoblacion);
        while (!funcionObjetivo.agotada()) {
            sort(poblacion.begin(), poblacion.end(), [](const Individuo& primero, const Individuo& segundo) {
                return primero.conflictos < segundo.conflictos;
            });

            nuevaPoblacion.clear();
            for (int elite = 0; elite < parametros.numeroElites && elite < parametros.tamanoPoblacion; elite++)
                nuevaPoblacion.push_back(poblacion[elite]);

            while ((int)nuevaPoblacion.size() < parametros.tamanoPoblacion) {
                if (funcionObjetivo.agotada()) return construirResultado(mejorIndividuo, funcionObjetivo);

                const Individuo& padre1 = seleccionPorTorneo(poblacion);
                const Individuo& padre2 = seleccionPorTorneo(poblacion);

                Individuo hijo;
                if (generador.real() < parametros.probabilidadCruza)
                    cruzaOX(padre1.tablero, padre2.tablero, hijo.tablero);
                else
                    hijo.tablero = padre1.tablero;
                if (generador.real() < parametros.probabilidadMutacion) mutacionIntercambio(hijo.tablero);

                hijo.conflictos = funcionObjetivo.evaluar(hijo.tablero);
                if (hijo.conflictos < mejorIndividuo.conflictos) mejorIndividuo = hijo;
                if (mejorIndividuo.conflictos == 0) return construirResultado(mejorIndividuo, funcionObjetivo);
                nuevaPoblacion.push_back(hijo);
            }

            poblacion.swap(nuevaPoblacion);
        }
        return construirResultado(mejorIndividuo, funcionObjetivo);
    }

private:
    const Individuo& seleccionPorTorneo(const vector<Individuo>& poblacion) {
        int indiceGanador = generador.entero(0, parametros.tamanoPoblacion - 1);
        for (int competidor = 1; competidor < parametros.tamanoTorneo; competidor++) {
            int indiceRival = generador.entero(0, parametros.tamanoPoblacion - 1);
            if (poblacion[indiceRival].conflictos < poblacion[indiceGanador].conflictos) indiceGanador = indiceRival;
        }
        return poblacion[indiceGanador];
    }

    void cruzaOX(const Tablero& padre1, const Tablero& padre2, Tablero& hijo) {
        int inicioSegmento = generador.entero(0, NUM_REINAS - 1);
        int finSegmento = generador.entero(0, NUM_REINAS - 1);
        if (inicioSegmento > finSegmento) swap(inicioSegmento, finSegmento);

        array<bool, NUM_REINAS> columnaUsada{};
        hijo.fill(-1);
        for (int fila = inicioSegmento; fila <= finSegmento; fila++) {
            hijo[fila] = padre1[fila];
            columnaUsada[padre1[fila]] = true;
        }
        int filaDestino = (finSegmento + 1) % NUM_REINAS;
        for (int desplazamiento = 0; desplazamiento < NUM_REINAS; desplazamiento++) {
            int columnaPadre2 = padre2[(finSegmento + 1 + desplazamiento) % NUM_REINAS];
            if (!columnaUsada[columnaPadre2]) {
                hijo[filaDestino] = columnaPadre2;
                columnaUsada[columnaPadre2] = true;
                filaDestino = (filaDestino + 1) % NUM_REINAS;
            }
        }
    }

    void mutacionIntercambio(Tablero& tablero) {
        int filaA = generador.entero(0, NUM_REINAS - 1);
        int filaB = generador.entero(0, NUM_REINAS - 2);
        if (filaB >= filaA) filaB++;
        swap(tablero[filaA], tablero[filaB]);
    }

    static Resultado construirResultado(const Individuo& mejorIndividuo, const FuncionObjetivo& funcionObjetivo) {
        Resultado resultado;
        resultado.encontroSolucion = (mejorIndividuo.conflictos == 0);
        resultado.conflictos = mejorIndividuo.conflictos;
        resultado.evaluaciones = funcionObjetivo.usadas();
        resultado.mejorTablero = mejorIndividuo.tablero;
        return resultado;
    }

    ParametrosAG parametros;
    GeneradorAleatorio generador;
};

int main() {
    Opciones opciones;
    cout << "Semilla inicial: ";
    cin >> opciones.semillaInicial;
    cout << "Numero de ejecuciones: ";
    cin >> opciones.numeroEjecuciones;
    cout << "Maximo de evaluaciones: ";
    cin >> opciones.maximoEvaluaciones;
    if (!cin || opciones.numeroEjecuciones < 1 || opciones.maximoEvaluaciones < 1) {
        cout << "Datos invalidos\n";
        return 1;
    }
    cout << "\n";
    AlgoritmoGenetico algoritmoGenetico;
    correrExperimento("AG", opciones, [&](uint32_t semilla, long maximoEvaluaciones) {
        return algoritmoGenetico.ejecutar(semilla, maximoEvaluaciones);
    });
    return 0;
}
