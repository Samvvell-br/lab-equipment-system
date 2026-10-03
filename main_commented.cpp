// =====================================================================================
//  SISTEMA DE GESTION DE EQUIPOS DE LABORATORIO
//  - Carga equipos y usuarios desde archivos de TEXTO (campos separados por '*').
//  - Guarda las sesiones de uso en un archivo BINARIO ("sesiones.dat").
//  - Usa memoria dinamica (new / delete[]) y recorre los arreglos con PUNTEROS.
// =====================================================================================

#include <iostream>   // cin y cout: entrada y salida por consola
#include <cstring>    // funciones de cadenas estilo C: strlen, strcpy, strcmp, strtok
#include <fstream>    // manejo de archivos: ifstream (leer), ofstream (escribir), fstream (ambos)
using namespace std;  // evita escribir std:: delante de cout, cin, ifstream, etc.

// Constante global con el nombre del archivo binario donde se guardan TODAS las sesiones.
// Es "const char*": un puntero a una cadena que no se puede modificar.
const char* ARCHIVO_SESIONES = "sesiones.dat";

// -------------------------------------------------------------------------------------
// ESTRUCTURA Equipo: representa un equipo del laboratorio (una linea del archivo de equipos)
// -------------------------------------------------------------------------------------
struct Equipo {
    int codigo;                    // identificador unico del equipo (se usa para buscarlo)
    char nombre[50];               // nombre del equipo (maximo 49 caracteres + '\0')
    char laboratorio[40];          // laboratorio al que pertenece
    char tipo[30];                 // tipo o categoria del equipo
    char estadoOperativo[20];      // "Disponible", "En uso", "Mantenimiento" u otro
    float costoEstimado;           // costo del equipo (se usa para calcular la penalizacion)
    int semestreMinimo;            // semestre minimo que debe cursar el usuario para usarlo
    char descripcionTecnica[100];  // descripcion tecnica libre
};

// -------------------------------------------------------------------------------------
// ESTRUCTURA Usuario: representa un estudiante (una linea del archivo de usuarios)
// -------------------------------------------------------------------------------------
struct Usuario {
    int codigoInstitucional;       // identificador unico del usuario
    char nombre[50];               // nombre del usuario
    char programaAcademico[50];    // carrera / programa
    int semestre;                  // semestre que cursa (se compara con semestreMinimo)
};

// -------------------------------------------------------------------------------------
// ESTRUCTURA SesionUso: un prestamo/uso de un equipo por un usuario.
// Se guarda COMPLETA en el archivo binario (cada registro ocupa sizeof(SesionUso) bytes).
// -------------------------------------------------------------------------------------
struct SesionUso {
    int codigoSesion;              // numero de sesion (1, 2, 3...). Coincide con su posicion en el archivo
    int codigoEquipo;              // "llave foranea": codigo del equipo usado
    int codigoUsuario;             // "llave foranea": codigo del usuario que lo usa
    char fecha[20];                // fecha escrita por el usuario (dd/mm/aaaa)
    int duracionProgramada;        // horas que se reservaron (1 a 8)
    int duracionReal;              // horas que realmente se uso (se llena al cerrar)
    bool cerrada;                  // false = sesion abierta, true = sesion cerrada
    char observacion[100];         // observaciones tecnicas al cerrar
    float penalizacion;            // dinero a pagar por exceder el tiempo
};

// -------------------------------------------------------------------------------------
// ingresarArchivo: pide por teclado el nombre de un archivo.
// PARAMETROS:
//   nombreArchivo[] -> arreglo: en C++ los arreglos SIEMPRE se pasan como puntero al
//                      primer elemento, asi que funciona "por referencia": lo que se
//                      escriba aqui queda guardado en la variable de main.
//   tam             -> POR VALOR: tamano maximo del arreglo (para no desbordarlo).
// -------------------------------------------------------------------------------------
void ingresarArchivo(char nombreArchivo[], int tam) {
    cout<<"Ingrese nombre del archivo: ";   // mensaje al usuario
    cin.getline(nombreArchivo, tam);        // lee la linea completa (admite espacios) hasta tam-1 caracteres
}

// -------------------------------------------------------------------------------------
// limpiarEspacios: quita los espacios al inicio y al final de un texto (como un "trim").
// PARAMETRO: texto[] -> arreglo (por referencia): la funcion MODIFICA el texto original.
// Se usa porque en el archivo los campos vienen como "  101 * Microscopio * ..."
// -------------------------------------------------------------------------------------
void limpiarEspacios(char texto[]) {
    int inicio = 0;                              // posicion del primer caracter util
    while (texto[inicio] == ' ') inicio++;       // avanza mientras haya espacios al inicio

    int fin = strlen(texto) - 1;                 // posicion del ultimo caracter
    while (fin >= inicio && texto[fin] == ' ') fin--;  // retrocede mientras haya espacios al final

    int j = 0;                                   // j = posicion donde se escribe
    for (int i = inicio; i <= fin; i++, j++) {   // i = posicion desde donde se lee
        texto[j] = texto[i];                     // corre los caracteres utiles al inicio del arreglo
    }
    texto[j] = '\0';                             // cierra la cadena con el terminador nulo
}

