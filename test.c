#include <stdio.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
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

	int fi;				//fe --> Descriptor de fichero de entrada.

	while (fgets(buf, 1024, stdin)) {
		
		//line->redirect_input=NULL;				//inicializamos entrada para que no se quede en un bucle infinito
		line = tokenize(buf);
		if (line==NULL){
			continue;
		}
		if(line->redirect_input!=NULL){
			//Codigo para leer de un fichero la entrada.(variable input = ...)
			fi = open(line->redirect_input, O_RDONLY);
			dup2(fi, STDIN_FILENO);					//Ahora stdin apunta al descriptor de ficheros fe
		}
		if(line->redirect_output!=NULL){
			//Codigo para escribir en un fichero. Igual conviene poner este if al final, puesto que escribir el output en un fichero tiene que ser lo ultimo
		}
		if(line->redirect_error!=NULL){
			//Codigo para escribir el error en un fichero. Igual conviene poner este if al final tambien
		}
		if(line->background){
			//Ver como hacerlo
		}	
		for(i=0; i<line->ncommands; i++){				//i Itera sobre la lista de comandos
			//Codigo para ejecutar uno o varios mandatos

			if(line->ncommands==1){				//Pongo este if porque de momento estoy haciendo que funcione para un comando. Veremos si se puede quitar al hacerlo para mas comandos
				pid_t pid;
				pid=fork();
				if(pid==0){
					execv(line->commands[i].filename, line->commands[i].argv);
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