/*lines, por Juan Ureña.
El programa recibe como datos de entrada
el texto a bucar y el direcotrio en el que
debemos buscarlo. Buscaremos el texto 
indicado con "fgrep" en todos los ficheros
acabados en ".txt" del directorio indicado. 
El resultado será almacenado en el fichero
"lines.out", en el repositorio padre del 
indicado, y los errores del "fgrep" son 
redirigidos a "/dev/null".*/

#include <stdio.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <err.h>
#include <dirent.h>
#include <fcntl.h>

char *ext=".txt";

	//Función para esperar a todos mis hijos.
int
wait_childs(int num_child)
{
	int status;
	int result=0;
	for(int x=0;x<num_child;x++){ 
		waitpid(-1, &status, 0);
		if (WIFEXITED(status)){
			if (WEXITSTATUS(status)>1){
	//El error con fgrep, es dos.
				result=1;
			}
		} 
	}
	return result;
}

	//Genero ruta absoluta con ruta y el nombre del fichero
char *
generate_path(char *path, char *file)
{
	char *absolute_path;
	absolute_path=malloc(strlen(path)+strlen(file)+2);
	if (absolute_path){
		strcpy(absolute_path, path);
		strcat(absolute_path, "/");
		strcat(absolute_path, file);
	}else{
		fprintf(stderr, "\nError en la memoria, parece estar llena\n");
		exit(EXIT_FAILURE);
	}
	return absolute_path;
} 
	
	//Hilo de nuestros hijos. 
void 
son_code(char *dir, char *file, char *text, char *output) 
{
	char *file_absolute_path;
	int file_out;
	int waste;

	//Errores a null
	waste=open("/dev/null", O_WRONLY);
	dup2(waste, STDERR_FILENO);
	close(waste);
	
	//Salida al fichero, con creación y append.
	file_out=open(output, O_CREAT | O_RDWR | O_APPEND,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );

	dup2(file_out, STDOUT_FILENO);
	close(file_out);
	free(output);

	file_absolute_path=generate_path(dir, file);
	//Comando.
	execl("/bin/fgrep", "/bin/fgrep", "-h", "-s", text, file_absolute_path, NULL); 
	err(1, NULL);
}

	//Compruebo que el fichero acaba en .txt
int
check_extension(char *name)
{
	int result=0;
	//Lo corto por el punto de la extensión
	char *end=strchr(name, '.');
	int equal;

	if (end){
		equal=strcmp(end, ext);
		if (!equal){
			result=1;
		}
	}
	return result;
}

	//Recorre el dir para hacer fgrep sobre los ficheros necesarios
int
fgrep_above_dir(char *dir, char *text, char *output)
{
	DIR *dire;
	struct dirent *dt;
	
	char *file;
	int num_child=0;
	int result=0;
	
	dire=opendir(dir);
	
	if (dire){
		while((dt=readdir(dire))){
			file=dt->d_name;
			if (check_extension(file)){
				num_child++;
				switch(fork())
				{
					case 0:
					//hijo	
						son_code(dir, file, text,output );
						break;
					default:
					//padre
						result=wait_childs(num_child);
						break;
				}
			} 
		}	
	}else{
	//No puedo abrir el dir por algun motivo
		fprintf(stderr, "\nError en el directorio, no permite leerlo\n");
		exit(EXIT_FAILURE);
	}
	return result;
}		

int 
main (int argc, char *argv[])
{
	int result;
	char *aux_for_path;
	char *output;
	int fd;
	
	char *dir=argv[2];
	char *text=argv[1];
	
	if (argc>2){
	
	//Genero la ruta donde estará mi fichero de salida.
		aux_for_path=generate_path(dir, "..");
		output=generate_path(aux_for_path, "lines.out");		
		free(aux_for_path);
		
	//Trunco mi fichero de salida si existe.
		fd=open(output,O_WRONLY|O_TRUNC);
		if (fd!=-1 && fd){
			close(fd);
		}
		
	//Recorro mi directorio y me devuelve el resultado global.
		result=fgrep_above_dir(dir, text, output);
	
		if (result){
	//Borro si tengo error. 
			unlink(output);
			fprintf(stderr, "\nError en los ficheros, algún fichero impide ejecutar fgrep correctamente\n");
		}
		free(output);
		exit(result);
		
	}else{
	//No nos meten directorio y/o texto
		fprintf(stderr, "\nError en la entrada: Se debe pasar como parámetros, Directorio y Texto con que comparar\n");
		exit(EXIT_FAILURE);
	}
}