// -------------------------------------------------------------------------------------
// limpiarEntrada: repara cin despues de un error y descarta lo que quedo en el buffer.
// No recibe parametros ni retorna nada.
// -------------------------------------------------------------------------------------
void limpiarEntrada() {
    cin.clear();                 // quita el estado de error de cin (por ej. si se escribio letra en vez de numero)
    cin.ignore(10000, '\n');     // descarta hasta 10000 caracteres o hasta el Enter
}

// -------------------------------------------------------------------------------------
// leerTexto: lee un texto con VALIDACION (no vacio, no mas largo que tam-1). Da 3 intentos.
// PARAMETROS:
//   mensaje -> const char* : texto a mostrar; "const" = la funcion NO lo puede modificar.
//   destino -> char*       : PUNTERO (por referencia) donde se guarda lo leido.
//   tam     -> int         : POR VALOR, tamano maximo del destino.
// RETORNA: true si se leyo bien, false si fallo 3 veces (la operacion se cancela).
// -------------------------------------------------------------------------------------
bool leerTexto(const char* mensaje, char* destino, int tam) {
    int intentos = 0;                            // contador de intentos fallidos

    while (intentos < 3) {                       // maximo 3 intentos
        cout << mensaje;                         // muestra el mensaje recibido

        if (!cin.getline(destino, tam)) {        // si getline falla (el texto era demasiado largo)...
            limpiarEntrada();                    // ...repara cin y limpia el buffer
            cout << "Texto invalido (maximo " << (tam - 1) << " caracteres).\n";
            intentos++;                          // cuenta el intento fallido
            continue;                            // vuelve al inicio del while
        }

        if (destino[0] == '\0') {                // si el primer caracter es el nulo, el texto esta vacio
            cout << "No puede quedar vacio.\n";
            intentos++;
            continue;
        }
        return true;                             // lectura correcta
    }

    cout << "Demasiados intentos, se cancela la operacion.\n";
    return false;                                // se agotaron los intentos
}

// -------------------------------------------------------------------------------------
// leerEntero: lee un numero entero validando que sea numero y que este en [minimo, maximo].
// PARAMETROS:
//   mensaje -> const char* : texto a mostrar (no se modifica).
//   destino -> int*        : PUNTERO (paso por referencia): el numero leido se guarda en
//                            la variable del que llama. Por eso se invoca con &variable.
//   minimo, maximo -> POR VALOR: limites permitidos.
// RETORNA: true si se leyo un valor valido, false si fallo 3 veces.
// -------------------------------------------------------------------------------------
bool leerEntero(const char* mensaje, int* destino, int minimo, int maximo) {
    int intentos = 0;                            // contador de intentos

    while (intentos < 3) {
        cout << mensaje;

        if (!(cin >> *destino)) {                // *destino = "el contenido de la direccion"; si no es numero falla
            limpiarEntrada();                    // repara cin
            cout << "Debe escribir un numero.\n";
            intentos++;
            continue;
        }
        limpiarEntrada();                        // quita el Enter que dejo cin >> (para que el proximo getline no lea vacio)

        if (*destino < minimo || *destino > maximo) {   // valida el rango
            cout << "El valor debe estar entre " << minimo << " y " << maximo << ".\n";
            intentos++;
            continue;
        }
        return true;                             // numero valido y en rango
    }

    cout << "Demasiados intentos, se cancela la operacion.\n";
    return false;
}

// -------------------------------------------------------------------------------------
// buscarEquipo: busqueda LINEAL de un equipo por su codigo usando ARITMETICA DE PUNTEROS.
// PARAMETROS:
//   equipos    -> Equipo* : puntero al inicio del arreglo dinamico de equipos.
//   numEquipos -> POR VALOR: cuantos equipos hay.
//   codigo     -> POR VALOR: codigo que se busca.
// RETORNA: puntero al equipo encontrado (se puede modificar desde fuera, ej. su estado)
//          o nullptr si no existe.
// -------------------------------------------------------------------------------------
Equipo* buscarEquipo(Equipo* equipos, int numEquipos, int codigo) {
    Equipo* p   = equipos;                       // p apunta al primer equipo
    Equipo* fin = equipos + numEquipos;          // fin apunta UNA posicion despues del ultimo
    while (p < fin) {                            // recorre mientras no llegue al final
        if (p->codigo == codigo) return p;       // "->" accede al campo de una struct por puntero
        p++;                                     // avanza al siguiente equipo (salta sizeof(Equipo) bytes)
    }
    return nullptr;                              // no se encontro
}

// -------------------------------------------------------------------------------------
// buscarUsuario: igual que buscarEquipo pero para usuarios (compara codigoInstitucional).
// RETORNA: puntero al usuario o nullptr.
// -------------------------------------------------------------------------------------
Usuario* buscarUsuario(Usuario* usuarios, int numUsuarios, int codigo) {
    Usuario* p   = usuarios;                     // inicio del arreglo
    Usuario* fin = usuarios + numUsuarios;       // una posicion despues del final
    while (p < fin) {
        if (p->codigoInstitucional == codigo) return p;  // encontrado
        p++;                                     // siguiente usuario
    }
    return nullptr;                              // no existe
}

