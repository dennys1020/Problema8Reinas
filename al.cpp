#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>

using namespace std;

const int N = 8;

const int TAM_POBLACION = 50;
const int ELITISMO = 2;
const int TAM_TORNEO = 3;
const double PROB_CRUZA = 0.9;
const double PROB_MUTACION = 0.2;

int maxEvaluaciones = 10000;
int evaluaciones = 0;
mt19937 rng;

struct Individuo {
    int x[N];
    int conflictos;
};

struct Resultado {
    bool exito;
    int conflictos;
    int evaluaciones;
    int mejor[N];
};

double uniforme() {
    return uniform_real_distribution<double>(0.0, 1.0)(rng);
}

int entero(int a, int b) {
    return uniform_int_distribution<int>(a, b)(rng);
}

void copiar(const int origen[], int destino[]) {
    for (int i = 0; i < N; i++) destino[i] = origen[i];
}

int evaluar(const int x[]) {
    evaluaciones++;
    int conflictos = 0;
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++)
            if (abs(x[i] - x[j]) == j - i) conflictos++;
    return conflictos;
}

void permutacionAleatoria(int x[]) {
    for (int i = 0; i < N; i++) x[i] = i;
    shuffle(x, x + N, rng);
}

const Individuo& torneo(const Individuo poblacion[]) {
    int mejor = entero(0, TAM_POBLACION - 1);
    for (int k = 1; k < TAM_TORNEO; k++) {
        int c = entero(0, TAM_POBLACION - 1);
        if (poblacion[c].conflictos < poblacion[mejor].conflictos) mejor = c;
    }
    return poblacion[mejor];
}

void cruzaOX(const int p1[], const int p2[], int hijo[]) {
    int a = entero(0, N - 1), b = entero(0, N - 1);
    if (a > b) swap(a, b);

    bool usado[N];
    for (int i = 0; i < N; i++) {
        hijo[i] = -1;
        usado[i] = false;
    }

    for (int i = a; i <= b; i++) {
        hijo[i] = p1[i];
        usado[p1[i]] = true;
    }

    int pos = (b + 1) % N;
    for (int k = 0; k < N; k++) {
        int gen = p2[(b + 1 + k) % N];
        if (!usado[gen]) {
            hijo[pos] = gen;
            usado[gen] = true;
            pos = (pos + 1) % N;
        }
    }
}

void mutacionIntercambio(int x[]) {
    int i = entero(0, N - 1), j = entero(0, N - 1);
    while (j == i) j = entero(0, N - 1);
    swap(x[i], x[j]);
}

Resultado armarResultado(const Individuo& mejor) {
    Resultado r{};
    r.exito = (mejor.conflictos == 0);
    r.conflictos = mejor.conflictos;
    r.evaluaciones = evaluaciones;
    copiar(mejor.x, r.mejor);
    return r;
}

Resultado ejecutarAG(unsigned semilla) {
    rng.seed(semilla);
    evaluaciones = 0;

    Individuo poblacion[TAM_POBLACION] = {};
    Individuo nueva[TAM_POBLACION] = {};
    Individuo mejor{};
    mejor.conflictos = 1000;

    for (int i = 0; i < TAM_POBLACION && evaluaciones < maxEvaluaciones; i++) {
        permutacionAleatoria(poblacion[i].x);
        poblacion[i].conflictos = evaluar(poblacion[i].x);
        if (poblacion[i].conflictos < mejor.conflictos) mejor = poblacion[i];
        if (mejor.conflictos == 0) return armarResultado(mejor);
    }

    while (mejor.conflictos > 0 && evaluaciones < maxEvaluaciones) {
        sort(poblacion, poblacion + TAM_POBLACION,
             [](const Individuo& a, const Individuo& b) { return a.conflictos < b.conflictos; });

        int tamNueva = 0;
        for (int i = 0; i < ELITISMO; i++) {
            nueva[tamNueva] = poblacion[i];
            tamNueva++;
        }

        while (tamNueva < TAM_POBLACION) {
            if (evaluaciones >= maxEvaluaciones) break;

            const Individuo& p1 = torneo(poblacion);
            const Individuo& p2 = torneo(poblacion);

            Individuo hijo{};
            if (uniforme() < PROB_CRUZA)
                cruzaOX(p1.x, p2.x, hijo.x);
            else
                copiar(p1.x, hijo.x);
            if (uniforme() < PROB_MUTACION) mutacionIntercambio(hijo.x);

            hijo.conflictos = evaluar(hijo.x);
            nueva[tamNueva] = hijo;
            tamNueva++;

            if (hijo.conflictos < mejor.conflictos) mejor = hijo;
            if (mejor.conflictos == 0) break;
        }

        if (tamNueva == TAM_POBLACION) {
            for (int i = 0; i < TAM_POBLACION; i++) poblacion[i] = nueva[i];
        }
    }
    return armarResultado(mejor);
}

void imprimirConfiguracion(const int x[]) {
    cout << "Configuracion (columna por fila): [";
    for (int i = 0; i < N; i++) cout << x[i] << (i < N - 1 ? ", " : "");
    cout << "]\n";
}

void imprimirTablero(const int x[]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) cout << (x[i] == j ? " Q" : " .");
        cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    unsigned semilla = (argc > 1) ? atoi(argv[1]) : 1;
    int ejecuciones = (argc > 2) ? atoi(argv[2]) : 1;
    if (argc > 3) maxEvaluaciones = atoi(argv[3]);

    if (ejecuciones == 1) {
        Resultado r = ejecutarAG(semilla);
        cout << "Semilla: " << semilla << "\n";
        cout << "Exito: " << (r.exito ? "si" : "no") << "\n";
        cout << "Conflictos: " << r.conflictos << "\n";
        cout << "Evaluaciones: " << r.evaluaciones << "\n";
        imprimirConfiguracion(r.mejor);
        imprimirTablero(r.mejor);
        return 0;
    }

    int exitos = 0, sumaEvalExito = 0, mejorEval = 1 << 30, peorEval = 0;
    int sumaConfFallos = 0, fallos = 0;

    cout << "Ejecucion,Algoritmo,Exito,Conflictos,Evaluaciones\n";
    for (int e = 1; e <= ejecuciones; e++) {
        Resultado r = ejecutarAG(semilla + e - 1);
        cout << e << ",AG," << (r.exito ? "si" : "no") << "," << r.conflictos << ","
             << r.evaluaciones << "\n";
        if (r.exito) {
            exitos++;
            sumaEvalExito += r.evaluaciones;
            mejorEval = min(mejorEval, r.evaluaciones);
            peorEval = max(peorEval, r.evaluaciones);
        } else {
            fallos++;
            sumaConfFallos += r.conflictos;
        }
    }

    cout << fixed << setprecision(2);
    cout << "\nEjecuciones exitosas: " << exitos << " de " << ejecuciones << "\n";
    cout << "Porcentaje de exito: " << 100.0 * exitos / ejecuciones << "%\n";
    if (exitos > 0) {
        cout << "Promedio de evaluaciones (exitosas): " << (double)sumaEvalExito / exitos << "\n";
        cout << "Mejor numero de evaluaciones: " << mejorEval << "\n";
        cout << "Peor numero de evaluaciones: " << peorEval << "\n";
    }
    if (fallos > 0)
        cout << "Promedio de conflictos finales (fallidas): " << (double)sumaConfFallos / fallos << "\n";
    return 0;
}