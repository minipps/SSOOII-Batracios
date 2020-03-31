#include "batracios.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#define MAX_PROCESOS (const int)25
#define MIN_VELOCIDAD (const int)0
#define MAX_VELOCIDAD (const int)1000
#define MAX_TICS_ANTES_DE_MUERTE (const int) 50
#define MIN_TMEDIO (const int)0
#define NUM_TRONCOS (const int)7
#define NUM_RANAS_MADRE (const int)4
#define MOVIMIENTO_TRONCOS (const int)1
#define SEMAFORO_BIBLIOTECA (const int)0
#define SEMAFORO_PROCESOS (const int)1
#define SEMAFORO_PRIMERMOVIMIENTO (const int)2
#define SEMAFORO_NACIDAS (const int)6
#define SEMAFORO_SALVADAS (const int)7
#define SEMAFORO_PERDIDAS (const int)8
#define SEMAFORO_SALTO_RANAS (const int)9
#define SEMAFORO_POSICIONES (const int)10
#define NUM_SEMAFOROS (const int)11
#define FILA_PRIMER_TRONCO (const int)4
#define FILA_ULTIMO_TRONCO (const int)10
#define WAIT (const int)-1
#define WAIT_ALL (const int)0
#define SIGNAL (const int)1
#define POSX_MAX (const int)79
#define POSX_MIN (const int)0

// Es necesario declarar la unión para que funcione en encina
union semun{
		int val;
		struct semid_ds* buf;
		unsigned short* array;
};
/* -------------------------------------------- PROTOTIPOS --------------------------------------------- */
void sigintHandlerPadre(int sig);
void sigintHandlerMadres(int sig);
void sigintHandlerRenacuajos(int sig);
void sigcldHandlerMadres(int sig);
void reservarIPC(void);
int limpiarRecursos(void);
int bucleRanasMadre(int i);
int bucleRanasHija(int*posX,int*posY,int i);
int operarSobreSemaforo(int semaforo,int indice,short op,short nsops,short flg);
void devolverPunterosContadores(int** ranasNacidas,int** ranasSalvadas,int** ranasPerdidas);
void devolverPunteroAPosicion(int** posX,int** posY);
void inicializarVectorMemoriaCompartida(int** vec, int tam, int pos);
int declararSemaforo(int semaforo,int indice,int op,int val);
/* ----------------------------------------------------------------------------------------------------- */

int semaforo = -1; // Iniciamos los IDs de los recursos IPC a -1, como se recomienda, para gestionar errores mejor
int idMemoria = -1;
int* memoriaCompartida = NULL;
int * arrayPID;


