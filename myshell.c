#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "parser.h"

//Para compilar: gcc -Wall -Wextra myshell.c libparser.a -o myshell -static
//Para ejecutar: ./myshell

struct tbackground{			//Tipo proceso en background
	int pid;				//para guardar el pid clave del proceso
	char *linea;			//para guardar el nombre de la linea
	int status;				//para comprobar si ha acabado o no
};

tline * line;
int **arraypipes;
int *arraypids;
int i;
int max, j;										//j sirve para contar de 0 a 1023 procesos en bg. max para la ultima posicion ocupada del array de procesos en background
struct tbackground procesos[1024];				//array para guardar los procesos que se ejecutan en background

int redirect(int option, char * redirection){	//Redirige entrada, salida y salida de error estandar. Option: STDIN_FILENO para redireccion de entrada, STDOUT_FILENO para salida estandar y STDERR_FILENO para salida de error. (Redirecciones de salida y error se tratan de la misma forma)
	int descriptor, nuevodescriptor;			
	nuevodescriptor = 0;
	if(option == STDIN_FILENO){
		if(redirection != NULL){
			descriptor = open(redirection, O_RDONLY | O_CLOEXEC);
			nuevodescriptor = dup2(descriptor, option);
		}
	}
	else {
		if(redirection != NULL){
			descriptor = open(redirection, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);	//Si el archivo no existe, lo creamos con modo por defecto (0666)
			nuevodescriptor = dup2(descriptor, option);
		}
	}
	return nuevodescriptor;							//Devuelve -1 si ha ocurrido algun error
}

int execcd(tline * line){							//Devuelve 1 si el mandato que hemos pasado es cd y 0 en caso contrario. Ejecuta el mandato cd
	if(strcmp(line->commands[i].argv[0], "cd") == 0 && (line->ncommands == 1)){
		if(line->commands[i].argc > 2){				//Si hay mas argumentos de la cuenta se imprime error y aborta ejecucion
			fprintf(stderr, "No se puede ejecutar el mandato cd: Numero de argumentos de cd (%d) debe ser 0 o 1\n", line->commands[i].argc);
		}
		else{
			char nuevaruta[1024];					//Aqui guardaremos la ruta del nuevo directorio al que nos moveremos para imprimirlo por pantalla
			if(line->commands[i].argc == 1){		//Si el mandato solo es cd (sin nada mas. ej: cd)
			char *ruta = getenv("HOME");
				if(chdir(ruta) == 0){
					getcwd(nuevaruta, 1024);
					printf(("%s\n"), nuevaruta);
				}
				else{
					fprintf(stderr, "cd: Error al cambiar al directorio %s\n", ruta);
				}
			}
			else {									//Si el mandato es cd Desktop (por ejemplo)
				char ruta[1024];
				if((char)line->commands[i].argv[1][0] == '/'){
					strcpy(ruta, line->commands[i].argv[1]);
				}
				else{
					getcwd(ruta, 1024);
					strcat(ruta, "/");
					strcat(ruta, line->commands[i].argv[1]);
				}
				if(chdir(ruta) == 0){
					getcwd(nuevaruta, 1024);
					printf("%s\n", nuevaruta);
				}
				else{
					fprintf(stderr, "cd: Error al cambiar al directorio %s\n", line->commands[i].argv[1]);
				}
			}
		}
		return 1;
	}
	else{
		if (strcmp(line->commands[i].argv[0], "cd") == 0 && (line->ncommands != 1)){		//Si ejecutamos mandato cd con pipes, imprimimos error y abortamos ejecucion
			fprintf(stderr, "No se puede ejecutar el mandato cd. Error al ejecutar el mandato %d\n", i);
			return 1;
		}
	}
	return 0;
}

int showmask(){								//Imprime por pantalla la mascara de permisos por defecto actual
    mode_t defaultperm;
	defaultperm = umask(0);
	umask(defaultperm);
    printf("%04o\n", defaultperm);
	return 0;
}

int execumask(){							//Cambia o imprime por pantalla la mascara de permisos por defecto
	if (strcmp(line->commands[i].argv[0], "umask") == 0 && (line->ncommands == 1)){
		if(line->commands[i].argc == 1){
			showmask();
		}
		else{
			if(line->commands[i].argc == 2){
				mode_t nuevamascara = 0;							//Comprobar que el numero que nos han pasado esta en octal
				char *mascara = line->commands[i].argv[1];
    			char c = *mascara;
    			while(c != '\0'){
        			if(c < '0' || c > '7') {	
						fprintf(stderr,"Expresion en octal no valida\n");
						return 1;
        			}
        			nuevamascara <<= 3;
        			nuevamascara += c - '0';
        			if(nuevamascara > 0777) {	
						fprintf(stderr,"Expresion en octal no valida. Se excede el maximo (0777)\n");
						return 1;
        			}
        			mascara += 1;
        			c = *mascara;
    			}
    			
    			umask(nuevamascara);
			}
			else{
				fprintf(stderr, "umask: Numero de argumentos (%d) debe ser 0 o 1\n", line->commands[i].argc);
			}
		}
		return 1;
	}
	else{
		if(strcmp(line->commands[i].argv[0], "umask") == 0 && (line->ncommands != 1)){
			fprintf(stderr, "No se puede ejecutar el mandato umask. Error al ejecutar el mandato %d.\n", i);
			return 1;
		}
	}
	return 0;
}

