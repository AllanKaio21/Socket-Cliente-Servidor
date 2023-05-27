//Código para compilar e executar: g++ -pthread servidor.cpp -o servidor && ./servidor
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>

/* Server port  */
#define PORT 4242

/* Buffer length */
#define BUFFER_LENGTH 4096

/* Maximo de clientes conectados */
#define MAX 100

/* Maximo de mensagens que podem ser produzidas */
#define MAX_MESSAGE 5

/* Tamanho maximo de um apelido */
#define SURNAME_MAX 10

/* Client and Server socket structures */
struct sockaddr_in client, server;

/* Semafaros: prod -> produtor, cons -> consumidor */
sem_t prod, cons;

/* Variavel de trava para alteração do buffer */
pthread_mutex_t mutex; 

/* Buffer que armazena proxima mensagem a ser enviada */
char buffer[BUFFER_LENGTH][MAX_MESSAGE], buffer_in[BUFFER_LENGTH];

/* Apelido do usuario que mandou a mensagem */
char surname[SURNAME_MAX][MAX];

/* Declaração das threads produtor e consumidor */
pthread_t threadProducer[MAX], threadConsumer;

/** Quantidade de clientes, 
*   Lista de clientes connectados,
*   id do cliente que digitou ultima mensagem,
*   Quantidade de mensagens no buffer
* */
int clientsQtd = 0, clientFd[MAX], clientId[MAX], msg_qtd = 0;

/* Função para separar dados das mensagens de acordo com delimitador definido */
char *param1, *param2, *param3, *param4;
void split(char str[100], char delim[]){
	int init_size = strlen(str), i = 0;
	char *ptr = strtok(str, delim);

	while(ptr != NULL)
	{
        if(i == 0){
            param1 = ptr;
        }else if(i == 1){
            param2 = ptr;
        }else if(i == 2) {
            param3 = ptr;
        }else if(i == 3) {
            param4 = ptr;
        }
        i++;
		ptr = strtok(NULL, delim);
	}
}

/* Remove cliente */
void rmClientFd(){ 
    int i, j;
    for (i = 0; i < clientsQtd; i++) {
        if (clientFd[i] == clientId[msg_qtd-1]) {
            clientFd[i] = -1;
            break;
        }
    }
}

/* Retorna o tipo da mensagem digitada */
int typeMsg(char *tipo){
    if (strcmp(tipo, "msg_cliente") == 0)
        return 1;
    else if (strcmp(tipo, "usuario_entra") == 0)
        return 2;
    else if (strcmp(tipo, "usuario_sai") == 0)
        return 3;
    else
        return 4;

}

/* Replica a mensagem para os clientes */
void reply() {
    for(int i = 0; i < clientsQtd; i++){
        if (clientFd[i] != clientId[msg_qtd-1] && clientFd[i] != -1)
            send(clientFd[i], buffer[msg_qtd-1], strlen(buffer[msg_qtd-1]), 0);
    }
}

/* Cria a mensagem a ser replicada aos clientes */
void createMSG(int type, char response[BUFFER_LENGTH], int fdClient){
    char msg[BUFFER_LENGTH];
    switch(type){
        case 1: 
            sprintf(msg, "\033[0;36m%s enviou: \033[0;37m", surname[fdClient]);
            strcat(msg, param3);
            msg[strlen(msg) - 1] = '\0';
            break;
        case 2:
            msg[strlen(msg) - 1] = '\0';
            sprintf(msg, "\033[0;32m%s se entrou na sala de conversa.", param3);
            strcpy(surname[fdClient], param3);
            break;
        case 3:
            msg[strlen(msg) - 1] = '\0';
            sprintf(msg, "\033[0;31m%s saiu da sala de conversa.", surname[fdClient]);
            break;
    }
    memset(response, 0x0, BUFFER_LENGTH);
    strcpy(response, msg);
}

/* Replicação da mensagem para todos os clientes */
int process(char response[BUFFER_LENGTH], int fdClient) {
    /* Separa a mensagem e salva os paramentros */
    char delim[] = "|";
    split(response, delim);
    int type = typeMsg(param2);
    switch(type){
        case 1:
            createMSG(1, response, fdClient);
            break;
        case 2:
            createMSG(2, response, fdClient);
            break;
        case 3: 
            createMSG(3, response, fdClient);
            break;
        default: printf("\nNão reconheço esse tipo!");
    }

    return type;
}

