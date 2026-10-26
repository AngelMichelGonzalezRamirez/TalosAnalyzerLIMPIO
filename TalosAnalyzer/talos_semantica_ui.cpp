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
#include <QScrollArea>
#include <QSignalBlocker>
#include <limits>

void TalosAnalyzer::parseII(){
    using namespace MatricesII;
    ui->JTASintaxis->clear();delete semantica;semantica=new TalosSemantica;
    const bool generar=habilitarSemantica->isChecked();
    if(tokens.empty()||tokens.back().second!=135){errorSintactico("Flujo sin EOF real");return;}
    for(size_t j=0;j+1<tokens.size();++j)if(tokens[j].second==135){errorSintactico("EOF duplicado/intermedio");return;}
    // Pila de reconocimiento: apuntador sp sobre arreglo de tamaño comprobado.
    int pila[8192],sp=0;pila[sp++]=-1-terminalMap["$"];pila[sp++]=1000;
    tokenIndex=0;auto entrada=getNextToken();std::pair<QString,int> ultimo;size_t ultimoIndice=0;
    size_t pasos=0,limite=10000+tokens.size()*100;
    while(sp&&++pasos<=limite){int x=pila[--sp];
        if(x>=3000){
            if(generar&&semantica->error.empty()){
                size_t offset=ultimoIndice<tokenInicios.size()?tokenInicios[ultimoIndice]:0;int lin=1,col=1;
                for(size_t k=0;k<offset&&k<codigo.size();++k){if(codigo[k]=='\n'){++lin;col=1;}else if((static_cast<unsigned char>(codigo[k])&192)!=128)++col;}
                semantica->accion(x,ultimo.first.toStdString(),ultimo.second,lin,col);
            }continue;
        }
        std::string categoria=categoriaToken(entrada);int columna=-1;
        for(int j=0;j<COLUMNAS;++j)if(categoria==terminales[j]){columna=j;break;}
        if(columna<0){errorSintactico("Terminal sin columna II: "+QString::fromStdString(categoria));return;}
        if(x<0){int esperado=-1-x;if(esperado!=columna){errorSintactico("Se esperaba "+QString::fromUtf8(terminales[esperado])+", se encontró "+entrada.first);return;}
            if(categoria=="$"){
                if(tokenIndex!=tokens.size()||sp){errorSintactico("EOF prematuro");return;}
                ui->JTASintaxis->appendPlainText("Cadena sintácticamente correcta");
                if(generar){semanticaValida=semantica->finalizar();
                    if(!semanticaValida)ui->JTAErrores->appendPlainText(QString("%1:%2  ").arg(semantica->errorLinea).arg(semantica->errorColumna)+QString::fromStdString(semantica->error));
                    movimiento->setRange(0,semantica->ne);movimiento->setValue(semantica->ne);mostrarMovimiento(semantica->ne);ejecutarSemantica->setEnabled(semanticaValida);
                }else estadoSemantico->setText("Sólo léxico y sintáctico");
                return;
            }
            ultimo=entrada;ultimoIndice=tokenIndex-1;entrada=getNextToken();continue;
        }
        int fila=x-1000;if(fila<0||fila>=FILAS){errorSintactico("Fila II fuera de rango");return;}
        int prod=MPredictiva[fila][columna];if(prod==666||prod<1||prod>PRODUCCIONES){errorSintactico("No hay producción de "+QString::fromUtf8(noTerminales[fila])+" con "+QString::fromStdString(categoria));return;}
        semantica->produccionesUsadas[prod]=true;
        const int *r=MProducciones[prod-1];int n=r[0];if(n<0||n>=ANCHO||sp+n>8192){errorSintactico("Límite de producción o pila sintáctica II");return;}
        for(int j=n;j>=1;--j)pila[sp++]=r[j];
    }
    errorSintactico("Límite de recorrido o pila agotada sin EOF");
}
