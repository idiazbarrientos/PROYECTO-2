#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>  

using namespace std;
using namespace std::chrono; 

/* Tipo nodo para árbol o Lista de árboles */
typedef struct _nodo
{
   char letra;           // Letra a la que hace referencia el nodo
   int frecuencia;       // Veces que aparece la letra en el texto o las letras
   _nodo *sig;           // Puntero a siguiente nodo de una lista enlazada
   _nodo *cero;          // Puntero a la rama cero de un árbol
   _nodo *uno;           // Puntero a la rama uno de un árbol
} tipoNodo;

/* Nodo para construir una lista para la tabla de códigos */
typedef struct _tabla
{
   char letra;           // Letra a la que hace referencia el nodo
   unsigned long int bits;  // Valor de la codificación de la letra
   char nbits;           // Número de bits de la codificación
   _tabla *sig;          // Siguiente elemento de la tabla
} tipoTabla;

/* Variables globales */
tipoTabla *Tabla;

/* Prototipos de funciones */
void Cuenta(tipoNodo* &Lista, char c);
void Ordenar(tipoNodo* &Lista);
void InsertarOrden(tipoNodo* &Cabeza, tipoNodo *e);
void BorrarArbol(tipoNodo *n);
void CrearTabla(tipoNodo *n, int l, int v);
void InsertarTabla(char c, int l, int v);
tipoTabla *BuscaCaracter(tipoTabla *Tabla, char c);

int main(int argc, char *argv[])
{
   tipoNodo *Lista = nullptr;       // Lista de letras y frecuencias
   tipoNodo *Arbol = nullptr;       // Árbol de letras y frecuencias
   std::ifstream fe;                // Fichero de entrada
   std::ofstream fs;                // Fichero de salida
   char c;                          // Variables auxiliares
   tipoNodo *p;
   tipoTabla *t;
   int nElementos;                  // Número de elementos en tabla
   long int Longitud = 0;           // Longitud del fichero original
   unsigned long int dWORD;         // Soble palabra usada durante la codificación
   int nBits;                       // Número de bits usados de dWORD

   if(argc < 3)
   {
      cout << "Usar:\n" << argv[0] << " <fichero_entrada> <fichero_salida>\n";
      return 1;
   }

   Lista = nullptr;

   // Fase 1: contar frecuencias
   fe.open(argv[1], std::ios::binary);
   if (!fe)
   {
      cerr << "Error al abrir el archivo de entrada.\n";
      return 1;
   }

   while (fe.get(c))
   {
      Longitud++;
      Cuenta(Lista, c);
   }
   fe.close();

   // Iniciar el cronómetro después de la lectura del archivo de entrada
   auto start = high_resolution_clock::now();

   // Ordenar la lista de menor a mayor
   Ordenar(Lista);

   // Crear el árbol
   Arbol = Lista;
   while (Arbol && Arbol->sig)
   {
      p = new tipoNodo;                     // Un nuevo árbol
      p->letra = 0;                         // No corresponde a ninguna letra
      p->uno = Arbol;                       // Rama uno
      Arbol = Arbol->sig;                   // Siguiente nodo en
      p->cero = Arbol;                      // Rama cero
      Arbol = Arbol->sig;                   // Siguiente nodo
      p->frecuencia = p->uno->frecuencia + p->cero->frecuencia; // Suma de frecuencias
      InsertarOrden(Arbol, p);              // Inserta en nuevo nodo
   }

   // Construir la tabla de códigos binarios
   Tabla = nullptr;
   CrearTabla(Arbol, 0, 0);

   // Crear fichero comprimido
   fs.open(argv[2], std::ios::binary);
   if (!fs)
   {
      cerr << "Error al abrir el archivo de salida.\n";
      return 1;
   }

   // Escribir la longitud del fichero original
   fs.write(reinterpret_cast<char*>(&Longitud), sizeof(long int));

   // Contar el número de elementos de tabla
   nElementos = 0;
   t = Tabla;
   while (t)
   {
      nElementos++;
      t = t->sig;
   }

   // Escribir el número de elementos de tabla
   fs.write(reinterpret_cast<char*>(&nElementos), sizeof(int));

   // Escribir tabla en fichero
   t = Tabla;
   while (t)
   {
      fs.write(&t->letra, sizeof(char));
      fs.write(reinterpret_cast<char*>(&t->bits), sizeof(unsigned long int));
      fs.write(&t->nbits, sizeof(char));
      t = t->sig;
   }

   // Codificación del fichero de entrada
   fe.open(argv[1], std::ios::binary);
   if (!fe)
   {
      cerr << "Error al abrir nuevamente el archivo de entrada.\n";
      return 1;
   }

   dWORD = 0;   // Valor inicial
   nBits = 0;   // Ningún bit

   while (fe.get(c))
   {
      // Busca c en tabla
      t = BuscaCaracter(Tabla, c);

      // Si nBits + t->nbits > 32, sacar un byte
      while (nBits + t->nbits > 32)
      {
         c = dWORD >> (nBits - 8);             // Extrae los 8 bits de mayor peso
         fs.write(&c, sizeof(char));           // Y lo escribe en el fichero
         nBits -= 8;                           // Ya tenemos hueco para 8 bits más
      }

      dWORD <<= t->nbits;         // Hacemos sitio para el nuevo caracter
      dWORD |= t->bits;           // Insertamos el nuevo caracter
      nBits += t->nbits;          // Actualizamos la cuenta de bits
   }

   // Extraer los cuatro bytes que quedan en dWORD
   while (nBits > 0)
   {
      if (nBits >= 8) c = dWORD >> (nBits - 8);
      else c = dWORD << (8 - nBits);
      fs.write(&c, sizeof(char));
      nBits -= 8;
   }

   fe.close();  // Cierra los ficheros
   fs.close();

   // Finalizar el cronómetro después de la compresión
   auto stop = high_resolution_clock::now();
   auto duration = duration_cast<milliseconds>(stop - start);
   cout << "Tiempo de compresión: " << duration.count() << " milisegundos" << endl;

   // Borrar Árbol
   BorrarArbol(Arbol);

   // Borrar Tabla
   while (Tabla)
   {
      t = Tabla;
      Tabla = t->sig;
      delete t;
   }

   return 0;
}

