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
#define MOVIMIENTO_TRONCOS (const int)0
#define SEMAFORO_BIBLIOTECA (const int)0
#define SEMAFORO_PROCESOS (const int)1
#define SEMAFORO_PRIMERMOVIMIENTO (const int)2
#define SEMAFORO_NACIDAS (const int)6
#define SEMAFORO_SALVADAS (const int)7
#define SEMAFORO_PERDIDAS (const int)8
#define WAIT (const int)-1
#define WAIT_ALL (const int)0
#define SIGNAL (const int)1

// Es necesario declarar la unión para que funcione en encina
union semun{
    int val;
    struct semid_ds * buf;
    unsigned short * array;
};
/* -------------- PROTOTIPOS ----------------- */
	void sigintHandlerPadre(int sig);
	void sigintHandlerMadres(int sig);
	void sigintHandlerRenacuajos(int sig);
	void reservarIPC(void);
	void limpiarRecursos(void);
	int bucleRanasMadre(int i);
	int bucleRanasHija(int *dx, int *dy, int i);
	int operarSobreSemaforo(int semaforo, int indice, short op, short nsops, short flg);
	void devolverPunterosContadores(int ** ranasNacidas, int ** ranasSalvadas, int ** ranasPerdidas);
/* ------------------------------------------- */

int semaforo = -1; // Iniciamos los IDs de los recursos IPC a -1, como se recomienda, para gestionar errores mejor
int idMemoria = -1;
int * memoriaCompartida = NULL;
pid_t arrayPID[25] = {-1};

