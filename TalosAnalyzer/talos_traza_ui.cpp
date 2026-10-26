#include "talosanalyzer.h"
#include "ui_talosanalyzer.h"
#include "talossemantica.h"
#include "talos_matrices_ii.h"
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QListWidget>
#include <QSlider>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QSignalBlocker>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QPainterPath>
#include <QPainter>
#include <QPolygonF>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <algorithm>

namespace {
const char *const pilaNombre[5]={"Operadores","Operandos","Tipos","Saltos","AVAIL"};
QTableWidget *tabla(const QStringList &columnas,QWidget *parent=nullptr){
    auto *t=new QTableWidget(0,columnas.size(),parent);t->setHorizontalHeaderLabels(columnas);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->verticalHeader()->hide();t->verticalHeader()->setDefaultSectionSize(25);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setWordWrap(false);return t;
}
void celda(QTableWidget *t,int r,int c,const QString &s){auto *v=new QTableWidgetItem(s);v->setToolTip(s);t->setItem(r,c,v);}
QString tipo(int t){return t>0&&t<=6?QString::fromUtf8(TiposII::nombreTipo[t]):QString();}
QString marca(int p,int v){return p==TalosSemantica::SALTOS?QString("FF S%1 · fondo falso").arg(-v-10000):QString("FF ( · fondo falso");}
bool esMarca(int p,int v){return (p==TalosSemantica::OPERADORES&&v==TiposII::FF)||(p==TalosSemantica::SALTOS&&v<0);}
}

