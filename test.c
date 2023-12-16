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
				wait(NULL);
			}
		}
			//Fin de codigo para ejecutar un mandato
			//Ahora codigo para ejecutar varios mandatos
		else{
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