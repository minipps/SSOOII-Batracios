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

#define MAX_SEMAFOROS (const int)25
#define MIN_VELOCIDAD (const int)0
#define MAX_VELOCIDAD (const int)1000
#define MIN_TMEDIO (const int)0
#define MOVIMIENTO_TRONCOS (const int)0


/* ----------- PROTOTIPOS -------------- */
	void sigintHandler(int sig);
	void limpiarRecursos(void);
	int bucleRanasMadre(int i);
/* ------------------------------------- */

//SEMÁFOROS
int semaforo = -1; // Iniciamos los IDs de los recursos IPC a -1, como se recomienda, para gestionar errores mejor
// BUZÓN
// MEMORIA COMPARTIDA
int idMemoria = -1;


/* ------------------------------------ */
	int main(int argc, char * argv[]) {
/* ------------------------------------ */
	struct sembuf sops[MAX_SEMAFOROS];
	struct sigaction sigint, viejoSigint;
	sigint.sa_handler = sigintHandler;
	int arg1, arg2;
	int i, retornoFork = 0;
	int vectorTroncos[7] = {3,4,5,6,7,8,9};
    int vectorAgua[7] = {7, 6, 5, 4, 3, 2, 1};
	int vectorDirs[7] = {DERECHA, IZQUIERDA, DERECHA, IZQUIERDA, DERECHA, IZQUIERDA, DERECHA};
	char * memoriaCompartida = NULL;
// PASO 3. CONTROLAR PASO DE ARGUMENTOS
	if(argc <= 2){
		printf("USO: ./batracios <velocidad> <tiempo medio>.\n");
		return 5;
	}

	arg1 = atoi(argv[1]);
	arg2 = atoi(argv[2]);

	if(arg1 < MIN_VELOCIDAD || arg1 > MAX_VELOCIDAD) {
		printf("<velocidad> debe estar entre %d y %d.\n", MIN_VELOCIDAD, MAX_VELOCIDAD);
		return 6;
	} else if(arg2 <= MIN_TMEDIO) {
		printf("<tiempo medio> debe ser mayor que %d.\n", MIN_TMEDIO);
		return 7;
	}

// PASO 1. CREAR SEMÁFORO Y MEMORIA COMPARTIDA
	// SEMÁFORO
	semaforo = semget(IPC_PRIVATE, MAX_SEMAFOROS, IPC_CREAT | 0600);
	if (semaforo < 0) {
		perror("main: semget");
		return 8;
	}
	// MEMORIA COMPARTIDA
	idMemoria = shmget(IPC_PRIVATE,4096,IPC_CREAT | 0600);
	if (idMemoria < 0) {
		perror("main: shmget");
		return 1;
	}
	memoriaCompartida = shmat(idMemoria, 0, 0); // el sistema decide dónde guardar la memoria compartida // no queremos flags especiales
	if (memoriaCompartida == NULL) {
		perror("main: shmat");
		return 9;
	}
	/* 
	/ AQUÍ VA EL POSIBLE BUZÓN
	*/

// PASO 2. CAPTAR CTRL+C Y ELIMINAR IPCS
	if(sigaction(SIGINT,&sigint,&viejoSigint)==-1) return 2;
		
// PASO 4. Llamar a BATR_inicio y BATR_fin
	BATR_inicio(arg1, semaforo, vectorTroncos, vectorAgua, vectorDirs, arg2, memoriaCompartida);
	// PASO 7. Crear procesos de las ranas madre
	for (i = 0; i < 3; i++) {
		switch(retornoFork) {
			case 0:
				retornoFork = fork();
				if (retornoFork < 0) {
					perror("main, creación ranas madre: fork");
					return 10;
				}
				break;
			default:
				if(sigaction(SIGINT, &viejoSigint, NULL)==-1) return 11;
				bucleRanasMadre(i);
				break;
		}
 	}
	// PASO 5/6. Mover troncos
	if (MOVIMIENTO_TRONCOS) {
		while(1) {
			for(i = 0; i < 7; i++) {
				BATR_avance_troncos(i);
				BATR_pausita();
			}
		}
	}
	while(1);
	BATR_fin();
	limpiarRecursos();
	return 0;
}



/* ----------------------------------- */
	void sigintHandler(int sig) {
/* ----------------------------------- */
	limpiarRecursos();
	_exit(0); // mejor para trabajar con señales
}

/* -------------------------------- */
	void limpiarRecursos(){ 
/* -------------------------------- */
	int i;
	struct shmid_ds shmid;

	if(semaforo > 0){
		if(semctl(semaforo, 0, IPC_RMID) < 0){
			perror("limpiarRecursos: semctl");
			_exit(3);
		}
	}

	if(shmctl(idMemoria,IPC_RMID, &shmid) < 0){
		perror("limpiarRecursos: shmctl");
		_exit(4);
	}
}

int bucleRanasMadre(int i) {
	int dx, dy;
	while(1) {
		BATR_descansar_criar();
		BATR_parto_ranas(i, &dx, &dy);
	}
}