void TalosAnalyzer::construirSemanticaUI(){
    auto *tabs=new QTabWidget(this);tabs->setObjectName("pestanasTalos");
    QWidget *original=takeCentralWidget();tabs->addTab(original,"Análisis léxico y sintáctico");setCentralWidget(tabs);
    auto *pagina=new QWidget;auto *vertical=new QVBoxLayout(pagina);
    auto *barra=new QHBoxLayout;
    habilitarSemantica=new QCheckBox("Semántica II");habilitarSemantica->setToolTip("Desmarcar conserva el reconocimiento de Autómatas I");habilitarSemantica->setChecked(true);habilitarSemantica->setObjectName("habilitarSemantica");barra->addWidget(habilitarSemantica);
    auto *inicio=new QPushButton("Inicio"),*atras=new QPushButton("◀ Anterior"),*adelante=new QPushButton("Siguiente ▶"),*fin=new QPushButton("Final");
    barra->addWidget(inicio);barra->addWidget(atras);movimiento=new QSlider(Qt::Horizontal);movimiento->setObjectName("movimientoSemantico");movimiento->setRange(0,0);barra->addWidget(movimiento,1);barra->addWidget(adelante);barra->addWidget(fin);
    estadoSemantico=new QLabel("Sin análisis semántico");barra->addWidget(estadoSemantico);vertical->addLayout(barra);
    detalleMovimiento=new QPlainTextEdit;detalleMovimiento->setObjectName("detalleMovimiento");detalleMovimiento->setReadOnly(true);detalleMovimiento->setFixedHeight(64);vertical->addWidget(detalleMovimiento);
    auto *vistas=new QTabWidget;vistas->setObjectName("vistasSemanticas");vertical->addWidget(vistas,1);

    auto *generacion=new QSplitter(Qt::Horizontal);vistas->addTab(generacion,"1 · Generación y pilas");
    auto *board=new QWidget;auto *bl=new QVBoxLayout(board);bl->setContentsMargins(0,0,0,0);
    bl->addWidget(new QLabel("Cuádruplos · los tipos son los que tenía cada operando al emitir; ? indica salto pendiente"));
    tablaCuadruplos=tabla({"Num","COPER","OP1 : tipo","OP2 : tipo","RES : tipo","Salto y relleno"});tablaCuadruplos->setObjectName("tablaCuadruplos");bl->addWidget(tablaCuadruplos,3);
    auto *pilas=new QHBoxLayout;
    for(int j=0;j<5;++j){auto *g=new QGroupBox(pilaNombre[j]);auto *l=new QVBoxLayout(g);vistasPilas[j]=new QListWidget;vistasPilas[j]->setObjectName(QString("pila%1").arg(j));l->addWidget(vistasPilas[j]);pilas->addWidget(g);}
    bl->addLayout(pilas,1);generacion->addWidget(board);
    auto *historia=new QTabWidget;historia->setMinimumWidth(450);generacion->addWidget(historia);generacion->setStretchFactor(0,3);generacion->setStretchFactor(1,2);
    tablaSaltos=tabla({"Q","Salto","Creado M","RES inicial","Rellenado M","RES actual"});tablaSaltos->setObjectName("historialSaltos");historia->addTab(tablaSaltos,"Saltos y rellenos");
    tablaHistoria=tabla({"M","Acción","Pila","Elemento o detalle"});tablaHistoria->setObjectName("historialPilas");historia->addTab(tablaHistoria,"PUSH y POP");
    tablaFondos=tabla({"Marca","Pila","PUSH M","POP M","Estado"});tablaFondos->setObjectName("historialFondos");historia->addTab(tablaFondos,"Fondos falsos");

    auto *datos=new QWidget;auto *dl=new QVBoxLayout(datos);vistas->addTab(datos,"2 · Símbolos tipos y funciones");
    auto *par=new QSplitter(Qt::Horizontal);dl->addWidget(par,2);
    auto grupoTabla=[&](const QString&title,const QStringList&heads,QTableWidget *&t,QWidget *parent){auto *g=new QGroupBox(title,parent);auto *l=new QVBoxLayout(g);t=tabla(heads);l->addWidget(t);return g;};
    par->addWidget(grupoTabla("Tabla de símbolos",{"NID","VARIABLE","TIPO","RAM","AP"},tablaSimbolos,nullptr));tablaSimbolos->setObjectName("tablaSimbolos");tablaSimbolos->setToolTip("RAM=1000+AP. AP es índice de MMemoria. El ámbito se antepone al nombre local.");
    par->addWidget(grupoTabla("Tabla de constantes",{"NCTE","NOMBRE","TIPO"},tablaConstantes,nullptr));tablaConstantes->setObjectName("tablaConstantes");
    dl->addWidget(grupoTabla("Funciones · firma con tipos explícitos",{"Nombre","Retorno","Firma","Entrada Q","Parámetros","Locales"},tablaFunciones,nullptr),1);tablaFunciones->setObjectName("tablaFunciones");

    auto *ejecucion=new QWidget;auto *el=new QVBoxLayout(ejecucion);vistas->addTab(ejecucion,"3 · Ejecución y diagrama de saltos");
    el->addWidget(new QLabel("Entradas read/input: una por línea. Ejecutar genera un recorrido real; el deslizador permite inspeccionarlo sin volver a ejecutar."));
    auto *ev=new QHBoxLayout;entradasSemanticas=new QPlainTextEdit;entradasSemanticas->setObjectName("entradasEjecucion");entradasSemanticas->setPlaceholderText("Ejemplo CASE: 15");entradasSemanticas->setFixedHeight(98);ev->addWidget(entradasSemanticas,1);
    salidaSemantica=new QPlainTextEdit;salidaSemantica->setObjectName("salidaEjecucion");salidaSemantica->setReadOnly(true);salidaSemantica->setFixedHeight(98);ev->addWidget(salidaSemantica,2);el->addLayout(ev);
    ejecutarSemantica=new QPushButton("Ejecutar cuádruplos y registrar recorrido");ejecutarSemantica->setObjectName("ejecutarCuadruplos");ejecutarSemantica->setEnabled(false);el->addWidget(ejecutarSemantica);
    auto *rb=new QHBoxLayout;auto *prev=new QPushButton("◀ Paso"),*next=new QPushButton("Paso ▶");rb->addWidget(prev);rb->addWidget(new QLabel("Paso ejecutado"));pasoEjecucion=new QSlider(Qt::Horizontal);pasoEjecucion->setObjectName("pasoEjecucion");pasoEjecucion->setRange(0,0);rb->addWidget(pasoEjecucion,1);rb->addWidget(next);estadoEjecucion=new QLabel("Sin ejecutar");rb->addWidget(estadoEjecucion);el->addLayout(rb);
    auto *recorrido=new QSplitter(Qt::Horizontal);el->addWidget(recorrido,1);
    tablaRecorrido=tabla({"Paso","PC","COPER","OP1 valor","OP2 valor","Resultado","Condición","Decisión","Siguiente PC"});tablaRecorrido->setObjectName("tablaRecorrido");recorrido->addWidget(tablaRecorrido);
    grafoSemantico=new QGraphicsView;grafoSemantico->setObjectName("diagramaSaltos");grafoSemantico->setMinimumWidth(390);grafoSemantico->setRenderHint(QPainter::Antialiasing);recorrido->addWidget(grafoSemantico);recorrido->setStretchFactor(0,3);recorrido->setStretchFactor(1,2);
    tabs->addTab(pagina,"Semántica y cuádruplos");resize(1580,940);
    connect(inicio,&QPushButton::clicked,this,[this]{movimiento->setValue(0);});connect(fin,&QPushButton::clicked,this,[this]{movimiento->setValue(movimiento->maximum());});
    connect(atras,&QPushButton::clicked,this,[this]{movimiento->setValue(movimiento->value()-1);});connect(adelante,&QPushButton::clicked,this,[this]{movimiento->setValue(movimiento->value()+1);});
    connect(movimiento,&QSlider::valueChanged,this,&TalosAnalyzer::mostrarMovimiento);
    connect(pasoEjecucion,&QSlider::valueChanged,this,&TalosAnalyzer::mostrarPasoEjecucion);
    connect(prev,&QPushButton::clicked,this,[this]{pasoEjecucion->setValue(pasoEjecucion->value()-1);});connect(next,&QPushButton::clicked,this,[this]{pasoEjecucion->setValue(pasoEjecucion->value()+1);});
    connect(ejecutarSemantica,&QPushButton::clicked,this,&TalosAnalyzer::ejecutarCuadruplos);
    connect(ui->JTACodigo,&QPlainTextEdit::textChanged,this,&TalosAnalyzer::reiniciarSemantica);
    connect(habilitarSemantica,&QCheckBox::toggled,this,&TalosAnalyzer::reiniciarSemantica);
    connect(tablaHistoria,&QTableWidget::cellClicked,this,[this](int r,int){auto *v=tablaHistoria->item(r,0);if(v)movimiento->setValue(v->text().toInt());});
    connect(tablaSaltos,&QTableWidget::cellClicked,this,[this](int r,int c){auto *v=tablaSaltos->item(r,c==4?4:2);if(v&&v->text().toInt()>0)movimiento->setValue(v->text().toInt());});
    connect(tablaFondos,&QTableWidget::cellClicked,this,[this](int r,int c){auto *v=tablaFondos->item(r,c==3?3:2);if(v&&v->text().toInt()>0)movimiento->setValue(v->text().toInt());});
    connect(tablaRecorrido,&QTableWidget::cellClicked,this,[this](int r,int){auto *v=tablaRecorrido->item(r,0);if(v)pasoEjecucion->setValue(v->text().toInt());});
}

