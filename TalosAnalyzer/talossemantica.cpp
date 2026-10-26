#include "talossemantica.h"
#include "talos_matrices_ii.h"
#include <cmath>
#include <cstdlib>
#include <cerrno>
#include <sstream>
#include <iomanip>
#include <limits>
using namespace TiposII;

TalosSemantica::TalosSemantica() {
    // T1 queda en el tope. La dirección temporal no cambia al devolverla a AVAIL.
    for(int j=NT-1;j>=0;--j) push(AVAIL,NS+NC+j);
}
void TalosSemantica::evento(int k,int a,int b,int c,int d,int e,const std::string &s) {
    if(ne>=NE) { if(error.empty()) {error="S099: capacidad de MMovimientos agotada";errorLinea=linea;errorColumna=columna;} return; }
    int *r=MMovimientos[ne];r[0]=k;r[1]=a;r[2]=b;r[3]=c;r[4]=d;r[5]=e;r[6]=linea;r[7]=columna;mensajes[ne++]=s;
}
void TalosSemantica::fallar(const std::string &s) {
    if(!error.empty()) return;
    error=s;errorLinea=linea;errorColumna=columna;evento(ERROR,0,0,0,0,0,s);
}
void TalosSemantica::push(int p,int v) {
    if(!error.empty())return;
    if(p<0||p>=5||topes[p]>=NP) {fallar("S090: desbordamiento de pila matricial");return;}
    MPilas[p][topes[p]++]=v;evento(PUSH,p,v,0,0,0,"PUSH");
}
int TalosSemantica::pop(int p) {
    if(!error.empty())return -1;
    if(p<0||p>=5||topes[p]<=0){fallar("S091: POP sobre pila vacía");return -1;}
    int v=MPilas[p][--topes[p]];evento(POP,p,v,0,0,0,"POP");return v;
}
int TalosSemantica::peek(int p)const {return p>=0&&p<5&&topes[p]>0?MPilas[p][topes[p]-1]:-1;}
int TalosSemantica::buscar(const std::string &s)const {for(int i=0;i<ns;++i)if(nombres[i]==s&&ambitos[i]==funcionActual+1)return i;for(int i=0;i<ns;++i)if(nombres[i]==s&&ambitos[i]==0)return i;return -1;}
int TalosSemantica::declarar(const std::string &s,int t,bool c,int origen) {
    if(funcionActual<0&&buscarFuncion(s)>=0){fallar("S001: variable global con nombre de función: "+s);return -1;}
    for(int j=0;j<ns;++j)if(nombres[j]==s&&ambitos[j]==funcionActual+1){fallar("S001: declaración duplicada de "+s);return -1;}
    if(ns>=NS){fallar("S092: tabla de símbolos llena");return -1;}
    if(t<1||t>5){fallar("S003: void no admite almacenamiento de variable");return -1;}
    int i=ns++;nombres[i]=s;ambitos[i]=funcionActual+1;MSimbolos[i][0]=t;MSimbolos[i][1]=i;MSimbolos[i][2]=c;
    MMemoria[i][0]=t;MMemoria[i][2]=c;
    if(origen>=0&&origen<NM){MMemoria[i][1]=1;numeros[i]=numeros[origen];textos[i]=textos[origen];}
    evento(SIMBOLO,i);return i;
}
int TalosSemantica::constante(const std::string &s,int tok) {
    int t=tok==102?1:(tok==103||tok==104?2:(tok==125?3:(tok==126?4:5)));
    std::string text;long double number=0;
    if(t==3||t==4){
        if(s.size()<2){fallar("S004: literal incompleto");return -1;}
        for(size_t j=1;j+1<s.size();++j){text+=s[j];if(t==4&&s[j]=='"'&&j+2<s.size()&&s[j+1]=='"')++j;}
        if(t==3&&text.size()!=1){fallar("S004: carácter ordinal debe ocupar un carácter ASCII");return -1;}
        if(t==3)number=static_cast<unsigned char>(text[0]);
    }else if(t==5)number=s=="true"?1:0;
    else {char *end=nullptr;errno=0;number=std::strtold(s.c_str(),&end);
        if(errno||!end||*end||!std::isfinite(number)||(t==1&&(number<std::numeric_limits<long long>::min()||number>std::numeric_limits<long long>::max()))){fallar("S005: número fuera de representación");return -1;}}
    for(int i=0;i<nc;++i){int m=MConstantes[i][1];if(MConstantes[i][0]==t&&numeros[m]==number&&textos[m]==text)return m;}
    if(nc>=NC){fallar("S093: tabla de constantes llena");return -1;}
    int i=nc++,m=NS+i;MConstantes[i][0]=t;MConstantes[i][1]=m;literales[i]=s;MMemoria[m][0]=t;MMemoria[m][1]=MMemoria[m][2]=1;numeros[m]=number;textos[m]=text;
    evento(CONSTANTE,i);return m;
}
void TalosSemantica::operando(int a){if(a<0||a>=NM){fallar("S094: dirección de operando fuera de MMemoria");return;}push(OPERANDOS,a);push(TIPOS,MMemoria[a][0]);}
int TalosSemantica::sacarOperando(int &t){t=pop(TIPOS);return pop(OPERANDOS);}
int TalosSemantica::temporal(int t){int a=pop(AVAIL);if(a<NS+NC||a>=NM){fallar("S095: AVAIL sin temporal disponible");return -1;}MMemoria[a][0]=t;return a;}
void TalosSemantica::liberar(int a){if(a>=NS+NC&&a<NM){for(int i=0;i<topes[AVAIL];++i)if(MPilas[AVAIL][i]==a){fallar("S096: liberación duplicada de temporal");return;}push(AVAIL,a);}}
int TalosSemantica::emitir(int op,int a,int b,int r,int t){
    if(!error.empty())return -1;
    if(nq>=NQ){fallar("S097: matriz de cuádruplos llena");return -1;}
    int *f=MCuadruplos[nq];f[0]=op;f[1]=a;f[2]=b;f[3]=r;tipoResultado[nq]=t;
    // Una dirección temporal puede tener otro tipo al final de la generación.
    // El cuadro conserva aquí los tipos exactos de sus tres campos de datos.
    for(int c=0;c<3;++c){int v=f[c+1];MTiposCuadruplo[nq][c]=v>=0&&v<NM?MMemoria[v][0]:0;}
    if(op>=GOTO&&op<=GOTOV)MTiposCuadruplo[nq][2]=0;
    if(op==ERA||op==GOSUB||op==ENDFUNC){MTiposCuadruplo[nq][0]=0;MTiposCuadruplo[nq][1]=0;}
    if(op==PARAM){MTiposCuadruplo[nq][1]=0;MTiposCuadruplo[nq][2]=0;}
    if(op==RET)MTiposCuadruplo[nq][2]=0;
    if(t&&op!=RET)MTiposCuadruplo[nq][2]=t;
    evento(CUADRUPLO,nq,op,a,b,r,"Generar cuádruplo");return ++nq;
}
void TalosSemantica::rellenar(int q,int dst){
    if(!error.empty())return;
    // Acción 5 de CASE rellena con CONT+1 antes de emitir el goto en CONT.
    // Por eso se admite nq+2 aquí. finalizar() exige destino <= nq+1 definitivo.
    if(q<1||q>nq||dst<1||dst>nq+2||dst>NQ+1||MCuadruplos[q-1][0]<GOTO||MCuadruplos[q-1][0]>GOTOV||MCuadruplos[q-1][3]!=-1){fallar("S098: relleno de salto inválido");return;}
    MCuadruplos[q-1][3]=dst;evento(RELLENO,q,dst,0,0,0,"Rellenar salto pendiente");
}
int TalosSemantica::binario(int op,int a,int b){
    if(a<0||b<0||a>=NM||b>=NM||op<0||op>O){fallar("S010: operandos inválidos");return -1;}
    int ta=MMemoria[a][0],tb=MMemoria[b][0],t=MCubo[op][ta][tb];
    if(!t){fallar(std::string("S011: tipos incompatibles en ")+nombreOp[op]+" ("+nombreTipo[ta]+", "+nombreTipo[tb]+")");return -1;}
    int r=temporal(t);emitir(op,a,b,r,t);return r;
}
void TalosSemantica::reducir(){
    int op=pop(OPERADORES);if(op<0||op==FF){fallar("S012: reducción sobre fondo falso");return;}
    int tb,ta,b=sacarOperando(tb),a=-1,r=-1;
    if(!error.empty())return;
    if(op==NO||op==NEG){int t=MCubo[op][tb][0];if(!t){fallar("S011: tipo incompatible con operador unario");return;}r=temporal(t);emitir(op,b,-1,r,t);liberar(b);}
    else {a=sacarOperando(ta);if(!error.empty())return;r=binario(op,a,b);if(!error.empty())return;liberar(a);if(b!=a)liberar(b);}
    if(!error.empty())return;operando(r);
}
void TalosSemantica::vaciar(){while(error.empty()&&topes[OPERADORES]&&peek(OPERADORES)!=FF)reducir();}
void TalosSemantica::operador(int op){
    // Reducir ANTES de apilar otro operador de igual prioridad: asociación izquierda.
    while(error.empty()&&topes[OPERADORES]&&peek(OPERADORES)!=FF&&MPrioridad[peek(OPERADORES)]>=MPrioridad[op])reducir();
    push(OPERADORES,op);
}
void TalosSemantica::inicioContexto(int kind){if(nf>=NF){fallar("S020: anidamiento excesivo");return;}int *f=MContextos[nf++];for(int j=0;j<12;++j)f[j]=0;f[0]=kind;f[1]=-10000-++serie;f[10]=nl;f[11]=serie;push(SALTOS,f[1]);}
int *TalosSemantica::contexto(int kind){if(nf<1||MContextos[nf-1][0]!=kind){fallar("S021: contexto de control desbalanceado");return nullptr;}return MContextos[nf-1];}
void TalosSemantica::pruebaIf(){int *f=contexto(1);if(!f)return;int t,a=sacarOperando(t);if(t!=5){fallar("S030: condición de IF requiere bool");return;}int q=emitir(GOTOF,a,-1,-1);push(SALTOS,q);f[4]=1;liberar(a);}
void TalosSemantica::siguienteIf(){int *f=contexto(1);if(!f)return;int q=emitir(GOTO,-1,-1,-1),falso=pop(SALTOS);rellenar(falso,nq+1);push(SALTOS,q);f[4]=0;}
void TalosSemantica::finalIf(){int *f=contexto(1);if(!f)return;while(error.empty()&&peek(SALTOS)!=f[1])rellenar(pop(SALTOS),nq+1);if(error.empty())pop(SALTOS);--nf;}
void TalosSemantica::pruebaCase(){
    int *f=contexto(2);if(!f)return;int a=f[2],lo=f[6],hi=f[7];
    if(lo<0||hi<0||MMemoria[lo][0]!=f[3]||MMemoria[hi][0]!=f[3]){fallar("S040: etiqueta CASE y selector deben tener el mismo tipo ordinal");return;}
    long double l=numeros[lo],h=numeros[hi];if(l>h){fallar("S041: rango CASE invertido");return;}
    for(int i=f[10];i<nl;++i)if(MIntervalos[i][0]==f[11]&&l<=numeros[MIntervalos[i][3]]&&h>=numeros[MIntervalos[i][2]]){fallar("S042: etiquetas CASE duplicadas o rangos superpuestos");return;}
    if(nl>=NL){fallar("S043: matriz de intervalos CASE llena");return;}
    int *rr=MIntervalos[nl++];rr[0]=f[11];rr[1]=f[3];rr[2]=lo;rr[3]=hi;
    int tipoSelector,selector=sacarOperando(tipoSelector);
    if(selector!=a){fallar("S044: selector CASE perdido durante comparación");return;}
    int r;
    if(f[8]<0)r=binario(IGUAL,a,lo); // Acción 2 del diagrama.
    else {int r1=f[8],r2=binario(MENORIG,a,hi);r=binario(Y,r1,r2);liberar(r1);liberar(r2);} // Acción 4.
    if(!error.empty())return;push(SALTOS,emitir(GOTOV,r,-1,-1));liberar(r);operando(a);++f[5];
}
void TalosSemantica::inicioRama(){int *f=contexto(2);if(!f)return;
    // Acción 5: verdaderos a CONT+1; el cuádruplo CONT es el goto de no coincidencia.
    int inicio=nq+2;for(int j=0;j<f[5]&&error.empty();++j)rellenar(pop(SALTOS),inicio);
    f[5]=0;push(SALTOS,emitir(GOTO,-1,-1,-1));
}
void TalosSemantica::finRama(){int *f=contexto(2);if(!f)return;
    // Acción 6: reservar salida de rama y apuntar el falso a la próxima prueba/ELSE.
    int noCoincide=pop(SALTOS);int salida=emitir(GOTO,-1,-1,-1);rellenar(noCoincide,nq+1);push(SALTOS,salida);
}
void TalosSemantica::finCase(){int *f=contexto(2);if(!f)return;
    int t,a=sacarOperando(t);if(a!=f[2]){fallar("S044: selector CASE perdido en la pila");return;}
    while(error.empty()&&peek(SALTOS)!=f[1])rellenar(pop(SALTOS),nq+1);
    if(error.empty())pop(SALTOS);liberar(a);--nf;
}
void TalosSemantica::accion(int ac,const std::string &lex,int tok,int lin,int col){
    using namespace MatricesII;
    if(!error.empty())return;linea=lin;columna=col;evento(ACCION,ac,0,0,0,0,lex);
    if(ac>=FUNC_NAME){accionModulo(ac,lex);return;}
    int t=0,a=-1,b=-1,*f=nullptr;
    switch(ac){
    case DECL_BEGIN:nPendientes=0;tipoDecl=0;break;
    case DECL_ID:if(nPendientes>=NS)fallar("S092: lista de declaración llena");else pendientes[nPendientes++]=lex;break;
    case TYPE:tipoDecl=tok==141?1:tok==142?2:tok==143?3:tok==144?4:tok==163?5:6;break;
    case DECL_END:for(int j=0;j<nPendientes&&error.empty();++j)declarar(pendientes[j],tipoDecl);nPendientes=0;break;
    case CONST_NAME:nombreConstante=lex;break;
    case CONST_END:a=sacarOperando(t);if(error.empty())declarar(nombreConstante,t,true,a);break;
    case VALUE:a=constante(lex,tok);if(error.empty())operando(a);break;
    case ORD_NEG:a=constante("-"+lex,102);if(error.empty())operando(a);break;
    case TARGET:a=buscar(lex);if(a<0)fallar("S002: identificador no declarado: "+lex);else operando(a);break;
    case ASSIGN:{int ts,td;b=sacarOperando(ts);a=sacarOperando(td);if(error.empty()){if(a<0||a>=ns)fallar("S082: el destino de asignación debe ser una variable");else if(MMemoria[a][2])fallar("S006: no se puede modificar una constante");else if(!MAsignacion[td][ts])fallar("S007: asignación con tipos incompatibles");else {emitir(ASIG,b,-1,a,td);liberar(b);}}break;}
    case READ:a=buscar(lex);if(a<0)fallar("S002: variable de entrada no declarada: "+lex);else if(MMemoria[a][2])fallar("S006: entrada sobre constante");else emitir(IN,-1,-1,a,MMemoria[a][0]);break;
    case MatricesII::OUT:a=sacarOperando(t);if(error.empty()){emitir(TiposII::OUT,-1,-1,a);liberar(a);}break;
    case PUSH_OP:for(int j=0;j<=NO;++j)if(lex==nombreOp[j]){operador(j);break;}break;
    case EXPR_END:vaciar();break;
    case FF_PUSH:push(OPERADORES,FF);break;
    case FF_POP:if(peek(OPERADORES)!=FF)fallar("S013: falta marca de fondo falso");else pop(OPERADORES);break;
    case NEG_PUSH:push(OPERADORES,NEG);break;
    case NEG_END:if(peek(OPERADORES)==NEG)reducir();break;
    case INC:case DEC:case POSTINC:case POSTDEC:
        if(ac==INC||ac==DEC){a=buscar(lex);t=a>=0?MMemoria[a][0]:0;}else a=sacarOperando(t);
        if(a<0){fallar("S002: variable de incremento no declarada");break;}
        if(MMemoria[a][2]||!(t==1||t==2)){fallar("S008: incremento requiere variable numérica modificable");break;}
        b=constante("1",102);emitir(ac==INC||ac==POSTINC?SUMA:RESTA,a,b,a,t);break;
    case IF_BEGIN:inicioContexto(1);break;
    case IF_TEST:pruebaIf();break;
    case IF_NEXT:siguienteIf();break;
    case IF_END:finalIf();break;
    case CASE_BEGIN:a=peek(OPERANDOS);t=peek(TIPOS);if(t!=1&&t!=3&&t!=5){fallar("S045: selector CASE debe ser int, char o bool");break;}inicioContexto(2);f=contexto(2);if(f){f[2]=a;f[3]=t;}break;
    case CASE_LOW:a=sacarOperando(t);f=contexto(2);if(f){f[6]=f[7]=a;f[8]=-1;}break;
    case CASE_RANGE_BEGIN:
        f=contexto(2);if(!f)break;
        if(f[6]<0||MMemoria[f[6]][0]!=f[3]){fallar("S040: tipo incompatible en límite CASE");break;}
        a=sacarOperando(t);if(a!=f[2]){fallar("S044: selector CASE perdido al iniciar rango");break;}
        f[8]=binario(MAYORIG,a,f[6]);if(error.empty())operando(a);break; // Acción 3, antes del límite superior.
    case CASE_HIGH:a=sacarOperando(t);f=contexto(2);if(f)f[7]=a;break;
    case CASE_TEST:pruebaCase();break;
    case CASE_LABELS_END:inicioRama();break;
    case CASE_ARM_END:finRama();break;
    case CASE_END:finCase();break;
    case WHILE_BEGIN:inicioContexto(3);f=contexto(3);if(f)f[8]=nq+1;break;
    case WHILE_TEST:f=contexto(3);a=sacarOperando(t);if(t!=5){fallar("S030: condición WHILE requiere bool");break;}if(f)push(SALTOS,emitir(GOTOF,a,-1,-1));liberar(a);break;
    case WHILE_END:f=contexto(3);if(f){int q=pop(SALTOS);emitir(GOTO,-1,-1,f[8]);rellenar(q,nq+1);pop(SALTOS);--nf;}break;
    case DO_BEGIN:inicioContexto(4);f=contexto(4);if(f)f[8]=nq+1;break;
    case DO_END:f=contexto(4);a=sacarOperando(t);if(t!=5){fallar("S030: condición DOWHILE requiere bool");break;}if(f){emitir(GOTOV,a,-1,f[8]);liberar(a);pop(SALTOS);--nf;}break;
    case FOR_BEGIN:a=buscar(lex);if(a<0||MMemoria[a][0]!=1||MMemoria[a][2]){fallar("S050: FOR requiere variable int modificable");break;}inicioContexto(5);f=contexto(5);if(f)f[2]=a;break;
    case FOR_INIT:f=contexto(5);a=sacarOperando(t);if(t!=1){fallar("S051: inicio FOR requiere int");break;}if(f){emitir(ASIG,a,-1,f[2],1);liberar(a);}break;
    case FOR_TEST:f=contexto(5);a=sacarOperando(t);if(t!=1){fallar("S052: límite FOR requiere int");break;}if(f){b=temporal(1);emitir(ASIG,a,-1,b,1);liberar(a);f[9]=b;f[8]=nq+1;int r=binario(MENORIG,f[2],b);push(SALTOS,emitir(GOTOF,r,-1,-1));liberar(r);}break;
    case FOR_END:f=contexto(5);if(f){int q=pop(SALTOS);a=constante("1",102);emitir(SUMA,f[2],a,f[2],1);emitir(GOTO,-1,-1,f[8]);rellenar(q,nq+1);liberar(f[9]);pop(SALTOS);--nf;}break;
    case UNSUPPORTED:fallar("S080: acción histórica reservada; no pertenece a las producciones activas");break;
    default:fallar("S089: acción semántica sin implementación");
    }
}
bool TalosSemantica::finalizar(){if(!error.empty())return false;
    if(funcionActual!=-1||nLlamadas){fallar("S088: módulo o llamada sin cerrar");return false;}
    if(nf||topes[OPERADORES]||topes[OPERANDOS]||topes[TIPOS]||topes[SALTOS]||topes[AVAIL]!=NT){fallar("S060: pilas o temporales sin cerrar");return false;}
    for(int j=0;j<nq;++j)if(MCuadruplos[j][0]>=GOTO&&MCuadruplos[j][0]<=GOTOV&&(MCuadruplos[j][3]<1||MCuadruplos[j][3]>nq+1)){fallar("S061: salto pendiente o fuera del programa");return false;}
    return true;
}
std::string TalosSemantica::referencia(int i)const {if(i<=-2000){int f=-2000-i;return f<nFunciones?"func "+funciones[f]:"?";}if(i<0)return "—";if(i<ns)return ambitos[i]?funciones[ambitos[i]-1]+"::"+nombres[i]:nombres[i];if(i>=NS&&i<NS+nc)return literales[i-NS];if(i>=NS+NC&&i<NM)return "T"+std::to_string(i-NS-NC+1);return "?";}
std::string TalosSemantica::valor(int a)const {if(a<0||a>=NM||!MMemoria[a][1])return "sin inicializar";int t=MMemoria[a][0];if(t==3||t==4)return textos[a];if(t==5)return numeros[a]?"true":"false";std::ostringstream s;s<<std::setprecision(15)<<numeros[a];return s.str();}
bool TalosSemantica::leerValor(int a,const std::string &s){
    int t=MMemoria[a][0];long double n=0;
    if(t==3||t==4){if(t==3&&s.size()!=1)return false;textos[a]=s;if(t==3)n=static_cast<unsigned char>(s[0]);}
    else if(t==5){if(s=="true"||s=="1")n=1;else if(s!="false"&&s!="0")return false;}
    else{char *end=nullptr;errno=0;n=std::strtold(s.c_str(),&end);if(s.empty()||errno||!end||*end||!std::isfinite(n)||(t==1&&(std::floor(n)!=n||n<std::numeric_limits<long long>::min()||n>std::numeric_limits<long long>::max())))return false;}
    numeros[a]=n;MMemoria[a][1]=1;return true;
}
bool TalosSemantica::ejecutar(const std::string inputs[],int n,int limite){
    salida.clear();errorEjecucion.clear();nr=0;if(!error.empty()){errorEjecucion="No ejecutar: hay errores de análisis";return false;}
    for(int a=0;a<NM;++a)if(!MMemoria[a][2])MMemoria[a][1]=0;
    int pc=1,ni=0,pasos=0,profundidad=0;bool instruccionAbierta=false;
    auto falla=[&](const std::string&s){errorEjecucion="Cuádruplo "+std::to_string(pc)+": "+s;};
    while(pc<=nq&&errorEjecucion.empty()){
        instruccionAbierta=false;
        if(++pasos>limite||nr>=NR){falla("R010: límite de ejecución; revisar ciclo");break;}
        const int *q=MCuadruplos[pc-1];int op=q[0],a=q[1],b=q[2],r=q[3];long double z=0;
        int paso=nr++;instruccionAbierta=true;int *tr=MRecorrido[paso];
        tr[0]=pc;tr[1]=pc+1;tr[2]=op;tr[3]=-1;tr[4]=0;tr[5]=profundidad;tr[6]=a;tr[7]=b;
        valoresRecorrido[paso][0]=a>=0&&a<NM?valor(a):"—";
        valoresRecorrido[paso][1]=b>=0&&b<NM?valor(b):"—";
        valoresRecorrido[paso][2]="—";
        if(op==ERA||op==GOSUB||op==ENDFUNC){valoresRecorrido[paso][0]=a>=0&&a<nFunciones?funciones[a]:"función inválida";valoresRecorrido[paso][1]=op==GOSUB?std::to_string(b):"—";}
        if(op==PARAM)valoresRecorrido[paso][1]="P"+std::to_string(b+1);
        // ERA salva la activación del llamador. PARAM copia por valor sin pisar
        // los parámetros del llamador recursivo. GOSUB activa el ámbito destino.
        if(op==ERA){
            if(a<0||a>=nFunciones||profundidad>=NACT){falla("R020: pila de activaciones agotada o función inválida");break;}
            int d=profundidad++;MActivaciones[d][0]=a;MActivaciones[d][3]=0;
            for(int k=0;k<NM;++k)if((k<ns&&ambitos[k]>0)||k>=NS+NC){
                MEstadoActivacion[d][k][0]=MMemoria[k][0];MEstadoActivacion[d][k][1]=MMemoria[k][1];
                MNumeroActivacion[d][k]=numeros[k];MTextoActivacion[d][k]=textos[k];
            }++pc;continue;
        }
        if(op==PARAM){
            if(!profundidad||r<0||r>=nFunciones||MActivaciones[profundidad-1][0]!=r||b<0||b>=MFunciones[r][3]||a<0||a>=NM){falla("R021: parámetro fuera de su activación");break;}
            if(!MMemoria[a][1]){falla("R003: argumento sin inicializar");break;}
            MArgumentos[profundidad-1][b]=numeros[a];MTextoArgumentos[profundidad-1][b]=textos[a];++pc;continue;
        }
        if(op==GOSUB){
            if(!profundidad||a<0||a>=nFunciones||MActivaciones[profundidad-1][0]!=a||b!=MFunciones[a][1]){falla("R022: GOSUB sin ERA válido");break;}
            int d=profundidad-1;MActivaciones[d][1]=pc+1;MActivaciones[d][2]=r;MActivaciones[d][3]=1;
            for(int k=0;k<ns;++k)if(ambitos[k]==a+1)MMemoria[k][1]=0;
            for(int k=NS+NC;k<NM;++k)MMemoria[k][1]=0;
            for(int j=0;j<MFunciones[a][3];++j){int k=MParametros[MFunciones[a][2]+j][1];
                numeros[k]=MArgumentos[d][j];textos[k]=MTextoArgumentos[d][j];MMemoria[k][1]=1;}
            tr[1]=b;tr[4]=1;pc=b;continue;
        }
        if(op==RET||op==ENDFUNC){
            int f=op==RET?r:a;
            if(!profundidad||f<0||f>=nFunciones||MActivaciones[profundidad-1][0]!=f||!MActivaciones[profundidad-1][3]){falla("R023: retorno sin activación");break;}
            if(op==ENDFUNC&&MFunciones[f][0]!=6){falla("R024: función termina sin valor de retorno");break;}
            if(op==RET&&(a<0||a>=NM||!MMemoria[a][1])){falla("R003: retorno sin inicializar");break;}
            long double numero=op==RET?numeros[a]:0;std::string texto=op==RET?textos[a]:"";
            int d=--profundidad,dest=MActivaciones[d][2],retorno=MActivaciones[d][1];
            for(int k=0;k<NM;++k)if((k<ns&&ambitos[k]>0)||k>=NS+NC){
                MMemoria[k][0]=MEstadoActivacion[d][k][0];MMemoria[k][1]=MEstadoActivacion[d][k][1];
                numeros[k]=MNumeroActivacion[d][k];textos[k]=MTextoActivacion[d][k];
            }
            if(op==RET){if(dest<NS+NC||dest>=NM){falla("R004: retorno fuera de temporales");break;}
                MMemoria[dest][0]=MFunciones[f][0];MMemoria[dest][1]=1;numeros[dest]=numero;textos[dest]=texto;}
            tr[1]=retorno;tr[4]=1;valoresRecorrido[paso][2]=op==RET?valor(dest):"void";pc=retorno;continue;
        }
        if(op==GOTO){tr[1]=r;tr[4]=1;pc=r;continue;}
        if(op==IN){if(ni>=n)falla("R001: faltan datos de entrada");else if(!leerValor(r,inputs[ni++]))falla("R002: entrada incompatible con "+std::string(nombreTipo[MMemoria[r][0]]));valoresRecorrido[paso][2]=valor(r);++pc;continue;}
        if(op==OUT){if(r<0||r>=NM||!MMemoria[r][1])falla("R003: lectura de valor sin inicializar");else salida+=valor(r)+"\n";valoresRecorrido[paso][2]=valor(r);++pc;continue;}
        if(a<0||a>=NM||!MMemoria[a][1]||(b>=0&&(b>=NM||!MMemoria[b][1]))){falla("R003: lectura de operando sin inicializar");break;}
        if(op==GOTOF||op==GOTOV){bool v=numeros[a]!=0;tr[3]=v?1:0;if((op==GOTOV&&v)||(op==GOTOF&&!v)){tr[1]=r;tr[4]=1;pc=r;continue;}++pc;continue;}
        if(r<0||r>=NM){falla("R004: destino fuera de memoria");break;}
        int t=tipoResultado[pc-1];if(r>=NS+NC)MMemoria[r][0]=t;
        if(op==ASIG){numeros[r]=numeros[a];textos[r]=textos[a];MMemoria[r][1]=1;valoresRecorrido[paso][2]=valor(r);++pc;continue;}
        long double x=numeros[a],y=b>=0?numeros[b]:0;int cmp=0;
        if(b>=0){int ta=MMemoria[a][0],tb=MMemoria[b][0];if((ta==3||ta==4)&&(tb==3||tb==4))cmp=textos[a].compare(textos[b]);else cmp=x<y?-1:x>y?1:0;}
        switch(op){case SUMA:z=x+y;break;case RESTA:z=x-y;break;case MULT:z=x*y;break;
        case DIV:if(y==0)falla("R005: división entre cero");else z=x/y;break;
        case MOD:if(y==0)falla("R005: módulo entre cero");else z=std::fmod(x,y);break;
        case POT:z=std::pow(x,y);break;case IGUAL:z=cmp==0;break;case DIF:z=cmp!=0;break;case MENOR:z=cmp<0;break;case MENORIG:z=cmp<=0;break;case MAYOR:z=cmp>0;break;case MAYORIG:z=cmp>=0;break;case Y:z=x!=0&&y!=0;break;case O:z=x!=0||y!=0;break;case NO:z=x==0;break;case NEG:z=-x;break;default:falla("R006: operación desconocida");}
        if(!std::isfinite(z)||(t==1&&(std::floor(z)!=z||z<std::numeric_limits<long long>::min()||z>std::numeric_limits<long long>::max())))falla("R007: resultado fuera de representación");
        if(errorEjecucion.empty()){numeros[r]=z;MMemoria[r][1]=1;valoresRecorrido[paso][2]=valor(r);}++pc;
    }
    if(!errorEjecucion.empty()&&nr&&instruccionAbierta)MRecorrido[nr-1][1]=-1;
    return errorEjecucion.empty();
}
