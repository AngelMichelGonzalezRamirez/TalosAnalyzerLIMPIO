#include "talosanalyzer.h"
#include "talossemantica.h"
#include <QCheckBox>
#include "ui_talosanalyzer.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <stack>
#include <QString>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <limits>


TalosAnalyzer::TalosAnalyzer(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::TalosAnalyzer),
    reservedWords({
        {"include", 136},
        {"lib", 137},
        {"endlib", 138},
        {"class", 139},
        {"endclass", 140},
        {"int", 141},
        {"float", 142},
        {"char", 143},
        {"string", 144},
        {"if", 145},
        {"elseif", 146},
        {"else", 147},
        {"endif", 148},
        {"do", 149},
        {"enddo", 150},
        {"while", 151},
        {"endwhile", 152},
        {"read", 153},
        {"write", 154},
        {"def", 155},
        {"const", 156},
        {"of", 157},
        {"dowhile", 158},
        {"for", 159},
        {"endfor", 160},
        {"function", 161},
        {"end_function", 162},
        {"bool", 163},
        {"void", 164},
        {"return", 165},
        {"to", 166},
        {"case",167}, {"end",168}, {"true",171}, {"false",172},
        {"input",153}, {"output",154}, {"integer",141}
    })

{
    ui->setupUi(this);

    memcpy(M, M2, sizeof(M));

    connect(ui->JBAnalizar, &QPushButton::clicked, this, &TalosAnalyzer::JBAnalizar);
    connect(ui->JBAbrir,    &QPushButton::clicked, this, &TalosAnalyzer::JBAbrir);
    connect(ui->JBGuardar,  &QPushButton::clicked, this, &TalosAnalyzer::JBGuardar);
    connect(ui->JBLimpiar,  &QPushButton::clicked, this, &TalosAnalyzer::JBLimpiar);
    connect(ui->JBSalir,    &QPushButton::clicked, this, &QWidget::close);

    ui->JTACodigo->setReadOnly(false);
    ui->JTATokens->setReadOnly(true);
    ui->JTASintaxis->setReadOnly(true);
    ui->JTAErrores->setReadOnly(true);
    construirSemanticaUI();
}  // Interfaz I conservada en la primera pestaña.


TalosAnalyzer::~TalosAnalyzer()
{
    delete semantica;
    delete ui;
}

void TalosAnalyzer::JBAnalizar()
{
    reiniciarSemantica();
    ui->JTASintaxis->clear();
    Analizar(); // Run lexical analysis
    if (ui->JTAErrores->toPlainText().isEmpty()) {
        if(habilitarSemantica->isChecked()) parseII();
        else parse(); // Recorrido de I conservado para funciones y corpus histórico.
    }
    else ui->JTASintaxis->appendPlainText("Sintaxis no ejecutada: hay errores léxicos.");
}

