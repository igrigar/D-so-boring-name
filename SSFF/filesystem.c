/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	filesystem.c
 * @brief Implementation of the core file system funcionalities and auxiliary
 *	functions.
 * @date	01/03/2017
 */

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "include/filesystem.h"		// Headers for the core functionality
#include "include/auxiliary.h"		// Headers for auxiliary functions
#include "include/metadata.h"		// Type and structure declaration of the file system
#include "include/crc.h"			// Headers for the CRC functionality


/*
 * @brief 	Generates the proper file system structure in a storage device, as
 *   designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{
	// Get size of the file.
	FILE *disk = fopen(DEVICE_IMAGE, "r");
	if (disk == NULL) return -1; // Error abriendo disco.
	fseek(disk, 0L, SEEK_END);
	if (ftell(disk) > deviceSize) return -1; // SFF mayor que tamaño disco.
	fclose(disk); // Ceramos el fichero, ya no es necesario.

	// Formateamos el disco.
	char b0[BLOCK_SIZE];
	bzero(b0, BLOCK_SIZE);
	for (int i = 0; i < (deviceSize/BLOCK_SIZE); i++)
	if (bwrite(DEVICE_IMAGE, i, b0) == -1) return -1; // Error formateo.

	// Creamos superbloque.
	superBlock = malloc(sizeof(sb_t));
	superBlock[0].magNum = 0xFF;
	superBlock[0].mapBlocks = 1;
	superBlock[0].inodeNum = MAX_INODO;
	superBlock[0].firstInode = 3;
	superBlock[0].dataNum = (deviceSize/BLOCK_SIZE) - 5;
	superBlock[0].firstData = 5;
	superBlock[0].devSize = deviceSize;

	// Gestión del mapa de memoria.
	mmap = malloc(sizeof(mmap_t));

	bzero(mmap->iNode, MAX_INODO);
	bzero(mmap->data, MAX_DATA);
	bzero(mmap->padding, PADDING_MMAP);

	// Reservamos memoria para los i-nodos.
	inodes = malloc(sizeof(inode_t) * MAX_INODO);
	// Limpiamos la estructura de i-nodos.
	for (int i = 0; i < MAX_INODO; i++) bzero(&(inodes[i]), sizeof(inode_t));

	// Guardamos todo en disco.
	if (unmountFS() == -1) return -1;

	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
	char buffer[BLOCK_SIZE];

	// Reservar toda la memoria para superbloque.
	superBlock = malloc(sizeof(sb_t));

	// Leer superbloque.
	if (bread(DEVICE_IMAGE, 1, buffer) == -1) return -1;
	sb_t *sb = (sb_t *) buffer;
	superBlock[0] = *sb;

	// Reservar memoria para el mapa de memoria.
	mmap = malloc(sizeof(mmap_t));

	// Leer mapa memoria.
	if (bread(DEVICE_IMAGE, 2,  buffer) == -1) return -1;

	mmap_t *map = (mmap_t *) buffer;
	mmap[0] = *map;

	// Reservar memoria para i-nodos.
	inodes = malloc(sizeof(inode_t) * MAX_INODO);
	// Leer i-nodos.
	int node = 0;
	for (int i = 0; i < ((superBlock[0].inodeNum * sizeof(inode_t))/BLOCK_SIZE); i++)
		if (bread(DEVICE_IMAGE, (i + superBlock[0].firstInode), buffer) == -1) return -1;
		else {
			for (int j = 0; j < 32; j++, node++) {
				char newINode[sizeof(inode_t)]; // Buffer que contiene el i-nodo.
				memcpy(newINode, buffer + (sizeof(inode_t) * j), sizeof(inode_t)); // Copiamos un i-nodo.
				inode_t *inode = (inode_t *) newINode; // Convertimos a estrucutura i-nodo.
				inodes[node] = *inode;
			}
		}

	// Limpiar tabla de ficheros abiertos.
	for (int i = 0; i < MAX_OPEN_FILES; i++) {
		openFiles[i][0] = -1; // Un i-nodo no puede tener valor negativo.
		openFiles[i][1] = 0;
	}

	return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	// Escribiendo superbloque.
	if (bwrite(DEVICE_IMAGE, 1, (char *) superBlock) == -1) return -1;
	// Escribiendo mapa de memoria.
	if (bwrite(DEVICE_IMAGE, 2, (char *) mmap) == -1) return -1;

	// Escribiendo inodos.
	for (int i = 0; i < ((superBlock[0].inodeNum * sizeof(inode_t))/BLOCK_SIZE); i++)
		if (bwrite(DEVICE_IMAGE, (i + superBlock[0].firstInode),
			((char *) inodes + (i * BLOCK_SIZE))) == -1) return -1;

	// Liberar las estructuras de memoria.

	return 0;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
	if (namei(fileName) != -1) return -1; // El fichero ya existe.
	int i = ialloc(), b = balloc();
	if (i == -1) return -2; // No hay mas espacio libre.
	if (b == -1) return -2; // No hay mas espacio libre.

	// Establecer datos del i-nodo.
	inodes[i].inode = i;
	strcpy(inodes[i].name, fileName);
	inodes[i].size = 0;
	inodes[i].firstBlock = b;

	return 0;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	int i = namei(fileName); // Obtenemos el numero de i-nodo.
	if (i == -1) return -1; // El fichero no existe.

	if (ifree(i) == -1) return -2;
	if (bfree(inodes[i].firstBlock) == -1) return -2;
	// Si existiese un segundo bloque de datos eliminarlo.
	return 0;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	int i = namei(fileName);
	if (i == -1) return -1; // El fichero no existe.

	// Buscamos entrada libre en tabla ficheros abiertos.
	// Comprobamos si el fichero ya está abierto.
	int pos = -1;
	for (int j = 0; j < MAX_OPEN_FILES; j++)
		if (openFiles[j][0] == -1 && pos == -1) pos = j;
		else if (openFiles[j][0] == i) return -2; // fichero ya abierto.

	if (pos != -1) {
		openFiles[pos][0] = i; // Damos el valor del i-nodo.
		openFiles[pos][1] = 0;
		return pos;
	}

	// No se pueden abrir mas ficheros.
	return -2;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	// Comprobar que el descriptor tiene un valor posible.
	if (fileDescriptor < 0 || fileDescriptor >= MAX_OPEN_FILES) return -1;
	if (openFiles[fileDescriptor][0] == -1) return -1; // Fichero cerrado.

	openFiles[fileDescriptor][0] = -1; // Marcamos el fichero como cerrado.
	openFiles[fileDescriptor][1] = 0; // Reestablecemos el puntero.

	return 0;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	// Descriptor de fichero no válido.
	if (fileDescriptor >= MAX_OPEN_FILES || fileDescriptor < 0) return -1;
	if (openFiles[fileDescriptor][0] == -1) return -1; // Fichero cerrado.

	// Intentando leer más que el tamaño máximo.
	if (numBytes + openFiles[fileDescriptor][1] >= MAX_FILE_SIZE)
		// Leemos el mayor número de bytes posible.
		numBytes -= ((numBytes + openFiles[fileDescriptor][1]) - MAX_FILE_SIZE);

	// Obtener el bloque de memoria.
	char block[BLOCK_SIZE];
	int blockNum = bmap(openFiles[fileDescriptor][0], openFiles[fileDescriptor][1]);

	// Leemos el bloque de memoria.
	if (bread(DEVICE_IMAGE, (blockNum/2 + superBlock[0].firstData), block) == -1) return -1;

	// Obtenemos el sub-bloque.
	char subBlock[BLOCK_SIZE/2];
	block_t *sb;

	// Calculamos en que sub-bloque se encuentra-
	int offset = (blockNum%2) * sizeof(block_t);
	strncpy(subBlock, (char *) block + offset, sizeof(block_t));
	sb = (block_t *) subBlock;

	// Relativizamos el puntero de lectura.
	int pointer = openFiles[fileDescriptor][1];
	while (pointer >= MAX_FILE_SIZE/2) pointer -= MAX_FILE_SIZE/2;

	// Escribimos en el sub-bloque.
	if (((MAX_FILE_SIZE/2) - pointer) < numBytes) { // Caso mitad dos bloques.
		int read = (MAX_FILE_SIZE/2) - pointer; // Cuanto se va a leer 1er bloque.

		strncpy(buffer, (char *) sb[0].data + pointer, numBytes); // Copiamos los datos.

		// Cambiamos el puntero del fichero.
		openFiles[fileDescriptor][1] += read;

		// Leemos recursivvamente cuanto quede.
		char newBuffer[numBytes - read];
		readFile(fileDescriptor, newBuffer, numBytes - read);
		strncpy(buffer + read, (char *) sb[0].data + pointer, (numBytes - read)); // Copiamos los datos.
	} else {
		strncpy(buffer, (char *) sb[0].data + pointer, numBytes); // Copiamos los datos.

		// Cambiamos el puntero del fichero.
		openFiles[fileDescriptor][1] += numBytes;
	}

	return numBytes;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	// Descriptor de fichero no válido.
	if (fileDescriptor >= MAX_OPEN_FILES || fileDescriptor < 0) return -1;
	if (openFiles[fileDescriptor][0] == -1) return -1; // Fichero cerrado.

	// Intentando escribir más que el tamaño máximo.
	if (numBytes + openFiles[fileDescriptor][1] >= MAX_FILE_SIZE)
		// Escribimos el mayor número de bytes posible.
		numBytes -= ((numBytes + openFiles[fileDescriptor][1]) - MAX_FILE_SIZE);

	// Obtener el bloque de memoria.
	char block[BLOCK_SIZE];
	int blockNum = bmap(openFiles[fileDescriptor][0], openFiles[fileDescriptor][1]);

	// Leemos el bloque de memoria.
	if (bread(DEVICE_IMAGE, (blockNum/2 + superBlock[0].firstData), block) == -1) return -1;

	// Obtenemos el sub-bloque.
	char subBlock[BLOCK_SIZE/2];
	block_t *sb;

	// Calculamos en que sub-bloque se encuentra-
	int offset = (blockNum%2) * sizeof(block_t);
	strncpy(subBlock, (char *) block + offset, sizeof(block_t));
	sb = (block_t *) subBlock;

	// Relativizamos el puntero de escritura.
	int pointer = openFiles[fileDescriptor][1];
	while (pointer >= MAX_FILE_SIZE/2) pointer -= MAX_FILE_SIZE/2;

	// Escribimos en el sub-bloque.
	if (((MAX_FILE_SIZE/2) - pointer) < numBytes) { // Caso mitad dos bloques.
		int written = (MAX_FILE_SIZE/2) - pointer;

		// Obtenemos, si fuese necesario y posible, el siguiente sub-bloque.
		if (inodes[openFiles[fileDescriptor][0]].size < MAX_FILE_SIZE/2 &&
				sb[0].next < 0) {
			// No tiene siguiente sub-bloque asignado pero puede pedir uno nuevo.
			sb[0].next = balloc(); // Intentamos obtener sub-bloque nuevo.
		}

		// Copiamos el máximo numero de bytes posible
		strncpy((char *) sb[0].data + pointer, buffer, (MAX_FILE_SIZE/2) - pointer);
		// Recomponemos el bloque de datos.
		strncpy((char *) block + offset, (char *) sb, sizeof(block_t));

		if (bwrite(DEVICE_IMAGE, (blockNum/2 + superBlock[0].firstData), block) == -1)
			return -1; // Escribimos el bloque.

		// Establecemos el nuevo tamaño de fichero.
		inodes[openFiles[fileDescriptor][0]].size = (MAX_FILE_SIZE/2) - 1;

		// Cambiamos el puntero del fichero.
		openFiles[fileDescriptor][1] += written;

		if (sb[0].next < 0) // No tiene siguiente sub-bloque asignado ni lo tendrá.
			return written;

		// Escribimos lo que queda. VIVA LA RECURSIÓN!!
		writeFile(fileDescriptor, (buffer + written), (numBytes - written));
	} else {
		strncpy((char *) sb[0].data + pointer, buffer, numBytes); // Copiamos los datos.

		// Recomponemos el bloque de datos.
		strncpy((char *) block + offset, (char *) sb, sizeof(block_t));
		if (bwrite(DEVICE_IMAGE, (blockNum/2 + superBlock[0].firstData), block) == -1)
			return -1; // Escribimos el bloque.

		// Establecemos el nuevo tamaño de fichero.
		if ((numBytes + openFiles[fileDescriptor][1]) > 
				inodes[openFiles[fileDescriptor][0]].size) {
			inodes[openFiles[fileDescriptor][0]].size = numBytes +
				openFiles[fileDescriptor][1];
		}

		// Cambiamos el puntero del fichero.
		openFiles[fileDescriptor][1] += numBytes;
	}

	return numBytes;
}


/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, int whence, long offset)
{
	// Descriptor de fichero no válido.
	if (fileDescriptor >= MAX_OPEN_FILES || fileDescriptor < 0) return -1;
	if (openFiles[fileDescriptor][0] == -1) return -1; // Fichero cerrado.

	int newPointer = openFiles[fileDescriptor][1] + offset;
	switch (whence) {
		case FS_SEEK_CUR:
			if (newPointer < 0 || newPointer > inodes[openFiles[fileDescriptor][0]].size)
				return -1;
			else openFiles[fileDescriptor][1] += offset;
		case FS_SEEK_END:
			openFiles[fileDescriptor][1] = inodes[openFiles[fileDescriptor][0]].size;
			break;
		case FS_SEEK_BEGIN:
			openFiles[fileDescriptor][1] = 0;
			break;
		default: return -1;
	}

	return 0;
}

/*
 * @brief 	Verifies the integrity of the file system metadata.
 * @return 	0 if the file system is correct, -1 if the file system is corrupted, -2 in case of error.
 */