// -------------------------------------------------------------------------------------
// estaDisponible: indica si un equipo esta en estado "Disponible".
// PARAMETRO: eq -> puntero a un equipo (no se copia la struct entera, solo su direccion).
// RETORNA: true si estadoOperativo es exactamente "Disponible" (strcmp devuelve 0 si son iguales).
// -------------------------------------------------------------------------------------
bool estaDisponible(Equipo* eq) {
    return strcmp(eq->estadoOperativo, "Disponible") == 0;
}

// -------------------------------------------------------------------------------------
// cargarEquipos: lee el archivo de texto de equipos y crea un ARREGLO DINAMICO con ellos.
// Formato de cada linea:  codigo * nombre * laboratorio * tipo * estado * costo * semestreMin * descripcion
// PARAMETROS:
//   nombreArchivo[] -> nombre del archivo (arreglo, se pasa como puntero).
//   equipos   -> Equipo** : PUNTERO A PUNTERO. Se necesita porque la funcion debe cambiar
//                el puntero "equipos" de main (asignarle la memoria creada con new).
//                Si se pasara Equipo* (por valor), el new se perderia al salir de la funcion.
//   numEquipos -> int* : por referencia, para devolver a main cuantos equipos se cargaron.
// -------------------------------------------------------------------------------------
void cargarEquipos(char nombreArchivo[], Equipo** equipos, int* numEquipos) {
    ifstream archivo(nombreArchivo);             // abre el archivo de texto en modo lectura

    if (!archivo) {                              // si no se pudo abrir (no existe, nombre mal escrito)
        cout<<"No se pudo abrir el archivo.\n";
        return;                                  // sale sin cargar nada
    }

    char linea[200];                             // buffer para leer cada linea
    while (archivo.getline(linea, 200)) {        // PRIMERA PASADA: solo cuenta las lineas
        (*numEquipos)++;                         // incrementa la variable de main (parentesis necesarios)
    }

    archivo.clear();                             // quita la marca de fin de archivo (EOF)
    archivo.seekg(0);                            // regresa el cursor de lectura al inicio del archivo
    *equipos = new Equipo[*numEquipos];          // reserva memoria dinamica exacta; se guarda en el puntero de main
    Equipo* p = *equipos;                        // p recorre el arreglo recien creado

    while (archivo.getline(linea, 200)) {        // SEGUNDA PASADA: lee y separa los datos
        char* token = strtok(linea, "*");        // strtok corta la linea en el primer '*' -> campo 1 (codigo)
        limpiarEspacios(token);                  // quita espacios sobrantes
        p->codigo = atoi(token);                 // atoi: convierte texto a int

        token = strtok(NULL, "*");               // NULL = continua cortando la MISMA linea -> campo 2
        limpiarEspacios(token);
        strcpy(p->nombre, token);                // strcpy copia el texto al campo nombre

        token = strtok(NULL, "*");               // campo 3: laboratorio
        limpiarEspacios(token);
        strcpy(p->laboratorio, token);

        token = strtok(NULL, "*");               // campo 4: tipo
        limpiarEspacios(token);
        strcpy(p->tipo, token);

        token = strtok(NULL, "*");               // campo 5: estado operativo
        limpiarEspacios(token);
        strcpy(p->estadoOperativo, token);

        token = strtok(NULL, "*");               // campo 6: costo estimado
        limpiarEspacios(token);
        p->costoEstimado = atof(token);          // atof: convierte texto a numero decimal

        token = strtok(NULL, "*");               // campo 7: semestre minimo
        limpiarEspacios(token);
        p->semestreMinimo = atoi(token);

        token = strtok(NULL, "*");               // campo 8: descripcion tecnica
        limpiarEspacios(token);
        strcpy(p->descripcionTecnica, token);

        p++;                                     // pasa al siguiente equipo del arreglo
    }

    archivo.close();                             // cierra el archivo
    cout<<"Se cargaron "<<*numEquipos<<" equipos correctamente.\n";
}

// -------------------------------------------------------------------------------------
// cargarUsuarios: misma logica que cargarEquipos pero para usuarios.
// Formato de cada linea:  codigoInstitucional * nombre * programaAcademico * semestre
// PARAMETROS: Usuario** (para crear el arreglo en main) e int* (para devolver la cantidad).
// -------------------------------------------------------------------------------------
void cargarUsuarios(char nombreArchivo[], Usuario** usuarios, int* numUsuarios) {

    ifstream archivo(nombreArchivo);             // abre el archivo de texto

    if (!archivo) {                              // valida que se abrio
        cout<<"No se pudo abrir el archivo.\n";
        return;
    }

    char linea[200];
    while (archivo.getline(linea, 200)) {        // 1a pasada: contar lineas = cantidad de usuarios
        (*numUsuarios)++;
    }

    archivo.clear();                             // limpia EOF
    archivo.seekg(0);                            // vuelve al inicio

    *usuarios = new Usuario[*numUsuarios];       // arreglo dinamico del tamano exacto
    Usuario* p = *usuarios;                      // puntero de recorrido

    while (archivo.getline(linea, 200)) {        // 2a pasada: llenar el arreglo
        char* token = strtok(linea, "*");        // campo 1: codigo institucional
        limpiarEspacios(token);
        p->codigoInstitucional = atoi(token);

        token = strtok(NULL, "*");               // campo 2: nombre
        limpiarEspacios(token);
        strcpy(p->nombre, token);

        token = strtok(NULL, "*");               // campo 3: programa academico
        limpiarEspacios(token);
        strcpy(p->programaAcademico, token);

        token = strtok(NULL, "*");               // campo 4: semestre
        limpiarEspacios(token);
        p->semestre = atoi(token);

        p++;                                     // siguiente usuario
    }

    archivo.close();
    cout<<"Se cargaron "<<*numUsuarios<<" usuarios correctamente.\n";
}

