#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <iostream>

using namespace std;

/* Tipo nodo para árbol o Lista de árboles */
typedef struct _nodo {
    unsigned char letra;    /* Letra a la que hace referencia el nodo */
    int frecuencia;         /* Veces que aparece la letra en el texto o las letras */
    struct _nodo *sig;      /* Puntero a siguiente nodo de una lista enlazada */
    struct _nodo *cero;     /* Puntero a la rama cero de un árbol */
    struct _nodo *uno;      /* Puntero a la rama uno de un árbol */
} tipoNodo;

/* Nodo para construir una lista para la tabla de codigos */
typedef struct _tabla {
    unsigned char letra;    /* Letra a la que hace referencia el nodo */
    unsigned long int bits; /* Valor de la codificación de la letra */
    unsigned char nbits;    /* Número de bits de la codificación */
    struct _tabla *sig;     /* Siguiente elemento de la tabla */
} tipoTabla;

/* Variables globales */
tipoTabla *Tabla;

/* Prototipos */
void Cuenta(tipoNodo** Lista, unsigned char c);
void Ordenar(tipoNodo** Lista);
void InsertarOrden(tipoNodo** Cabeza, tipoNodo *e);
void BorrarArbol(tipoNodo *n);
void CrearTabla(tipoNodo *n, int l, int v);
void InsertarTabla(unsigned char c, int l, int v);
tipoTabla *BuscaCaracter(tipoTabla *Tabla, unsigned char c);

int main(int argc, char *argv[]) {
    tipoNodo *Lista, *Arbol;
    FILE *fe, *fs;
    unsigned char c;
    tipoNodo *p;
    tipoTabla *t;
    int nElementos;
    long int Longitud;
    unsigned long int dWORD;
    int nBits;

    if(argc < 3) {
        printf("Usar:\n%s <fichero_entrada> <fichero_salida>\n", argv[0]);
        return 1;
    }

    vector<double> tiempos;
    for (int iter = 0; iter < 4; ++iter) {
        auto start =chrono::high_resolution_clock::now();

        Lista = NULL;
        Longitud = 0;

        /* Fase 1: contar frecuencias */
        fe = fopen(argv[1], "r");
        do {
            c = fgetc(fe);
            if(feof(fe)) break;
            Longitud++;
            Cuenta(&Lista, c);
        } while(1);
        fclose(fe);

        /* Ordenar la lista de menor a mayor */
        Ordenar(&Lista);

        /* Crear el arbol */
        Arbol = Lista;
        while(Arbol && Arbol->sig) {
            p = (tipoNodo *)malloc(sizeof(tipoNodo));
            p->letra = 0;
            p->uno = Arbol;
            Arbol = Arbol->sig;
            p->cero = Arbol;
            Arbol = Arbol->sig;
            p->frecuencia = p->uno->frecuencia + p->cero->frecuencia;
            InsertarOrden(&Arbol, p);
        }

        /* Construir la tabla de códigos binarios */
        Tabla = NULL;
        CrearTabla(Arbol, 0, 0);

        /* Crear fichero comprimido */
        fs = fopen(argv[2], "wb");
        fwrite(&Longitud, sizeof(long int), 1, fs);

        /* Cuenta el número de elementos de tabla */
        nElementos = 0;
        t = Tabla;
        while(t) {
            nElementos++;
            t = t->sig;
        }
        fwrite(&nElementos, sizeof(int), 1, fs);

        /* Escribir tabla en fichero */
        t = Tabla;
        while(t) {
            fwrite(&t->letra, sizeof(char), 1, fs);
            fwrite(&t->bits, sizeof(unsigned long int), 1, fs);
            fwrite(&t->nbits, sizeof(char), 1, fs);
            t = t->sig;
        }

        /* Codificación del fichero de entrada */
        fe = fopen(argv[1], "r");
        dWORD = 0;
        nBits = 0;
        do {
            c = fgetc(fe);
            if(feof(fe)) break;
            t = BuscaCaracter(Tabla, c);
            while(nBits + t->nbits > 32) {
                c = dWORD >> (nBits-8);
                fwrite(&c, sizeof(char), 1, fs);
                nBits -= 8;
            }
            dWORD <<= t->nbits;
            dWORD |= t->bits;
            nBits += t->nbits;
        } while(1);

        while(nBits > 0) {
            if(nBits >= 8) c = dWORD >> (nBits-8);
            else c = dWORD << (8-nBits);
            fwrite(&c, sizeof(char), 1, fs);
            nBits -= 8;
        }

        fclose(fe);
        fclose(fs);

        BorrarArbol(Arbol);

        while(Tabla) {
            t = Tabla;
            Tabla = t->sig;
            free(t);
        }

        auto end =chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;
        tiempos.push_back(elapsed.count());

        // Imprimir el tiempo en la terminal
        cout << "Iteracion " << iter + 1 << ": " << elapsed.count() << " segundos" << std::endl;
    }

    // Calcular el promedio
    double suma = 0;
    for(const auto& tiempo : tiempos) {
        suma += tiempo;
    }
    double promedio = suma / tiempos.size();

    // Escribir tiempos y promedio en el archivo CSV
    ofstream file("tiemposCodificar.csv");
    for (const auto& tiempo : tiempos) {
        file << tiempo << "\n";
    }
    file << "Promedio," << promedio << "\n";
    file.close();

    return 0;
}