int checkFS(void)
{
	return -2;
}

/*
 * @brief 	Verifies the integrity of a file.
 * @return 	0 if the file is correct, -1 if the file is corrupted, -2 in case of error.
 */
int checkFile(char *fileName)
{
	return -2;
}



/******************************************************************************
 *                            FUNCIONES AUXILIARES                            *
 ******************************************************************************/

/*
 * @Brief: buscar un i-nodo por nombre.
 * @Return: número de i-nodo si existe, -1 si no.
 */
int namei(char *name) {
	for (int i  = 0; i < superBlock[0].inodeNum; i++) // Recorrer toda la lista de i-nodos.
		if (!strcmp(inodes[i].name, name)) return inodes[i].inode; // i-nodo con mismo nombre.

	return -1;
}

/*
 * @Brief: buscar un i-nodo libre a traves del mapa de i-nodos.
 * @Return: número de i-nodo libre si lo hay, -1 si no hay i-nodos libres.
 */
int ialloc() {
	for (int i = 0; i < superBlock[0].inodeNum; i++) // Recorrer todo el mapa de i-nodos.
		if (mmap->iNode[i] == 0) { // i-nodo libre encontrado.
			mmap->iNode[i] = 1; // Marcar i-nodo como ocupado.

			return i;
		}

	return -1;
}