// -------------------------------------------------------------------------------------
// consultarEstadoLaboratorio (OPCION 3): muestra todos los equipos de un laboratorio y
// un resumen: cuantos disponibles / en uso / en mantenimiento, costo total y % disponible.
// PARAMETROS: equipos (puntero al arreglo) y numEquipos POR VALOR -> SOLO LECTURA, no modifica nada.
// -------------------------------------------------------------------------------------
void consultarEstadoLaboratorio(Equipo* equipos, int numEquipos) {
    if (numEquipos == 0) {                       // si no se han cargado equipos
        cout << "Primero debe cargar los equipos (opcion 1).\n";
        return;
    }

    char laboratorio[40];                        // nombre del laboratorio a consultar
    if (!leerTexto("Nombre del laboratorio: ", laboratorio, 40)) return;  // si falla 3 veces, cancela

    int encontrados   = 0;                       // equipos que pertenecen al laboratorio
    int disponibles   = 0;                       // contador por estado
    int enUso         = 0;
    int mantenimiento = 0;
    int otroEstado    = 0;
    double costoTotal = 0;                       // acumulador del costo

    cout << "\n===== ESTADO OPERATIVO DEL LABORATORIO " << laboratorio << " =====\n";

    Equipo* p   = equipos;                       // recorrido con punteros
    Equipo* fin = equipos + numEquipos;
    while (p < fin) {
        if (strcmp(p->laboratorio, laboratorio) == 0) {   // si el equipo es de ese laboratorio (distingue mayusculas)
            cout << "\nCodigo: " << p->codigo << " | " << p->nombre << "\n";
            cout << "  Tipo: " << p->tipo << " | Estado: " << p->estadoOperativo << "\n";
            cout << "  Semestre minimo: " << p->semestreMinimo
                 << " | Costo estimado: " << (long)p->costoEstimado << "\n";  // (long) evita notacion cientifica (1e+06)
            cout << "  " << p->descripcionTecnica << "\n";

            encontrados++;                                  // cuenta el equipo
            costoTotal = costoTotal + p->costoEstimado;     // suma su costo

            if (strcmp(p->estadoOperativo, "Disponible") == 0)         disponibles++;    // clasifica por estado
            else if (strcmp(p->estadoOperativo, "En uso") == 0)        enUso++;
            else if (strcmp(p->estadoOperativo, "Mantenimiento") == 0) mantenimiento++;
            else                                                       otroEstado++;
        }
        p++;                                     // siguiente equipo
    }

    if (encontrados == 0) {                      // el laboratorio no tiene equipos (o se escribio mal)
        cout << "No hay equipos registrados en ese laboratorio.\n";
        return;                                  // evita la division por cero de abajo
    }

    cout << "\n--- RESUMEN ---\n";
    cout << "Equipos en el laboratorio: " << encontrados << "\n";
    cout << "  Disponibles   : " << disponibles << "\n";
    cout << "  En uso        : " << enUso << "\n";
    cout << "  Mantenimiento : " << mantenimiento << "\n";
    cout << "  Otro estado   : " << otroEstado << "\n";
    cout << "Costo total de los equipos: " << (long)costoTotal << "\n";
    cout << "Porcentaje disponible: " << (disponibles * 100) / encontrados << " %\n";  // division entera

    if (disponibles == 0)                        // alerta si no hay nada libre
        cout << "ATENCION: no hay equipos libres para programar en este laboratorio.\n";
}

// -------------------------------------------------------------------------------------
// siguienteCodigoSesion: calcula el codigo de la proxima sesion.
// Idea: como cada registro mide sizeof(SesionUso) bytes, tamano_archivo / sizeof = cantidad
// de sesiones guardadas. El siguiente codigo es esa cantidad + 1.
// RETORNA: int con el nuevo codigo (1 si el archivo aun no existe).
// -------------------------------------------------------------------------------------
int siguienteCodigoSesion() {
    ifstream archivo(ARCHIVO_SESIONES, ios::binary);   // abre el binario en lectura
    if (!archivo) return 1;                            // si no existe, es la primera sesion

    archivo.seekg(0, ios::end);                        // mueve el cursor al final del archivo
    long total = (long)archivo.tellg() / (long)sizeof(SesionUso);  // tellg = bytes totales -> / tamano registro
    archivo.close();

    return (int)total + 1;                             // codigo consecutivo
}

