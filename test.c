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

struct tbackground{
	int pid;				//para guardar el pid clave del proceso
	char *linea;			//para guardar el nombre de la linea
	int status;			//para comprobar si ha acabado o no
};

tline * line;
int **arraypipes;
int *arraypids;
int i;
int max, j;					//j sirve para contar de 0 a 1023 procesos en bg. max para la ultima posicion ocupada del array de procesos en background
struct tbackground procesos[1024];	//array para guardar los procesos

int redirect(int option, char * redirection){	//option: 0 para redireccion de entrada, cualquier otra cosa para redireccion de salida o error (Redirecciones de salida y error se tratan de la misma forma)
	int descriptor, nuevodescriptor;			//Usare 0 para redireccion de entrada, 1 para redireccion de salida y 2 para redireccion de error
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
	return nuevodescriptor;	//Devuelve -1 si ha ocurrido algun error
}

int execcd(tline * line){	//Devuelve 0 si el mandato que hemos pasado es cd y 1 en caso contrario
	if(strcmp(line->commands[i].argv[0], "cd") == 0 && (line->ncommands == 1)){
		if(line->commands[i].argc > 2){		//Si hay mas argumentos de la cuenta se imprime error y aborta ejecucion
			fprintf(stderr, "No se puede ejecutar el mandato cd: Numero de argumentos de cd (%d) debe ser 0 o 1\n", line->commands[i].argc);
		}
		else{
			char nuevaruta[1024];					//Aqui guardaremos la ruta del nuevo directorio al que nos moveremos para imprimirlo por pantalla
			if(line->commands[i].argc == 1){	//Si el mandato solo es cd
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
				//printf("%s, --> %c\n", line->commands[i].argv[1], line->commands[i].argv[1][0]);
				if((char)line->commands[i].argv[1][0] == '/'){
					strcpy(ruta, line->commands[i].argv[1]);
				//	printf("%s\n", ruta);
				}
				else{
					getcwd(ruta, 1024);
					strcat(ruta, "/");
					strcat(ruta, line->commands[i].argv[1]);
				//	printf("%s\n", ruta);
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

int showmask(){
    mode_t defaultperm;
	defaultperm = umask(0);
	umask(defaultperm);
    printf("%04o\n", defaultperm);
	return 0;
}

int execumask(){
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

int execmandato(){
	pid_t pid;
	pid=fork();

	if(pid==0){
		int nuevodescriptor;
				
		nuevodescriptor = redirect(STDIN_FILENO, line->redirect_input);
		redirect(STDOUT_FILENO, line->redirect_output);
		redirect(STDERR_FILENO, line->redirect_error);

		if(nuevodescriptor != -1){
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
		//	procesos[max].status = waitpid(procesos[max].pid, &procesos[max].status, WNOHANG);
		//	waitpid(procesos[max].pid, &procesos[max].status, WNOHANG);
			max++;
			max = max % 1024;
		}
	}
	return 0;
}

int execnmandatos(){
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

			//int fe;
			//fe = open(line->redirect_error, O_APPEND | O_CREAT | O_WRONLY, 0666);
			//dup2(fe, STDERR_FILENO);
	redirect(STDERR_FILENO, line->redirect_error);
	for(i=0; i<line->ncommands; i++){			//Ejecutamos los n mandatos

		pid=fork();

		if(pid==0){
			execcd(line);
			execumask();
			//redirect(STDERR_FILENO, line->redirect_error);
			if(i==0){
				close(arraypipes[0][0]);
				nuevodescriptor = redirect (STDIN_FILENO, line->redirect_input);
				dup2(arraypipes[0][1], STDOUT_FILENO);
				//printf("Ejecutado comando %d\n", i);
				if(nuevodescriptor != -1){
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
				//printf("Ejecutado comando %d\n", i);
				execv(line->commands[i].filename, line->commands[i].argv);
				fprintf(stderr, "%s: No se encuentra el mandato. Error ejecutando comando %d.\n", line->commands[i].argv[0], i);
				exit(1);
			}
			else{
				close(arraypipes[i-1][1]);
				close(arraypipes[i][0]);
				dup2(arraypipes[i-1][0], STDIN_FILENO);
				dup2(arraypipes[i][1], STDOUT_FILENO);
				//printf("Ejecutado comando %d\n", i);
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
				//	procesos[max].status = waitpid(procesos[max].pid, &procesos[max].status, WNOHANG);
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

void abortar(){
	printf("\n");
}

void salir(){
	exit(0);
}

int jobs(){
	//printf("%d", waitpid(procesos[0].pid, &procesos[0].status, WNOHANG));
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

void mostrarprocesosterminados(){
	//printf("%d", waitpid(procesos[0].pid, &procesos[0].status, WNOHANG));
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

void updatebackgroundstatus(){
	for(j=0; j<1023; j++){
		procesos[j].status = waitpid(procesos[j].pid, &procesos[j].status, WNOHANG);
	}
}

int main(void) {
/*	char buf[1024];
	tline * line;
	int i,j;

	printf("==> ");	
	while (fgets(buf, 1024, stdin)) {
		
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}
		if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
		}
		if (line->redirect_output != NULL) {
			printf("redirección de salida: %s\n", line->redirect_output);
		}
		if (line->redirect_error != NULL) {
			printf("redirección de error: %s\n", line->redirect_error);
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
		} 
		for (i=0; i<line->ncommands; i++) {
			printf("orden %d (%s):\n", i, line->commands[i].filename);
			for (j=0; j<line->commands[i].argc; j++) {
				printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
			}
		}
		printf("==> ");	
	}
*/

max = 0;
char buf[1024];
	//int i;

	printf("msh> ");	
	signal(SIGINT, SIG_IGN);

	while (fgets(buf, 1024, stdin)) {

		updatebackgroundstatus();

		signal(SIGINT, abortar);
		//line->redirect_input=NULL;				//inicializamos entrada para que no se quede en un bucle infinito
		line = tokenize(buf);

		if (line==NULL){
			continue;
		}
		if(line->background==1){
			//Ver como hacerlo
			procesos[max].linea = (char *)malloc(1024*sizeof(char));
			strcpy(procesos[max].linea, buf);
		}	
			//Codigo para ejecutar uno o varios mandatos

			//NOTA: Antes de entrar aqui, comprobar que el mandato introducido es valido
		if(line->ncommands==1){						//Pongo este if porque de momento estoy haciendo que funcione para un comando. Veremos si se puede quitar al hacerlo para mas comandos
			
			i=0;

			if(!execcd(line) && !execumask() && !jobs()){

				if(strcmp(line->commands[0].argv[0], "exit") == 0){
					salir();
				}
				execmandato();

			}
		}
			//Fin de codigo para ejecutar un mandato
			//Ahora codigo para ejecutar varios mandatos
		else{
			
			execnmandatos();

		}

		mostrarprocesosterminados();
		printf("msh> ");
		signal(SIGINT, SIG_IGN);
		//mostrarprocesosterminados();
	}
	return 0;
}

//gcc -Wall -Wextra ./test.c ./libparser.a -o ./test -static
//./test