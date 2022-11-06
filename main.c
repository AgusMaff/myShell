#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "jobsControl.h"

#define MAX 256

char *getString(char []);
void cdFunc(char *,char *);
void echoFunc(char *);
void act3(char *,char *);
void act4(char *);
void act5(char *,char *);

int n_job;

int main(int argc, char* argv[]){
    char line[MAX];
    char *str;
    char *cmd;
    char *arg;
    char *usr;
    char host[MAX];
    char *path;

    usr = getenv("LOGNAME"); //Obtengo el usuario directamente desde la variable de entorno LOGNAME
    path = getenv("PWD"); //Obtengo el usuario directamente desde la variable de entorno PWD
    if(argc==2 && strcmp(argv[1],"batchfile.txt")==0){ //Si cuando se corre la linea ./myShell batchfile se le dice a myShell que tome los comandos a ejecutar directamente desde un txt
        act4(argv[1]);
        exit(0); //Una vez ejecutadas todas las lineas de comando se cierra myShell utilizando la system call exit()
    }else{
        if(argc==1 && usr !=NULL){ //Si no se le pasa ningun argumento y se puede encontrar el nombre de usuario se pasa a la siguiente verificacion
            if(gethostname(&host[0],MAX)==0){ //Si se puede encontrar el nombre del host entonces se inicia myShell con un MOTD
                printf("# Bienvenido a myShell #\n"); //MOTD
                while(1) { //Este while tiene su condicion verdadera para que myShell nunca se cierre
                    printf("%s@%s:~%s ", usr, host, path); //Imprime constantemente el prompt con el formato usuario@host:~path_actual
                    scanf("%[^\n]%*c", line); //myShell espera que ingresemos una linea de entrada
                    str = getString(line); //se convierte la linea de entrada en string para su manipulacion
                    cmd = strtok(str, " "); //se separa el comando de la linea de entrada
                    arg = strtok(0, "\n"); //se separan los argumentos de la linea de entrada
                    if(arg==NULL){arg="NO_ARG";}
                    if(strstr(arg,"&")!=NULL){
                        act5(cmd,arg);
                    }else{
                        if (strcmp(cmd, "cd") == 0) { //si el comando es cd, entonces realiza la funcion de cambio de directorio
                            cdFunc(path,arg);
                        } else if (strcmp(cmd, "clr") == 0) { //si cmd es clr, realiza la funcion de limpiar la pantalla
                            system("clear"); //utilizo la funcion system para ejecutar el comando clear
                        } else if (strcmp(cmd, "echo") == 0) { //si cmd es echo, se realiza la funcion de imprimir por temrinal un mensaje indicado o una variable de entorno usando la opcion $ENVAR
                            switch (fork()) { //Se usa la funcion fork para duplicar el proceso actual
                                case 0: //si fork() retorna 0 (es decir es el hijo), realiza la funcion echo
                                    echoFunc(arg);
                                    break;
                                case -1: //si fork retorna -1, fallo en la clonacion del proceso entonces cancela el comando
                                    fprintf(stderr, "Fallo la clonacion del proceso padre"); //imprime un mensaje de error
                                    break;
                                default:
                                    wait(NULL); //si fork() no devuelve ni un 0 ni -1 (es el proceso padrde), entonces espera a que el proceso hijo termine antes de seguir avanzando
                            }
                        } else if (strcmp(cmd, "quit") == 0) { //si cmd es quit, entonces se termina el proceso de myShell
                            exit(0);
                        } else {
                            switch(fork()){ //si cmd no corresponde a ningun comando interno de myShell entonces los va a procesar como programas aparte
                                case 0: //si fork retorna 0, entonces el hijo realiza la funcion act3
                                    act3(cmd,arg);
                                    break;
                                default:
                                    wait(NULL); //el proceso padre espera a que el proceso hijo termine antes de avanzar
                            }
                        }
                    }
                    free(str); //se libera memoria alocada
                }
            }else{
                fprintf(stderr,"No se pudo iniciar myShell\n"); //mensaje de error si no se puede iniciar myShell
                exit(0);
            }
        }else{
            fprintf(stderr,"No se pudo iniciar myShell\n"); //mensaje de error si no se puede iniciar myShell
            exit(0);
        }
    }
}
char *getString(char line[]){ //recibe el buffer con la linea de entrada del tipo char [] y lo convierte en string
    char *str;
    str=(char *)malloc(MAX*sizeof(char)); //aloco una porcion de memoria suficiente para a almacenar todos los elementos de line
    for(int i=0;i<200;i++){
        str[i]=line[i]; //cada elemento i de str almacena cada elemento i de line respectivamente
    }
    return str; //retorna el string de line
}
void cdFunc(char *path,char *arg){ //realiza la funcion cuando se ingresa el comando cd
    if (!strcmp(arg, "-")) { //verifica si el caracter "-" forma parte de los argumentos, ya que el comando cd soporta la opcion "cd -" para volver al directorio anterior
        char *opath = getenv("OLDPWD"); //asigna el valor de la variable de entorno OLDPWD a opath
        if (opath != NULL) {chdir(opath);setenv("OLDPWD", getenv("PWD"), 1);} //si opath no es NULL, se usa la funcion chdir para volver al directorio anterior y se le asigna a OLDPWD el valor de PWD
        else fprintf(stderr, "No se pudo obtener OLDPWD\n"); //de lo contrario se imprime un mensaje de error
    } else {
        chdir(arg); //si no se encuntra la opcion "-" en el comando de entrada, entonces se cambia de directorio usando chdir
        setenv("OLDPWD", getenv("PWD"), 1); //se le asigna el valor de PWD a OLDPWD
    }
    getcwd(path, MAX); //obtengo el directorio actual y lo guardo en path para actualizar el prompt
}
void echoFunc(char *arg){ //realiza la funcion del comando echo
    if(strstr(arg,"$")==NULL) { //pregunta si se encuentra el $ en el argumento, de no ser asi, entonces el argumento es un mensaje que se debe imprimir por pantalla
        char *arg_list[] = {"echo", arg, NULL};
        execvp("echo", arg_list); //la funcion execvp reemplaza el proceso que ejecute la funcion, es por eso que debe ejecutar con el proceso hijo para que este realice la funcion y al finalizar se cierre el proceso
    }else{ //si se encuentra el $, entonces se debe imprimir la variable en entorno correspondiente
        char *en_var;
        en_var=strtok(arg,"$"); //separo el $ del nombre de la variable de entorno
        if(getenv(en_var)==NULL){ // sin no se encuentra la variable de entorno se imprime un espacio en blanco
            printf(" \n");
        }else {
            printf("%s\n", getenv(en_var)); //imprime el valor de la variable de entorno
        }
        exit(0); //termina el proceso hijo
        }
}
void act3(char *cmd,char *arg) { //realiza la actividad 3, la cual consiste en que si el comando de linea no pertenece a ninguno de los comando internos, entonces los toma como la invocacion de un programa
    int flag = 0;
    int i = 0;
    char *rest = arg;
    char **args;
    if(strcmp(arg,"NO_ARG")!=0) { //pregunta si hay un argumento o no
        while (1) {
            if (strstr(rest, " ")) { //-l /proc
                if (flag == 0) { //el flag se usa para verificar si es ya se aloco memoria con anterioridad o no. De ser asi, entonces va a separar el primer argumento de la linea de comando, aloco por primera vez una posicion de memoria y lo asigno a la variable args
                    strtok_r(arg, " ", &rest);
                    args = (char **) malloc(sizeof arg);
                    args[i] = arg;
                    flag++; //flag, ademas de verificar si es la primera vez que aloco memoria o no, me sirve como variable incremental que ya voy a explicar su otra funcion
                    i++; //cuenta el numero de argumento ingresados
                } else {
                    flag++; //incrementa el flag otra vez
                    arg = rest; //guardo el resto de argumentos para seguir separandolos desde donde lo deje la ultima vez con el strtok
                    strtok_r(0, " ", &rest);
                    args = (char **) realloc(args, flag * sizeof(arg)); //uso la variable flag para decirle el tamaño de memoria nuevo que yo quiero incrementar
                    args[i] = arg;
                    i++;
                    arg = rest;
                }
            } else { //entra en este else si se ingreso un solo argumento o solo queda uno
                if (flag == 1) { flag++; }
                if(strcmp(rest,"&")!=0) {
                    arg = rest;
                    args = realloc(args, flag * sizeof(arg));
                    args[i] = arg;
                    i++;
                }
                break;
            }
        }
        char *argument_list[i + 2]; //es la lista de argumento que voy a usar para pasarla a la funcion execvp, es de tamaño i+2 ya que tambien debo agregarle el comando al comienzo y un NULL al final
        for (int j = 0; j <= i + 1; j++) {
            if (j == 0) { argument_list[j] = cmd; }
            else if (j != i + 1) { argument_list[j] = args[j - 1]; }
            else if (j == i + 1) { argument_list[j] = NULL; }
        }
        execvp(cmd,argument_list);
        exit(0);
    }else{//si no hay argumento, entonces ejecuta solo el comando ingresado
        char* argument_list[]={cmd,NULL};
        execvp(cmd,argument_list);
    }
}
void act4(char *arg){
     FILE *file;
     file=fopen(arg,"r"); //abro el archivo batchfile.txt para leerlo
     while(feof(file)==0){ //mientras no este en el EOF del archivo, voy a estar iterando
         char *buff[MAX];
         fgets((char*)buff,sizeof(buff),file); //obtengo la primera linea de codigo del archivo y la guardo en el buffer
         system((char*)buff); //utilizo la systema call para ejecutar la linea guatdada en el buffer
     }
}
void act5(char *command,char *arguments){
    int n = 0;
    char** argv = malloc(sizeof(char*));
    //guarda el comando a ejecutar en el primer elemento del argv
    argv[n] = malloc(sizeof(char) * strlen(command));
    strcpy(argv[(n)++], command);

    //se procede con el resto de argumentos
    char* token = strtok(arguments, " ");
    while (token != NULL)
    {
        argv[n] = malloc(sizeof(char) * strlen(token));
        strcpy(argv[(n)++], token);
        argv = realloc(argv, sizeof(char*) * (n + 1));
        token = strtok(NULL, " ");
    }
    argv[n] = NULL; //fin de argumentos

    //se construye la estructura del proceso utilizada para su manejo
    Process *process = malloc(sizeof(Process));
    process->argc = n;
    process->argv = argv;
    process->next = NULL;
    process->id   = -1;
    process->status = PROCESS_NEW;

    //se construye la estructura del job utilizada para su manejo
    Job *job = malloc(sizeof(Job));
    job->next = NULL;
    job->job_id = -1;
    job->head_process = process; //el proceso pasa a formar parte del trabajo
    job->pgid = -1;
    if(!strcmp(argv[n-1],"&")) //se determina el modo de ejecución
    {
        job->execution_mode = JOBS_BEHIND;
        free(argv[n-1]);
        argv[n-1] = NULL;
    }
    else
        job->execution_mode = JOBS_AHEAD;

    launchJob(job); //lanzamos el trabajo
}