// -------------------------------------------------------------------------------------
// guardarSesion: agrega una sesion al FINAL del archivo binario.
// PARAMETRO: s -> SesionUso POR VALOR: se recibe una COPIA de la struct (no se modifica la original).
// -------------------------------------------------------------------------------------
void guardarSesion(SesionUso s) {
    ofstream archivo(ARCHIVO_SESIONES, ios::binary | ios::app);   // binario + append (agrega al final, lo crea si no existe)
    archivo.write((char*)&s, sizeof(SesionUso));   // escribe los bytes de la struct; (char*) porque write pide bytes
    archivo.close();
}

// -------------------------------------------------------------------------------------
// programarSesion (OPCION 4): registra una nueva sesion de uso.
// REGLAS DE NEGOCIO: el usuario y el equipo deben existir, el equipo debe estar "Disponible"
// y el semestre del usuario debe ser >= semestreMinimo del equipo.
// PARAMETROS: punteros a los arreglos (permiten MODIFICAR el estado del equipo) y cantidades por valor.
// -------------------------------------------------------------------------------------
void programarSesion(Equipo* equipos, int numEquipos, Usuario* usuarios, int numUsuarios) {
    if (numEquipos == 0 || numUsuarios == 0) {   // necesita ambos datos cargados
        cout << "Debe cargar equipos (opcion 1) y usuarios (opcion 2) antes de programar.\n";
        return;
    }

    int codigoUsuario;                           // se llena por referencia en leerEntero (&codigoUsuario)
    if (!leerEntero("Codigo institucional del usuario: ", &codigoUsuario, 1, 999999999)) return;

    Usuario* usuario = buscarUsuario(usuarios, numUsuarios, codigoUsuario);  // busca al usuario
    if (usuario == nullptr) {                    // VALIDACION 1: el usuario existe
        cout << "No existe un usuario con ese codigo.\n";
        return;
    }
    cout << "Usuario: " << usuario->nombre << " | " << usuario->programaAcademico
         << " | semestre " << usuario->semestre << "\n";

    int codigoEquipo;
    if (!leerEntero("Codigo del equipo: ", &codigoEquipo, 1, 999999999)) return;

    Equipo* equipo = buscarEquipo(equipos, numEquipos, codigoEquipo);  // busca el equipo
    if (equipo == nullptr) {                     // VALIDACION 2: el equipo existe
        cout << "No existe un equipo con ese codigo.\n";
        return;
    }
    cout << "Equipo: " << equipo->nombre << " | laboratorio " << equipo->laboratorio
         << " | estado " << equipo->estadoOperativo << "\n";

    if (!estaDisponible(equipo)) {               // VALIDACION 3: el equipo esta libre
        cout << "No se puede programar: el equipo esta en estado "
             << equipo->estadoOperativo << ".\n";
        return;
    }

    if (usuario->semestre < equipo->semestreMinimo) {   // VALIDACION 4: el usuario tiene el semestre requerido
        cout << "No se puede programar: el equipo exige semestre "
             << equipo->semestreMinimo << " y el usuario cursa semestre "
             << usuario->semestre << ".\n";
        return;
    }

    SesionUso nueva;                                     // struct local con la nueva sesion
    nueva.codigoSesion  = siguienteCodigoSesion();       // codigo consecutivo
    nueva.codigoEquipo  = equipo->codigo;                // relaciona la sesion con el equipo
    nueva.codigoUsuario = usuario->codigoInstitucional;  // relaciona la sesion con el usuario

    if (!leerTexto("Fecha de la sesion (dd/mm/aaaa): ", nueva.fecha, 20)) return;   // se escribe directo en la struct

    if (!leerEntero("Duracion estimada en horas (1 a 8): ", &nueva.duracionProgramada, 1, 8)) return;  // valida 1..8

    nueva.duracionReal = 0;                              // aun no se ha usado
    nueva.cerrada = false;                               // la sesion queda ABIERTA
    strcpy(nueva.observacion, "Sesion programada");      // observacion inicial
    nueva.penalizacion = 0;                              // sin penalizacion por ahora

    guardarSesion(nueva);                                // se guarda (por valor) en el archivo binario

    strcpy(equipo->estadoOperativo, "En uso");           // cambia el estado en MEMORIA (via puntero)

    cout << "\nSESION PROGRAMADA\n";                     // comprobante
    cout << "  Codigo de sesion: " << nueva.codigoSesion << "\n";
    cout << "  Equipo          : " << equipo->nombre << " (" << equipo->codigo << ")\n";
    cout << "  Usuario         : " << usuario->nombre << " (" << usuario->codigoInstitucional << ")\n";
    cout << "  Fecha           : " << nueva.fecha << "\n";
    cout << "  Duracion        : " << nueva.duracionProgramada << " horas\n";
    cout << "  El equipo queda en estado: " << equipo->estadoOperativo << "\n";
}

