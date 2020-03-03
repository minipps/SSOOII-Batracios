#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include "batracios.h"
#define MAX_SEMAFOROS 32
int semaforos[MAX_SEMAFOROS] = {-1}; //Quizás no sea necesario un array, mirar semget.
int idMemoria = -1;

void sigintHandler(int sig);
void limpiarRecursos(void);

int main(int argc, char * argv) {
	idMemoria = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0600);
	if (idMemoria < 0) {
		perror("main: shmget");
		return 1;
	}
	while(1);
	limpiarRecursos();
	return 0;
}

void sigintHandler(int sig) {
	limpiarRecursos();
	_exit(0);
}

void limpiarRecursos(void) {
	int i;
	struct shmid_ds shmid;
	for (i = 0; i < MAX_SEMAFOROS; i++) {
		if (semaforos[i] > 0) {
			if (semctl(semaforos[i], 0, IPC_RMID) < 0) {
				perror("limpiarRecursos: semctl");
				_exit(2);
			}
		}
	}
	if (shmctl(idMemoria, IPC_RMID, &shmid) < 0) {
		perror("limpiarMemoria: shmctl");
		_exit(3);	
	}
}