void TalosAnalyzer::reiniciarSemantica(){
    semanticaValida=false;if(!movimiento)return;QSignalBlocker a(movimiento),b(pasoEjecucion);
    movimiento->setRange(0,0);pasoEjecucion->setRange(0,0);estadoSemantico->setText("Sin análisis semántico");estadoEjecucion->setText("Sin ejecutar");detalleMovimiento->clear();salidaSemantica->clear();
    for(auto *t:{tablaCuadruplos,tablaSimbolos,tablaConstantes,tablaFunciones,tablaHistoria,tablaSaltos,tablaFondos,tablaRecorrido})t->setRowCount(0);
    for(auto *v:vistasPilas)v->clear();if(grafoSemantico->scene())grafoSemantico->scene()->clear();ejecutarSemantica->setEnabled(false);
}

QString TalosAnalyzer::describirEvento(int i)const{
    using S=TalosSemantica;if(!semantica||i<0||i>=semantica->ne)return {};
    const int *e=semantica->MMovimientos[i];int k=e[0],p=e[1],v=e[2];
    if(k==S::PUSH||k==S::POP){QString x;
        if(esMarca(p,v))x=marca(p,v);
        else if(p==S::OPERADORES)x=TiposII::nombreOp[v];
        else if(p==S::TIPOS)x=tipo(v);
        else if(p==S::SALTOS)x=QString("Q%1 · referencia de salto").arg(v);
        else x=QString::fromStdString(semantica->referencia(v))+(p==S::AVAIL?" · temporal libre":QString(" [AP %1]").arg(v));
        return QString(k==S::PUSH?"PUSH %1 → %2":"POP %1 ← %2").arg(pilaNombre[p],x);
    }
    if(k==S::RELLENO)return QString("RELLENAR Q%1: PENDIENTE (?) → Q%2").arg(e[1]).arg(e[2]);
    if(k==S::CUADRUPLO){int op=e[2];QString x=QString("EMITIR Q%1: %2").arg(e[1]+1).arg(TiposII::nombreOp[op]);if(op>=TiposII::GOTO&&op<=TiposII::GOTOV)x+=e[5]<0?" · RES=PENDIENTE (?)":QString(" · destino Q%1").arg(e[5]);return x;}
    if(k==S::ACCION)return QString::fromUtf8(MatricesII::nombreAccion[e[1]-3000])+" · "+QString::fromStdString(semantica->mensajes[i]);
    if(k==S::SIMBOLO){int n=e[1];return "DECLARAR "+QString::fromStdString(semantica->referencia(n))+" : "+tipo(semantica->MSimbolos[n][0]);}
    if(k==S::CONSTANTE){int n=e[1];return "CONSTANTE "+QString::fromStdString(semantica->literales[n])+" : "+tipo(semantica->MConstantes[n][0]);}
    return QString::fromStdString(semantica->mensajes[i]);
}