// -------------------------------------------------------------------------------------
// cerrarSesion (OPCION 5): cierra una sesion abierta, registra horas reales, observaciones,
// calcula la penalizacion y actualiza el estado del equipo.
// Usa ACCESO DIRECTO (aleatorio) al archivo binario: va directo a la posicion del registro
// sin leer los anteriores, y lo SOBREESCRIBE en el mismo lugar.
// -------------------------------------------------------------------------------------
void cerrarSesion(Equipo* equipos, int numEquipos) {
    if (equipos == nullptr || numEquipos == 0) {         // se necesitan los equipos para cambiar su estado
        cout << "Primero cargue los equipos (opcion 1).\n";
        return;
    }

    fstream archivo(ARCHIVO_SESIONES, ios::in | ios::out | ios::binary);  // lectura Y escritura, binario
    if (!archivo) {                                      // si el archivo no existe no hay sesiones
        cout << "No hay sesiones registradas.\n";
        return;
    }

    archivo.seekg(0, ios::end);                          // al final para medir el archivo
    long total = (long)archivo.tellg() / (long)sizeof(SesionUso);   // cantidad de sesiones

    int codigo;
    cout << "Codigo de la sesion a cerrar: ";
    cin >> codigo;                                       // (sin validacion con leerEntero)
    cin.ignore(1000, '\n');                              // limpia el Enter

    if (codigo < 1 || codigo > total) {                  // el codigo debe existir en el archivo
        cout << "Esa sesion no existe.\n";
        archivo.close();
        return;
    }

    long posicion = (long)(codigo - 1) * (long)sizeof(SesionUso);  // byte donde empieza ese registro (sesion 1 -> byte 0)

    SesionUso s;                                         // aqui se carga la sesion
    archivo.seekg(posicion, ios::beg);                   // seekg = mueve el cursor de LECTURA (get)
    archivo.read((char*)&s, sizeof(SesionUso));          // lee el registro completo en s

    if (s.cerrada) {                                     // no se puede cerrar dos veces
        cout << "Esa sesion ya fue cerrada.\n";
        archivo.close();
        return;
    }

    Equipo* pEquipo = buscarEquipo(equipos, numEquipos, s.codigoEquipo);  // equipo de la sesion
    if (pEquipo == nullptr) {
        cout << "El equipo de esa sesion no esta cargado en memoria.\n";
        archivo.close();
        return;
    }

    cout << "Equipo: " << pEquipo->nombre << "\n";
    cout << "Duracion programada: " << s.duracionProgramada << " horas\n";

    cout << "Duracion real (horas): ";
    cin >> s.duracionReal;                               // horas reales de uso
    cin.ignore(1000, '\n');

    cout << "Observaciones tecnicas: ";
    cin.getline(s.observacion, 100);                     // reemplaza "Sesion programada"

    int horasExtra = s.duracionReal - s.duracionProgramada;   // horas que se paso
    if (horasExtra > 0)
        s.penalizacion = horasExtra * 0.03 * pEquipo->costoEstimado;  // 3% del costo del equipo por cada hora extra
    else
        s.penalizacion = 0;                              // si no se paso, no paga

    char respuesta[10];
    cout << "Se reporta dano en el equipo? (si/no): ";
    cin.getline(respuesta, 10);
    if (strcmp(respuesta, "si") == 0) {                  // si hay dano...
        strcpy(pEquipo->estadoOperativo, "Mantenimiento");   // ...el equipo pasa a mantenimiento
        cout << "Estado del equipo cambiado a mantenimiento.\n";
    } else {                                             // cualquier otra respuesta...
        strcpy(pEquipo->estadoOperativo, "Disponible");  // ...el equipo queda libre de nuevo
    }

    s.cerrada = true;                                    // marca la sesion como cerrada

    archivo.seekp(posicion, ios::beg);                   // seekp = mueve el cursor de ESCRITURA (put) al mismo registro
    archivo.write((char*)&s, sizeof(SesionUso));         // sobreescribe el registro actualizado
    archivo.close();

    cout << "Sesion cerrada. Penalizacion: $" << s.penalizacion << "\n";
}