void TalosAnalyzer::Analizar()
{
    ui->JTATokens->clear();
    ui->JTAErrores->clear();
    tokenInicios.clear();
    tokens.clear(); // Assuming a std::vector<std::pair<QString, int>> tokens member is added to store tokens
    tokenIndex = 0;
    lexemaInicio = 0;

    QString texto = ui->JTACodigo->toPlainText();
    texto.replace("\r\n", "\n");
    texto.replace('\r', '\n');
    if (!texto.isEmpty() && texto.front().unicode() == 0xFEFF) texto.remove(0, 1);
    codigo = texto.toStdString();
    if (codigo.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        Error(555, "entrada demasiado grande para el índice entero");
        return;
    }

    size_t eofInicio = codigo.size();
    int i = 0;
    int estado = 0;
    std::string lexema = "";
    std::stack<QString> tokenStack;
    int charCount = 0;

    while (i < codigo.length()) {
        char c = codigo[i];
        // MSeparadores: columnas ':' y '.'. M2[26][32] permanece byte a byte igual.
        static const int MSeparadores[2][2] = { {169,1}, {-1,170} };
        if (estado==3 && c=='.' && i+1<static_cast<int>(codigo.size()) && codigo[i+1]=='.') {
            Token(102,QString::fromStdString(lexema)); tokens.push_back({QString::fromStdString(lexema),102});
            estado=0; lexema.clear(); continue; // No consumir el punto del rango.
        }
        if (estado==0 && (c==':' || c=='.')) {
            lexemaInicio=static_cast<size_t>(i);
            int columnaSeparador=c==':'?0:1;
            int aceptado=MSeparadores[0][columnaSeparador];
            int longitud=1;
            if (aceptado==1) {
                aceptado=134;
                if(i+1<static_cast<int>(codigo.size()) && codigo[i+1]=='.') { aceptado=MSeparadores[1][1]; longitud=2; }
            }
            QString texto=QString::fromStdString(codigo.substr(i,longitud));
            Token(aceptado,texto); tokens.push_back({texto,aceptado}); i+=longitud; continue;
        }

        if ((c == ' ' || c == '\t' || c == '\n' || c == '\b' || c == '\r') && estado == 0) {
            i++;
            continue;
        }

        lexemaInicio = lexema.empty() ? static_cast<size_t>(i) : lexemaInicio;
        if (estado < 0 || estado >= 26) { Error(555, "estado fuera de M2"); break; }
        int columna = relaciona(c);
        int anchoCaracter=1;
        // Clasificación adicional de letras latinas UTF-8 (opción, área).
        // Columnas: prefijo, mínimo segundo byte, máximo segundo byte, columna M2.
        static const int MLatin[4][4]={{195,128,150,2},{195,152,159,2},{195,160,182,1},{195,184,191,1}};
        if ((estado==0||estado==1||estado==2) && i+1<static_cast<int>(codigo.size()))
            for(int j=0;j<4;++j) if(static_cast<unsigned char>(c)==MLatin[j][0] &&
                static_cast<unsigned char>(codigo[i+1])>=MLatin[j][1] && static_cast<unsigned char>(codigo[i+1])<=MLatin[j][2]) {
                columna=MLatin[j][3];anchoCaracter=2;break;
            }
        // E/e son letras en identificadores; exponentes en el estado numérico.
        if ((estado == 0 || estado == 1 || estado == 2) && (c == 'e' || c == 'E'))
            columna = c == 'e' ? 1 : 2;
        const int *fila = M2[estado]; // Apuntador a la fila validada; no modifica M2.
        bool is_specific_transition = (columna >= 1 && columna <= 32 && fila[columna - 1] != 555);
        int siguiente_estado = is_specific_transition ? fila[columna - 1] : fila[30];

        if (siguiente_estado >= 100 && siguiente_estado <= 135) { // Terminal states
            if (is_specific_transition && columna != 31) {
                lexema += c;
                if (estado == 15) charCount++;
                i++;
            }
            if (siguiente_estado == 135) {
                // EOF: termina esta lectura; no se procesa el texto posterior.
                eofInicio = static_cast<size_t>(i - 1);
                lexema.clear();
                break;
            }
            QString lexemaQString = QString::fromStdString(lexema);
            if (siguiente_estado == 101) { // Identifier or reserved word
                std::string lexemaStd = lexema;
                auto it = reservedWords.find(lexemaStd);
                if (it != reservedWords.end()) {
                    int tokenType = it->second;
                    Token(tokenType, lexemaQString);
                    tokens.push_back({lexemaQString, tokenType});
                    tokenStack.push(lexemaQString + "\t" + getTokenType(tokenType));
                } else {
                    Token(101, lexemaQString);
                    tokens.push_back({lexemaQString, 101});
                    tokenStack.push(lexemaQString + "\t" + getTokenType(101));
                }
            } else if (siguiente_estado == 125) { // Character constant
                if (charCount == 0) {
                    Error(505, lexemaQString);
                } else if (charCount > 1) {
                    Error(506, lexemaQString);
                } else {
                    Token(siguiente_estado, lexemaQString);
                    tokens.push_back({lexemaQString, siguiente_estado});
                    tokenStack.push(lexemaQString + "\t" + getTokenType(siguiente_estado));
                }
            } else { // Other tokens, including EOF
                Token(siguiente_estado, lexemaQString);
                tokens.push_back({lexemaQString, siguiente_estado});
                tokenStack.push(lexemaQString + "\t" + getTokenType(siguiente_estado));
            }
            while (!tokenStack.empty()) tokenStack.pop();
            estado = 0;
            lexema = "";
            charCount = 0;
            if (siguiente_estado == 135) { // EOF found
                break;
            }
        } else if (siguiente_estado == 555 || siguiente_estado >= 500) { // Errors
            // Recuperar un carácter con más de un símbolo sin convertir su resto en tokens.
            if (estado == 16 && c != '\'' && c != '\n' && c != '\r') {
                while (i < static_cast<int>(codigo.size()) && codigo[i] != '\'' &&
                       codigo[i] != '\n' && codigo[i] != '\r') lexema += codigo[i++];
                if (i < static_cast<int>(codigo.size()) && codigo[i] == '\'') {
                    lexema += codigo[i++];
                    siguiente_estado = 506;
                } else siguiente_estado = 507;
            }
            if (lexema.empty() || (is_specific_transition && columna != 31)) {
                lexema += c;
                i++;
            }
            Error(siguiente_estado, QString::fromStdString(lexema));
            estado = 0;
            lexema = "";
            charCount = 0;
        } else { // Intermediate state
            lexema += c;
            if (estado == 15) charCount++;
            estado = siguiente_estado;
            if(anchoCaracter==2)lexema+=codigo[i+1];
            i+=anchoCaracter;
        }
    }

    // Handle end of file if no $ is found
    if (!lexema.empty()) {
        int siguiente_estado = M2[estado][30];
        if (estado == 22) siguiente_estado = M2[22][10]; // comentario termina también en EOF
        if (estado == 15 || estado == 16 || estado == 17 || estado == 23 || estado == 24) {
            siguiente_estado = 507; // Unclosed literal
        }
        QString lexemaQString = QString::fromStdString(lexema);
        if (siguiente_estado >= 100 && siguiente_estado <= 135) {
            if (siguiente_estado == 101) {
                std::string lexemaStd = lexema;
                auto it = reservedWords.find(lexemaStd);
                if (it != reservedWords.end()) {
                    int tokenType = it->second;
                    Token(tokenType, lexemaQString);
                    tokens.push_back({lexemaQString, tokenType});
                    tokenStack.push(lexemaQString + "\t" + getTokenType(tokenType));
                } else {
                    Token(101, lexemaQString);
                    tokens.push_back({lexemaQString, 101});
                    tokenStack.push(lexemaQString + "\t" + getTokenType(101));
                }
            } else if (siguiente_estado == 125) {
                if (charCount == 0) {
                    Error(505, lexemaQString);
                } else if (charCount > 1) {
                    Error(506, lexemaQString);
                } else {
                    Token(siguiente_estado, lexemaQString);
                    tokens.push_back({lexemaQString, siguiente_estado});
                    tokenStack.push(lexemaQString + "\t" + getTokenType(siguiente_estado));
                }
            } else {
                Token(siguiente_estado, lexemaQString);
                tokens.push_back({lexemaQString, siguiente_estado});
                tokenStack.push(lexemaQString + "\t" + getTokenType(siguiente_estado));
            }
            while (!tokenStack.empty()) tokenStack.pop();
        } else {
            Error(siguiente_estado >= 500 ? siguiente_estado : 555, lexemaQString);
        }
    }
    tokenInicios.push_back(eofInicio);
    tokens.push_back({"$", 135}); // Add EOF token
}

