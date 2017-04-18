/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	01/03/2017
 */

#include "include/filesystem.h"		// Headers for the core functionality
#include "include/auxiliary.h"		// Headers for auxiliary functions
#include "include/metadata.h"		// Type and structure declaration of the file system
#include "include/crc.h"			// Headers for the CRC functionality
#include <stdio.h>
#include <string.h>

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
 
int mkFS(long deviceSize)
{	

	if(deviceSize>MAX_DISK_SIZE || deviceSize<MIN_DISK_SIZE){ return -1; }

	sbloques[0].numMagico = 1234;
	sbloques[0].numInodos = numInodo;
	sbloques[0].numBloquesMapaInodos = 15;
	sbloques[0].numBloquesMapaDatos = 15;
	sbloques[0].tamDispositivo = deviceSize;

	int i;
	
	for (i=0; i<sbloques[0].numInodos; i++)
	i_map[i] = 0; // free

	for (i=0; i<sbloques[0].numBloquesDatos; i++)
	b_map[i] = 0; // free

	for (i=0; i<sbloques[0].numInodos; i++)
	memset(&(inodos[i]), 0, sizeof(TipoInodoDisco) );

	// escribir los valores por defecto al disco
  sync();
	return 0;
	
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{

    int r;
    
    // leer bloque 1 de disco en sbloques[0]
    r=bread("disk.dat", 1, ((char *)&(sbloques[0])) );
    if(r==-1){return r; }
    
    printf("Hola\n"); 
    
    int i;
    // leer los bloques para el mapa de i-nodos
    for (i=0; i<sbloques[0].numBloquesMapaInodos; i++){
    r=bread("disk.dat", 2+i, ((char *)i_map + i*BLOCK_SIZE)) ;}
    
    if(r==-1){return r;}
    
    // leer los bloques para el mapa de bloques de datos
    for (i=0; i<sbloques[0].numBloquesMapaDatos; i++)
    r=bread("disk.dat", 2+i+sbloques[0].numBloquesMapaInodos, ((char *)b_map + i*BLOCK_SIZE));
    
    if(r==-1){return r;}
    
    // leer los i-nodos a memoria
    for (i=0; i<(sbloques[0].numInodos*sizeof(TipoInodoDisco)/BLOCK_SIZE); i++)
    r=bread("disk.dat", i+sbloques[0].primerInodo, ((char *)inodos + i*BLOCK_SIZE)); 
    
    //Si bread no funciona, devolverá -1 antes o en esta última versión. Si no es así, devolverá
    //0 en este último return
    
    int k=checkFS();
    if(k!=0) return -2;
    
    return r;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{

  int i;
  for (i=0; i<sbloques[0].numInodos; i++) {
      if (inodos_x[i].abierto == 1) {
      return -1;
        }
      }
        
    sync();
  	return 0;
}

int ialloc ( void )
{
  // buscar un i-nodo libre
  int i;
  for (i=0; i<sbloques[0].numInodos; i++)
  {
  if (i_map[i] == 0) {
  // inodo ocupado ahora
  i_map[i] = 1;
  // valores por defecto en el i-nodo
  memset(&(inodos[i]), 0,
  sizeof(TipoInodoDisco));
  // devolver identificador de i-nodo
  return i;
  }
  }
  return -2;
}

int alloc ( void )
{
  char b[BLOCK_SIZE];
  int i;
  for (i=0; i<sbloques[0].numBloquesDatos; i++)
  {
  if (b_map[i] == 0) {
  // bloque ocupado ahora
  b_map[i] = 1;
  // valores por defecto en el bloque
  memset(b, 0, BLOCK_SIZE);
  bwrite("disk.dat", sbloques[0].primerBloqueDatos + i, b);
  // devolver identificador del bloque
  return i;
  }
  }
  return -2;
}


int free2 ( int block_id )
{
  // comprobar validez de block_id
  if (block_id > sbloques[0].numBloquesDatos)
  return -1;
  // liberar bloque
  b_map[block_id] = 0;
  return 1;
}


int ifree ( int inodo_id )
{
  // comprobar validez de inodo_id
  if (inodo_id > sbloques[0].numInodos)
  return -1;
  // liberar i-nodo
  i_map[inodo_id] = 0;
  return 1;
}

int namei ( char *fname )
{
  // buscar i-nodo con nombre <fname>
  int i;
  for (i=0; i<sbloques[0].numInodos; i++)
  {
  if (! strcmp(inodos[i].nombre, fname))
  return i;
  }
  return -2;
}

int bmap ( int inodo_id, int offset )
{
  // comprobar validez de inodo_id
  if (inodo_id > sbloques[0].numInodos)
  return -1;
  // bloque de datos asociado
  if (offset < BLOCK_SIZE)
  return inodos[inodo_id].bloqueDirecto;
  return -1;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */

int createFile(char *fileName)
{
  
    int b_id, inodo_id ;
    
    
    inodo_id = ialloc() ;
    if (inodo_id == -2) {return inodo_id; }
    if (inodo_id < 0) { return -1 ; }
    
    b_id = alloc();
    if (b_id == -2) {return b_id; }
    if (b_id < 0) { return -1 ; }
    
    strcpy(inodos[inodo_id].nombre, fileName);
    inodos[inodo_id].bloqueDirecto = b_id ;
    inodos_x[inodo_id].posicion = 0;
    inodos_x[inodo_id].abierto = 1;
    
    return 0;
    
    //Se devuelve -2 en los ialloc y alloc si hay algún error
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{

    int inodo_id ;
    inodo_id = namei(fileName) ;
    
    if (inodo_id == -2) {return inodo_id; }
    if (inodo_id < 0)
    return -1 ;
    
    free2(inodos[inodo_id].bloqueDirecto);
    memset(&(inodos[inodo_id]),
    0,
    sizeof(TipoInodoDisco));
    ifree(inodo_id) ;
    return 0; 
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{

  int inodo_id ;
  
  // buscar el inodo asociado al nombre
  inodo_id = namei(fileName) ;
  
  if (inodo_id == -2) {return inodo_id; }
  if (inodo_id < 0)
  return -1 ;
  
  // Controlo que no esté ya abierto
  if (inodos_x[inodo_id].abierto == 1)
  return -2;
  
  // iniciar sesión de trabajo
  inodos_x[inodo_id].posicion = 0;
  inodos_x[inodo_id].abierto = 1;
  
  int j=checkFile(fileName);
  if(j!=0) return -2;
  
  return inodo_id;

}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
  // comprobar descriptor válido
  if ( (fileDescriptor < 0) || (fileDescriptor > sbloques[0].numInodos-1) )
  return -1 ;
  
  // cerrar sesión de trabajo
  inodos_x[fileDescriptor].posicion = 0;
  inodos_x[fileDescriptor].abierto = 0;
  return 0; 
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
  char b[BLOCK_SIZE] ;
  int b_id ;
  int r ;
  
  if (inodos_x[fileDescriptor].posicion+numBytes > inodos[fileDescriptor].tamanyo)
  numBytes = inodos[fileDescriptor].tamanyo - inodos_x[fileDescriptor].posicion;
  
  if (numBytes <= 0)
  return 0;
  
  b_id = bmap(fileDescriptor, inodos_x[fileDescriptor].posicion);
  
  r = bread("disk.dat", b_id, b);
  if(r == -1) {return r; }
  
  memmove(buffer, b+inodos_x[fileDescriptor].posicion, numBytes);
  inodos_x[fileDescriptor].posicion += numBytes;
  return numBytes; 

}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
  char b[BLOCK_SIZE] ;
  int b_id ;
  int r ;
  
  if (inodos_x[fileDescriptor].posicion+numBytes > BLOCK_SIZE)
  numBytes = BLOCK_SIZE - inodos_x[fileDescriptor].posicion;
  
  if (numBytes <= 0)
  return 0;
  
  b_id = bmap(fileDescriptor, inodos_x[fileDescriptor].posicion);
  
  r = bread("disk.dat", b_id, b);
  if(r == -1) {return r; }
  
  memmove(b+inodos_x[fileDescriptor].posicion, buffer, numBytes);
  r = bwrite("disk.dat", b_id, b);
  if(r == -1) {return r; }
  
  inodos_x[fileDescriptor].posicion += numBytes;
  return numBytes; 
  
}


/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, int whence, long offset)
{
  int p = i_map[fileDescriptor];
	
	if (whence == FS_SEEK_CUR)		
	{
		p = p+offset;				//move the pointer of the file for 'offset' places
		if ( (p>=0) && (p<=FILE_SIZE) )
			i_map[fileDescriptor] = p;
		else
			return -1;			//if it's not possible return -1
	}
	else if (whence == FS_SEEK_BEGIN)		//if whence is FS_SEEK_BEGIN, set pointer of the file to 0
		i_map[fileDescriptor] = 0;
	else if (whence == FS_SEEK_END)			//if it's FS_SEEK_END, set it to the end of the file
		i_map[fileDescriptor] = FILE_SIZE-1;
	else
		return -1;

  return 0;
}

/*
 * @brief 	Verifies the integrity of the file system metadata.
 * @return 	0 if the file system is correct, -1 if the file system is corrupted, -2 in case of error.
 */
int checkFS(void)
{
  int i;
  for (i=0; i<sbloques[0].numInodos; i++) {
      if (inodos_x[i].abierto == 1) {
      return -2;
        }
      }
  
  int redundancia=CRC32((unsigned char *)&(sbloques[0]), sbloques[0].tamDispositivo);  
  if(redundancia!=0) return -1;
  
  return redundancia;

}

/*
 * @brief 	Verifies the integrity of a file.
 * @return 	0 if the file is correct, -1 if the file is corrupted, -2 in case of error.
 */
int checkFile(char *fileName)
{
  
  int inodo_id=namei(fileName);
  if (inodo_id < 0) return -2;
  if (inodos_x[inodo_id].abierto == 1) return -2;

  int redundancia=CRC32((unsigned char *)&(inodos[inodo_id]), inodos[inodo_id].tamanyo);

  if(redundancia!=0) return -1;

	return redundancia;
}

