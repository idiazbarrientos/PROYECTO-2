#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <iostream>
#include <numeric> // Para std::accumulate

/* Tipo nodo para árbol */
typedef struct _nodo {
   unsigned char letra;           /* Letra a la que hace referencia el nodo */
   unsigned long int bits;        /* Valor de la codificación de la letra */
   char nbits;                    /* Número de bits de la codificación */
   struct _nodo *cero;            /* Puntero a la rama cero de un árbol */
   struct _nodo *uno;             /* Puntero a la rama uno de un árbol */
} tipoNodo;                       /* Nombre del tipo */

/* Funciones prototipo */
void BorrarArbol(tipoNodo *n);

int main(int argc, char *argv[]) {
   if(argc < 3) {
      printf("Usar:\n%s <fichero_entrada> <fichero_salida>\n", argv[0]);
      return 1;
   }

   std::vector<double> tiempos;
   for (int iter = 0; iter < 20; ++iter) {
      auto start = std::chrono::high_resolution_clock::now();

      tipoNodo *Arbol;        /* Arbol de codificación */
      long int Longitud;      /* Longitud de fichero */
      int nElementos;         /* Elementos de árbol */
      unsigned long int bits; /* Almacen de bits para decodificación */
      FILE *fe, *fs;          /* Ficheros de entrada y salida */

      tipoNodo *p, *q;        /* Auxiliares */
      unsigned char a;
      int i, j;

      /* Crear un arbol con la información de la tabla */
      Arbol = (tipoNodo *)malloc(sizeof(tipoNodo)); /* un nodo nuevo */
      Arbol->letra = 0;
      Arbol->uno = Arbol->cero = NULL;
      fe = fopen(argv[1], "rb");
      fread(&Longitud, sizeof(long int), 1, fe); /* Lee el número de caracteres */
      fread(&nElementos, sizeof(int), 1, fe); /* Lee el número de elementos */
      for(i = 0; i < nElementos; i++) /* Leer todos los elementos */
      {
         p = (tipoNodo *)malloc(sizeof(tipoNodo)); /* un nodo nuevo */
         fread(&p->letra, sizeof(char), 1, fe); /* Lee el carácter */
         fread(&p->bits, sizeof(unsigned long int), 1, fe); /* Lee el código */
         fread(&p->nbits, sizeof(char), 1, fe); /* Lee la longitud */
         p->cero = p->uno = NULL;
         /* Insertar el nodo en su lugar */
         j = 1 << (p->nbits-1);
         q = Arbol;
         while(j > 1) {
            if(p->bits & j) /* es un uno*/
               if(q->uno) q = q->uno;   /* Si el nodo existe, nos movemos a él */
               else {                   /* Si no existe, lo creamos */
                  q->uno = (tipoNodo *)malloc(sizeof(tipoNodo)); /* un nodo nuevo */
                  q = q->uno;
                  q->letra = 0;
                  q->uno = q->cero = NULL;
               }
            else /* es un cero */
               if(q->cero) q = q->cero; /* Si el nodo existe, nos movemos a él */
               else {                   /* Si no existe, lo creamos */
                  q->cero = (tipoNodo *)malloc(sizeof(tipoNodo)); /* un nodo nuevo */
                  q = q->cero;
                  q->letra = 0;
                  q->uno = q->cero = NULL;
               }
            j >>= 1;  /* Siguiente bit */
         }
         /* Ultimo Bit */
         if(p->bits & 1) /* es un uno*/
            q->uno = p;
         else            /* es un cero */
            q->cero = p;
      }
      /* Leer datos comprimidos y extraer al fichero de salida */
      bits = 0;
      fs = fopen(argv[2], "w");
      /* Lee los primeros cuatro bytes en la doble palabra bits */
      fread(&a, sizeof(char), 1, fe);
      bits |= a;
      bits <<= 8;
      fread(&a, sizeof(char), 1, fe);
      bits |= a;
      bits <<= 8;
      fread(&a, sizeof(char), 1, fe);
      bits |= a;
      bits <<= 8;
      fread(&a, sizeof(char), 1, fe);
      bits |= a;
      j = 0; /* Cada 8 bits leemos otro byte */
      q = Arbol;
      /* Bucle */
      do {
         if(bits & 0x80000000) q = q->uno; else q = q->cero; /* Rama adecuada */
         bits <<= 1;           /* Siguiente bit */
         j++;
         if(8 == j) {          /* Cada 8 bits */
            i = fread(&a, sizeof(char), 1, fe); /* Leemos un byte desde el fichero */
            bits |= a;                    /* Y lo insertamos en bits */
            j = 0;                        /* No quedan huecos */
         }
         if(!q->uno && !q->cero) {        /* Si el nodo es una letra */
            putc(q->letra, fs);           /* La escribimos en el fichero de salida */
            Longitud--;                   /* Actualizamos longitud que queda */
            q = Arbol;                    /* Volvemos a la raiz del árbol */
         }
      } while(Longitud);                  /* Hasta que acabe el fichero */
      /* Procesar la cola */

      fclose(fs);                         /* Cerramos ficheros */
      fclose(fe);

      BorrarArbol(Arbol);                 /* Borramos el árbol */

      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      tiempos.push_back(elapsed.count());

      // Imprimir el tiempo en la terminal
      std::cout << "Iteracion " << iter + 1 << ": " << elapsed.count() << " segundos" << std::endl;
   }

   // Calcular el promedio de los tiempos
   double promedio = std::accumulate(tiempos.begin(), tiempos.end(), 0.0) / tiempos.size();

   // Escribir los tiempos y el promedio en el archivo CSV
   std::ofstream file("tiempos.csv");
   for (const auto &tiempo : tiempos) {
      file << tiempo << "\n";
   }
   file << "Promedio: " << promedio << "\n";
   file.close();

   return 0;
}

/* Función recursiva para borrar un arbol */
void BorrarArbol(tipoNodo *n) {
   if(n->cero) BorrarArbol(n->cero);
   if(n->uno)  BorrarArbol(n->uno);
   free(n);
}