int TalosAnalyzer::relaciona(char c) {
    if (c == 'E') return 6;
    if (c == 'e') return 7;
    if (c == '+') return 8;
    if (c == '-') return 9;
    if (c == '_') return 4;
    if (c == '.') return 5;
    if (c == '\t') return 10;
    if (c == '\n') return 11;
    if (c == '\b') return 12;
    if (c == '&') return 13;
    if (c == '|') return 14;
    if (c == '=') return 15;
    if (c == '<') return 16;
    if (c == '>') return 17;
    if (c == '!') return 18;
    if (c == '\'') return 19;
    if (c == '"') return 20;
    if (c == '#') return 21;
    if (c == ',') return 22;
    if (c == ';') return 23;
    if (c == '[') return 24;
    if (c == ']') return 25;
    if (c == '(') return 26;
    if (c == ')') return 27;
    if (c == '/') return 28;
    if (c == '%') return 29;
    if (c == '*') return 30;
    if (c == '$') return 32;
    if (c >= 'a' && c <= 'z') return 1;
    if (c >= 'A' && c <= 'Z') return 2;
    if (c >= '0' && c <= '9') return 3;
    return 31; // different
}

QString TalosAnalyzer::getTokenType(int e)
{
    if (e >= 136 && e <= 162) {
        for (const auto& pair : reservedWords) {
            if(pair.first=="input"||pair.first=="output"||pair.first=="integer") continue; // Mostrar el nombre canónico.
            if (pair.second == e) {
                QString word = QString::fromStdString(pair.first);
                return word + " Palabra reservada " + word;
            }
        }
        return "Palabra reservada desconocida";
    }

    switch (e) {
    case 167: return "Case";
    case 168: return "End";
    case 169: return "Dos puntos";
    case 170: return "Rango inclusivo";
    case 171: return "Verdadero";
    case 172: return "Falso";
    case 101: return "Identificador";
    case 102: return "Numero Entero";
    case 103: return "Numero Flotante";
    case 104: return "Notacion cientifica";
    case 105: return "Suma";
    case 106: return "Resta";
    case 107: return "Multiplicacion";
    case 108: return "Division";
    case 109: return "Asigna";
    case 110: return "Igual";
    case 111: return "Menor";
    case 112: return "Menor Igual";
    case 113: return "Mayor";
    case 114: return "Mayor Igual";
    case 115: return "Diferente";
    case 116: return "Not";
    case 117: return "And";
    case 118: return "Or";
    case 119: return "Parentesis abre";
    case 120: return "Parentesis cierra";
    case 121: return "Corchete abre";
    case 122: return "Corchete cierra";
    case 123: return "Punto y coma";
    case 124: return "Coma";
    case 125: return "Constante Caracter";
    case 126: return "Constante String";
    case 127: return "Hashtag";
    case 128: return "Modulus";
    case 129: return "Incremento Unario";
    case 130: return "Decremento Unario";
    case 131: return "Comentario en linea";
    case 132: return "Comentario en bloque";
    case 133: return "Potencia";
    case 134: return "Punto";
    case 135: return "EOF";
    case 136: return "Include";
    case 137: return "Library";
    case 138: return "Endlib";
    case 139: return "Class";
    case 140: return "Endclass";
    case 141: return "Int";
    case 142: return "Float";
    case 143: return "Char";
    case 144: return "String";
    case 145: return "If";
    case 146: return "Elseif";
    case 147: return "Else";
    case 148: return "Endif";
    case 149: return "Do";
    case 150: return "Enddo";
    case 151: return "While";
    case 152: return "Endwhile";
    case 153: return "Read";
    case 154: return "Write";
    case 155: return "Def";
    case 156: return "Const";
    case 157: return "Of";
    case 158: return "Dowhile";
    case 159: return "For";
    case 160: return "Endfor";
    case 161: return "Function";
    case 162: return "Endfunction";
    case 163: return "Bool";
    case 164: return "Void";
    case 165: return "Return";
    case 166: return "To";
    default: return "Token desconocido";
    }
}

void TalosAnalyzer::Token(int e, const QString &lexema)
{
    tokenInicios.push_back(lexemaInicio);
    QString tipo = getTokenType(e);
    ui->JTATokens->appendPlainText(lexema + "\t" + tipo);
}

