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

int
main(void) {
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
	tline * line;
	int i,j;

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
		for(i=0; i<line->ncommands; i++){				//i Itera sobre la lista de comandos
			//Codigo para ejecutar uno o varios mandatos

			//NOTA: Antes de entrar aqui, comprobar que el mandato introducido es valido
			if(line->ncommands==1){						//Pongo este if porque de momento estoy haciendo que funcione para un comando. Veremos si se puede quitar al hacerlo para mas comandos
				
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

				for(j=0;j>line->commands[i].argc; j++){		//j Intera sobre los argumentos pasados a cada comando

				

				}
			}
			//Fin de codigo para ejecutar uno o varios mandatos
		}
		
		printf("msh> ");
	}
	return 0;
}

//gcc -Wall -Wextra ./test.c ./libparser.a -o ./test -static
//./test