/* ------------------------------------ */
int main(int argc,char* argv[]) {
		/* ------------------------------------ */
		struct sigaction sigint,viejoSigint,sigintMadres,sigcldPadre,viejoSigcld;
		sigint.sa_handler = sigintHandlerPadre;
		sigint.sa_flags = 0;
		sigintMadres.sa_handler = sigintHandlerMadres;
		sigintMadres.sa_flags = 0;
		sigcldPadre.sa_handler = SIG_IGN;
		sigcldPadre.sa_flags = 0;
		int arg1,arg2,i,j,retornoFork = 0;
		int* ranasNacidas,* ranasSalvadas,* ranasMuertas,* vecPos;
		int vectorTroncos[7] = {6,7,8,9,10,11,12};
		int vectorAgua[7] = {7,6,5,4,3,2,1};
		int vectorDirs[7] = {DERECHA,IZQUIERDA,DERECHA,IZQUIERDA,DERECHA,IZQUIERDA,DERECHA};
		// PASO 3. CONTROLAR PASO DE ARGUMENTOS
		if(argc <= 2) {
				printf("USO: ./batracios <velocidad> <tiempo medio>.\n");
				return 5;
		}

		arg1 = atoi(argv[1]);
		arg2 = atoi(argv[2]);

		if(arg1 < MIN_VELOCIDAD || arg1 > MAX_VELOCIDAD) {
				printf("<velocidad> debe estar entre %d y %d.\n",MIN_VELOCIDAD,MAX_VELOCIDAD);
				return 6;
		} else if(arg2 <= MIN_TMEDIO) {
				printf("<tiempo medio> debe ser mayor que %d.\n",MIN_TMEDIO);
				return 7;
		}

		reservarIPC();
		inicializarVectorMemoriaCompartida(&vecPos, MAX_PROCESOS * 2, 516);
		inicializarVectorMemoriaCompartida(&arrayPID, MAX_PROCESOS*2, 600);
		devolverPunterosContadores(&ranasNacidas,&ranasSalvadas,&ranasMuertas);
		*ranasNacidas = 0;
		*ranasSalvadas = 0;
		*ranasMuertas = 0;
		// PASO 2. CAPTAR CTRL+C Y SIGCLD
		if(sigaction(SIGINT,&sigint,&viejoSigint) == -1) return 2;
		if(sigaction(SIGCLD,&sigcldPadre,&viejoSigcld) == -1) return 15;
		// PASO 4. Llamar a BATR_inicio y BATR_fin
		BATR_inicio(arg1,semaforo,vectorTroncos,vectorAgua,vectorDirs,arg2,(char*)memoriaCompartida);
		// PASO 7. CREAR PROCESOS MADRE
		for(i = 0; i < NUM_RANAS_MADRE; i++) {
				retornoFork = fork();
				switch(retornoFork) {
				case 0:
						if(sigaction(SIGINT,&sigintMadres,NULL) == -1) return 11;
						if(sigaction(SIGCLD,&viejoSigcld,NULL) == -1) return 16;
						operarSobreSemaforo(semaforo,SEMAFORO_PROCESOS,WAIT,1,0);
						bucleRanasMadre(i);
						break;
				default:
						if (retornoFork < 0) {
								perror("main: creación ranas madre");
						}
						arrayPID[i] = retornoFork;
						break;
				}
		}
		// PASO 5. MOVER TRONCOS
		while(1) {
				if(MOVIMIENTO_TRONCOS) {
						for(i = 0; i < 7; i++) {
								operarSobreSemaforo(semaforo,SEMAFORO_SALTO_RANAS,WAIT,1,0);
								BATR_avance_troncos(i);
								for(j = 1; j < MAX_PROCESOS * 2; j = j + 2) {
										if (vecPos[j] == (6 - i) + FILA_PRIMER_TRONCO) {
												if (vectorDirs[6 - i] == DERECHA) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       //El vector empieza por la fila de arriba, nosotros por la de abajo
														//fprintf(stderr,"Moviendo rana a la derecha, de %d, %d",vecPos[j - 1],vecPos[j]);
														vecPos[j - 1] = vecPos[j - 1] + 1;
														//fprintf(stderr," a %d %d\n",						   vecPos[j - 1],vecPos[j]);
												} else {
														//fprintf(stderr,"Moviendo rana a la izquierda, de %d, %d",vecPos[j - 1],vecPos[j]);
														vecPos[j - 1] = vecPos[j - 1] - 1;
														//fprintf(stderr," a %d %d\n",							 vecPos[j - 1],vecPos[j]);
												}
										}
								}
								operarSobreSemaforo(semaforo,SEMAFORO_SALTO_RANAS,SIGNAL,1,0);
								BATR_pausita();
						}
				}
		}
		BATR_fin();
		limpiarRecursos();
		return 0;
}

