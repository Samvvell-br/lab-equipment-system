#include <iostream>
#include <cstring>
#include <fstream>
using namespace std;

const char* ARCHIVO_SESIONES = "sesiones.dat";

struct Equipo {
    int codigo;
    char nombre[50];
    char laboratorio[40];
    char tipo[30];
    char estadoOperativo[20];
    float costoEstimado;
    int semestreMinimo;
    char descripcionTecnica[100];
};

struct Usuario {
    int codigoInstitucional;
    char nombre[50];
    char programaAcademico[50];
    int semestre;
};

struct SesionUso {
    int codigoSesion;
    int codigoEquipo;
    int codigoUsuario;
    char fecha[20];
    int duracionProgramada;
    int duracionReal;
    bool cerrada;
    char observacion[100];
    float penalizacion;
};

void ingresarArchivo(char nombreArchivo[], int tam) {
    cout<<"Ingrese nombre del archivo: ";
    cin.getline(nombreArchivo, tam);
}

void limpiarEspacios(char texto[]) {
    int inicio = 0;
    while (texto[inicio] == ' ') inicio++;

    int fin = strlen(texto) - 1;
    while (fin >= inicio && texto[fin] == ' ') fin--;

    int j = 0;
    for (int i = inicio; i <= fin; i++, j++) {
        texto[j] = texto[i];
    }
    texto[j] = '\0';
}

void limpiarEntrada() {
    cin.clear();
    cin.ignore(10000, '\n');
}

bool leerTexto(const char* mensaje, char* destino, int tam) {
    int intentos = 0;

    while (intentos < 3) {
        cout << mensaje;

        if (!cin.getline(destino, tam)) {
            limpiarEntrada();
            cout << "Texto invalido (maximo " << (tam - 1) << " caracteres).\n";
            intentos++;
            continue;
        }

        if (destino[0] == '\0') {
            cout << "No puede quedar vacio.\n";
            intentos++;
            continue;
        }
        return true;
    }

    cout << "Demasiados intentos, se cancela la operacion.\n";
    return false;
}

bool leerEntero(const char* mensaje, int* destino, int minimo, int maximo) {
    int intentos = 0;

    while (intentos < 3) {
        cout << mensaje;

        if (!(cin >> *destino)) {
            limpiarEntrada();
            cout << "Debe escribir un numero.\n";
            intentos++;
            continue;
        }
        limpiarEntrada();

        if (*destino < minimo || *destino > maximo) {
            cout << "El valor debe estar entre " << minimo << " y " << maximo << ".\n";
            intentos++;
            continue;
        }
        return true;
    }

    cout << "Demasiados intentos, se cancela la operacion.\n";
    return false;
}

Equipo* buscarEquipo(Equipo* equipos, int numEquipos, int codigo) {
    Equipo* p   = equipos;
    Equipo* fin = equipos + numEquipos;
    while (p < fin) {
        if (p->codigo == codigo) return p;
        p++;
    }
    return nullptr;
}

Usuario* buscarUsuario(Usuario* usuarios, int numUsuarios, int codigo) {
    Usuario* p   = usuarios;
    Usuario* fin = usuarios + numUsuarios;
    while (p < fin) {
        if (p->codigoInstitucional == codigo) return p;
        p++;
    }
    return nullptr;
}

bool estaDisponible(Equipo* eq) {
    return strcmp(eq->estadoOperativo, "Disponible") == 0;
}