/*
 * @Brief: liberar un i-nodo dado su número.
 * @Return: 0 si se ha liberado correctamente, -1 si no.
 */
int ifree(int i) {
	if (i > superBlock[0].inodeNum || i < 0) return -1; // Número i-nodo errneo.
	mmap->iNode[i] = 0; // Marcar i-nodo como libre.

	// "Limpiar" el contenido del i-nodo.
	//inodes[i].inode = -1;
	//inodes[i].size = -1;
	//inodes[i].firstBlock = -1;
	bzero(inodes[i].name, FILE_NAME_SIZE);

	return 0;
}

/*
 * @Brief: buscar un sub-bloque de memoria libre.
 * @Return: numero de sub-bloque libre si existe, -1 si no.
 */
int balloc() {
	for (int i = 0; i < (superBlock[0].dataNum * 2); i++) // Recorrer todo el mapa de datos.
		if (mmap->data[i] == 0) {
			mmap->data[i] = 1;

			// Proceso de reseteo del sub-bloque.
			// Obtenemos el bloque.
			char block[BLOCK_SIZE];
			if (bread(DEVICE_IMAGE, (i/2 + superBlock[0].firstData), block) == -1) return -1;

			// Creamos las variables apropiadas.
			char subBlock[BLOCK_SIZE/2];
			block_t *sb;

			// Calculamos en que sub-bloque se encuentra.
			int offset = (i%2) * sizeof(block_t);

			// Obtenemos el sub-bloque.
			strncpy(subBlock, (char *) block + offset, sizeof(block_t));
			sb = (block_t *) subBlock;

			// Establecemos los valores "por defecto".
			memset(sb->data, -1, ((BLOCK_SIZE/2) - 1));
			sb->next = -1;

			// Re-componemos el bloque y lo escribimos a disco.
			strncpy((char *) block + offset, (char *) sb, sizeof(block_t));
			if (bwrite(DEVICE_IMAGE, (i/2 + superBlock[0].firstData), block) == -1)
				return -1;

			return i;
		}
	return -1;
}