void TalosAnalyzer::mostrarMovimiento(int paso){
    using S=TalosSemantica;if(!semantica||paso<0||paso>semantica->ne)return;
    // Reconstrucción por eventos. Nunca se usa el RES final al consultar un paso anterior.
    int pilas[5][S::NP]{},top[5]{},q[S::NQ][4]{},creado[S::NQ]{},relleno[S::NQ]{},nq=0,ns=0,nc=0;
    // Historial grande en memoria dinámica: el apuntador evita agotar la pila de C++.
    int (*fondos)[5]=new int[S::NE][5]{};int nf=0,marcas[5][S::NP]{},tm[5]{};
    tablaHistoria->setRowCount(paso);
    for(int i=0;i<paso;++i){const int *e=semantica->MMovimientos[i];int k=e[0],p=e[1],v=e[2];
        if(k==S::PUSH&&top[p]<S::NP){pilas[p][top[p]++]=v;if(esMarca(p,v)&&tm[p]<S::NP){int *f=fondos[nf];f[0]=p;f[1]=v;f[2]=i+1;marcas[p][tm[p]++]=nf++;}}
        else if(k==S::POP&&top[p]>0){--top[p];if(esMarca(p,v)&&tm[p])fondos[marcas[p][--tm[p]]][3]=i+1;}
        else if(k==S::CUADRUPLO){int z=e[1];for(int j=0;j<4;++j)q[z][j]=e[j+2];nq=z+1;creado[z]=i+1;}
        else if(k==S::RELLENO){q[e[1]-1][3]=e[2];relleno[e[1]-1]=i+1;}
        else if(k==S::SIMBOLO)ns=e[1]+1;
        else if(k==S::CONSTANTE)nc=e[1]+1;
        celda(tablaHistoria,i,0,QString::number(i+1));celda(tablaHistoria,i,1,k==S::PUSH?"PUSH":k==S::POP?"POP":k==S::RELLENO?"RELLENO":k==S::CUADRUPLO?"CUÁDRUPLO":k==S::ACCION?"ACCIÓN":"TABLA");
        celda(tablaHistoria,i,2,k==S::PUSH||k==S::POP?pilaNombre[p]:"—");celda(tablaHistoria,i,3,describirEvento(i));
        if(k==S::RELLENO)for(int c=0;c<4;++c)tablaHistoria->item(i,c)->setBackground(QColor(34,88,53));
    }
    tablaHistoria->scrollToBottom();tablaCuadruplos->setRowCount(nq);tablaSaltos->setRowCount(0);
    for(int i=0;i<nq;++i){int op=q[i][0];celda(tablaCuadruplos,i,0,QString::number(i+1));celda(tablaCuadruplos,i,1,TiposII::nombreOp[op]);
        for(int c=1;c<=3;++c){QString s=QString::fromStdString(semantica->campoCuadruplo(op,c,q[i][c]));int t=semantica->MTiposCuadruplo[i][c-1];if(c==1&&(op==TiposII::ERA||op==TiposII::GOSUB||op==TiposII::ENDFUNC))t=semantica->MFunciones[q[i][1]][0];if(c==2&&op==TiposII::PARAM)t=semantica->MParametros[semantica->MFunciones[q[i][3]][2]+q[i][2]][2];if(c==3&&op==TiposII::RET)t=semantica->MFunciones[q[i][3]][0];if(t)s+=" : "+tipo(t);celda(tablaCuadruplos,i,c+1,s);}
        QString ruta="—";
        if(op>=TiposII::GOTO&&op<=TiposII::GOTOV){
            ruta=q[i][3]<0?QString("? · pendiente desde M%1").arg(creado[i]):relleno[i]?QString("? → Q%1 · relleno M%2").arg(q[i][3]).arg(relleno[i]):QString("Q%1 · directo").arg(q[i][3]);
            int r=tablaSaltos->rowCount();tablaSaltos->insertRow(r);
            QString vals[]={QString::number(i+1),QString::fromUtf8(TiposII::nombreOp[op]),QString::number(creado[i]),relleno[i]||q[i][3]<0?"PENDIENTE (?)":QString::number(q[i][3]),relleno[i]?QString::number(relleno[i]):"—",q[i][3]<0?"PENDIENTE (?)":QString("Q%1").arg(q[i][3])};
            for(int c=0;c<6;++c)celda(tablaSaltos,r,c,vals[c]);
            for(int c=0;c<6;++c)tablaSaltos->item(r,c)->setBackground(q[i][3]<0?QColor(100,63,14):QColor(32,74,49));
        }
        celda(tablaCuadruplos,i,5,ruta);
    }
    tablaFondos->setRowCount(nf);
    for(int i=0;i<nf;++i){int *f=fondos[i];QString vs[]={marca(f[0],f[1]),pilaNombre[f[0]],QString::number(f[2]),f[3]?QString::number(f[3]):"—",f[3]?"RETIRADO por POP":"ACTIVO"};for(int c=0;c<5;++c)celda(tablaFondos,i,c,vs[c]);}
    tablaSimbolos->setRowCount(ns);
    for(int i=0;i<ns;++i){celda(tablaSimbolos,i,0,QString::number(i+1));celda(tablaSimbolos,i,1,QString::fromStdString(semantica->referencia(i)));celda(tablaSimbolos,i,2,tipo(semantica->MSimbolos[i][0]));celda(tablaSimbolos,i,3,QString::number(1000+semantica->MSimbolos[i][1]));celda(tablaSimbolos,i,4,QString::number(semantica->MSimbolos[i][1]));}
    tablaConstantes->setRowCount(nc);for(int i=0;i<nc;++i){celda(tablaConstantes,i,0,QString::number(i+1));celda(tablaConstantes,i,1,QString::fromStdString(semantica->literales[i]));celda(tablaConstantes,i,2,tipo(semantica->MConstantes[i][0]));}
    // La firma completa es un dato de compilación terminada, no un dato anticipado del paso.
    tablaFunciones->setRowCount(paso==semantica->ne?semantica->nFunciones:0);
    if(paso==semantica->ne)for(int i=0;i<semantica->nFunciones;++i){int *f=semantica->MFunciones[i];QString firma=QString::fromStdString(semantica->funciones[i])+" : "+tipo(f[0])+" (";
        for(int j=0;j<f[3];++j){int *p=semantica->MParametros[f[2]+j];if(j)firma+=", ";firma+=QString::fromStdString(semantica->nombres[p[1]])+" : "+tipo(p[2]);}firma+=")";
        QString v[]={QString::fromStdString(semantica->funciones[i]),tipo(f[0]),firma,QString::number(f[1]),QString::number(f[3]),QString::number(f[7])};for(int c=0;c<6;++c)celda(tablaFunciones,i,c,v[c]);}
    for(int p=0;p<5;++p){vistasPilas[p]->clear();if(!top[p])vistasPilas[p]->addItem("VACÍA · ver historial");
        for(int j=top[p]-1;j>=0;--j){int v=pilas[p][j];QString s;
            if(esMarca(p,v))s=marca(p,v);else if(p==S::OPERADORES)s=TiposII::nombreOp[v];else if(p==S::TIPOS)s=tipo(v);else if(p==S::SALTOS)s=QString("Q%1").arg(v);else s=QString::fromStdString(semantica->referencia(v));
            if(p==S::OPERANDOS&&j<top[S::TIPOS])s+=" : "+tipo(pilas[S::TIPOS][j]);
            vistasPilas[p]->addItem(s);
        }
    }
    estadoSemantico->setText(QString("M %1/%2 · CONT=%3%4").arg(paso).arg(semantica->ne).arg(nq+1).arg(paso==semantica->ne?(semanticaValida?" · correcto":" · error"):""));
    detalleMovimiento->setPlainText(paso?QString("M%1 · %2:%3\n%4").arg(paso).arg(semantica->MMovimientos[paso-1][6]).arg(semantica->MMovimientos[paso-1][7]).arg(describirEvento(paso-1)):"Inicio: los movimientos PUSH/POP, marcas de fondo falso, saltos pendientes y rellenos quedan conservados en los historiales.");
    delete[] fondos;
}