void cargarEquipos(char nombreArchivo[], Equipo** equipos, int* numEquipos) {
    ifstream archivo(nombreArchivo);

    if (!archivo) {
        cout<<"No se pudo abrir el archivo.\n";
        return;
    }

    char linea[200];
    while (archivo.getline(linea, 200)) {
        (*numEquipos)++;
    }

    archivo.clear();
    archivo.seekg(0);
    *equipos = new Equipo[*numEquipos];
    Equipo* p = *equipos;

    while (archivo.getline(linea, 200)) {
        char* token = strtok(linea, "*");
        limpiarEspacios(token);
        p->codigo = atoi(token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        strcpy(p->nombre, token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        strcpy(p->laboratorio, token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        strcpy(p->tipo, token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        strcpy(p->estadoOperativo, token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        p->costoEstimado = atof(token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        p->semestreMinimo = atoi(token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        strcpy(p->descripcionTecnica, token);

        p++;
    }

    archivo.close();
    cout<<"Se cargaron "<<*numEquipos<<" equipos correctamente.\n";
}

void cargarUsuarios(char nombreArchivo[], Usuario** usuarios, int* numUsuarios) {

    ifstream archivo(nombreArchivo);

    if (!archivo) {
        cout<<"No se pudo abrir el archivo.\n";
        return;
    }

    char linea[200];
    while (archivo.getline(linea, 200)) {
        (*numUsuarios)++;
    }

    archivo.clear();
    archivo.seekg(0);

    *usuarios = new Usuario[*numUsuarios];
    Usuario* p = *usuarios;

    while (archivo.getline(linea, 200)) {
        char* token = strtok(linea, "*");
        limpiarEspacios(token);
        p->codigoInstitucional = atoi(token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        strcpy(p->nombre, token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        strcpy(p->programaAcademico, token);

        token = strtok(NULL, "*");
        limpiarEspacios(token);
        p->semestre = atoi(token);

        p++;
    }

    archivo.close();
    cout<<"Se cargaron "<<*numUsuarios<<" usuarios correctamente.\n";
}

void consultarEstadoLaboratorio(Equipo* equipos, int numEquipos) {
    if (numEquipos == 0) {
        cout << "Primero debe cargar los equipos (opcion 1).\n";
        return;
    }

    char laboratorio[40];
    if (!leerTexto("Nombre del laboratorio: ", laboratorio, 40)) return;

    int encontrados   = 0;
    int disponibles   = 0;
    int enUso         = 0;
    int mantenimiento = 0;
    int otroEstado    = 0;
    double costoTotal = 0;

    cout << "\n===== ESTADO OPERATIVO DEL LABORATORIO " << laboratorio << " =====\n";

    Equipo* p   = equipos;
    Equipo* fin = equipos + numEquipos;
    while (p < fin) {
        if (strcmp(p->laboratorio, laboratorio) == 0) {
            cout << "\nCodigo: " << p->codigo << " | " << p->nombre << "\n";
            cout << "  Tipo: " << p->tipo << " | Estado: " << p->estadoOperativo << "\n";
            cout << "  Semestre minimo: " << p->semestreMinimo
                 << " | Costo estimado: " << (long)p->costoEstimado << "\n";
            cout << "  " << p->descripcionTecnica << "\n";

            encontrados++;
            costoTotal = costoTotal + p->costoEstimado;

            if (strcmp(p->estadoOperativo, "Disponible") == 0)         disponibles++;
            else if (strcmp(p->estadoOperativo, "En uso") == 0)        enUso++;
            else if (strcmp(p->estadoOperativo, "Mantenimiento") == 0) mantenimiento++;
            else                                                       otroEstado++;
        }
        p++;
    }

    if (encontrados == 0) {
        cout << "No hay equipos registrados en ese laboratorio.\n";
        return;
    }

    cout << "\n--- RESUMEN ---\n";
    cout << "Equipos en el laboratorio: " << encontrados << "\n";
    cout << "  Disponibles   : " << disponibles << "\n";
    cout << "  En uso        : " << enUso << "\n";
    cout << "  Mantenimiento : " << mantenimiento << "\n";
    cout << "  Otro estado   : " << otroEstado << "\n";
    cout << "Costo total de los equipos: " << (long)costoTotal << "\n";
    cout << "Porcentaje disponible: " << (disponibles * 100) / encontrados << " %\n";

    if (disponibles == 0)
        cout << "ATENCION: no hay equipos libres para programar en este laboratorio.\n";
}

int siguienteCodigoSesion() {
    ifstream archivo(ARCHIVO_SESIONES, ios::binary);
    if (!archivo) return 1;

    archivo.seekg(0, ios::end);
    long total = (long)archivo.tellg() / (long)sizeof(SesionUso);
    archivo.close();

    return (int)total + 1;
}

void guardarSesion(SesionUso s) {
    ofstream archivo(ARCHIVO_SESIONES, ios::binary | ios::app);
    archivo.write((char*)&s, sizeof(SesionUso));
    archivo.close();
}

void programarSesion(Equipo* equipos, int numEquipos, Usuario* usuarios, int numUsuarios) {
    if (numEquipos == 0 || numUsuarios == 0) {
        cout << "Debe cargar equipos (opcion 1) y usuarios (opcion 2) antes de programar.\n";
        return;
    }

    int codigoUsuario;
    if (!leerEntero("Codigo institucional del usuario: ", &codigoUsuario, 1, 999999999)) return;

    Usuario* usuario = buscarUsuario(usuarios, numUsuarios, codigoUsuario);
    if (usuario == nullptr) {
        cout << "No existe un usuario con ese codigo.\n";
        return;
    }
    cout << "Usuario: " << usuario->nombre << " | " << usuario->programaAcademico
         << " | semestre " << usuario->semestre << "\n";

    int codigoEquipo;
    if (!leerEntero("Codigo del equipo: ", &codigoEquipo, 1, 999999999)) return;

    Equipo* equipo = buscarEquipo(equipos, numEquipos, codigoEquipo);
    if (equipo == nullptr) {
        cout << "No existe un equipo con ese codigo.\n";
        return;
    }
    cout << "Equipo: " << equipo->nombre << " | laboratorio " << equipo->laboratorio
         << " | estado " << equipo->estadoOperativo << "\n";

    if (!estaDisponible(equipo)) {
        cout << "No se puede programar: el equipo esta en estado "
             << equipo->estadoOperativo << ".\n";
        return;
    }

    if (usuario->semestre < equipo->semestreMinimo) {
        cout << "No se puede programar: el equipo exige semestre "
             << equipo->semestreMinimo << " y el usuario cursa semestre "
             << usuario->semestre << ".\n";
        return;
    }

    SesionUso nueva;
    nueva.codigoSesion  = siguienteCodigoSesion();
    nueva.codigoEquipo  = equipo->codigo;
    nueva.codigoUsuario = usuario->codigoInstitucional;

    if (!leerTexto("Fecha de la sesion (dd/mm/aaaa): ", nueva.fecha, 20)) return;

    if (!leerEntero("Duracion estimada en horas (1 a 8): ", &nueva.duracionProgramada, 1, 8)) return;

    nueva.duracionReal = 0;
    nueva.cerrada = false;
    strcpy(nueva.observacion, "Sesion programada");
    nueva.penalizacion = 0;

    guardarSesion(nueva);

    strcpy(equipo->estadoOperativo, "En uso");

    cout << "\nSESION PROGRAMADA\n";
    cout << "  Codigo de sesion: " << nueva.codigoSesion << "\n";
    cout << "  Equipo          : " << equipo->nombre << " (" << equipo->codigo << ")\n";
    cout << "  Usuario         : " << usuario->nombre << " (" << usuario->codigoInstitucional << ")\n";
    cout << "  Fecha           : " << nueva.fecha << "\n";
    cout << "  Duracion        : " << nueva.duracionProgramada << " horas\n";
    cout << "  El equipo queda en estado: " << equipo->estadoOperativo << "\n";
}

void cerrarSesion(Equipo* equipos, int numEquipos) {
    if (equipos == nullptr || numEquipos == 0) {
        cout << "Primero cargue los equipos (opcion 1).\n";
        return;
    }

    fstream archivo(ARCHIVO_SESIONES, ios::in | ios::out | ios::binary);
    if (!archivo) {
        cout << "No hay sesiones registradas.\n";
        return;
    }

    archivo.seekg(0, ios::end);
    long total = (long)archivo.tellg() / (long)sizeof(SesionUso);

    int codigo;
    cout << "Codigo de la sesion a cerrar: ";
    cin >> codigo;
    cin.ignore(1000, '\n');

    if (codigo < 1 || codigo > total) {
        cout << "Esa sesion no existe.\n";
        archivo.close();
        return;
    }

    long posicion = (long)(codigo - 1) * (long)sizeof(SesionUso);

    SesionUso s;
    archivo.seekg(posicion, ios::beg);
    archivo.read((char*)&s, sizeof(SesionUso));

    if (s.cerrada) {
        cout << "Esa sesion ya fue cerrada.\n";
        archivo.close();
        return;
    }

    Equipo* pEquipo = buscarEquipo(equipos, numEquipos, s.codigoEquipo);
    if (pEquipo == nullptr) {
        cout << "El equipo de esa sesion no esta cargado en memoria.\n";
        archivo.close();
        return;
    }

    cout << "Equipo: " << pEquipo->nombre << "\n";
    cout << "Duracion programada: " << s.duracionProgramada << " horas\n";

    cout << "Duracion real (horas): ";
    cin >> s.duracionReal;
    cin.ignore(1000, '\n');

    cout << "Observaciones tecnicas: ";
    cin.getline(s.observacion, 100);

    int horasExtra = s.duracionReal - s.duracionProgramada;
    if (horasExtra > 0)
        s.penalizacion = horasExtra * 0.03 * pEquipo->costoEstimado;
    else
        s.penalizacion = 0;

    char respuesta[10];
    cout << "Se reporta dano en el equipo? (si/no): ";
    cin.getline(respuesta, 10);
    if (strcmp(respuesta, "si") == 0) {
        strcpy(pEquipo->estadoOperativo, "Mantenimiento");
        cout << "Estado del equipo cambiado a mantenimiento.\n";
    } else {
        strcpy(pEquipo->estadoOperativo, "Disponible");
    }

    s.cerrada = true;

    archivo.seekp(posicion, ios::beg);
    archivo.write((char*)&s, sizeof(SesionUso));
    archivo.close();

    cout << "Sesion cerrada. Penalizacion: $" << s.penalizacion << "\n";
}

void informeUsoIntensivo(Equipo* equipos, int numEquipos) {
    if (equipos == nullptr || numEquipos == 0) {
        cout << "Primero cargue los equipos (opcion 1).\n";
        return;
    }

    ifstream archivo(ARCHIVO_SESIONES, ios::binary);
    if (!archivo) {
        cout << "No hay sesiones registradas.\n";
        return;
    }

    int* horas = new int[numEquipos];
    int* pIni = horas;
    while (pIni < horas + numEquipos) {
        *pIni = 0;
        pIni++;
    }

    SesionUso s;
    while (archivo.read((char*)&s, sizeof(SesionUso))) {
        if (!s.cerrada) continue;

        Equipo* pEq = equipos;
        int* pH = horas;
        while (pEq < equipos + numEquipos) {
            if (pEq->codigo == s.codigoEquipo) {
                *pH += s.duracionReal;
                break;
            }
            pEq++;
            pH++;
        }
    }
    archivo.close();

    cout << "\n--- USO INTENSIVO POR LABORATORIO ---\n";

    Equipo* pLab = equipos;
    while (pLab < equipos + numEquipos) {

        bool repetido = false;
        Equipo* pAnt = equipos;
        while (pAnt < pLab) {
            if (strcmp(pAnt->laboratorio, pLab->laboratorio) == 0) {
                repetido = true;
                break;
            }
            pAnt++;
        }

        if (!repetido) {
            Equipo* pMejor = nullptr;
            int maxHoras = -1;

            Equipo* pBusca = equipos;
            int* pHB = horas;
            while (pBusca < equipos + numEquipos) {
                if (strcmp(pBusca->laboratorio, pLab->laboratorio) == 0 && *pHB > maxHoras) {
                    maxHoras = *pHB;
                    pMejor = pBusca;
                }
                pBusca++;
                pHB++;
            }

            if (pMejor != nullptr && maxHoras > 0)
                cout << "--" << pLab->laboratorio << ": " << pMejor->nombre
                     << " (" << maxHoras << " horas)\n";
            else
                cout << "--" << pLab->laboratorio << ": sin horas registradas\n";
        }

        pLab++;
    }

    delete[] horas;
}

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

    float* penal = new float[numUsuarios];
    int* conteo = new int[numUsuarios];
    bool* mostrado = new bool[numUsuarios];

    float* pP = penal;
    int* pC = conteo;
    bool* pM = mostrado;
    while (pP < penal + numUsuarios) {
        *pP = 0;
        *pC = 0;
        *pM = false;
        pP++; pC++; pM++;
    }

    SesionUso s;
    while (archivo.read((char*)&s, sizeof(SesionUso))) {
        if (!s.cerrada) continue;

        Usuario* pU = usuarios;
        pP = penal;
        pC = conteo;
        while (pU < usuarios + numUsuarios) {
            if (pU->codigoInstitucional == s.codigoUsuario) {
                *pP += s.penalizacion;
                (*pC)++;
                break;
            }
            pU++; pP++; pC++;
        }
    }
    archivo.close();

    cout << "\n--- TOP 3 USUARIOS CRITICOS ---\n";

    int puesto = 1;
    while (puesto <= 3) {
        Usuario* pMejor = nullptr;
        bool* pMarcaMejor = nullptr;
        float mejorIndice = -1;

        Usuario* pU = usuarios;
        pP = penal;
        pC = conteo;
        pM = mostrado;
        while (pU < usuarios + numUsuarios) {
            if (*pC > 0 && *pM == false) {
                float indice = *pP / *pC;
                if (indice > mejorIndice) {
                    mejorIndice = indice;
                    pMejor = pU;
                    pMarcaMejor = pM;
                }
            }
            pU++; pP++; pC++; pM++;
        }

        if (pMejor == nullptr) break;

        cout << puesto << ". " << pMejor->nombre
             << " (cod " << pMejor->codigoInstitucional << ")"
             << " - indice de criticidad: " << mejorIndice << "\n";

        *pMarcaMejor = true;
        puesto++;
    }

    if (puesto == 1) cout << "Ningun usuario tiene sesiones cerradas.\n";

    delete[] penal;
    delete[] conteo;
    delete[] mostrado;
}

void liberarMemoria(Equipo** equipos, int* numEquipos,
                    Usuario** usuarios, int* numUsuarios) {
    delete[] *equipos;
    *equipos = nullptr;
    *numEquipos = 0;

    delete[] *usuarios;
    *usuarios = nullptr;
    *numUsuarios = 0;

    cout << "Memoria liberada. Saliendo del sistema.\n";
}

int main() {
    int menu;

    Equipo* equipos = nullptr;
    int numEquipos = 0;

    Usuario* usuarios = nullptr;
    int numUsuarios = 0;

    char nombreArchivoEquipos[100];
    char nombreArchivoUsuarios[100];

    int lecturasFallidas = 0;

    do {
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

        if (!(cin >> menu)) {
            limpiarEntrada();
            menu = 0;
            lecturasFallidas++;
            if (lecturasFallidas >= 3) {
                cout<<"\nDemasiados errores de lectura. Saliendo del sistema\n";
                break;
            }
        } else {
            limpiarEntrada();
            lecturasFallidas = 0;
        }

        switch (menu) {
            case 1:
                ingresarArchivo(nombreArchivoEquipos, 100);
                cargarEquipos(nombreArchivoEquipos, &equipos, &numEquipos);
                break;
            case 2:
                ingresarArchivo(nombreArchivoUsuarios, 100);
                cargarUsuarios(nombreArchivoUsuarios, &usuarios, &numUsuarios);
                break;
            case 3:
                consultarEstadoLaboratorio(equipos, numEquipos);
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
                liberarMemoria(&equipos, &numEquipos, &usuarios, &numUsuarios);
                break;
            default:
                cout<<"Opcion invalida.\n";
        }
    } while (menu != 8);

    return 0;
}