/* Actualiza la cuenta de frecuencia del carácter c */
void Cuenta(tipoNodo* &Lista, char c)
{
   tipoNodo *p, *a, *q;

   if(!Lista)  // Si la lista está vacía, el nuevo nodo será Lista
   {
      Lista = new tipoNodo;       // Un nodo nuevo
      Lista->letra = c;           // Para c
      Lista->frecuencia = 1;      // en su 1ª aparición
      Lista->sig = Lista->cero = Lista->uno = nullptr;
   }
   else
   {
      // Buscar el caracter en la lista (ordenada por letra)
      p = Lista;
      a = nullptr;
      while(p && p->letra < c)
      {
         a = p;         // Guardamos el elemento actual para insertar
         p = p->sig;    // Avanzamos al siguiente
      }
      
      // Dos casos:
      // 1) La letra c se encontró
      if(p && p->letra == c)
         p->frecuencia++;   // Actualizar frecuencia
      else
      // 2) La letra c no se encontró
      {
         // Insertar un elemento nuevo
         q = new tipoNodo;
         q->letra = c;
         q->frecuencia = 1;
         q->cero = q->uno = nullptr;
         q->sig = p;         // Insertar entre los nodos p
         if(a)
            a->sig = q;      // y a
         else
            Lista = q;       // Si a es nullptr

            Lista = q;       // Si `a` es `nullptr`, el nuevo es el primero
      }
   }
}

/* Ordena Lista de menor a mayor por frecuencias */
void Ordenar(tipoNodo* &Lista)
{
   tipoNodo *Lista2, *a;

   if(!Lista) return;   // Lista vacía
   Lista2 = Lista;
   Lista = nullptr;
   while(Lista2)
   {
      a = Lista2;              // Toma los elementos de Lista2
      Lista2 = a->sig;
      InsertarOrden(Lista, a); // Y los inserta por orden en Lista
   }
}

/* Inserta el elemento `e` en la Lista ordenado por frecuencia de menor a mayor */
void InsertarOrden(tipoNodo* &Cabeza, tipoNodo *e)
{
   tipoNodo *p, *a;

   if(!Cabeza) // Si `Cabeza` es `nullptr`, `e` es el primer elemento
   {
      Cabeza = e;
      Cabeza->sig = nullptr;
   }
   else
   {
      // Buscar el caracter en la lista (ordenada por frecuencia)
      p = Cabeza;
      a = nullptr;
      while(p && p->frecuencia < e->frecuencia)
      {
         a = p;         // Guardamos el elemento actual para insertar
         p = p->sig;    // Avanzamos al siguiente
      }
      
      // Insertar el elemento
      e->sig = p;
      if(a)
         a->sig = e;   // Insertar entre `a` y `p`
      else
         Cabeza = e;    // El nuevo es el primero
   }
}

/* Función recursiva para crear Tabla */
void CrearTabla(tipoNodo *n, int l, int v)
{
   if(n->uno)  CrearTabla(n->uno, l+1, (v<<1)|1);
   if(n->cero) CrearTabla(n->cero, l+1, v<<1);
   if(!n->uno && !n->cero) InsertarTabla(n->letra, l, v);
}

/* Insertar un elemento en la tabla */
void InsertarTabla(char c, int l, int v)
{
   tipoTabla *t, *p, *a;

   t = new tipoTabla;         // Crea un elemento de tabla
   t->letra = c;              // Y lo inicializa
   t->bits = v;
   t->nbits = l;

   if(!Tabla)   // Si `Tabla` es `nullptr`, entonces el elemento `t` es el primero
   {
      Tabla = t;
      Tabla->sig = nullptr;
   }
   else
   {
      // Buscar el caracter en la lista (ordenada por frecuencia)
      p = Tabla;
      a = nullptr;
      while(p && p->letra < t->letra)
      {
         a = p;         // Guardamos el elemento actual para insertar
         p = p->sig;    // Avanzamos al siguiente
      }
      
      // Insertar el elemento
      t->sig = p;
      if(a)
         a->sig = t;   // Insertar entre `a` y `p`
      else
         Tabla = t;    // El nuevo es el primero
   }
}

/* Buscar un caracter en la tabla, devuelve un puntero al elemento de la tabla */
tipoTabla *BuscaCaracter(tipoTabla *Tabla, char c)
{
   tipoTabla *t;

   t = Tabla;
   while(t && t->letra != c)
      t = t->sig;
   
   return t;
}

/* Función recursiva para borrar un árbol */
void BorrarArbol(tipoNodo *n)
{
   if(n->cero) BorrarArbol(n->cero);
   if(n->uno)  BorrarArbol(n->uno);
   delete n;
}
