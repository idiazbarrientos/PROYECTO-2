#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <numeric> // Para std::accumulate y std::divides
dsfsdf
using namespace std;
using namespace std::chrono;

/* Tipo nodo para árbol */
typedef struct _nodo
{
    char letra;             /* Letra a la que hace referencia el nodo */
    unsigned long int bits; /* Valor de la codificación de la letra */
    char nbits;             /* Número de bits de la codificación */
    _nodo *cero;            /* Puntero a la rama cero de un árbol */
    _nodo *uno;             /* Puntero a la rama uno de un árbol */
} tipoNodo;

/* Funciones prototipo */
void BorrarArbol(tipoNodo *n);

int main(int argc, char *argv[])
{
    tipoNodo *Arbol;        /* Arbol de codificación */
    long int Longitud;      /* Longitud de fichero */
    int nElementos;         /* Elementos de árbol */
    unsigned long int bits; /* Almacen de bits para decodificación */
    ifstream fe;            /* Fichero de entrada */
    ofstream fs;            /* Fichero de salida */
    tipoNodo *p, *q;        /* Auxiliares */
    unsigned char a;
    int i, j;

    if (argc < 3)
    {
        cout << "Usar:" << endl;
        cout << argv[0] << " <fichero_entrada> <fichero_salida>" << endl;
        return 1;
    }

    int numEjecuciones = 20; // Número de veces que se ejecutará la descompresión

    vector<long long> tiempos; // Vector para almacenar los tiempos de cada ejecución

    for (int ejecucion = 0; ejecucion < numEjecuciones; ++ejecucion)
    {
        /* Crear un árbol con la información de la tabla */
        Arbol = new tipoNodo; /* Un nodo nuevo */
        Arbol->letra = 0;
        Arbol->uno = Arbol->cero = nullptr;
        fe.open(argv[1], ios::binary);
        fe.read(reinterpret_cast<char*>(&Longitud), sizeof(long int)); /* Lee el número de caracteres */
        fe.read(reinterpret_cast<char*>(&nElementos), sizeof(int)); /* Lee el número de elementos */
        for (i = 0; i < nElementos; i++) /* Leer todos los elementos */
        {
            p = new tipoNodo; /* Un nodo nuevo */
            fe.read(reinterpret_cast<char*>(&p->letra), sizeof(char)); /* Lee el carácter */
            fe.read(reinterpret_cast<char*>(&p->bits), sizeof(unsigned long int)); /* Lee el código */
            fe.read(reinterpret_cast<char*>(&p->nbits), sizeof(char)); /* Lee la longitud */
            p->cero = p->uno = nullptr;
            /* Insertar el nodo en su lugar */
            j = 1 << (p->nbits - 1);
            q = Arbol;
            while (j > 1)
            {
                if (p->bits & j) /* Es un uno */
                {
                    if (q->uno) q = q->uno; /* Si el nodo existe, nos movemos a él */
                    else /* Si no existe, lo creamos */
                    {
                        q->uno = new tipoNodo; /* Un nodo nuevo */
                        q = q->uno;
                        q->letra = 0;
                        q->uno = q->cero = nullptr;
                    }
                }
                else /* Es un cero */
                {
                    if (q->cero) q = q->cero; /* Si el nodo existe, nos movemos a él */
                    else /* Si no existe, lo creamos */
                    {
                        q->cero = new tipoNodo; /* Un nodo nuevo */
                        q = q->cero;
                        q->letra = 0;
                        q->uno = q->cero = nullptr;
                    }
                }
                j >>= 1; /* Siguiente bit */
            }
            /* Último Bit */
            if (p->bits & 1) /* Es un uno */
                q->uno = p;
            else /* Es un cero */
                q->cero = p;
        }

        /* Leer datos comprimidos y extraer al fichero de salida */
        bits = 0;

        // Medición del tiempo de descompresión
        auto start = high_resolution_clock::now();

        fs.open(argv[2], ios::binary);
        /* Lee los primeros cuatro bytes en la doble palabra bits */
        fe.read(reinterpret_cast<char*>(&a), sizeof(char));
        bits |= a;
        bits <<= 8;
        fe.read(reinterpret_cast<char*>(&a), sizeof(char));
        bits |= a;
        bits <<= 8;
        fe.read(reinterpret_cast<char*>(&a), sizeof(char));
        bits |= a;
        bits <<= 8;
        fe.read(reinterpret_cast<char*>(&a), sizeof(char));
        bits |= a;
        j = 0; /* Cada 8 bits leemos otro byte */
        q = Arbol;
        /* Bucle */
        do {
            if (bits & 0x80000000) q = q->uno;
            else q = q->cero; /* Rama adecuada */
            bits <<= 1; /* Siguiente bit */
            j++;
            if (8 == j) /* Cada 8 bits */
            {
                fe.read(reinterpret_cast<char*>(&a), sizeof(char)); /* Leemos un byte desde el fichero */
                bits |= a; /* Y lo insertamos en bits */
                j = 0; /* No quedan huecos */
            }
            if (!q->uno && !q->cero) /* Si el nodo es una letra */
            {
                fs.put(q->letra); /* La escribimos en el fichero de salida */
                Longitud--; /* Actualizamos longitud que queda */
                q = Arbol; /* Volvemos a la raíz del árbol */
            }
        } while (Longitud); /* Hasta que acabe el fichero */

        // Finalización de la medición del tiempo
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();

        cout << "Tiempo de descompresión (Ejecución " << ejecucion + 1 << "): " << duration << " ms" << endl;

        /* Cerramos los ficheros */
        fs.close();
        fe.close();

        /* Borrar el árbol */
        BorrarArbol(Arbol);

        /* Almacenamos el tiempo de esta ejecución en el vector */
        tiempos.push_back(duration);
    }

    /* Calculamos el promedio de los tiempos */
    if (!tiempos.empty())
    {
        double promedio = accumulate(tiempos.begin(), tiempos.end(), 0.0) / tiempos.size();
        cout << "Tiempo promedio de descompresión: " << promedio << " ms" << endl;
    }

    /* Escribir los tiempos en un archivo CSV */
    ofstream csvFile("tiempo_descompresion.csv", ios::app);
    if (csvFile.is_open())
    {
        for (int ejecucion = 0; ejecucion < numEjecuciones; ++ejecucion)
        {
            csvFile << argv[1] << "," << tiempos[ejecucion] << " ms" << endl;
        }
        csvFile.close();
    }
    else
    {
        cerr << "Error al abrir el archivo CSV para escribir los tiempos de descompresión." << endl;
    }

    return 0;
}

/* Función recursiva para borrar un árbol */
void BorrarArbol(tipoNodo *n)
{
    if (n->cero) BorrarArbol(n->cero);
    if (n->uno) BorrarArbol(n->uno);
    delete n;
}


