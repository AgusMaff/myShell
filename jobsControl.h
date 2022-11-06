#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define PROCESS_DONE 0          //proceso terminado
#define PROCESS_RUNNING 1       //proceso en ejecución
#define PROCESS_NEW 2           //proceso recien creado
#define JOBS_JOBNOTFUND -1      //no se encuentra el job en la lista
#define JOBS_JOBREMOVED 1       //job removido satisfactoriamente
#define JOBS_BEHIND 0           //job en segundo plano
#define JOBS_AHEAD 1            //job en primer plano
#define JOBS_DONE 1             //job terminado
#define JOBS_RUNNIG 0           //job en ejecución


//Estructura para control de procesos y nodo de la lista
typedef struct Process
{
    pid_t id;
    int argc;
    char **argv;
    int status;
    struct Process *next;
}Process;

//Estructura para control de jobs y nodo de la lista
typedef struct Job
{
    Process *head_process;
    int pipe_io[2];
    int pipe_err[2];
    pid_t pgid;
    int execution_mode;
    int job_id;
    struct Job *next;
}Job;

extern Job *head_job; //cabeza de la lista
extern const char* status_string[]; //convertir int de status de proceso en string


/**
 * @brief Agrega un job a la lista de jobs.
 * @param j job a agregar en la lista.
 */
void addJob(Job *new_job);

/**
 * @brief Elimina un job de la lista y libera la memoria utilizada.
 *
 * @param id_remove id del job a eliminar.
 */
int removeJob(int id_remove);

/**
 * @brief se libera memoria alocada para un job.
 * @param *free_job job a liberar
 */
void freeJob(Job *free_job);

/**
 * @brief Obtiene un job a partir de su ID.
 * @param id id del job a obtener.
 * @return Job* job solicitado. en caso de no existir job* = NULL
 */
Job* findJobByID(int id_job);

/**
 * @brief obtiene el ultimo job de la lista.
 * @return Job* ultimo job
 */
Job* findLastJob();

/**
 * @brief obtiene el padre de un job dado.
 * @param id_job id del job al cual encontar el padre
 * @return Job* padre del job
 */
Job* findFather(int id_job);

/**
 * @brief Ejecuta todos los procesos de un job.
 * @param *job_launch job a ejecutar.
 */
void launchJob(Job *job_launch);

/**
 * @brief se imprimen los datos del job y sus procesos
 * @param j* job a imprimir
 */
void printJobStatus(Job *j);

/**
 * @brief Espera a la finalizacion de todos los procesos de un job.
 * @param *job_wait trabajo al cual se debe esperar.
 * @return int estado del trabajo al terminar.
 */
int jobWait(Job *job_wait);

/**
 * @brief ejecuta un nuevo proceso.
 * @param j job que contiene el proceso.
 * @param p proceso a ejecutar.
 * @return int estado del proceso ejecutado.
 */
int launchProcess(Process *p, Job *j);

/**
 * @brief Obtiene el job al cual pertenece un proceso a partir de su id de proceso.
 * @param pid id del proceso al cual buscarle su job.
 * @return job* job al cual pertenece el proceso. NULL en caso de no existir.
 */
Job *getProcessOwner(pid_t pid);

/**
 * @brief obtiiene un proceso dentro de un job a partir de un id
 * @param id id del proceso a buscar.
 * @param *j job en el cual buscar
 * @return Process* proceso obtenido. NULL en caso de no existir
 */
Process *getProcessInJob(Job *j, pid_t id);

/**
 * @brief obtiene la cantidad de procesos activos en un job
 * @param j*
 * @return int cuenta de procesos activos
 */
int getProcessCount(Job *j);

/**
 * @brief Inicializa el job control.
 */
void initJobControl();

/**
 * @brief handler de sañales
 * @param signal señal recibida.
 */
void signalHandler(int signal);

/**
 * @brief retorna si los procesos del job estan completos
 * @param *job job a revisar.
 * @return int 1 si el job termino 0 si no
 */
int jobIsComplete(Job *job);

/**
 * @brief determina si un trabajo completo su ejecucion.
 * @param *job job a consultar
 * @return int 1 si el jobe esta completo. 0 si está en ejecución
 * */

void printJobPipe(Job *job);