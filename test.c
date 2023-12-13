#include <stdio.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "parser.h"

tline * line;
int **arraypipes;
int *arraypids;
int i;

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

char buf[1024];
	//int i;

	printf("msh> ");	

	while (fgets(buf, 1024, stdin)) {
		
		//line->redirect_input=NULL;				//inicializamos entrada para que no se quede en un bucle infinito
		line = tokenize(buf);
		if (line==NULL){
			continue;
		}
		if(line->background){
			//Ver como hacerlo
		}	
			//Codigo para ejecutar uno o varios mandatos

			//NOTA: Antes de entrar aqui, comprobar que el mandato introducido es valido
		if(line->ncommands==1){						//Pongo este if porque de momento estoy haciendo que funcione para un comando. Veremos si se puede quitar al hacerlo para mas comandos
			
			i=0;
			pid_t pid;
			pid=fork();

			if(pid==0){
				int fi, fo, fe, nuevodescriptor;			//Input & output & error file descriptor
				nuevodescriptor = 0;										//Inicializamos nuevodescriptor para que si no entra por el if, se ejecute el comando
				if(line->redirect_input!=NULL){
					fi = open(line->redirect_input, O_RDONLY | O_CLOEXEC);
					nuevodescriptor = dup2(fi, STDIN_FILENO);				//Ahora stdin apunta al descriptor de ficheros fe
				}
				if(line->redirect_output!=NULL){
					fo = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);	//Si el archivo no existe, lo creamos con modo por defecto (0666)
					dup2(fo, STDOUT_FILENO);
				}
				if(line->redirect_error!=NULL){
					fe = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);	//Si el archivo de salida de error no existe, lo creamos con modo por defecto(0666)
					dup2(fe, STDERR_FILENO);
				}
				if(nuevodescriptor != -1){			//Si no se ha producido ningun error, ejecutamos
					execv(line->commands[i].filename, line->commands[i].argv);
					} 
				else{	//Se imprime un mensaje de error en la salida de error predeterminada, en caso de que sucediera algun problema
					perror("Error: Check input redirection. No such file or directory");
					exit(0);
				}
			}
			else{
				wait(NULL);
			}
		}
			//Fin de codigo para ejecutar un mandato
			//Ahora codigo para ejecutar varios mandatos
		else{
			//printf("%d", line->ncommands);
			int npipes, fi, fo;
			pid_t pid;
			npipes = line->ncommands-1;
			arraypipes = (int**)malloc(npipes*sizeof(int*));
			arraypids = (int*)malloc((line->ncommands)*sizeof(int));
			for(i=0; i<npipes; i++){				//Creamos el array de pipes (array de arrays de 2 enteros)
				arraypipes[i]=(int*)malloc(2*sizeof(int));
			}
			for(i=0; i<npipes; i++){				//inicializamos los pipes
				pipe(arraypipes[i]);					//El pipe n-1 enlaza los procesos n-1 y n
			}
			for(i=0; i<line->ncommands; i++){				//creamos procesos hijos
				pid=fork();
				if(pid==0){
					if(i==0){
						close(arraypipes[0][0]);
						if(line->redirect_input != NULL){
							fi = open(line->redirect_input, O_RDONLY | O_CLOEXEC);
							dup2(fi, STDIN_FILENO);
						}
						dup2(arraypipes[0][1], STDOUT_FILENO);
						//printf("Ejecutado comando %d\n", i);
						execv(line->commands[i].filename, line->commands[i].argv);
						printf("Error ejecutando comando %d\n", i);
					}
					else if (i==line->ncommands-1){
						close(arraypipes[i-1][1]);
						if(line->redirect_output != NULL){
							fo = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0666);	//Si el archivo no existe, lo creamos con modo por defecto (0666)
							dup2(fo, STDOUT_FILENO);
						}
						dup2(arraypipes[i-1][0], STDIN_FILENO);
						//printf("Ejecutado comando %d\n", i);
						execv(line->commands[i].filename, line->commands[i].argv);
						printf("Error ejecutando comando %d\n", i);
					}
					else{
						close(arraypipes[i-1][1]);
						close(arraypipes[i][0]);
						dup2(arraypipes[i-1][0], STDIN_FILENO);
						dup2(arraypipes[i][1], STDOUT_FILENO);
						//printf("Ejecutado comando %d\n", i);
						execv(line->commands[i].filename, line->commands[i].argv);
						printf("Error ejecutando comando %d\n", i);
					}
				}
				else{
					arraypids[i]=pid;
					if(i!=line->ncommands-1){
						close(arraypipes[i][1]);
					}
				}
			}
			for(i=0; i<line->ncommands; i++){
				waitpid(arraypids[i], NULL, 0);
			}
		}

		printf("msh> ");
	}
	return 0;
}

//gcc -Wall -Wextra ./test.c ./libparser.a -o ./test -static
//./test