void TalosAnalyzer::Error(int e, const QString &lexema)
{
    QString msg;
    switch (e) {
    case 500: msg = QString("500: espera digito despues del punto en '%1'").arg(lexema); break;
    case 501: msg = QString("501: espera '+'/'-' o digito en '%1'").arg(lexema); break;
    case 502: msg = QString("502: espera digito despues del '+'/'-' en '%1'").arg(lexema); break;
    case 503: msg = QString("503: espera '&' despues de '&' en '%1'").arg(lexema); break;
    case 504: msg = QString("504: espera '|' despues de '|' en '%1'").arg(lexema); break;
    case 505:
        if (lexema == "''") {
            msg = "505: constante de carácter vacía";
        } else {
            msg = "505: literal de carácter mal formado en '" + lexema + "'";
        }
        break;
    case 506: msg = QString("506: múltiples caracteres en '%1'").arg(lexema); break;
    case 507: msg = "507: literal o comentario sin cerrar/mal formado en '" + lexema + "'"; break;
    case 555: msg = QString("555: elemento no reconocido en '%1'").arg(lexema); break;
    default:  msg = QString("%1: error desconocido en '%2'").arg(e).arg(lexema); break;
    }
    if (e == 506 && !lexema.startsWith('\''))
        msg = "506: elemento no reconocido en '" + lexema + "'";
    size_t linea = 1, columna = 1;
    for (size_t j = 0; j < lexemaInicio && j < codigo.size(); ++j) {
        if (codigo[j] == '\n') { ++linea; columna = 1; }
        else if((static_cast<unsigned char>(codigo[j])&192)!=128) ++columna;
    }
    ui->JTAErrores->appendPlainText(QString("%1:%2  ").arg(linea).arg(columna) + msg);
}

// Misma lógica de archivos para los botones y las pruebas; sin cambiar matrices.
bool TalosAnalyzer::cargarArchivo(const QString &fileName, QString &detalle) {
    detalle.clear();
    if (fileName.isEmpty()) return false; // cancelar conserva el contenido
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { detalle=file.errorString(); return false; }
    QTextStream in(&file);
    const QString texto=in.readAll();
    if (in.status()!=QTextStream::Ok || file.error()!=QFileDevice::NoError) {
        detalle="No se pudo completar la lectura: " + file.errorString();
        return false;
    }
    file.close();
    ui->JTACodigo->setPlainText(texto);
    ui->JTATokens->clear(); ui->JTASintaxis->clear(); ui->JTAErrores->clear();
    tokens.clear(); tokenInicios.clear(); tokenIndex=0; codigo.clear(); lexemaInicio=0;
    return true;
}

bool TalosAnalyzer::guardarArchivo(const QString &fileName, QString &detalle) {
    detalle.clear();
    if (fileName.isEmpty()) return false;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { detalle=file.errorString(); return false; }
    QTextStream out(&file);
    out << ui->JTACodigo->toPlainText();
    out.flush();
    const bool completo=out.status()==QTextStream::Ok && file.flush() && file.error()==QFileDevice::NoError;
    if (!completo) detalle="No se pudo completar la escritura: " + file.errorString();
    file.close();
    return completo;
}

void TalosAnalyzer::JBAbrir() {
    const QString fileName=QFileDialog::getOpenFileName(this,tr("Abrir Archivo Talos"),"",tr("Talos Files (*.tls)"));
    if (fileName.isEmpty()) return;
    QString detalle;
    if (!cargarArchivo(fileName,detalle)) QMessageBox::warning(this,tr("Error de lectura"),detalle);
}

void TalosAnalyzer::JBGuardar() {
    const QString fileName=QCoreApplication::applicationDirPath()+"/archivo_guardado.tls";
    QString detalle;
    if (guardarArchivo(fileName,detalle))
        QMessageBox::information(this,tr("Archivo Guardado"),tr("Archivo guardado en:\n%1").arg(fileName));
    else QMessageBox::warning(this,tr("Error de escritura"),detalle);
}

void TalosAnalyzer::JBLimpiar()
{
    reiniciarSemantica();
    ui->JTACodigo->clear();
    ui->JTATokens->clear();
    ui->JTASintaxis->clear();
    ui->JTAErrores->clear();
    tokens.clear(); tokenInicios.clear(); tokenIndex = 0; codigo.clear(); lexemaInicio = 0;
}

#include <map>  // Asegúrate de incluir esta línea si no está ya en el .cpp

// Inicialización del mapa no terminal
std::map<std::string, int> TalosAnalyzer::nonTerminalMap = {
    {"PROGRAM", 0}, {"PROGRAM2", 1}, {"DECLARA_CLASS", 2}, {"DECLARA_LIBRARY", 3},
    {"DECLARACONST", 4}, {"IDCOF", 5}, {"IDCOF'", 6}, {"DECLARAVAR", 7},
    {"TIPO", 8}, {"VALOR", 9}, {"IDCIGUAL", 10}, {"IDCIGUAL'", 11},
    {"PARAMETROS", 12}, {"PARAMETROS'", 13}, {"DECLARA_FUNCTION", 14},
    {"ESTATUTOS", 15}, {"ESTATUTOS'", 16}, {"EST_ASIG", 17}, {"IDCREAD", 18},
    {"IDCREAD'", 19}, {"EST_READ", 20}, {"WRITE2", 21}, {"WRITE2'", 22},
    {"EST_WRITE", 23}, {"PREINCUNARIO", 24}, {"POSTINCUNARIO", 25}, {"EST_DO", 26},
    {"IF1", 27}, {"IF2", 28}, {"EST_IF", 29}, {"EST_WHILE", 30}, {"EST_FOR", 31},
    {"EST_RETURN", 32}, {"EXPR", 33}, {"EXPR'", 34}, {"EXPR2", 35}, {"EXPR2'", 36},
    {"EXPR3", 37}, {"EXPR4", 38}, {"EXPR41", 39}, {"EXPR5", 40}, {"EXPR5'", 41},
    {"TERM", 42}, {"TERM'", 43}, {"OPREL", 44}, {"FACT", 45}, {"IDCC", 46},
    {"IDCC'", 47}, {"LLAMADA_F", 48}
};

