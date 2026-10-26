#include "talossemantica.h"
#include "talos_matrices_ii.h"
using namespace TiposII;

// Acciones de los diagramas DECLARA_FUNCTION, PARAMETROS, LLAMADA_F y EST_RETURN.
// Sin árbol sintáctico: el parser ejecuta estos enteros desde MProducciones.
int TalosSemantica::buscarFuncion(const std::string &s) const {
    for(int i=0;i<nFunciones;++i)if(funciones[i]==s)return i;
    return -1;
}
std::string TalosSemantica::campoCuadruplo(int op,int columna,int v) const {
    // No confundir números de función/parámetro/cuádruplo con índices de memoria.
    if((columna==1&&(op==ERA||op==GOSUB||op==ENDFUNC))||
       (columna==3&&(op==PARAM||op==RET)))return v>=0&&v<nFunciones?funciones[v]:"?";
    if(columna==2&&op==PARAM)return "P"+std::to_string(v+1);
    if((columna==3&&op>=GOTO&&op<=GOTOV)||(columna==2&&op==GOSUB))return v<0?"PENDIENTE":std::to_string(v);
    return referencia(v);
}
bool TalosSemantica::retornaEnTodosLosCaminos(int f) const {
    // Recorrido de flujo por índices. ENDFUNC alcanzable significa que falta return.
    int pila[NQ],sp=0;bool visitado[NQ]{};
    int inicio=MFunciones[f][1],fin=MFunciones[f][5];
    pila[sp++]=inicio;visitado[inicio-1]=true;
    while(sp){int pc=pila[--sp];const int *q=MCuadruplos[pc-1];
        if(q[0]==ENDFUNC)return false;
        if(q[0]==RET)continue;
        int destinos[2]={pc+1,-1};
        if(q[0]==GOTO)destinos[0]=q[3];
        else if(q[0]==GOTOF||q[0]==GOTOV)destinos[1]=q[3];
        for(int d:destinos)if(d>=inicio&&d<=fin&&!visitado[d-1]){
            visitado[d-1]=true;pila[sp++]=d;
        }
    }
    return true;
}
void TalosSemantica::accionModulo(int ac,const std::string &lex){
    using namespace MatricesII;
    int a=-1,t=0,f=-1;
    switch(ac){
    case FUNC_NAME:
        if(nFunciones>=NFUNC){fallar("S081: directorio de funciones lleno");break;}
        if(buscarFuncion(lex)>=0||buscar(lex)>=0){fallar("S081: nombre de función duplicado: "+lex);break;}
        funcionActual=nFunciones++;funciones[funcionActual]=lex;
        MFunciones[funcionActual][2]=nParametros;
        MFunciones[funcionActual][4]=emitir(GOTO,-1,-1,-1);
        MFunciones[funcionActual][6]=ns;nPendientes=0;
        break;
    case FUNC_TYPE:
        MFunciones[funcionActual][0]=tipoDecl;break;
    case PARAM_ID:
        if(nPendientes>=NS){fallar("S092: lista de parámetros llena");break;}
        pendientes[nPendientes++]=lex;break;
    case PARAM_TYPE:
        if(tipoDecl==6){fallar("S083: un parámetro no puede ser void");break;}
        for(int i=0;i<nPendientes&&error.empty();++i){
            if(nParametros>=NPARAM){fallar("S083: matriz de parámetros llena");break;}
            a=declarar(pendientes[i],tipoDecl);if(!error.empty())break;
            int *p=MParametros[nParametros++];p[0]=funcionActual;p[1]=a;p[2]=tipoDecl;
            ++MFunciones[funcionActual][3];
        }nPendientes=0;break;
    case FUNC_PARAMS_END:
        if(nPendientes)fallar("S083: parámetros sin tipo");break;
    case FUNC_BODY:
        MFunciones[funcionActual][1]=nq+1;
        MFunciones[funcionActual][7]=ns-MFunciones[funcionActual][6]-MFunciones[funcionActual][3];break;
    case FUNC_END:
        f=funcionActual;MFunciones[f][5]=emitir(ENDFUNC,f,-1,-1);
        if(MFunciones[f][0]!=6&&!retornaEnTodosLosCaminos(f)){fallar("S086: falta return en algún camino de "+funciones[f]);break;}
        rellenar(MFunciones[f][4],nq+1);funcionActual=-1;break;
    case NAME_REF:case STMT_NAME:
        a=buscar(lex);f=buscarFuncion(lex);
        // Una variable local puede ocultar una variable global, no se confunde con función.
        if(a>=0)operando(a);
        else if(f>=0){push(OPERANDOS,-2000-f);push(TIPOS,MFunciones[f][0]);}
        else fallar("S002: identificador o función no declarado: "+lex);
        break;
    case REF_END:
        if(peek(OPERANDOS)<=-2000)fallar("S082: falta llamada con paréntesis a la función");break;
    case CALL_BEGIN:
        a=sacarOperando(t);f=-2000-a;
        if(f<0||f>=nFunciones){fallar("S082: el identificador no es una función");break;}
        if(nLlamadas>=NFUNC){fallar("S084: llamadas anidadas excesivas");break;}
        MLlamadas[nLlamadas][0]=f;MLlamadas[nLlamadas][1]=0;++nLlamadas;
        emitir(ERA,f,-1,-1);push(OPERADORES,FF);break;
    case CALL_ARG:
        if(!nLlamadas){fallar("S084: argumento sin llamada");break;}
        f=MLlamadas[nLlamadas-1][0];a=buscar(lex);t=MLlamadas[nLlamadas-1][1];
        if(a<0){fallar("S002: argumento no declarado: "+lex);break;}
        if(t>=MFunciones[f][3]){fallar("S084: sobran argumentos en "+funciones[f]);break;}
        if(!MAsignacion[MParametros[MFunciones[f][2]+t][2]][MMemoria[a][0]]){fallar("S085: tipo incompatible de argumento en "+funciones[f]);break;}
        emitir(PARAM,a,t,f);++MLlamadas[nLlamadas-1][1];break;
    case CALL_END:case CALL_STMT_END:
        if(!nLlamadas){fallar("S084: cierre de llamada sin inicio");break;}
        f=MLlamadas[nLlamadas-1][0];t=MFunciones[f][0];
        if(MLlamadas[nLlamadas-1][1]!=MFunciones[f][3]){fallar("S084: faltan argumentos en "+funciones[f]);break;}
        if(peek(OPERADORES)!=FF){fallar("S013: llamada perdió su fondo falso");break;}pop(OPERADORES);--nLlamadas;
        if(ac==CALL_END&&t==6){fallar("S087: una función void no produce operando");break;}
        a=t==6?-1:temporal(t);emitir(GOSUB,f,MFunciones[f][1],a,t);
        if(ac==CALL_END)operando(a);else liberar(a);break;
    case RETURN_VALUE:
        if(funcionActual<0){fallar("S086: return fuera de una función");break;}
        a=sacarOperando(t);f=funcionActual;
        if(MFunciones[f][0]==6||!MAsignacion[MFunciones[f][0]][t]){fallar("S087: tipo de return incompatible");break;}
        emitir(RET,a,-1,f,MFunciones[f][0]);liberar(a);break;
    case LIB_NAME:
        for(int j=0;j<nLibrerias;++j)if(librerias[j]==lex)return;
        if(nLibrerias>=32){fallar("S089: directorio de bibliotecas lleno");break;}
        librerias[nLibrerias++]=lex;break;
    default:fallar("S089: acción de módulo sin implementación");
    }
}