/* Thread Produotora de novas mensagens recebidas */
void* producer(void* arg){
    /* Recebo o clientFd como parametro da Thread */
    int fdClient = *(int*)arg;
    char response[BUFFER_LENGTH];

    /* Envio da mensagem de boas vindas ao cliente */
    strcpy(response, "bom|msg_servidor|Olá! Seja bem-vindo!|eom\0");
    send(fdClient, response, strlen(response), 0);
    memset(response, 0x0, BUFFER_LENGTH);
    
    /* Recebe novas mensagens do cliente */
    int message_len;
    while(1){
        memset(response, 0x0, BUFFER_LENGTH);
        if((message_len = recv(fdClient, response, BUFFER_LENGTH, 0)) > 0) {
            /* Decrementa -1 do produtor */
            sem_wait(&prod);
            /* Processa o comando */
            int type = process(response, fdClient);

            /* Trava para alteração do buffer em região critica */
            pthread_mutex_lock(&mutex);

            memset(buffer[msg_qtd], 0x0, BUFFER_LENGTH);
            strcpy(buffer[msg_qtd], response);
            clientId[msg_qtd++] = fdClient;

            /* Libera mutex */
            pthread_mutex_unlock(&mutex);

            /* Incrementa +1 para ativar o consumidor */
            sem_post(&cons);

            /* Retorno verdadeiro para fechar thread produtor */
            if (type == 3) {
                /* Seto cliente como -1 para identificar sua saída */
                rmClientFd();
                break;
            }
        }
    }
    /* Finaliza a thread */
    pthread_exit((void*) 0);
}

/* Thread Consumidora de novas mensagens recebidas */
void* consumer(void* arg){
    /* Recebo o clientFd como parametro da Thread */
    int serverFd = *(int*)arg;

    while(1) {
        /* Trava até consumir todas as mensagens */
        pthread_mutex_lock(&mutex);

        for(int i = 0; i < msg_qtd; i++){
            /* Decrementa -1 em consumo */
            sem_wait(&cons);

            /* Realiza o consumo da mensagem */
            printf("\n%s", buffer[msg_qtd-1]);
            fflush(stdout);
            memset(buffer_in, 0x0, BUFFER_LENGTH);
            strcpy(buffer_in, "bom|msg_servidor|");
            strcat(buffer_in, buffer[i]);
            strcat(buffer_in, "|eom");
            strcpy(buffer[i], buffer_in);
            reply();
            memset(buffer[i], 0x0, BUFFER_LENGTH);

            /* Incrementa +1 ao produtor */
            sem_post(&prod);
        }
        
        msg_qtd = 0;

        /* Libera trava */
        pthread_mutex_unlock(&mutex);
    }

    /* finaliza thread */
    pthread_exit((void*) 0);
}

/*
 * Main execution of the server program of the simple protocol
 */
int main(void) {
    /* File descriptors of client and server */
    int serverfd;

    fprintf(stdout, "\033[0;33m Iniciando servidor!\n");

    /* Creates a IPv4 socket */
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverfd == -1) {
        perror("Can't create the server socket:");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "\033[0;33m Server socket created with fd: %d\n", serverfd);


    /* Defines the server socket properties */
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    memset(server.sin_zero, 0x0, 8);

    /* Handle the error of the port already in use */
    int yes = 1;
    if(setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,
                  &yes, sizeof(int)) == -1) {
        perror("Socket options error:");
        return EXIT_FAILURE;
    }

    /* bind the socket to a port */
    if(bind(serverfd, (struct sockaddr*)&server, sizeof(server)) == -1 ) {
        perror("Socket bind error:");
        return EXIT_FAILURE;
    }

    /* Starts to wait connections from clients */
    if(listen(serverfd, 1) == -1) {
        perror("Listen error:");
        return EXIT_FAILURE;
    }

    fprintf(stdout, "\033[0;33m Listening on port %d\n", PORT);
    fprintf(stdout, "\033[1;33m -== Servidor rodando esperando clientes =--\n");

    /* Inicialização do semafaro */
    sem_init(&cons, 0, 0);
	sem_init(&prod, 0, MAX_MESSAGE);

    /* Inicializa a thread consumidor */
    if (pthread_create(&threadConsumer, NULL, consumer, &serverfd) != 0)
    {
        printf("Erro na Thread consumidor\n");
        return EXIT_FAILURE;
    }

    while(1){
        /* Aceita novos clientes no servidor */
        socklen_t client_len = sizeof(client);

        if ((clientFd[clientsQtd] = accept(serverfd,
            (struct sockaddr *) &client, &client_len)) == -1) {
            perror("\nErro ao aceitar cliente: ");
            return EXIT_FAILURE;
        }

        /* Inicializa a thread produtor para cada novo cliente */
        if (pthread_create(&threadProducer[clientFd[clientsQtd]], NULL, producer, &clientFd[clientsQtd]) != 0)
        {
            printf("Erro na Thread produtor\n");
            return EXIT_FAILURE;
        }

        /* Incrementa +1 a quantidade de clientes conectados */
        clientsQtd++;

        /* Para de receber clientes caso atingir maxímo permitido*/
        while(clientsQtd == MAX-serverfd);
    }
    
    /* Close the local socket */
    close(serverfd);

    printf("Connection closed\n\n");

    return EXIT_SUCCESS;
}