// -------------------------------------------------------------------------------------
// informeUsoIntensivo (OPCION 6): por cada laboratorio muestra el equipo con MAS horas
// reales de uso (solo cuenta sesiones cerradas).
// Tecnica: arreglo dinamico "horas" PARALELO al arreglo de equipos (horas[i] <-> equipos[i]).
// -------------------------------------------------------------------------------------
void informeUsoIntensivo(Equipo* equipos, int numEquipos) {
    if (equipos == nullptr || numEquipos == 0) {
        cout << "Primero cargue los equipos (opcion 1).\n";
        return;
    }

    ifstream archivo(ARCHIVO_SESIONES, ios::binary);     // solo lectura
    if (!archivo) {
        cout << "No hay sesiones registradas.\n";
        return;
    }

    int* horas = new int[numEquipos];                    // arreglo dinamico: horas acumuladas por equipo
    int* pIni = horas;                                   // puntero para inicializar
    while (pIni < horas + numEquipos) {
        *pIni = 0;                                       // todas las posiciones en 0
        pIni++;
    }

    SesionUso s;
    while (archivo.read((char*)&s, sizeof(SesionUso))) { // lee sesion por sesion (acceso SECUENCIAL) hasta el final
        if (!s.cerrada) continue;                        // ignora las sesiones abiertas

        Equipo* pEq = equipos;                           // recorre equipos...
        int* pH = horas;                                 // ...y horas al mismo tiempo (arreglos paralelos)
        while (pEq < equipos + numEquipos) {
            if (pEq->codigo == s.codigoEquipo) {         // encontro el equipo de la sesion
                *pH += s.duracionReal;                   // suma sus horas reales
                break;
            }
            pEq++;
            pH++;
        }
    }
    archivo.close();

    cout << "\n--- USO INTENSIVO POR LABORATORIO ---\n";

    Equipo* pLab = equipos;                              // recorre equipos para sacar cada laboratorio
    while (pLab < equipos + numEquipos) {

        bool repetido = false;                           // evita imprimir el mismo laboratorio 2 veces
        Equipo* pAnt = equipos;
        while (pAnt < pLab) {                            // revisa los equipos ANTERIORES
            if (strcmp(pAnt->laboratorio, pLab->laboratorio) == 0) {
                repetido = true;                         // ese laboratorio ya se proceso
                break;
            }
            pAnt++;
        }

        if (!repetido) {                                 // primera vez que aparece este laboratorio
            Equipo* pMejor = nullptr;                    // equipo con mas horas
            int maxHoras = -1;                           // maximo encontrado

            Equipo* pBusca = equipos;
            int* pHB = horas;
            while (pBusca < equipos + numEquipos) {      // busca el maximo SOLO dentro de este laboratorio
                if (strcmp(pBusca->laboratorio, pLab->laboratorio) == 0 && *pHB > maxHoras) {
                    maxHoras = *pHB;
                    pMejor = pBusca;
                }
                pBusca++;
                pHB++;
            }

            if (pMejor != nullptr && maxHoras > 0)       // hay al menos una hora registrada
                cout << "--" << pLab->laboratorio << ": " << pMejor->nombre
                     << " (" << maxHoras << " horas)\n";
            else
                cout << "--" << pLab->laboratorio << ": sin horas registradas\n";
        }

        pLab++;
    }

    delete[] horas;                                      // libera el arreglo dinamico (evita fuga de memoria)
}

// -------------------------------------------------------------------------------------
// rankingUsuariosCriticos (OPCION 7): TOP 3 de usuarios con mayor INDICE DE CRITICIDAD.
//   indice = penalizacion total / numero de sesiones cerradas del usuario
// Usa 3 arreglos dinamicos paralelos al arreglo de usuarios:
//   penal[i]    -> suma de penalizaciones del usuario i
//   conteo[i]   -> cuantas sesiones cerradas tiene
//   mostrado[i] -> si ya salio en el ranking (para no repetirlo)
// -------------------------------------------------------------------------------------
void rankingUsuariosCriticos(Usuario* usuarios, int numUsuarios) {
    if (usuarios == nullptr || numUsuarios == 0) {
        cout << "Primero cargue los usuarios (opcion 2).\n";
        return;
    }

    ifstream archivo(ARCHIVO_SESIONES, ios::binary);
    if (!archivo) {
        cout << "No hay sesiones registradas.\n";
        return;
    }

    float* penal = new float[numUsuarios];               // arreglos dinamicos paralelos
    int* conteo = new int[numUsuarios];
    bool* mostrado = new bool[numUsuarios];

    float* pP = penal;                                   // un puntero por arreglo
    int* pC = conteo;
    bool* pM = mostrado;
    while (pP < penal + numUsuarios) {                   // inicializa los tres a la vez
        *pP = 0;
        *pC = 0;
        *pM = false;
        pP++; pC++; pM++;
    }

    SesionUso s;
    while (archivo.read((char*)&s, sizeof(SesionUso))) { // recorre todas las sesiones
        if (!s.cerrada) continue;                        // solo cerradas

        Usuario* pU = usuarios;                          // reinicia los punteros al inicio
        pP = penal;
        pC = conteo;
        while (pU < usuarios + numUsuarios) {
            if (pU->codigoInstitucional == s.codigoUsuario) {  // encontro al usuario de la sesion
                *pP += s.penalizacion;                   // acumula su penalizacion
                (*pC)++;                                 // cuenta una sesion mas
                break;
            }
            pU++; pP++; pC++;                            // avanzan juntos
        }
    }
    archivo.close();

    cout << "\n--- TOP 3 USUARIOS CRITICOS ---\n";

    int puesto = 1;
    while (puesto <= 3) {                                // SELECCION del maximo 3 veces (sin ordenar el arreglo)
        Usuario* pMejor = nullptr;                       // usuario con mayor indice en esta vuelta
        bool* pMarcaMejor = nullptr;                     // apunta a su casilla en "mostrado"
        float mejorIndice = -1;

        Usuario* pU = usuarios;
        pP = penal;
        pC = conteo;
        pM = mostrado;
        while (pU < usuarios + numUsuarios) {
            if (*pC > 0 && *pM == false) {               // tiene sesiones (evita dividir por 0) y no ha salido
                float indice = *pP / *pC;                // indice de criticidad
                if (indice > mejorIndice) {              // nuevo maximo
                    mejorIndice = indice;
                    pMejor = pU;
                    pMarcaMejor = pM;
                }
            }
            pU++; pP++; pC++; pM++;
        }

        if (pMejor == nullptr) break;                    // no quedan mas usuarios con sesiones

        cout << puesto << ". " << pMejor->nombre
             << " (cod " << pMejor->codigoInstitucional << ")"
             << " - indice de criticidad: " << mejorIndice << "\n";

        *pMarcaMejor = true;                             // lo marca para no volver a elegirlo
        puesto++;
    }

    if (puesto == 1) cout << "Ningun usuario tiene sesiones cerradas.\n";   // nunca se imprimio nadie

    delete[] penal;                                      // libera los 3 arreglos dinamicos
    delete[] conteo;
    delete[] mostrado;
}