int execmandato(){				//Si la linea introducida por teclado esta solo formada por un mandato, se ejecuta
	pid_t pid;
	pid=fork();

	if(pid==0){
		int nuevodescriptor;
				
		nuevodescriptor = redirect(STDIN_FILENO, line->redirect_input);
		redirect(STDOUT_FILENO, line->redirect_output);
		redirect(STDERR_FILENO, line->redirect_error);

		if(nuevodescriptor != -1){
			if(line->background){
				signal(SIGINT, SIG_IGN);
			}
			execv(line->commands[i].filename, line->commands[i].argv);
			fprintf(stderr, "%s: No se encuentra el mandato. Error ejecutando comando %d.\n", line->commands[i].argv[0], i);
		} 
		else{
			fprintf(stderr, "%s: Error. Check input redirection. No such file or directory\n", line->redirect_input);
			exit(1);
		}
		exit(1);
	}
	else{
		if(!line->background){
			wait(NULL);
		}
		else{
			procesos[max].pid = pid;
			max++;
			max = max % 1024;
		}
	}
	return 0;
}

int execnmandatos(){			//Si la linea introducida por teclado esta formada por varios mandatos y pipes, se ejecuta
	int npipes, nuevodescriptor;
	pid_t pid;
	npipes = line->ncommands-1;
	arraypipes = (int**)malloc(npipes*sizeof(int*));
	arraypids = (int*)malloc((line->ncommands)*sizeof(int));
			
	for(i=0; i<npipes; i++){
		arraypipes[i]=(int*)malloc(2*sizeof(int));
	}

	for(i=0; i<npipes; i++){
		pipe(arraypipes[i]);
	}

	redirect(STDERR_FILENO, line->redirect_error);
	for(i=0; i<line->ncommands; i++){			//Ejecutamos los n mandatos

		pid=fork();

		if(pid==0){
			execcd(line);
			execumask();
			if(i==0){
				close(arraypipes[0][0]);
				nuevodescriptor = redirect (STDIN_FILENO, line->redirect_input);
				dup2(arraypipes[0][1], STDOUT_FILENO);
				if(nuevodescriptor != -1){
					if(line->background){
						signal(SIGINT, SIG_IGN);
					}
					execv(line->commands[i].filename, line->commands[i].argv);
					fprintf(stderr, "%s: No se encuentra el mandato. Error ejecutando comando %d.\n", line->commands[i].argv[0], i);
				}
				else{
					fprintf(stderr, "%s: Error. Check input redirection. No such file or directory\n", line->redirect_input);
				}
				exit(1);
			}
			else if (i==line->ncommands-1){
				close(arraypipes[i-1][1]);
				redirect(STDOUT_FILENO, line->redirect_output);
				dup2(arraypipes[i-1][0], STDIN_FILENO);
				if(line->background){
					signal(SIGINT, SIG_IGN);
				}
				execv(line->commands[i].filename, line->commands[i].argv);
				fprintf(stderr, "%s: No se encuentra el mandato. Error ejecutando comando %d.\n", line->commands[i].argv[0], i);
				exit(1);
			}
			else{
				close(arraypipes[i-1][1]);
				close(arraypipes[i][0]);
				dup2(arraypipes[i-1][0], STDIN_FILENO);
				dup2(arraypipes[i][1], STDOUT_FILENO);
				if(line->background){
					signal(SIGINT, SIG_IGN);
				}
				execv(line->commands[i].filename, line->commands[i].argv);
				fprintf(stderr, "%s: No se encuentra el mandato. Error ejecutando comando %d.\n", line->commands[i].argv[0], i);
				exit(1);
			}
		}

		else{
			arraypids[i]=pid;
			if(i!=line->ncommands-1){
				close(arraypipes[i][1]);
			}
		}
	}

	if(!line->background){
		for(i=0; i<line->ncommands; i++){
			waitpid(arraypids[i], NULL, 0);
		}
	}
	else{
		for(i=0; i< line->ncommands; i++){
			if(i == line->ncommands-1){
				procesos[max].pid = pid;
				max++;
				max = max % 1024;
			}
			else{
				waitpid(procesos[max].pid, &procesos[max].status, WNOHANG);
			}
		}
	}
	return 0;
}

void abortar(){				//Salto de linea para abortar la ejecucion de un proceso en foreground
	printf("\n");
}

void salir(){				//Mata todos los procesos en ejecucion en background y sale del programa
	for(j=0; j<1023; j++){
		kill(procesos[j].pid, SIGKILL);
	}
	exit(0);
}