/* ----------------------------------- */
void sigintHandlerPadre(int sig) {
		/* ----------------------------------- */
		int i,ret;
		int* ranasNacidas,* ranasSalvadas,* ranasMuertas;
		for (i = 0; i < MAX_PROCESOS; i++) {
				if (arrayPID[i] > 0) {
						kill(arrayPID[i],SIGINT);
				}
		}
		for (i = 0; i < MAX_PROCESOS; i++) {
				if (arrayPID[i] > 0) {
						waitpid(arrayPID[i],NULL,0);
				}
		}
		if ((semctl(semaforo,SEMAFORO_BIBLIOTECA,GETVAL) <= 0)) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 //Si el semáforo de la biblioteca queda bloqueado
				operarSobreSemaforo(semaforo,SEMAFORO_BIBLIOTECA,SIGNAL,1,0);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 //hacemos un signal para que el programa no se quede pillado.
		}
		devolverPunterosContadores(&ranasNacidas,&ranasSalvadas,&ranasMuertas);
		BATR_comprobar_estadIsticas(*ranasNacidas,*ranasSalvadas,*ranasMuertas);
		BATR_fin();
		ret = limpiarRecursos();
		if (ret >= 0) {
				exit(0);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 // mejor para trabajar con señales
		} else {
				exit(ret);
		}
}

