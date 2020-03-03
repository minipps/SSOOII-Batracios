#include "batracios.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/stat.h> // tuberías
#include <sys/msg.h> // buzones
#include <sys/shm.h> // memoria compartida
#include <sys/types.h>
#include <unistd.h>

#define MAX_SEMAFOROS 32

/* ----------- PROTOTIPOS -------------- */
	void sigintHandler(int sig);
	void limpiarRecursos(void);
/* ------------------------------------- */

//SEMÁFOROS
int semaforos[MAX_SEMAFOROS] = {-1};
// BUZÓN
// MEMORIA COMPARTIDA
int idMemoria;


/* ------------------------------------ */
	int main(int argc, char * argv) {
/* ------------------------------------ */
	struct sembuf sops[MAX_SEMAFOROS];
	struct sigaction sigint;
	sigint.sa_handler = sigintHandler;

// PASO 1. CREAR SEMÁFORO Y MEMORIA COMPARTIDA
	// SEMÁFORO
	semaforos[0] = semget(IPC_PRIVATE,MAX_SEMAFOROS,IPC_CREAT | 0600);

	// MEMORIA COMPARTIDA
	idMemoria = shmget(IPC_PRIVATE,4096,IPC_CREAT | 0600);
	if(idMemoria < 0){
		perror("main: shmget");
		return 1;
	}
	
	// AQUÍ VA EL POSIBLE BUZÓN
	//
	//
	//

// PASO 2. CAPTAR CTRL+C Y ELIMINAR IPCS
	if(sigaction(SIGINT,&sigint,NULL)==-1)
		return 2;

	while(1);
	limpiarRecursos();
	return 0;
}

/* ----------------------------------- */
	void sigintHandler(int sig){
/* ----------------------------------- */
	limpiarRecursos();
	_exit(0); // mejor para trabajar con señales
}

/* ---------------------------- */
	void limpiarRecursos(){
/* ---------------------------- */
	int i;
	struct shmid_ds shmid;

	for(i=0; i<MAX_SEMAFOROS; i++){
		if(semaforos[i] > 0){
			if(semctl(semaforos[i], 0, IPC_RMID) < 0){
				perror("limpiarRecursos: semctl");
				_exit(3);
			}
		}
	}

	if(shmctl(idMemoria,IPC_RMID, &shmid) < 0){
		perror("limpiarRecursos: shmctl");
		_exit(4);
	}
}
