/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	01/03/2017
 */

#define PADDING_SB 2020
#define PADDING_INODO 20
#define PADDING_MMAP 1894

#define MAX_INODO 64
#define MAX_DATA 90
#define MAX_OPEN_FILES 32

#define FILE_NAME_SIZE 32

/******************************************************************************
 *                            ESTRUCTURAS EN DISCO                            *
 ******************************************************************************/

// Estructura de superbloque. Tamaño: 28B + padding.
typedef struct {
    unsigned int magNum; // Número mágico del superbloque.
    unsigned int mapBlocks; // Número de bloques que ocupa el mapa.
    unsigned int inodeNum; // Número de i-nodos.
    unsigned int firstInode; // Bloque donde empiezan los i-nodos.
    unsigned int dataNum; // Número de bloques de datos.
    unsigned int firstData; // Bloque donde empiezan los datos.
    unsigned int devSize; // Tamaño del dispositivo.
    char padding[PADDING_SB]; // Relleno.
} sb_t;

// Estructura de i-nodo.
typedef struct {
    unsigned int inode; // Número de i-nodo.
    char name[FILE_NAME_SIZE]; // Nombre del archivo del i-nodo.
    unsigned int size; // Tamaño del fichero.
    unsigned int firstBlock; // Primer bloque directo.
    char padding[PADDING_INODO];
} inode_t;

// Estructura de mapa de memoria. Bytes.
typedef struct {
    char iNode[MAX_INODO]; // Mapa de i-nodos libres.
    char data[MAX_DATA]; // Mapa de bloques de memoria libres.
    char padding[PADDING_MMAP]; // Relleno para llegar al tamaño de bloque.
} mmap_t;

// Estructura para almacenar un sub-bloque de datos.
typedef struct {
    char data[(MAX_FILE_SIZE/2) - 1];
    char next;
} block_t;


/******************************************************************************
 *                           ESTRUCTURAS EN MEMORIA                           *
 ******************************************************************************/

/******************************************************************************
 *                             VARIABLES GLOBALES                             *
 ******************************************************************************/

sb_t *superBlock; // Superbloque.
inode_t *inodes; // i-nodos.
mmap_t *mmap; // Mapa de memoria.
int openFiles[MAX_OPEN_FILES][2]; // Tabla de i-nodos con punteros de posición.
