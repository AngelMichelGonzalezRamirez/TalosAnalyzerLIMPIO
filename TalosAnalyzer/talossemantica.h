#ifndef TALOSSEMANTICA_H
#define TALOSSEMANTICA_H
#include <string>
#include "talos_tipos_ii.h"

// No AST, generador de parser ni tokenizer externo. Filas, columnas y topes comprobados.
class TalosSemantica {
public:
    enum { NS=256, NC=1024, NT=128, NM=NS+NC+NT, NQ=4096, NP=512, NF=64, NE=32768, NL=2048 };
    enum Pila { OPERADORES, OPERANDOS, TIPOS, SALTOS, AVAIL };
    enum Evento { PUSH=1, POP, CUADRUPLO, RELLENO, SIMBOLO, CONSTANTE, ACCION, ERROR };
    // MSímbolos: tipo, AP (índice de MMemoria), esConstante. RAM = 1000 + AP.
    int MSimbolos[NS][3]{}; std::string nombres[NS]; int ns=0;
    // MConstantes: tipo, AP. MCuadruplos: COPER,OP1,OP2,RES. Números externos desde 1.
    int MConstantes[NC][2]{}; std::string literales[NC]; int nc=0;
    int MCuadruplos[NQ][4]{}; int tipoResultado[NQ]{}; int nq=0;
    int MTiposCuadruplo[NQ][3]{}; // Tipos OP1, OP2 y RES al emitir, antes de reutilizar T.
    enum { NR=100000 };
    // Recorrido ejecutado: PC, siguiente PC, operador, condición (-1/0/1), tomó salto,
    // profundidad, OP1 y OP2. Valores copiados antes de que cambie la memoria.
    int MRecorrido[NR][8]{}, nr=0;
    std::string valoresRecorrido[NR][3];
    // MMemoria: tipo, inicializado, solo lectura. Valores en arreglos paralelos.
    int MMemoria[NM][3]{}; long double numeros[NM]{}; std::string textos[NM];
    int MPilas[5][NP]{}, topes[5]{};
    // Eventos inmutables para reconstruir el estado histórico en el deslizador Qt.
    int MMovimientos[NE][8]{}; int ne=0;
    std::string mensajes[NE], error; int linea=1,columna=1,errorLinea=1,errorColumna=1;
    std::string salida, errorEjecucion;
    // Directorio: tipo retorno, entrada, primer parámetro, cantidad, salto inicial,
    // fin, primer símbolo local, número de locales. Ámbito 0 = global; f+1 = función.
    enum { NFUNC=64, NPARAM=256, NACT=64 };
    int MFunciones[NFUNC][8]{}, nFunciones=0, funcionActual=-1;
    std::string funciones[NFUNC]; int ambitos[NS]{};
    int MParametros[NPARAM][3]{}, nParametros=0; // función, símbolo destino, tipo
    int MLlamadas[NFUNC][3]{}, nLlamadas=0; // función, argumento siguiente, como estatuto
    // Memoria de activación matricial. Se guarda cada local y temporal, no los globales.
    int MActivaciones[NACT][4]{}, MEstadoActivacion[NACT][NM][2]{};
    long double MNumeroActivacion[NACT][NM]{};
    std::string MTextoActivacion[NACT][NM];
    long double MArgumentos[NACT][NPARAM]{};
    std::string MTextoArgumentos[NACT][NPARAM];
    std::string librerias[32]; int nLibrerias=0;
    bool produccionesUsadas[256]{}; // Evidencia del recorrido de MProducciones, sin cambiar la tabla.
    TalosSemantica();
    void accion(int codigo,const std::string &lexema,int token,int lin,int col);
    bool finalizar();
    bool ejecutar(const std::string entradas[],int cantidad,int limite=100000);
    std::string referencia(int indice) const;
    std::string campoCuadruplo(int op,int columna,int valor) const;
    std::string valor(int indice) const;
private:
    // MContextos: clase, marca, selector/variable, tipo, falso pendiente, nVerdaderos,
    // inferior, superior, inicioCuádruplo, límiteFijo, primerIntervalo, idDeCASE.
    int MContextos[NF][12]{}, nf=0, serie=0;
    // MIntervalos: idCASE,tipo,referenciaInferior,referenciaSuperior.
    int MIntervalos[NL][4]{}, nl=0;
    std::string pendientes[NS], nombreConstante; int nPendientes=0,tipoDecl=0;
    void fallar(const std::string &s);
    void evento(int clase,int a=0,int b=0,int c=0,int d=0,int e=0,const std::string &s="");
    void push(int pila,int valor); int pop(int pila); int peek(int pila) const;
    int buscar(const std::string &s) const;
    int declarar(const std::string &s,int tipo,bool constante=false,int origen=-1);
    int constante(const std::string &s,int token);
    void operando(int direccion); int sacarOperando(int &tipo);
    int temporal(int tipo); void liberar(int direccion);
    int emitir(int op,int a,int b,int r,int tipo=0); void rellenar(int q,int destino);
    int binario(int op,int a,int b); void reducir(); void vaciar(); void operador(int op);
    void inicioContexto(int tipo); int *contexto(int tipo);
    void pruebaIf(); void siguienteIf(); void finalIf();
    void pruebaCase(); void inicioRama(); void finRama(); void finCase();
    bool leerValor(int dir,const std::string &s);
    int buscarFuncion(const std::string &s) const;
    void accionModulo(int codigo,const std::string &lexema);
    bool retornaEnTodosLosCaminos(int funcion) const;
};
#endif