// -------------------------------------------------------------------------------------
// liberarMemoria (OPCION 8): libera los arreglos dinamicos de equipos y usuarios.
// Recibe PUNTERO A PUNTERO para poder dejar los punteros de main en nullptr
// y los contadores en 0 (asi no quedan "punteros colgantes").
// -------------------------------------------------------------------------------------
void liberarMemoria(Equipo** equipos, int* numEquipos,
                    Usuario** usuarios, int* numUsuarios) {
    delete[] *equipos;           // libera el arreglo creado en cargarEquipos (delete[] porque se creo con new[])
    *equipos = nullptr;          // el puntero de main ya no apunta a memoria liberada
    *numEquipos = 0;

    delete[] *usuarios;          // libera el arreglo de usuarios (delete[] sobre nullptr es seguro)
    *usuarios = nullptr;
    *numUsuarios = 0;

    cout << "Memoria liberada. Saliendo del sistema.\n";
}

// =====================================================================================
// main: punto de entrada. Declara los datos principales y muestra el MENU en un ciclo.
// =====================================================================================
int main() {
    int menu;                                    // opcion elegida

    Equipo* equipos = nullptr;                   // puntero al arreglo dinamico de equipos (vacio al inicio)
    int numEquipos = 0;                          // cantidad de equipos cargados

    Usuario* usuarios = nullptr;                 // puntero al arreglo dinamico de usuarios
    int numUsuarios = 0;                         // cantidad de usuarios cargados

    char nombreArchivoEquipos[100];              // nombre del .txt de equipos
    char nombreArchivoUsuarios[100];             // nombre del .txt de usuarios

    int lecturasFallidas = 0;                    // errores seguidos al leer la opcion

    do {                                         // do-while: el menu se muestra al menos una vez
        cout<<"\n--------MENU PRINCIPAL--------\n";
        cout<<"1.Cargar equipos desde archivo de texto \n";
        cout<<"2.Cargar usuarios desde archivo de texto \n";
        cout<<"3.Consultar estado operativo del laboratorio \n";
        cout<<"4.Programar una sesion de uso de equipo \n";
        cout<<"5.Registrar cierre de sesion y observaciones \n";
        cout<<"6.Generar informe de uso intensivo de equipos \n";
        cout<<"7.Generar ranking de usuarios criticos\n";
        cout<<"8.Salir \n";
        cout<<"Elige una opcion: ";

        if (!(cin >> menu)) {                    // si escribio algo que no es numero
            limpiarEntrada();                    // repara cin
            menu = 0;                            // opcion invalida -> cae en default
            lecturasFallidas++;
            if (lecturasFallidas >= 3) {         // 3 errores seguidos: sale del programa
                cout<<"\nDemasiados errores de lectura. Saliendo del sistema\n";
                break;                           // rompe el do-while
            }
        } else {
            limpiarEntrada();                    // quita el Enter del buffer
            lecturasFallidas = 0;                // reinicia el contador de errores
        }

        switch (menu) {                          // ejecuta la opcion
            case 1:
                ingresarArchivo(nombreArchivoEquipos, 100);                  // pide el nombre del archivo
                cargarEquipos(nombreArchivoEquipos, &equipos, &numEquipos);  // & = se pasa la DIRECCION para que la funcion los modifique
                break;
            case 2:
                ingresarArchivo(nombreArchivoUsuarios, 100);
                cargarUsuarios(nombreArchivoUsuarios, &usuarios, &numUsuarios);
                break;
            case 3:
                consultarEstadoLaboratorio(equipos, numEquipos);   // solo consulta
                break;
            case 4:
                programarSesion(equipos, numEquipos, usuarios, numUsuarios);
                break;
            case 5:
                cerrarSesion(equipos, numEquipos);
                break;
            case 6:
                informeUsoIntensivo(equipos, numEquipos);
                break;
            case 7:
                rankingUsuariosCriticos(usuarios, numUsuarios);
                break;
            case 8:
                liberarMemoria(&equipos, &numEquipos, &usuarios, &numUsuarios);  // se pasan direcciones para ponerlos en nullptr/0
                break;
            default:
                cout<<"Opcion invalida.\n";      // cualquier otro numero
        }
    } while (menu != 8);                         // repite hasta elegir Salir

    return 0;                                    // fin correcto del programa
}