// Inicialización del mapa terminal
std::map<std::string, int> TalosAnalyzer::terminalMap = {
    {"include", 0}, {"def", 1}, {"const", 2}, {"function", 3}, {"class", 4},
    {"id", 6}, {"=", 10}, {";", 11}, {",", 12}, {"(", 13}, {")", 14},
    {"ctestring", 15}, {"read", 16}, {"write", 17}, {"++", 18}, {"--", 19},
    {"do", 20}, {"if", 21}, {"elseif", 22}, {"else", 23}, {"endif", 24},
    {"while", 25}, {"for", 26}, {"return", 27}, {"$", 28},
    {"end_function", 29}, {"dowhile", 30}, {"endfor", 31}, {"cteentera", 32},
    {"ctereal", 33}, {"ctenotacion", 34}, {"ctecaracter", 35}, {"of", 36},
    {"!", 37}, {"||", 38}, {"&&", 39}, {"==", 40}, {"!=", 41}, {"<", 42},
    {"<=", 43}, {">", 44}, {">=", 45}, {"+", 46}, {"-", 47}, {"*", 48},
    {"/", 49}, {"%", 50},
    {"**", 5},
    {".", 7},
    {"bool", 8},
    {"char", 9},
    {"endclass", 51},
    {"enddo", 52},
    {"endwhile", 53},
    {"float", 54},
    {"int", 55},
    {"lib", 56},
    {"string", 57},
    {"to", 58},
    {"void", 59}
};

// Function to get the next token from the lexical analyzer
std::pair<QString, int> TalosAnalyzer::getNextToken() {
    while (tokenIndex < tokens.size()) {
        auto token = tokens[tokenIndex++];
        if (token.second == 131 || token.second == 132) continue;
        return token;
    }
    return {"$", 135}; // Return EOF if no more tokens
}

// Categorías de los códigos aceptados por la matriz original.
std::string TalosAnalyzer::categoriaToken(const std::pair<QString, int> &token) {
        switch (token.second) {
        case 153: return "read";
        case 154: return "write";
        case 141: return "int";
        case 101: return "id";
        case 102: return "cteentera";
        case 103: return "ctereal";
        case 104: return "ctenotacion";
        case 125: return "ctecaracter";
        case 126: return "ctestring";
        case 135: return "$";
        default: return token.first.toStdString();
        }
}

void TalosAnalyzer::errorSintactico(const QString &mensaje) {
    QString resultado=mensaje;
    if (tokenIndex>0 && tokenInicios.size()==tokens.size() && tokenIndex<=tokens.size()) {
        const size_t offset=tokenInicios[tokenIndex-1];
        size_t linea=1, columna=1;
        for(size_t j=0;j<offset && j<codigo.size();++j) {
            if(codigo[j]=='\n') { ++linea; columna=1; }
            else if((static_cast<unsigned char>(codigo[j])&192)!=128) ++columna;
        }
        const QString token=tokens[tokenIndex-1].second==135 ? "<EOF>" : tokens[tokenIndex-1].first;
        resultado=QString("%1:%2  Token %3: ").arg(linea).arg(columna).arg(token)+mensaje;
    }
    ui->JTASintaxis->appendPlainText(resultado);
    ui->JTAErrores->appendPlainText(resultado);
}

