#include "jobsControl.h"

Job *head_job = NULL;
const char* status_string[] = {"done", "runnig", "new" };

void addJob(Job *new_job)
{
    /*algoritmo básico de adicion de elemento a una lista */
    new_job->next =  NULL;
    Job *last = findLastJob();

    if(!last) //si no hay ningun elemento, este será la cabecera
    {
        new_job->job_id = 1;
        head_job = new_job;
    }
    else //se linquea al next del ultimo elemento
    {
        new_job->job_id = last->job_id + 1;
        last->next = new_job;
    }
}

int removeJob(int id_remove)
{
    /*algoritmo básico de remocion de elemento de una lista */
    Job *job_remove = findJobByID(id_remove);
    if(job_remove == NULL)
        return JOBS_JOBNOTFUND;
    if(head_job == job_remove)     //si el elemento a remover es la cabeza
        head_job = head_job->next; //convertir al siguiente a la cabeza
    else
    {
        Job *father = findFather(job_remove->job_id);  //buscar el padre del elemento a remover
        father->next = job_remove->next; //unir el padre con el siguiente del elemento a eliminar
    }
    freeJob(job_remove);
    return JOBS_JOBREMOVED;
}

void freeJob(Job *job_free)
{
    /*antes de liberar el job hace falta liberar la memoria alocada
    a los procesos que contiene*/

    Process *p = job_free->head_process;
    while (p)
    {
        Process *aux_next = p->next;
        for(int i = 0; i < p->argc; i++)
            free(p->argv[i]);
        free(p->argv);
        free(p);
        p = aux_next;
    }
    free(job_free); //ahora si se libera la memoria del job
}


Job* findJobByID(int id_job)
{
    /*recorrer la lista hasta que coincida con el id y retornar el job*/
    for(Job* j= head_job; j ; j = j->next)
        if(j->job_id == id_job)
            return j;
    return NULL;
}

Job* findLastJob()
{
    /*recorrer la lista hasta que no haya elemento siguiente y retornar*/
    Job *last = head_job;
    if (last == NULL) return NULL;

    while (last->next)
        last = last->next;

    return last;
}

Job* findFather(int id_job)
{
    /*recorrer la lista hasta encontrar que el siguiente al elemento
    es el cual al que encontrarle el padre, se retorna el elemento*/
    for(Job* j= head_job; j ; j = j->next)
        if(j->next->job_id == id_job)
            return j;
    return NULL;
}


void launchJob(Job *job_launch)
{
    int status;
    addJob(job_launch);
    if(pipe(job_launch->pipe_io) < 0 || pipe(job_launch->pipe_err)< 0)
        exit(EXIT_FAILURE); //error en las pipes
    for(Process *p = job_launch->head_process; p; p = p->next)
        status = launchProcess(p,job_launch); //se lanzan todos los procesos
    if(status >= 0 && job_launch->execution_mode == JOBS_AHEAD)
        removeJob(job_launch->job_id); //si ya no quedan procesos en ejecucion se elimina el job
    else if(job_launch->execution_mode == JOBS_BEHIND)
        printJobStatus(job_launch); //se imprime el job status
}

void printJobStatus(Job *j)
{
    printf("\n[%d]", j->job_id);
    for(Process *p = j->head_process; p; p = p->next)
        printf(" %s %d %s",status_string[p->status],p->id,p->argv[0]);
    printf("\n");
}

int jobWait(Job *job_wait)
{
    int active_process = getProcessCount(job_wait);
    int wait_count = 0;
    //se esperan todos los procesos del grupo
    do
    {
        int status;
        int pid = waitpid(-job_wait->pgid,&status, WUNTRACED);
        if(pid < 0) return -1;
        wait_count++;
        if(WIFEXITED(status))
        {
            //cuando un proceso termina le cambiamos el valor en la estructura
            Process *p = getProcessInJob(job_wait, pid);
            p->status = PROCESS_DONE;
        }
    } while (wait_count < active_process);
    return 0;
}

