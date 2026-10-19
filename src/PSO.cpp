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

struct ParametrosPSO {
    int numeroParticulas = 20;
    double inercia = 0.3;
    double coeficienteCognitivo = 1.5;
    double coeficienteSocial = 1.5;
    int velocidadMaxima = NUM_REINAS - 1;
    double probabilidadTurbulencia = 1.0;
};

typedef pair<int, int> Intercambio;
typedef vector<Intercambio> Velocidad;

struct Particula {
    Tablero posicion{};
    Velocidad velocidad;
    int conflictos = 0;
    Tablero mejorPosicionPersonal{};
    int conflictosMejorPersonal = 0;
};

void aplicarVelocidad(Tablero& posicion, const Velocidad& velocidad) {
    for (const Intercambio& intercambio : velocidad) swap(posicion[intercambio.first], posicion[intercambio.second]);
}

class PSO {
public:
    explicit PSO(const ParametrosPSO& parametrosIniciales = ParametrosPSO()) : parametros(parametrosIniciales) {}

    Resultado ejecutar(uint32_t semilla, long maximoEvaluaciones) {
        generador.sembrar(semilla);
        FuncionObjetivo funcionObjetivo(maximoEvaluaciones);
        Tablero mejorPosicionGlobal{};
        int conflictosMejorGlobal = 1 << 30;

        vector<Particula> enjambre(parametros.numeroParticulas);
        for (Particula& particula : enjambre) {
            if (funcionObjetivo.agotada())
                return construirResultado(mejorPosicionGlobal, conflictosMejorGlobal, funcionObjetivo);
            generador.permutacionAleatoria(particula.posicion);
            particula.velocidad.clear();
            int intercambiosIniciales = generador.entero(0, parametros.velocidadMaxima);
            for (int contador = 0; contador < intercambiosIniciales; contador++)
                particula.velocidad.push_back(intercambioAleatorio());
            particula.conflictos = funcionObjetivo.evaluar(particula.posicion);
            particula.mejorPosicionPersonal = particula.posicion;
            particula.conflictosMejorPersonal = particula.conflictos;
            if (particula.conflictos < conflictosMejorGlobal) {
                mejorPosicionGlobal = particula.posicion;
                conflictosMejorGlobal = particula.conflictos;
            }
            if (conflictosMejorGlobal == 0)
                return construirResultado(mejorPosicionGlobal, conflictosMejorGlobal, funcionObjetivo);
        }

        while (!funcionObjetivo.agotada()) {
            for (Particula& particula : enjambre) {
                if (funcionObjetivo.agotada()) break;

                actualizarVelocidad(particula, mejorPosicionGlobal);
                aplicarVelocidad(particula.posicion, particula.velocidad);
                particula.conflictos = funcionObjetivo.evaluar(particula.posicion);

                if (particula.conflictos <= particula.conflictosMejorPersonal) {
                    particula.mejorPosicionPersonal = particula.posicion;
                    particula.conflictosMejorPersonal = particula.conflictos;
                }
                if (particula.conflictosMejorPersonal <= conflictosMejorGlobal) {
                    mejorPosicionGlobal = particula.mejorPosicionPersonal;
                    conflictosMejorGlobal = particula.conflictosMejorPersonal;
                }
                if (conflictosMejorGlobal == 0)
                    return construirResultado(mejorPosicionGlobal, conflictosMejorGlobal, funcionObjetivo);
            }
        }
        return construirResultado(mejorPosicionGlobal, conflictosMejorGlobal, funcionObjetivo);
    }

private:
    Intercambio intercambioAleatorio() {
        int filaA = generador.entero(0, NUM_REINAS - 1);
        int filaB = generador.entero(0, NUM_REINAS - 2);
        if (filaB >= filaA) filaB++;
        return make_pair(filaA, filaB);
    }

    void agregarDiferencia(Velocidad& velocidadNueva, Tablero& posicionTentativa, const Tablero& posicionDestino,
                           double probabilidadAceptar) {
        for (int fila = 0; fila < NUM_REINAS; fila++) {
            if (posicionTentativa[fila] == posicionDestino[fila]) continue;
            if (generador.real() >= probabilidadAceptar) continue;
            int filaConColumnaBuscada = 0;
            while (posicionTentativa[filaConColumnaBuscada] != posicionDestino[fila]) filaConColumnaBuscada++;
            swap(posicionTentativa[fila], posicionTentativa[filaConColumnaBuscada]);
            velocidadNueva.push_back(make_pair(fila, filaConColumnaBuscada));
        }
    }

    void actualizarVelocidad(Particula& particula, const Tablero& mejorPosicionGlobal) {
        Velocidad velocidadNueva;
        Tablero posicionTentativa = particula.posicion;

        for (const Intercambio& intercambio : particula.velocidad)
            if (generador.real() < parametros.inercia) {
                velocidadNueva.push_back(intercambio);
                swap(posicionTentativa[intercambio.first], posicionTentativa[intercambio.second]);
            }
        agregarDiferencia(velocidadNueva, posicionTentativa, particula.mejorPosicionPersonal,
                          min(1.0, parametros.coeficienteCognitivo * generador.real()));
        agregarDiferencia(velocidadNueva, posicionTentativa, mejorPosicionGlobal,
                          min(1.0, parametros.coeficienteSocial * generador.real()));
        if ((int)velocidadNueva.size() > parametros.velocidadMaxima) velocidadNueva.resize(parametros.velocidadMaxima);
        if (generador.real() < parametros.probabilidadTurbulencia) velocidadNueva.push_back(intercambioAleatorio());
        particula.velocidad = velocidadNueva;
    }

    static Resultado construirResultado(const Tablero& mejorPosicionGlobal, int conflictosMejorGlobal,
                                        const FuncionObjetivo& funcionObjetivo) {
        Resultado resultado;
        resultado.encontroSolucion = (conflictosMejorGlobal == 0);
        resultado.conflictos = conflictosMejorGlobal;
        resultado.evaluaciones = funcionObjetivo.usadas();
        resultado.mejorTablero = mejorPosicionGlobal;
        return resultado;
    }

    ParametrosPSO parametros;
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
    PSO pso;
    correrExperimento("PSO", opciones, [&](uint32_t semilla, long maximoEvaluaciones) {
        return pso.ejecutar(semilla, maximoEvaluaciones);
    });
    return 0;
}