/*
 * @Brief: liberar un bloque dado su número.
 * @Return: 0 si se ha liberado correctamente, -1 si no.
 */
int bfree(int i) {
	if (i > (superBlock[0].dataNum * 2) || i < 0) return -1; // Número i-nodo errneo.
	mmap->data[i] = 0; // Marcar i-nodo como libre.

	// Buscamos el segundo bloque si lo hay.
	char block[BLOCK_SIZE];
	if (bread(DEVICE_IMAGE, i/2 + superBlock[0].firstData, block) == -1) return -1;

	char subBlock[BLOCK_SIZE/2];
	block_t *sb;

	int offset = (i%2) * sizeof(block_t);
	strncpy(subBlock, (char *) block + offset, sizeof(block_t));
	sb = (block_t *) subBlock;

	if (sb->next != -1) // Existe el siguiente bloque.
		if (bfree(sb->next) == -1) return -1; // Recursivo == escalable.

	return 0;
}

/*
 * @Brief: dado un offset y un i-nodo, calcular cual es el bloque en el que se
 *	encuentran alojados los datos.
 * @Return: si existe, el bloque de datos asociado, -1 en cualquier otro caso.
 */
int  bmap(int i, int offset) {
	if (i < 0 || i >= MAX_INODO) return -1; // i-nodo erróneo.

	if (offset <= ((BLOCK_SIZE/2) - 1)) return inodes[i].firstBlock; // Puntero en primer bloque.

	// Buscando bloque encadenado, si lo hubiese.
	char block[BLOCK_SIZE];
	if (bread(DEVICE_IMAGE, inodes[i].firstBlock/2 + superBlock[0].firstData,
			block) == -1) return -1;

	char subBlock[BLOCK_SIZE/2];
	block_t *sb;

	offset = (inodes[i].firstBlock%2) * sizeof(block_t);
	strncpy(subBlock, (char *) block + offset, sizeof(block_t));
	sb = (block_t *) subBlock;

	return sb->next;
}