void TalosAnalyzer::ejecutarCuadruplos(){
    if(!semanticaValida||!semantica)return;std::string datos[1024];QString entrada=entradasSemanticas->toPlainText();QStringList lineas=entrada.split('\n');int n=entrada.isEmpty()?0:lineas.size();
    if(n>1024){salidaSemantica->setPlainText("R001: máximo 1024 datos de entrada");return;}
    for(int i=0;i<n;++i)datos[i]=lineas[i].toStdString();semantica->ejecutar(datos,n);
    pasoEjecucion->setRange(0,semantica->nr);pasoEjecucion->setValue(semantica->nr);mostrarPasoEjecucion(semantica->nr);
}

void TalosAnalyzer::mostrarPasoEjecucion(int paso){
    if(!semantica||paso<0||paso>semantica->nr)return;using namespace TiposII;
    // Ventana de 1000 filas: el deslizador permite consultar todo MRecorrido sin
    // crear cientos de miles de widgets. No se trunca ni se altera la matriz.
    int inicio=std::max(0,paso-1000),fin=std::min(semantica->nr,std::max(1000,paso));tablaRecorrido->setRowCount(fin-inicio);
    for(int i=inicio;i<fin;++i){const int *r=semantica->MRecorrido[i];int fila=i-inicio;QString condicion=r[3]<0?"—":r[3]?"VERDADERO":"FALSO";
        QString decision=r[1]<0?"ERROR":r[2]==GOTOF||r[2]==GOTOV?(r[4]?"SALTA":"NO SALTA"):r[4]?"TRANSFERENCIA":"SECUENCIAL";
        QString vals[]={QString::number(i+1),QString("Q%1").arg(r[0]),nombreOp[r[2]],QString::fromStdString(semantica->valoresRecorrido[i][0]),QString::fromStdString(semantica->valoresRecorrido[i][1]),QString::fromStdString(semantica->valoresRecorrido[i][2]),condicion,decision,r[1]<0?"ERROR":r[1]==semantica->nq+1?"FIN":QString("Q%1").arg(r[1])};
        for(int j=0;j<3;++j){int t=semantica->MTiposCuadruplo[r[0]-1][j];if(t&&vals[j+3]!="—")vals[j+3]+=" : "+tipo(t);}
        for(int c=0;c<9;++c){celda(tablaRecorrido,fila,c,vals[c]);if(i>=paso)tablaRecorrido->item(fila,c)->setForeground(QColor(135,135,135));}
    }
    QString salida;for(int i=0;i<paso;++i)if(semantica->MRecorrido[i][2]==OUT&&semantica->MRecorrido[i][1]>=0)salida+=QString::fromStdString(semantica->valoresRecorrido[i][2])+"\n";
    if(paso==semantica->nr&&!semantica->errorEjecucion.empty())salida+=QString::fromStdString(semantica->errorEjecucion);
    salidaSemantica->setPlainText(salida);estadoEjecucion->setText(QString("%1 / %2 · filas %3–%4").arg(paso).arg(semantica->nr).arg(inicio+1).arg(fin));
    if(paso>0){tablaRecorrido->selectRow(paso-1-inicio);tablaRecorrido->scrollToItem(tablaRecorrido->item(paso-1-inicio,0));}
    dibujarFlujo(paso);
}

