/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	01/03/2017
 */

#ifndef _METADATA_H_
#define _METADATA_H_

#define PADDING_SB
#define PADDING_INODO

#define numInodo 64
#define numBloquesDato 2048


typedef struct{
	unsigned int numMagico; /* Número mágico del superbloque: 0x000D5500 */
	unsigned int numBloquesMapaInodos; /* Número de bloques del mapa inodos */
	unsigned int numBloquesMapaDatos; /* Número de bloques del mapa datos */
	unsigned int numInodos; /* Número de inodos en el dispositivo */
	unsigned int primerInodo; /* Número bloque del 1º inodo del disp. (inodo raíz) */
	unsigned int numBloquesDatos; /* Número de bloques de datos en el disp. */
	unsigned int primerBloqueDatos; /* Número de bloque del 1º bloque de datos */
	unsigned int tamDispositivo; /* Tamaño total del disp. (en bytes) */
	char relleno[PADDING_SB]; /* Campo de relleno (para completar un bloque) */
} TipoSuperbloque;

typedef struct {
	char nombre[32]; /* Nombre del fichero/directorio asociado */
	unsigned int tamanyo; /* Tamaño actual del fichero en bytes */
	unsigned int bloqueDirecto; /* Número del bloque directo */
	char relleno[PADDING_INODO]; /* Campo relleno para llenar un bloque */
} TipoInodoDisco;

TipoSuperbloque sbloques[1];
int i_map [numInodo];
char b_map [numBloquesDato];
TipoInodoDisco inodos [numInodo];

struct{
	int posicion;
	int abierto;

} inodos_x [numInodo];

#endif