/* ------------------------------------ */
	int main(int argc, char * argv[]) {
/* ------------------------------------ */
	struct sembuf sops[MAX_PROCESOS];
	struct sigaction sigint, viejoSigint, sigintMadres, sigcldPadre, viejoSigcld;
	sigint.sa_handler = sigintHandlerPadre;
	sigint.sa_flags = 0;
	sigintMadres.sa_handler = sigintHandlerMadres;
	sigintMadres.sa_flags = 0;
	sigcldPadre.sa_handler = SIG_IGN;
	sigcldPadre.sa_flags = 0;
	int arg1, arg2, i, retornoFork = 0;
	int * ranasNacidas, * ranasSalvadas, * ranasMuertas;
	int vectorTroncos[7] = {6,7,8,9,10,11,12};
  int vectorAgua[7] = {7, 6, 5, 4, 3, 2, 1};
	int vectorDirs[7] = {DERECHA, IZQUIERDA, DERECHA, IZQUIERDA, DERECHA, IZQUIERDA, DERECHA};
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

	reservarIPC();

	devolverPunterosContadores(&ranasNacidas, &ranasSalvadas, &ranasMuertas);
	*ranasNacidas = 0;
	*ranasSalvadas = 0;
	*ranasMuertas = 0;
	// PASO 2. CAPTAR CTRL+C Y SIGCLD
	if(sigaction(SIGINT,&sigint,&viejoSigint)==-1) return 2;
	if(sigaction(SIGCLD,&sigcldPadre, &viejoSigcld)==-1) return 15;
	// PASO 4. Llamar a BATR_inicio y BATR_fin
	BATR_inicio(arg1, semaforo, vectorTroncos, vectorAgua, vectorDirs, arg2, (char *)memoriaCompartida);
	// PASO 7. CREAR PROCESOS MADRE
	for(i=0; i<NUM_RANAS_MADRE; i++){
		retornoFork = fork();
		switch(retornoFork){
			case 0:
				if(sigaction(SIGINT,&sigintMadres, NULL)==-1) return 11;
				if(sigaction(SIGCLD,&viejoSigcld, NULL)==-1) return 16;
				operarSobreSemaforo(semaforo, SEMAFORO_PROCESOS, WAIT, 1, 0);
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
	while(1){
		if(MOVIMIENTO_TRONCOS){
			for(i=0; i<NUM_TRONCOS; i++){
				BATR_avance_troncos(i);
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
	int i;
	int * ranasNacidas, * ranasSalvadas, * ranasMuertas;
	for (i = 0; i < MAX_PROCESOS; i++) {
		if (arrayPID[i] > 0) {
			kill(arrayPID[i], SIGINT);
		}
	}
	for (i = 0; i < MAX_PROCESOS; i++) {
		if (arrayPID[i] > 0) {
			waitpid(arrayPID[i], NULL, 0);
		}
	}
	if ((operarSobreSemaforo(semaforo, SEMAFORO_BIBLIOTECA, WAIT, 1, IPC_NOWAIT | SEM_UNDO) == -1) && (errno == EAGAIN)) { //Si el semáforo de la biblioteca queda bloqueado
		operarSobreSemaforo(semaforo, SEMAFORO_BIBLIOTECA, SIGNAL, 1, 0); //hacemos un signal para que el programa no se quede pillado.
	}
	BATR_fin();
	devolverPunterosContadores(&ranasNacidas, &ranasSalvadas, &ranasMuertas);
	BATR_comprobar_estadIsticas(*ranasNacidas, *ranasSalvadas, *ranasMuertas);
	limpiarRecursos();
	_exit(0); // mejor para trabajar con señales
}

/* ---------------------- */
	void reservarIPC(){
/* ---------------------- */
	// PASO 1. CREAR SEMÁFORO Y MEMORIA COMPARTIDA
	semaforo = semget(IPC_PRIVATE, MAX_PROCESOS, IPC_CREAT | 0600);
	if (semctl(semaforo, SEMAFORO_PROCESOS, SETVAL, MAX_PROCESOS-1) < 0) {
		perror("main: semctl SEMAFORO_PROCESOS");
		_exit(9);
	}
	if (semctl(semaforo, SEMAFORO_PRIMERMOVIMIENTO, SETVAL, 1) < 0) {
		perror("main: semctl SEMAFORO_PRIMERMOVIMIENTO");
		_exit(12);
	}
	if (semctl(semaforo, SEMAFORO_PRIMERMOVIMIENTO+1, SETVAL, 1) < 0) {
		perror("main: semctl SEMAFORO_PRIMERMOVIMIENTO");
		_exit(13);
	}
	if (semctl(semaforo, SEMAFORO_PRIMERMOVIMIENTO+2, SETVAL, 1) < 0) {
		perror("main: semctl SEMAFORO_PRIMERMOVIMIENTO");
		_exit(15);
	}
	if (semctl(semaforo, SEMAFORO_PRIMERMOVIMIENTO+3, SETVAL, 1) < 0) {
		perror("main: semctl SEMAFORO_PRIMERMOVIMIENTO");
		_exit(16);
	}
	if (semctl(semaforo, SEMAFORO_NACIDAS, SETVAL, 1) < 0) {
		perror("main: semctl SEMAFORO_NACIDAS");
		_exit(17);
	}
	if (semctl(semaforo, SEMAFORO_SALVADAS, SETVAL, 1) < 0) {
		perror("main: semctl SEMAFORO_SALVADAS");
		_exit(18);
	}
	if (semctl(semaforo, SEMAFORO_PERDIDAS, SETVAL, 1) < 0) {
		perror("main: semctl SEMAFORO_PERDIDAS");
		_exit(19);
	}
	if (semaforo < 0) {
		perror("main: semget");
		_exit(8);
	}
	// MEMORIA COMPARTIDA
	idMemoria = shmget(IPC_PRIVATE,4096,IPC_CREAT | 0600);
	if (idMemoria < 0) {
		perror("main: shmget");
		_exit(1);
	}
	memoriaCompartida = shmat(idMemoria, 0, 0); // el sistema decide dónde guardar la memoria compartida // no queremos flags especiales
	if (memoriaCompartida == NULL) {
		perror("main: shmat");
		_exit(9);
	}
}
/* ---------------------------- */
	void limpiarRecursos(){
/* ---------------------------- */
	int i;
	struct shmid_ds shmid;
	if(semaforo > 0){
		if(semctl(semaforo, 0, IPC_RMID) < 0){
			perror("limpiarRecursos: semctl");
			_exit(3);
		}
	}
	if(shmdt(memoriaCompartida) < 0) {
		perror("limpiarRecursos: shmdt");
		_exit(16);
	}
	if(shmctl(idMemoria,IPC_RMID, &shmid) < 0){
		perror("limpiarRecursos: shmctl");
		_exit(4);
	}
}

/* ------------------------------ */
	int bucleRanasMadre(int i){
/* ------------------------------ */
	int retorno, dx, dy;
	int * ranasNacidas;
	sigset_t mascara;
	struct sigaction sigintRenacuajos;
	sigintRenacuajos.sa_handler = sigintHandlerRenacuajos;
	sigintRenacuajos.sa_flags = 0;
	sigfillset(&mascara);
	devolverPunterosContadores(&ranasNacidas, NULL, NULL);
	int j = 0;
	while(1){
		operarSobreSemaforo(semaforo, SEMAFORO_PROCESOS, WAIT, 1, 0);
		operarSobreSemaforo(semaforo, SEMAFORO_PRIMERMOVIMIENTO+i, WAIT, 1, 0);
		BATR_descansar_criar();
		BATR_parto_ranas(i, &dx, &dy);
		retorno = fork();
		if (retorno < 0) {
			perror("bucleRanasMadre: fork");
			return 20;
		}
		switch(retorno){
			case 0:
				if(sigaction(SIGINT,&sigintRenacuajos, NULL)==-1) return 11;
				bucleRanasHija(&dx,&dy, i); // falta guardar el valor que devuelve
				break;
			default:
				arrayPID[j] = retorno;
				j++;
				sigprocmask(SIG_SETMASK, &mascara, NULL);
				operarSobreSemaforo(semaforo, SEMAFORO_NACIDAS, WAIT, 1, 0);
				*ranasNacidas += 1;
				operarSobreSemaforo(semaforo, SEMAFORO_NACIDAS, SIGNAL, 1, 0);
				sigprocmask(SIG_UNBLOCK, &mascara, NULL);
				break;
		}
	}
}

/* ------------------------------------------ */
	int bucleRanasHija(int *dx, int *dy, int i){
/* ------------------------------------------ */
	int *ranasSalvadas, *ranasPerdidas;
	int contadorBloqueo = 0;
	sigset_t mascara;
	sigfillset(&mascara);
	devolverPunterosContadores(NULL, &ranasSalvadas, &ranasPerdidas);
	const int DIRECCIONES[] = {IZQUIERDA, DERECHA};
	while(1){
		if ((contadorBloqueo > MAX_TICS_ANTES_DE_MUERTE) && MOVIMIENTO_TRONCOS){ // si pasan MAX_TICS_ANTES_DE_MUERTE tics y una rana está no ha avanzado fila, explota
			BATR_explotar(*dx, *dy);
			sigprocmask(SIG_SETMASK, &mascara, NULL);
			operarSobreSemaforo(semaforo, SEMAFORO_PERDIDAS, WAIT, 1, 0);
			*ranasPerdidas += 1;
			operarSobreSemaforo(semaforo, SEMAFORO_PERDIDAS, SIGNAL, 1, 0);
			sigprocmask(SIG_UNBLOCK, &mascara, NULL);
			raise(SIGINT);
		}
		int estaEnPrimeraCasilla = (*dy == 1)? 1 : 0;
		int rnd = rand() % 2;
		if(!BATR_puedo_saltar(*dx,*dy, ARRIBA)){
			BATR_avance_rana_ini(*dx, *dy);
			BATR_avance_rana(dx, dy, ARRIBA);
			BATR_pausa();
			BATR_avance_rana_fin(*dx, *dy);
			contadorBloqueo = 0;
			if (estaEnPrimeraCasilla) {
				operarSobreSemaforo(semaforo, SEMAFORO_PRIMERMOVIMIENTO+i, SIGNAL, 1, 0);
			}
		} else if (!BATR_puedo_saltar(*dx, *dy, DIRECCIONES[rnd])) {
			BATR_avance_rana_ini(*dx, *dy);
			BATR_avance_rana(dx, dy, DIRECCIONES[rnd]);
			BATR_pausa();
			BATR_avance_rana_fin(*dx, *dy);
			if (estaEnPrimeraCasilla) {
				operarSobreSemaforo(semaforo, SEMAFORO_PRIMERMOVIMIENTO+i, SIGNAL, 1, 0);
			}
		} else if (!BATR_puedo_saltar(*dx, *dy, DIRECCIONES[1 - rnd])) {
			BATR_avance_rana_ini(*dx, *dy);
			BATR_avance_rana(dx, dy, DIRECCIONES[1 - rnd]);
			BATR_pausa();
			BATR_avance_rana_fin(*dx, *dy);
			if (estaEnPrimeraCasilla) {
				operarSobreSemaforo(semaforo, SEMAFORO_PRIMERMOVIMIENTO+i, SIGNAL, 1, 0);
			}
		} else {
			contadorBloqueo++;
			BATR_pausita();
		}
		if (*dy == 11) { //Rana salvada
			operarSobreSemaforo(semaforo, SEMAFORO_SALVADAS, WAIT, 1, 0);
			*ranasSalvadas += 1;
			operarSobreSemaforo(semaforo, SEMAFORO_SALVADAS, SIGNAL, 1, 0);
			raise(SIGINT); // aquí hace el signal ya
		}
	}
}

/* ----------------------------------------------------------------------------------------- */
int operarSobreSemaforo(int semaforo, int indice, short op, short nsops, short flg) {
/* ----------------------------------------------------------------------------------------- */
	struct sembuf sop = {indice, op, flg};
	return semop(semaforo, &sop, nsops);
}

/* ------------------------------------- */
void sigintHandlerMadres(int sig) {
/* ------------------------------------- */
	sigset_t mascara;
	int i;
	sigfillset(&mascara);
	sigprocmask(SIG_SETMASK, &mascara, NULL);
	for (i = 0; i < MAX_PROCESOS; i++) {
		if (arrayPID[i] > 0) {
			kill(arrayPID[i], SIGINT);
		}
	}
	if (operarSobreSemaforo(semaforo, SEMAFORO_PROCESOS, SIGNAL, 1, 0) < 0) {
		perror("sigintHandlerMadres: operacion");
		_exit(20);
	}
	sigprocmask(SIG_UNBLOCK, &mascara, NULL);
	_exit(0);
}

/* ---------------------------------------- */
void sigintHandlerRenacuajos(int sig) {
/* ---------------------------------------- */
	sigset_t mascara;
	int i;
	sigfillset(&mascara);
	sigprocmask(SIG_SETMASK, &mascara, NULL);
	if (operarSobreSemaforo(semaforo, SEMAFORO_PROCESOS, SIGNAL, 1, 0) < 0) {
		perror("sigintHandlerRenacuajos: operacion");
		_exit(20);
	}
	sigprocmask(SIG_UNBLOCK, &mascara, NULL);
	_exit(0);
}

/* ------------------------------------------------------------------------------------------------------ */
void devolverPunterosContadores(int ** ranasNacidas, int ** ranasSalvadas, int ** ranasPerdidas) {
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