int jobs(){					//Muestra la lista de procesos en background activos
	if(line->ncommands==1 && strcmp(line->commands[i].argv[i], "jobs") == 0){
		for(j=0; j<1023; j++){
			procesos[j].status = waitpid(procesos[j].pid, &procesos[j].status, WNOHANG);
			if (procesos[j].status!=-1 && procesos[j].linea != NULL) {
				printf("[%d] Running 			%s", j, procesos[j].linea);
			}
			else if(procesos[j].linea != NULL){
				printf("[%d] Done 			%s", j, procesos[j].linea);
				procesos[j].status = 0;
				free(procesos[j].linea);
				procesos[j].linea = NULL;
			}
		}
		return 1;
	}
	else if(line->ncommands!=1 && strcmp(line->commands[i].argv[i], "jobs") == 0){
		fprintf(stderr, "No se puede ejecutar el mandato jobs. Error al ejecutar el mandato %d\n", i);
		return 1;
	}
	return 0;
}

void mostrarprocesosterminados(){	//Funcion para mostrar los procesos que han terminado. Se ejecuta cada vez que leemos una nueva linea, despues de que sea ejecutada
	for(j=0; j<1023; j++){
		procesos[j].status = waitpid(procesos[j].pid, &procesos[j].status, WNOHANG);
		if (procesos[j].status==-1 && procesos[j].linea != NULL) {
			printf("Proceso [%d] con pid %d terminado: %s", j, procesos[j].pid, procesos[j].linea);
			procesos[j].status = 0;
			free(procesos[j].linea);
			procesos[j].linea = NULL;
		}
	}
}

void updatebackgroundstatus(){	//Funcion para actualizar el estado de los procesos en background
	for(j=0; j<1023; j++){
		procesos[j].status = waitpid(procesos[j].pid, &procesos[j].status, WNOHANG);
	}
}

int foreground(){		//Funcion para pasar un proceso de background a foreground
	int ultimo;			//Guardamos en ultimo la ultima posicion del array de procesos que es valida (que tiene algun proceso)
	ultimo = -1;
	for(j=1023; j>=0; j--){
		if(ultimo == -1 && procesos[j].linea != NULL){
			ultimo = j;
		}
	}
	if(strcmp(line->commands[i].argv[i], "fg") == 0){
		if(line->ncommands == 1 && ultimo != -1){
			if(line->commands[i].argc == 1){				//Si el mandato introducido es fg solo, tomamos la posicion dada por ultimo
				fprintf(stdout, "%s", procesos[ultimo].linea);
				waitpid(procesos[ultimo].pid, &procesos[ultimo].status, 0);
			}
			else if (line->commands[i].argc == 2){
				char *posicion = line->commands[i].argv[1];	//Si el mandato introducido es fg con algo mas, se compureba que ese algo mas sea un numero valido y se toma ese proceso de la lista
				int a = atoi(posicion);
				if(a >= 0 && a<1024) {
					if(procesos[a].linea != NULL){
						fprintf(stdout, "%s", procesos[a].linea);
						waitpid(procesos[a].pid, &procesos[a].status, 0);
					}
					else{
						fprintf(stderr, "fg: Error. No se encuentra el mandato\n");
					}
				}
				else{
					fprintf(stderr, "fg: Numero introducido (%d) no es valido\n", a);
				}
			}
			else{
				fprintf(stderr, "fg: Numero de argumentos (%d) debe ser 0 o 1\n", line->commands[i].argc);
			}
		}
		else if(ultimo == -1){
			fprintf(stderr, "fg: No se encuentra el mandato en bg\n");
		}
		else{
			fprintf(stderr, "No se puede ejecutar el mandato fg. Error al ejecutar el mandato %d\n", i);
		}

		return 1;
	}
	return 0;
}

int main(void) {

max = 0;
char buf[1024];

	printf("msh> ");	
	signal(SIGINT, SIG_IGN);

	while (fgets(buf, 1024, stdin)) {

		updatebackgroundstatus();		//Actualizamos el estado de los procesos en background, para que si alguno ha acabado, mostrarlo al final

		signal(SIGINT, abortar);		//Establecemos senal Ctrl+C para procesos en foreground
		line = tokenize(buf);

		if (line==NULL){
			continue;
		}
		if(line->background==1){
			procesos[max].linea = (char *)malloc(1024*sizeof(char));
			strcpy(procesos[max].linea, buf);
		}	
		
			//Codigo para ejecutar un solo mandato

		if(line->ncommands==1){
			
			i=0;

			if(!execcd(line) && !execumask() && !jobs() && !foreground()){

				if(strcmp(line->commands[0].argv[0], "exit") == 0){
					salir();
				}
				execmandato();

			}
		}
			//Fin de codigo para ejecutar un solo mandato
			//Ahora codigo para ejecutar varios mandatos
		else{
			
			execnmandatos();

		}

			//Fin de codigo para ejcutar varios mandatos

		mostrarprocesosterminados();		//Mostramos si ha habido procesos en background que han terminado
		printf("msh> ");
		signal(SIGINT, SIG_IGN);			//Ignoramos Ctrl+C mientras leemos nueva linea
	}
	return 0;
}