void TalosAnalyzer::dibujarFlujo(int paso){
    if(!semantica)return;using namespace TiposII;
    auto *escena=grafoSemantico->scene();if(!escena){escena=new QGraphicsScene(grafoSemantico);grafoSemantico->setScene(escena);}escena->clear();
    int visitas[TalosSemantica::NQ+2]{},ramas[TalosSemantica::NQ+2][2]{};for(int i=0;i<paso;++i){const int *r=semantica->MRecorrido[i];++visitas[r[0]];if(r[1]>=1)++ramas[r[0]][r[4]?1:0];}
    int actual=paso?semantica->MRecorrido[paso-1][0]:0,siguiente=paso?semantica->MRecorrido[paso-1][1]:0;
    auto flecha=[&](int de,int a,const QString &texto,bool sec){
        if(a<1||a>semantica->nq+1)return;double y1=(de-1)*74+24,y2=(a-1)*74+24;bool elegida=de==actual&&a==siguiente;int op=semantica->MCuadruplos[de-1][0];if(elegida&&(op==GOTOF||op==GOTOV))elegida=sec==!semantica->MRecorrido[paso-1][4];QColor color=elegida?QColor("#eab308"):ramas[de][sec?0:1]?QColor("#4ade80"):QColor("#94a3b8");QPen pen(color,elegida?3:1);
        QPainterPath camino;if(sec){camino.moveTo(160,y1+24);camino.lineTo(160,y2-24);}else{double x=365+(de%4)*18;camino.moveTo(320,y1);camino.lineTo(x,y1);camino.lineTo(x,y2);camino.lineTo(320,y2);}escena->addPath(camino,pen);
        auto *t=escena->addText(texto,QFont("Consolas",9));t->setDefaultTextColor(color);t->setPos(sec?166:326,sec?y1+24:y1-20);
        QPolygonF cabeza;if(sec)cabeza<<QPointF(160,y2-24)<<QPointF(156,y2-32)<<QPointF(164,y2-32);else cabeza<<QPointF(320,y2)<<QPointF(328,y2-4)<<QPointF(328,y2+4);escena->addPolygon(cabeza,pen,QBrush(color));
    };
    for(int i=1;i<=semantica->nq;++i){const int *q=semantica->MCuadruplos[i-1];int op=q[0];if(op==GOTO)flecha(i,q[3],QString("goto → Q%1").arg(q[3]),false);
        else if(op==GOTOF||op==GOTOV){flecha(i,q[3],op==GOTOV?"V · salta":"F · salta",false);flecha(i,i+1,op==GOTOV?"F · sigue":"V · sigue",true);}
        else if(op==GOSUB)flecha(i,q[2],"llamada",false);
        else if(op!=RET&&op!=ENDFUNC)flecha(i,i+1,"",true);
    }
    if(actual&&semantica->MRecorrido[paso-1][4]&&(semantica->MRecorrido[paso-1][2]==RET||semantica->MRecorrido[paso-1][2]==ENDFUNC))flecha(actual,siguiente,"retorno real",false);
    QGraphicsRectItem *seleccion=nullptr;
    for(int i=1;i<=semantica->nq+1;++i){double y=(i-1)*74;QColor relleno=i==actual?QColor("#155e75"):visitas[i]?QColor("#14532d"):QColor("#243447");auto *r=escena->addRect(0,y,320,48,QPen(QColor("#cbd5e1")),QBrush(relleno));
        QString texto=i==semantica->nq+1?"FIN · salida del programa":QString("Q%1  %2").arg(i).arg(nombreOp[semantica->MCuadruplos[i-1][0]]);
        if(i<=semantica->nq){const int *q=semantica->MCuadruplos[i-1];QString detalle;for(int c=1;c<=3;++c){if(c>1)detalle+=" | ";detalle+=QString::fromStdString(semantica->campoCuadruplo(q[0],c,q[c]));}r->setToolTip(texto+"\n"+detalle);texto+="\n"+detalle.left(42);}
        auto *t=escena->addText(texto,QFont("Consolas",9));t->setDefaultTextColor(Qt::white);t->setPos(4,y);if(i==actual)seleccion=r;
    }
    escena->setSceneRect(-5,-5,455,(semantica->nq+1)*74);if(seleccion)grafoSemantico->ensureVisible(seleccion,15,40);
}
