#ifndef TALOSANALYZER_H
#define TALOSANALYZER_H

#include <QMainWindow>
#include <QStack>
#include <QPair>
#include <vector>
#include <map>
#include <string>

class TalosSemantica;
class QTabWidget; class QTableWidget; class QListWidget; class QSlider;
class QLabel; class QPlainTextEdit; class QPushButton; class QCheckBox;
class QGraphicsView;

QT_BEGIN_NAMESPACE
namespace Ui { class TalosAnalyzer; }
QT_END_NAMESPACE

class TalosAnalyzer : public QMainWindow
{
    Q_OBJECT

public:
    explicit TalosAnalyzer(QWidget *parent = nullptr);
    ~TalosAnalyzer();

private slots:
    void JBAnalizar();
    void JBAbrir();
    void JBGuardar();
    void JBLimpiar();

private:
    friend struct TalosOriginalProbe; // Solo pruebas externas sobre el mismo driver.
    Ui::TalosAnalyzer *ui;
    size_t lexemaInicio = 0;
    std::vector<size_t> tokenInicios; // Posiciones paralelas; matrices y tokens originales intactos.
    int M[26][32];  // Matriz para el analizador léxico
    static const int M2[26][32];  // Matriz estática para el analizador léxico
    int M3[26][32];
    static const int predictiveMatrix[49][60];

    // Variables para el analizador léxico
    std::map<std::string, int> reservedWords;  // Palabras reservadas y sus tokens
    std::vector<std::pair<QString, int>> tokens;  // Lista de tokens generados por el léxico
    size_t tokenIndex = 0;  // Índice del token actual para el análisis sintáctico

    // Variables para el analizador sintáctico
    static const std::vector<std::vector<std::string>> productions;  // Lista de producciones
    std::map<std::pair<std::string, std::string>, int> predictiveTable;  // Matriz predictiva
    std::string codigo;  // Código fuente a analizar
    size_t pos = 0;  // Posición actual en el código fuente
    std::pair<QString, int> currentToken;  // Token actual (lexema, tipo)

    // Mapas para índices de la tabla predictiva
    static std::map<std::string, int> nonTerminalMap;  // Declaración del mapa no terminal como estático
    static std::map<std::string, int> terminalMap;

    // Métodos para el analizador léxico
    void Analizar();  // Método principal del analizador léxico
    int relaciona(char c);  // Mapea carácter a columna de la matriz
    void Token(int e, const QString &lexema);  // Procesa un token aceptado
    QString getTokenType(int e);  // Obtiene el tipo de token como string
    void Error(int e, const QString &lexema);  // Procesa un error léxico

    // Nuevos métodos para el analizador sintáctico
    std::pair<QString, int> getNextToken();  // Obtiene el siguiente token del léxico
    std::string categoriaToken(const std::pair<QString, int> &token);
    void errorSintactico(const QString &mensaje);
    void parse();  // Método principal del analizador sintáctico

    // Automatas II: los índices, matrices y parser de I se conservan como referencia.
    friend struct TalosSemanticaProbe;
    TalosSemantica *semantica = nullptr;
    QTableWidget *tablaCuadruplos=nullptr, *tablaSimbolos=nullptr, *tablaConstantes=nullptr;
    QTableWidget *tablaHistoria=nullptr, *tablaSaltos=nullptr, *tablaFondos=nullptr;
    QTableWidget *tablaFunciones=nullptr, *tablaRecorrido=nullptr;
    QGraphicsView *grafoSemantico=nullptr;
    QSlider *pasoEjecucion=nullptr;
    QLabel *estadoEjecucion=nullptr;
    QListWidget *vistasPilas[5]{};
    QSlider *movimiento=nullptr;
    QLabel *estadoSemantico=nullptr;
    QPlainTextEdit *detalleMovimiento=nullptr, *entradasSemanticas=nullptr, *salidaSemantica=nullptr;
    QPushButton *ejecutarSemantica=nullptr;
    QCheckBox *habilitarSemantica=nullptr;
    bool semanticaValida=false;
    void construirSemanticaUI();
    void reiniciarSemantica();
    void mostrarMovimiento(int paso);
    void ejecutarCuadruplos();
    void parseII();
    QString describirEvento(int indice) const;
    void mostrarPasoEjecucion(int paso);
    void dibujarFlujo(int paso);

    // Otras utilidades
    bool cargarArchivo(const QString &fileName, QString &detalle);
    bool guardarArchivo(const QString &fileName, QString &detalle);
    void clearTextEdit();  // Limpia todos los QTextEdit
};

#endif // TALOSANALYZER_H