void Cuenta(tipoNodo** Lista, unsigned char c) {
    tipoNodo *p, *a, *q;

    if(!*Lista) {
        *Lista = (tipoNodo *)malloc(sizeof(tipoNodo));
        (*Lista)->letra = c;
        (*Lista)->frecuencia = 1;
        (*Lista)->sig = (*Lista)->cero = (*Lista)->uno = NULL;
    } else {
        p = *Lista;
        a = NULL;
        while(p && p->letra < c) {
            a = p;
            p = p->sig;
        }
        if(p && p->letra == c) p->frecuencia++;
        else {
            q = (tipoNodo *)malloc(sizeof(tipoNodo));
            q->letra = c;
            q->frecuencia = 1;
            q->cero = q->uno = NULL;
            q->sig = p;
            if(a) a->sig = q;
            else *Lista = q;
        }
    }
}

void Ordenar(tipoNodo** Lista) {
    tipoNodo *Lista2, *a;

    if(!*Lista) return;
    Lista2 = *Lista;
    *Lista = NULL;
    while(Lista2) {
        a = Lista2;
        Lista2 = a->sig;
        InsertarOrden(Lista, a);
    }
}

void InsertarOrden(tipoNodo** Cabeza, tipoNodo *e) {
    tipoNodo *p, *a;

    if(!*Cabeza) {
        *Cabeza = e;
        (*Cabeza)->sig = NULL;
    } else {
        p = *Cabeza;
        a = NULL;
        while(p && p->frecuencia < e->frecuencia) {
            a = p;
            p = p->sig;
        }
        e->sig = p;
        if(a) a->sig = e;
        else *Cabeza = e;
    }
}

void CrearTabla(tipoNodo *n, int l, int v) {
    if(n->uno) CrearTabla(n->uno, l+1, (v<<1)|1);
    if(n->cero) CrearTabla(n->cero, l+1, v<<1);
    if(!n->uno && !n->cero) InsertarTabla(n->letra, l, v);
}

void InsertarTabla(unsigned char c, int l, int v) {
    tipoTabla *t, *p, *a;

    t = (tipoTabla *)malloc(sizeof(tipoTabla));
    t->letra = c;
    t->bits = v;
    t->nbits = l;

    if(!Tabla) {
        Tabla = t;
        Tabla->sig = NULL;
    } else {
        p = Tabla;
        a = NULL;
        while(p && p->letra < t->letra) {
            a = p;
            p = p->sig;
        }
        t->sig = p;
        if(a) a->sig = t;
        else Tabla = t;
    }
}

tipoTabla *BuscaCaracter(tipoTabla *Tabla, unsigned char c) {
    tipoTabla *t;
    t = Tabla;
    while(t && t->letra != c) t = t->sig;
    return t;
}

void BorrarArbol(tipoNodo *n) {
    if(n->cero) BorrarArbol(n->cero);
    if(n->uno) BorrarArbol(n->uno);
    free(n);
}