/*
 * @Brief: calcular el valor de reduncancia cíclica de los distintos artefactos
 *	presentes en el sistema.
 * @Return: valor de redundanca cíclica, -1 en caso de error.
 */
uint32_t computeCRC(int i, int type) {
	char *buffer, block[BLOCK_SIZE];
	int size;
	uint32_t crc;

	switch (type) {
		case CRC_SB:
			// Tamaño de los metadatos: #Bloques inodos + mapa memoria.
			size = (superBlock[0].inodeNum * sizeof(inode_t)) + BLOCK_SIZE;
			buffer = malloc(size);

			// Obtenemos el bloque de mapa memoria.
			if (bread(DEVICE_IMAGE, 2, block) == -1) return -1;
			strncpy((char *) buffer, block, BLOCK_SIZE); // Lo copiamos al nuevo buffer.

			// Obtenemos los i-nodos.
			for (int i = 0; 
					i < (superBlock[0].inodeNum * sizeof(inode_t)) /BLOCK_SIZE;
					i++) {
				if (bread(DEVICE_IMAGE, i + superBlock[0].firstInode, block) == -1)
					return -1; // Leemos el bloque de i-nodos.
				strncpy((char *) buffer + ((1 + i) * BLOCK_SIZE), block, BLOCK_SIZE);
			}

			crc = CRC32((const unsigned char *) buffer, size);
			free(buffer);
			return crc;
		case CRC_FILE:
			if (i < 0 || i >= MAX_INODO) return -1; // i-nodo erróneo.

			// Reservamos la memoria.
			size = inodes[i].size;
			buffer = malloc(size);

			// Buscando bloque encadenado, si lo hubiese.
			if (bread(DEVICE_IMAGE, inodes[i].firstBlock/2 + superBlock[0].firstData,
				block) == -1) return -1;

			char subBlock[BLOCK_SIZE/2];
			block_t *sb;

			int offset = (inodes[i].firstBlock%2) * sizeof(block_t);
			strncpy(subBlock, (char *) block + offset, sizeof(block_t));
			sb = (block_t *) subBlock;

			if (sb->next == -1) { // Solo un sub-bloque.
				strncpy((char *) buffer, sb->data, size);
				crc = CRC32((const unsigned char *) buffer, size);
				free(buffer);
				return crc;
			}

			// Primer sub-bloque.
			strncpy((char *) buffer, sb->data, (BLOCK_SIZE/2) - 1);

			// Segundo sub-bloque.
			if (bread(DEVICE_IMAGE, sb->next/2 + superBlock[0].firstData,
				block) == -1)	return -1;

			offset = (sb->next%2) * sizeof(block_t);
			strncpy(subBlock, (char *) block + offset, sizeof(block_t));
			sb = (block_t *) subBlock;

			// Copiamos lo que falta.
			strncpy((char *) buffer + (BLOCK_SIZE/2) - 1, sb->data,
				size - (BLOCK_SIZE/2) - 1);

			crc = CRC32((const unsigned char *) buffer, size);
			free(buffer);
			return crc;
		default: return -1;
	}
}