/* ---------------------- */
void reservarIPC(){
		/* ---------------------- */
		// PASO 1. CREAR SEMÁFORO Y MEMORIA COMPARTIDA
		semaforo = semget(IPC_PRIVATE,NUM_SEMAFOROS,IPC_CREAT | 0666);
		if (semaforo < 0) {
			perror("reservarIPC: semget");
			exit(22);
		}
		if (declararSemaforo(semaforo,SEMAFORO_PROCESOS,SETVAL,MAX_PROCESOS - 1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_PROCESOS");
				exit(9);
		}
		if (declararSemaforo(semaforo,SEMAFORO_PRIMERMOVIMIENTO,SETVAL,1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_PRIMERMOVIMIENTO");
				exit(12);
		}
		if (declararSemaforo(semaforo,SEMAFORO_PRIMERMOVIMIENTO + 1,SETVAL,1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_PRIMERMOVIMIENTO");
				exit(13);
		}
		if (declararSemaforo(semaforo,SEMAFORO_PRIMERMOVIMIENTO + 2,SETVAL,1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_PRIMERMOVIMIENTO");
				exit(15);
		}
		if (declararSemaforo(semaforo,SEMAFORO_PRIMERMOVIMIENTO + 3,SETVAL,1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_PRIMERMOVIMIENTO");
				exit(16);
		}
		if (declararSemaforo(semaforo,SEMAFORO_NACIDAS,SETVAL,1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_NACIDAS");
				exit(17);
		}
		if (declararSemaforo(semaforo,SEMAFORO_SALVADAS,SETVAL,1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_SALVADAS");
				exit(18);
		}
		if (declararSemaforo(semaforo,SEMAFORO_PERDIDAS,SETVAL,1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_PERDIDAS");
				exit(19);
		}
		if (declararSemaforo(semaforo,SEMAFORO_SALTO_RANAS,SETVAL,1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_SALTO_RANAS");
				exit(20);
		}
		if (declararSemaforo(semaforo,SEMAFORO_POSICIONES,SETVAL,1) < 0) {
				perror("reservarIPC: declararSemaforo SEMAFORO_POSICIONES");
				exit(21);
		}
		// MEMORIA COMPARTIDA
		idMemoria = shmget(IPC_PRIVATE,4096,IPC_CREAT | 0666);
		if (idMemoria < 0) {
				perror("reservarIPC: shmget");
				exit(1);
		}
		memoriaCompartida = shmat(idMemoria,0,0);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 // el sistema decide dónde guardar la memoria compartida // no queremos flags especiales
		if (memoriaCompartida == NULL) {
				perror("reservarIPC: shmat");
				exit(9);
		}
}
/* ---------------------------- */
int limpiarRecursos(){
		/* ---------------------------- */
		sigset_t mascara;
		sigfillset(&mascara);
		sigprocmask(SIG_SETMASK,&mascara,NULL);
		if(semaforo > 0) {
				if(semctl(semaforo,0,IPC_RMID) < 0) {
						perror("limpiarRecursos: declararSemaforo");
						return(-3);
				}
		}
		if(shmdt((void*)memoriaCompartida) < 0) {
				perror("limpiarRecursos: shmdt");
				return(-16);
		}
		if(shmctl(idMemoria,IPC_RMID,NULL) < 0) {
				perror("limpiarRecursos: shmctl");
				return(-4);
		}
		sigprocmask(SIG_UNBLOCK,&mascara,NULL);
		return 0;
}

/* ------------------------------ */
int bucleRanasMadre(int i){
		/* ------------------------------ */
		int retorno,posX,posY;
		int* ranasNacidas;
		sigset_t mascara;
		struct sigaction sigintRenacuajos,sigcldMadres;
		sigintRenacuajos.sa_handler = sigintHandlerRenacuajos;
		sigintRenacuajos.sa_flags = 0;
		sigcldMadres.sa_handler = SIG_IGN;
		sigcldMadres.sa_flags = 0;
		if (sigaction(SIGCLD,&sigcldMadres,NULL) == -1) {
				perror("sigaction madres");
		}
		sigfillset(&mascara);
		devolverPunterosContadores(&ranasNacidas,NULL,NULL);
		int j = 0;
		while(1) {
				operarSobreSemaforo(semaforo,SEMAFORO_PROCESOS,			   WAIT,1,0);
				operarSobreSemaforo(semaforo,SEMAFORO_PRIMERMOVIMIENTO + i,WAIT,1,0);
				BATR_descansar_criar();
				BATR_parto_ranas(i,&posX,&posY);
				retorno = fork();
				if (retorno < 0) {
						perror("bucleRanasMadre: fork");
						return 20;
				}
				switch(retorno) {
				case 0:
						if(sigaction(SIGINT,&sigintRenacuajos,NULL) == -1) return 11;
						bucleRanasHija(&posX,&posY,i);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       // falta guardar el valor que devuelve
						break;
				default:
						for (j = 0; j < MAX_PROCESOS*2; j++) {
							if (arrayPID[j] < 0) {
								arrayPID[j] = retorno;
								break;
							}
						}
						sigprocmask(SIG_SETMASK,&mascara,NULL);
						operarSobreSemaforo(semaforo,SEMAFORO_NACIDAS,WAIT,	 1,0);
						*ranasNacidas += 1;
						operarSobreSemaforo(semaforo,SEMAFORO_NACIDAS,SIGNAL,1,0);
						sigprocmask(SIG_UNBLOCK,&mascara,NULL);
						break;
				}
		}
}

/* ------------------------------------------ */
int bucleRanasHija(int*dx,int*dy,int i){
		/* ------------------------------------------ */
		int*ranasSalvadas,*ranasPerdidas;
		int contadorBloqueo = 0;
		int* posX,* posY;
		sigset_t mascara;
		sigfillset(&mascara);
		devolverPunterosContadores(NULL,&ranasSalvadas,&ranasPerdidas);
		devolverPunteroAPosicion(&posX,&posY);
		*posX = *dx;
		*posY = *dy;
		const int DIRECCIONES[] = {IZQUIERDA,DERECHA};
		while(1) {
				//Explotamos a las ranas si salen fuera del tablero o se quedan atascadas en la versión sin movimiento de troncos.
				operarSobreSemaforo(semaforo,SEMAFORO_SALTO_RANAS,WAIT,1,0);
				if (*posX > POSX_MAX || *posX < POSX_MIN) {
					operarSobreSemaforo(semaforo,SEMAFORO_SALTO_RANAS,SIGNAL,1,0);
					sigprocmask(SIG_SETMASK,&mascara,NULL);
					operarSobreSemaforo(semaforo,SEMAFORO_PERDIDAS,WAIT,  1,0);
					*ranasPerdidas += 1;
					operarSobreSemaforo(semaforo,SEMAFORO_PERDIDAS,SIGNAL,1,0);
					sigprocmask(SIG_UNBLOCK,&mascara,NULL);
					*posX = -1;
					*posY = -1;
					raise(SIGINT);
				}
				if (((contadorBloqueo > MAX_TICS_ANTES_DE_MUERTE) && !MOVIMIENTO_TRONCOS)) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  // si pasan MAX_TICS_ANTES_DE_MUERTE tics y una rana está no ha avanzado fila, explota
						BATR_explotar(*posX,*posY);
						operarSobreSemaforo(semaforo,SEMAFORO_SALTO_RANAS,SIGNAL,1,0);
						sigprocmask(SIG_SETMASK,&mascara,NULL);
						operarSobreSemaforo(semaforo,SEMAFORO_PERDIDAS,WAIT,  1,0);
						*ranasPerdidas += 1;
						operarSobreSemaforo(semaforo,SEMAFORO_PERDIDAS,SIGNAL,1,0);
						sigprocmask(SIG_UNBLOCK,&mascara,NULL);
						*posX = -1;
						*posY = -1;
						raise(SIGINT);
				}
				int estaEnPrimeraCasilla = (*posY == 1) ? 1 : 0;
				int rnd = rand() % 2;
				//fprintf(stderr,"%d: X %d | Y %d (pre-salto)\n",getpid(),*posX,*posY);
				if(!BATR_puedo_saltar(*posX,*posY,ARRIBA)) {
						BATR_avance_rana_ini(*posX,*posY);
						BATR_avance_rana(posX,posY,ARRIBA);
						BATR_pausa();
						BATR_avance_rana_fin(*posX,*posY);
						//fprintf(stderr,"%d: X %d | Y %d (post-salto)\n",getpid(),*posX,*posY);
						operarSobreSemaforo(semaforo,SEMAFORO_SALTO_RANAS,SIGNAL,1,0);
						contadorBloqueo = 0;
						if (estaEnPrimeraCasilla) {
								operarSobreSemaforo(semaforo,SEMAFORO_PRIMERMOVIMIENTO + i,SIGNAL,1,0);
						}
				} else if (!BATR_puedo_saltar(*posX,*posY,DIRECCIONES[rnd])) {
						BATR_avance_rana_ini(*posX,*posY);
						BATR_avance_rana(posX,posY,DIRECCIONES[rnd]);
						BATR_pausa();
						BATR_avance_rana_fin(*posX,*posY);
						contadorBloqueo++;
						//fprintf(stderr,"%d: X %d | Y %d (post-salto)\n",getpid(),*posX,*posY);
						operarSobreSemaforo(semaforo,SEMAFORO_SALTO_RANAS,SIGNAL,1,0);
						if (estaEnPrimeraCasilla) {
								operarSobreSemaforo(semaforo,SEMAFORO_PRIMERMOVIMIENTO + i,SIGNAL,1,0);
						}
				} else if (!BATR_puedo_saltar(*posX,*posY,DIRECCIONES[1 - rnd])) {
						BATR_avance_rana_ini(*posX,*posY);
						BATR_avance_rana(posX,posY,DIRECCIONES[1 - rnd]);
						BATR_pausa();
						BATR_avance_rana_fin(*posX,*posY);
						contadorBloqueo++;
						//fprintf(stderr,"%d: X %d | Y %d (post-salto)\n",getpid(),*posX,*posY);
						operarSobreSemaforo(semaforo,SEMAFORO_SALTO_RANAS,SIGNAL,1,0);
						if (estaEnPrimeraCasilla) {
								operarSobreSemaforo(semaforo,SEMAFORO_PRIMERMOVIMIENTO + i,SIGNAL,1,0);
						}
				} else {
						operarSobreSemaforo(semaforo,SEMAFORO_SALTO_RANAS,SIGNAL,1,0);
						contadorBloqueo++;
						BATR_pausita();
				}
				if (*posY > FILA_ULTIMO_TRONCO) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 //Rana salvada
						operarSobreSemaforo(semaforo,SEMAFORO_SALVADAS,WAIT,  1,0);
						*ranasSalvadas += 1;
						operarSobreSemaforo(semaforo,SEMAFORO_SALVADAS,SIGNAL,1,0);
						*posX = -1;
						*posY = -1;
						raise(SIGINT);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 // aquí hace el signal ya
				}
		}
}

/* ----------------------------------------------------------------------------------------- */
int operarSobreSemaforo(int semaforo,int indice,short op,short nsops,short flg) {
		/* ----------------------------------------------------------------------------------------- */
		struct sembuf sop = {indice,op,flg};
		return semop(semaforo,&sop,nsops);
}

/* ------------------------------------- */
void sigintHandlerMadres(int sig) {
		/* ------------------------------------- */
		sigset_t mascara;
		int i;
		sigfillset(&mascara);
		sigprocmask(SIG_SETMASK,&mascara,NULL);
		for (i = 0; i < MAX_PROCESOS; i++) {
				if (arrayPID[i] > 0) {
						kill(arrayPID[i],SIGINT);
				}
		}
		if (operarSobreSemaforo(semaforo,SEMAFORO_PROCESOS,SIGNAL,1,0) < 0) {
				perror("sigintHandlerMadres: operacion");
				_exit(20);
		}
		sigprocmask(SIG_UNBLOCK,&mascara,NULL);
		_exit(0);
}

/* ---------------------------------------- */
void sigintHandlerRenacuajos(int sig) {
		/* ---------------------------------------- */
		sigset_t mascara;
		int i;
		sigfillset(&mascara);
		sigprocmask(SIG_SETMASK,&mascara,NULL);
		if (operarSobreSemaforo(semaforo,SEMAFORO_PROCESOS,SIGNAL,1,0) < 0) {
				perror("sigintHandlerRenacuajos: operacion");
				_exit(20);
		}
		for (i = 0; i < MAX_PROCESOS*2; i++) {
			if (arrayPID[i] == getpid()) {
				arrayPID[i] = -1;
			}
		}
		sigprocmask(SIG_UNBLOCK,&mascara,NULL);
		_exit(0);
}

/* ------------------------------------------------------------------------------------------------------ */
void devolverPunterosContadores(int** ranasNacidas,int** ranasSalvadas,int** ranasPerdidas) {
		/* ------------------------------------------------------------------------------------------------------ */
		if (memoriaCompartida != NULL) {
				if (ranasNacidas != NULL) {
						*ranasNacidas = &(memoriaCompartida[513]);
				}
				if (ranasSalvadas != NULL) {
						*ranasSalvadas = &(memoriaCompartida[514]);
				}
				if (ranasPerdidas != NULL) {
						*ranasPerdidas = &(memoriaCompartida[515]);
				}
		}
}

/* --------------------------------------------------- */
void inicializarVectorMemoriaCompartida(int** vec, int tam, int pos) {
/* --------------------------------------------------- */
		int* temp = NULL;
		int i;
		if (memoriaCompartida != NULL) {
				temp = &(memoriaCompartida[pos]);
				for (i = 0; i < tam; i++) {
						temp[i] = -1;
				}
		}
		*vec = temp;
}

/* ------------------------------------------------------------ */
void devolverPunteroAPosicion(int** posX,int** posY) {
/* ------------------------------------------------------------ */
		int* posiciones;
		int i;
		if (posX == NULL || posY == NULL) return;
		if (memoriaCompartida != NULL) {
				posiciones = &(memoriaCompartida[516]);
				for (i = 0; i < MAX_PROCESOS * 2; i = i + 2) {
						if ((posiciones[i] < 0) && (posiciones[i + 1] < 0)) {
								*posX = &(posiciones[i]);
								*posY = &(posiciones[i + 1]);
						}
				}
		}
}

void sigcldHandlerMadres(int sig) {
		pid_t pid;
		int status;
		while((pid = waitpid(-1,&status,WNOHANG)) > 0);
}

int declararSemaforo(int semaforo,int indice,int op,int val) {
	union semun {
					int val;
					struct semid_ds *buf;
					unsigned short  *array;
	  } temp;
		temp.val = val;
		return semctl(semaforo,indice,op,temp);
}