// Syntactic analysis function
void TalosAnalyzer::parse() {
    ui->JTASintaxis->clear();
    if (tokens.empty() || tokens.back().second != 135) {
        ui->JTAErrores->appendPlainText("Error interno: flujo de tokens sin EOF real.");
        ui->JTASintaxis->appendPlainText("Sintaxis no ejecutada: flujo sin EOF real.");
        return;
    }
    for (size_t j = 0; j + 1 < tokens.size(); ++j) {
        if (tokens[j].second == 135) {
            ui->JTAErrores->appendPlainText("Error interno: EOF antes del final del flujo de tokens.");
            ui->JTASintaxis->appendPlainText("Sintaxis no ejecutada: EOF duplicado/intermedio.");
            return;
        }
    }
    std::stack<std::string> stack;
    stack.push("$");
    stack.push("PROGRAM"); // Initial symbol
    tokenIndex = 0; // Reset token index
    auto currentToken = getNextToken();

    size_t pasos = 0;
    const size_t limite = tokens.size() > (std::numeric_limits<size_t>::max() - 10000) / 100
        ? std::numeric_limits<size_t>::max() : 10000 + tokens.size() * 100;
    while (!stack.empty()) {
        if (++pasos > limite) { errorSintactico("Error interno: límite del recorrido de la pila."); return; }
        std::string top = stack.top();
        stack.pop();
        const std::string entrada = categoriaToken(currentToken);

        if (top == "$" && entrada == "$" && tokenIndex == tokens.size()) {
            ui->JTASintaxis->appendPlainText("Cadena sintácticamente correcta");
            return;
        }
        const auto nt = nonTerminalMap.find(top);
        if (nt == nonTerminalMap.end()) {
            if (top == entrada && entrada != "$") currentToken = getNextToken();
            else { errorSintactico("Error sintáctico: se esperaba " + QString::fromStdString(top) +
                         ", se encontró " + currentToken.first); return; }
            continue;
        }
        const auto terminal = terminalMap.find(entrada);
        if (terminal == terminalMap.end()) {
            errorSintactico("Contrato original: terminal sin columna: " + QString::fromStdString(entrada));
            return;
        }
        const int row = nt->second, col = terminal->second;
        if (row < 0 || row >= 49 || col < 0 || col >= 60) {
            errorSintactico(QString("Contrato original: índice [%1][%2] fuera de predictiveMatrix[49][60].").arg(row).arg(col));
            return;
        }
        const int production = predictiveMatrix[row][col];
        if (production == 666) {
            errorSintactico("Error sintáctico: no hay producción para " + QString::fromStdString(top) +
                  " con " + QString::fromStdString(entrada));
            return;
        }
        if (production <= 0 || static_cast<size_t>(production) > productions.size()) {
            errorSintactico(QString("Contrato original: celda [%1][%2] referencia producción %3; existen %4.")
                      .arg(row).arg(col).arg(production).arg(productions.size()));
            return;
        }
        const auto &prod = productions[production - 1];
        for (auto it = prod.rbegin(); it != prod.rend(); ++it)
            if (!it->empty()) stack.push(*it);
    }
    errorSintactico("Error interno: pila agotada sin consumir EOF.");
}


    const int TalosAnalyzer::M2[26][32] = {
                //  1    2    3   4   5     6    7    8    9   10   11   12    13   14   15   16   17   18    19    20   21   22   23   24   25   26  27    28    29   30     31   32
                //  l    L    d   _    .    E    e    +    -   \t   \n   \b    &    |    =    <    >    !     '      "   #    ,    ;    [    ]    (    )     /    %     *     ≠     $
    /*0*/       {   1,   2,   3, 555, 134, 555, 555,  19,  20,   0,   0,   0,  13,  14,   9,  10,  11,  12,   15,   17, 127, 124, 123, 121, 122, 119, 120,   21, 128,   25,  506,  135 }, // estado 0
/*1*/       {   1,   2,   2,   2, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  101,  555 }, // Cambiado de 100 a 101
    /*2*/       {   2,   2,   2,   2, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  101,  555 }, // 2
    /*3*/       { 555, 555,   3, 555,   4, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  102,  555 }, // 3
    /*4*/       { 555, 555,   5, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  500,  555 }, // 4
    /*5*/       { 555, 555,   5, 555, 555,   6,   6, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  103,  555 }, // 5
    /*6*/       { 555, 555,   8, 555, 555, 555, 555,   7,   7, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  501,  555 }, // 6
    /*7*/       { 555, 555,   8, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  502,  555 }, // 7
    /*8*/       { 555, 555,   8, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  104,  555 }, // 8
    /*9*/       { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 110, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  109,  555 }, // 9
    /*10*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 112, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  111,  555 }, // 10
    /*11*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 114, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  113,  555 }, // 11
    /*12*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 115, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  116,  555 }, // 12
    /*13*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 117, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  503,  555 }, // 13
    /*14*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 118, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  504,  555 }, // 14
    /*15*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  505,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  16 ,  555 }, // 15
    /*16*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  125,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  507,  555 }, // 16
    /*17*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  18,  555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  17 ,  555 }, // 17
    /*18*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  17,  555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  126,  555 }, // 18
    /*19*/      { 555, 555, 555, 555, 555, 555, 555, 129, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  105,  555 }, // 19
    /*20*/      { 555, 555, 555, 555, 555, 555, 555, 555, 130, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  106,  555 }, // 20
    /*21*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  22,  555,  23,   108,  555 }, // 21
    /*22*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 131, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  555,  22 ,  555 }, // 22
    /*23*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  24,   23 ,  555 }, // 23
    /*24*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  132, 555,  24,   23 ,  555 }, // 24
    /*25*/      { 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555,  555,  555, 555, 555, 555, 555, 555, 555, 555,  555, 555,  133,  107,  555 }  // 25
        };


    const std::vector<std::vector<std::string>> TalosAnalyzer::productions = {
        {"DECLARA_LIBRARY", "PROGRAM2", "DECLARA_CLASS"}, // 1
        {"def", "IDCOF", "of", "TIPO", ";", "PROGRAM2"},                       // 2
        {"DECLARACONST", "PROGRAM2"},                     // 3
        {"DECLARA_FUNCTION", "PROGRAM2"},                 // 4
        {},                                               // 5 (ε)
        {"class", "ESTATUTOS", "endclass"},               // 6
        {"include", "id", ".", "lib", ";", "DECLARA_LIBRARY"}, // 7
        {},                                               // 8 (ε)
        {"const", "id", "=", "VALOR", ";"}, // 9
        {},                                               // 10 (ε) RETIRADA: no seleccionar en predictiveMatrix.
        {"id", "IDCOF'"},                                 // 11
        {",", "IDCOF"},                                   // 12
        {},                                               // 13 (ε)
        {"def", "IDCOF", "of", "TIPO", ";", "DECLARAVAR"}, // 14
        {},                                               // 15 (ε)
        {"int"},                                          // 16
        {"float"},                                        // 17
        {"char"},                                         // 18
        {"string"},                                       // 19
        {"bool"},                                         // 20
        {"void"},                                         // 21
        {"cteentera"},                                    // 22
        {"ctereal"},                                      // 23
        {"ctenotacion"},                                  // 24
        {"ctecaracter"},                                  // 25
        {"ctestring"},                                    // 26
        {"id", "IDCIGUAL'"},                              // 27
        {",", "IDCIGUAL"},                                // 28
        {},                                               // 29 (ε)
        {"IDCIGUAL", "=", "TIPO", "PARAMETROS'"},         // 30
        {},                                               // 31 (ε)
        {",", "IDCIGUAL", "=", "TIPO", "PARAMETROS'"},                              // 32
        {},                                               // 33 (ε)
        {"function", "id", "=", "TIPO", "(", "PARAMETROS", ")", "DECLARAVAR", "ESTATUTOS", "end_function"}, // 34
        {},                                               // 35 (ε) RETIRADA: no seleccionar en predictiveMatrix.
        {"EST_RETURN", ";", "ESTATUTOS"},                 // 36
        {"EST_WRITE", ";", "ESTATUTOS"},                  // 37
        {"EST_READ", ";", "ESTATUTOS"},                   // 38
        {"PREINCUNARIO", ";", "ESTATUTOS"},               // 39
        {"EST_IF", "ESTATUTOS"},                          // 40
        {"EST_WHILE", "ESTATUTOS"},                       // 41
        {"EST_DO", "ESTATUTOS"},                          // 42
        {"EST_FOR", "ESTATUTOS"},                         // 43
        {},                                               // 44 (ε)
        {"id", "ESTATUTOS'"},                             // 45
        {"EST_ASIG", ";", "ESTATUTOS"},                   // 46
        {"POSTINCUNARIO", ";", "ESTATUTOS"},              // 47
        {},                                               // 48 (ε) RETIRADA: no seleccionar en predictiveMatrix.
        {"=", "EXPR"},                                    // 49
        {"id", "IDCREAD'"},                               // 50
        {",", "IDCREAD"},                                 // 51
        {},                                               // 52 (ε)
        {"read", "(", "IDCREAD", ")"},                    // 53
        {"EXPR", "WRITE2'"},                              // 54
        {",", "WRITE2"},                                  // 55
        {},                                               // 56 (ε)
        {"write", "(", "WRITE2", ")"},                    // 57
        {"++", "id"},                                     // 58
        {"--", "id"},                                     // 59
        {"++"},                                           // 60
        {"--"},                                           // 61
        {"do", "ESTATUTOS", "dowhile", "(", "EXPR", ")", "enddo"}, // 62
        {"elseif", "(", "EXPR", ")", "ESTATUTOS", "IF1"}, // 63
        {},                                               // 64 (ε)
        {"else", "ESTATUTOS"},                            // 65
        {},                                               // 66 (ε)
        {"if", "(", "EXPR", ")", "ESTATUTOS", "IF1", "IF2", "endif"}, // 67
        {"while", "(", "EXPR", ")", "ESTATUTOS", "endwhile"}, // 68
        {"for", "id", "(", "EXPR", "to", "EXPR", ")", "ESTATUTOS", "endfor"}, // 69
        {"return", "EXPR"},                               // 70
        {"EXPR2", "EXPR'"},                               // 71
        {"||", "EXPR"},                                   // 72
        {},                                               // 73 (ε)
        {"EXPR3", "EXPR2'"},                              // 74
        {"&&", "EXPR2"},                                  // 75
        {},                                               // 76 (ε)
        {"EXPR4"},                                        // 77
        {"!", "EXPR4"},                                   // 78
        {"EXPR5", "EXPR41"},                              // 79
        {"OPREL", "EXPR5"},                               // 80
        {},                                               // 81 (ε)
        {"TERM", "EXPR5'"},                               // 82
        {"+", "EXPR5"},                                   // 83
        {"-", "EXPR5"},                                   // 84
        {},                                               // 85 (ε)
        {"FACT", "TERM'"},                                // 86
        {"*", "TERM"},                                    // 87
        {"/", "TERM"},                                    // 88
        {"%", "TERM"},                                    // 89
        {},                                               // 90 (ε)
        {"=="},                                           // 91
        {"!="},                                           // 92
        {"<"},                                            // 93
        {"<="},                                           // 94
        {">"},                                            // 95
        {">="},                                           // 96
        {"id", "LLAMADA_F"},                              // 97
        {"VALOR"},                                        // 98
        {"(", "EXPR", ")"},                               // 99
        {"id", "IDCC'"},                                  // 100
        {},                                               // 101 (ε)
        {",", "id", "IDCC'"},                                    // 102
        {},                                               // 103 (ε)
        {"(", "IDCC", ")"},                               // 104
        {}, // 105 LLAMADA_F; Diagrama p.12: bypass de LLAMADA_F; admite identificador simple.
        {"**", "TERM"}, // 106 TERM'; Diagrama p.12 y DOCX V2: operador de potencia **.
    };


    const int TalosAnalyzer::predictiveMatrix[49][60]= {
        // Columnas conservadas/completadas: 0=include, 1=def, 2=const, 3=function, 4=class, 5=**, 6=id, 7=., 8=bool, 9=char, 10==, 11=;, 12=,, 13=(, 14=), 15=ctestring, 16=read, 17=write, 18=++, 19=--, 20=do, 21=if, 22=elseif, 23=else, 24=endif, 25=while, 26=for, 27=return, 28=$, 29=end_function, 30=dowhile, 31=endfor, 32=cteentera, 33=ctereal, 34=ctenotacion, 35=ctecaracter, 36=of, 37=!, 38=||, 39=&&, 40===, 41=!=, 42=<, 43=<=, 44=>, 45=>=, 46=+, 47=-, 48=*, 49=/, 50=%, 51=endclass, 52=enddo, 53=endwhile, 54=float, 55=int, 56=lib, 57=string, 58=to, 59=void
        // Fila 0: PROGRAM
        {1, 1, 1, 1, 1, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 1: PROGRAM2
        {666, 2, 3, 4, 5, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 2: DECLARA_CLASS
        {666, 666, 666, 666, 6, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 3: DECLARA_LIBRARY
        {7, 8, 8, 8, 8, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 4: DECLARACONST
        {666, 666, 9, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 5: IDCOF
        {666, 666, 666, 666, 666, 666, 11, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 6: IDCOF'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 12, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 13, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 7: DECLARAVAR
        {666, 14, 666, 666, 666, 666, 15, 666, 666, 666, 666, 666, 666, 666, 666, 666, 15, 15, 15, 15, 15, 15, 666, 666, 666, 15, 15, 15, 666, 15, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 8: TIPO
        {666, 666, 666, 666, 666, 666, 666, 666, 20, 18, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 17, 16, 666, 19, 666, 21},
        // Fila 9: VALOR
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 26, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 22, 23, 24, 25, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 10: IDCIGUAL
        {666, 666, 666, 666, 666, 666, 27, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 11: IDCIGUAL'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 29, 666, 28, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 12: PARAMETROS
        {666, 666, 666, 666, 666, 666, 30, 666, 666, 666, 666, 666, 666, 666, 31, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 13: PARAMETROS'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 32, 666, 33, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 14: DECLARA_FUNCTION
        {666, 666, 666, 34, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 15: ESTATUTOS
        {666, 666, 666, 666, 666, 666, 45, 666, 666, 666, 666, 666, 666, 666, 666, 666, 38, 37, 39, 39, 42, 40, 44, 44, 44, 41, 43, 36, 666, 44, 44, 44, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 44, 666, 44, 666, 666, 666, 666, 666, 666},
        // Fila 16: ESTATUTOS'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 46, 666, 666, 666, 666, 666, 666, 666, 47, 47, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 17: EST_ASIG
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 49, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 18: IDCREAD
        {666, 666, 666, 666, 666, 666, 50, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 19: IDCREAD'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 51, 666, 52, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 20: EST_READ
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 53, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 21: WRITE2
        {666, 666, 666, 666, 666, 666, 54, 666, 666, 666, 666, 666, 666, 54, 666, 54, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 54, 54, 54, 54, 666, 54, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 22: WRITE2'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 55, 666, 56, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 23: EST_WRITE
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 57, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 24: PREINCUNARIO
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 58, 59, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 25: POSTINCUNARIO
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 60, 61, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 26: EST_DO
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 62, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 27: IF1
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 63, 64, 64, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 28: IF2
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 65, 66, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 29: EST_IF
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 67, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 30: EST_WHILE
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 68, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 31: EST_FOR
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 69, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 32: EST_RETURN
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 70, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 33: EXPR
        {666, 666, 666, 666, 666, 666, 71, 666, 666, 666, 666, 666, 666, 71, 666, 71, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 71, 71, 71, 71, 666, 71, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 34: EXPR'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 73, 73, 666, 73, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 72, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 73, 666},
        // Fila 35: EXPR2
        {666, 666, 666, 666, 666, 666, 74, 666, 666, 666, 666, 666, 666, 74, 666, 74, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 74, 74, 74, 74, 666, 74, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 36: EXPR2'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 76, 76, 666, 76, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 76, 75, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 76, 666},
        // Fila 37: EXPR3
        {666, 666, 666, 666, 666, 666, 77, 666, 666, 666, 666, 666, 666, 77, 666, 77, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 77, 77, 77, 77, 666, 78, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 38: EXPR4
        {666, 666, 666, 666, 666, 666, 79, 666, 666, 666, 666, 666, 666, 79, 666, 79, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 79, 79, 79, 79, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 39: EXPR41
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 81, 81, 666, 81, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 81, 81, 80, 80, 80, 80, 80, 80, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 81, 666},
        // Fila 40: EXPR5
        {666, 666, 666, 666, 666, 666, 82, 666, 666, 666, 666, 666, 666, 82, 666, 82, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 82, 82, 82, 82, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 41: EXPR5'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 85, 85, 666, 85, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 85, 85, 85, 85, 85, 85, 85, 85, 83, 84, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 85, 666},
        // Fila 42: TERM
        {666, 666, 666, 666, 666, 666, 86, 666, 666, 666, 666, 666, 666, 86, 666, 86, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 86, 86, 86, 86, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 43: TERM'
        {666, 666, 666, 666, 666, 106, 666, 666, 666, 666, 666, 90, 90, 666, 90, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 87, 88, 89, 666, 666, 666, 666, 666, 666, 666, 90, 666},
        // Fila 44: OPREL
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 91, 92, 93, 94, 95, 96, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 45: FACT
        {666, 666, 666, 666, 666, 666, 97, 666, 666, 666, 666, 666, 666, 99, 666, 98, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 98, 98, 98, 98, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 46: IDCC
        {666, 666, 666, 666, 666, 666, 100, 666, 666, 666, 666, 666, 666, 666, 101, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 47: IDCC'
        {666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 102, 666, 103, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666},
        // Fila 48: LLAMADA_F
        {666, 666, 666, 666, 666, 105, 666, 666, 666, 666, 666, 105, 105, 104, 105, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 666, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 105, 666, 666, 666, 666, 666, 666, 666, 105, 666},
    };