int launchProcess(Process *p, Job *j)
{
    int status;
    p->status = PROCESS_RUNNING;
    pid_t child = fork(); //se inicia proceso hijo
    if(child == -1) //error en la creación
    {
        fprintf(stderr,"\033[0;31m"); //rojo
        fprintf(stderr,"Error al crear proceso hijo");
        fprintf(stderr,"\033[0;37m"); //blanco
        exit(EXIT_FAILURE);
    }
    else if(child == 0) //el proceso hijo
    {
        /*se le asignan las señales con comportamiento
        por defecto al proceso hijo*/
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        /*configuración de pipes entre proceso hijo
        y padre*/
        close(j->pipe_io[0]);
        close(j->pipe_err[0]);
        dup2(j->pipe_io[1], STDOUT_FILENO);
        dup2(j->pipe_err[1],STDERR_FILENO);
        close(j->pipe_io[1]);
        close(j->pipe_err[1]);

        //ejecución del comando
        if(execvp(p->argv[0],p->argv) < 0)
        {
            fprintf(stderr,"\033[0;31m"); //rojo
            fprintf(stderr,"No se puede ejecutar ese proceso");
            fprintf(stderr,"\033[0;37m"); //blanco
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else
    {
        p->id = child; //se guarda el id del proceso hijo
        if(j->pgid < 1)
            j->pgid = p->id; //se establece el group id con el id del primer proceso
        setpgid(p->id,j->pgid); //se setea el group id

        /*Si el job se tiene que ejecutar en primer plano
        se debe esperar a que todos los procesos asociados al job
        terminen*/
        if(j->execution_mode == JOBS_AHEAD)
        {
            tcsetpgrp(0, j->pgid);
            status = jobWait(j); //esperar a todos los procesos
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(0, getpid());
            signal(SIGTTOU, SIG_DFL);
            printJobPipe(j); //imprimir los valores guardados en la pipe
        }
    }
    return status;
}

Process *getProcessInJob(Job *j, pid_t id)
{
    /*Se recorre la lista de procesos hasta que coincida un id*/
    for(Process *p = j->head_process; p; p = p->next)
        if(p->id == id)
            return p;
    return NULL;
}

int getProcessCount(Job *j)
{
    /*Cuando un proceso está en estado DONE se suma a la cuenta*/
    int count = 0;
    for(Process *p = j->head_process; p; p = p->next)
        if(p->status != PROCESS_DONE)
            count++;
    return count;
}

Job *getProcessOwner(pid_t pid)
{
    /*Se recorre la lista de jobs y a cada job se le recorre
    la lista de procesos*/
    for(Job *j = head_job; j; j = j->next)
        for(Process *p = j->head_process; p; p = p->next)
            if(p->id == pid)
                return j;
    return NULL;
}

void initJobControl()
{
    /*es necesario activar la captura de señales de los procesos
    hijos*/
    struct sigaction sigint_action = {
            .sa_handler = signalHandler,
            .sa_flags = 0
    };
    sigemptyset(&sigint_action.sa_mask);
    sigaction(SIGCHLD, &sigint_action, NULL);

    pid_t pid = getpid();
    setpgid(pid, pid);
    tcsetpgrp(0, pid);
}

void signalHandler(int signal) //handler de señales de procesos hijos, asi determinar cuando finalizan
{
    int status;
    pid_t pid;

    //wait no bloqueante de procesos hijos
    while ((pid = waitpid(WAIT_ANY, &status, WNOHANG|WUNTRACED|WCONTINUED)) > 0)
    {
        Job *job_owner = getProcessOwner(pid);
        if(WIFEXITED(status))
        {
            Process *p = getProcessInJob(job_owner, pid);
            p->status = PROCESS_DONE;
        }
        //cuandos se termina el job se hace el print del status, del contenido de los pipe y se elimina
        if(jobIsComplete(job_owner) == JOBS_DONE)
        {
            printJobStatus(job_owner);
            printJobPipe(job_owner);
            removeJob(job_owner->job_id);
        }
    }
}

int jobIsComplete(Job *job)
{
    for(Process *p = job->head_process; p; p = p->next)
        if(p->status != PROCESS_DONE)
            return JOBS_RUNNIG; //si un solo proceso todavia esta en ejecucion en
    return JOBS_DONE;
}

void printJobPipe(Job *job)
{
    char c;

    close(job->pipe_io[1]);
    close(job->pipe_err[1]);
    fcntl(job->pipe_io[0], F_SETFL, fcntl(job->pipe_io[0], F_GETFL) | O_NONBLOCK);
    fcntl(job->pipe_err[0], F_SETFL, fcntl(job->pipe_err[0], F_GETFL) | O_NONBLOCK);

    while (read(job->pipe_err[0], &c, sizeof(c)) > 0)
        printf("%c",c);

    while (read(job->pipe_io[0], &c, sizeof(c)) > 0)
        printf("%c",c);

    close(job->pipe_io[0]);
    close(job->pipe_err[0]);
}
