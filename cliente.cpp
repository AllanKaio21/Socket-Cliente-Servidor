//Código para compilar e executar:  g++ cliente.cpp -o cliente && ./cliente
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

/* Sockets buffers length */
#define LEN 4096

/* Tamanho maximo do apelido */
#define SURNAME_MAX 10

/* Apelido do usuario */
char surname[SURNAME_MAX];

/* Client file descriptor for the local socket */
int sockfd;

/* Receive buffer */
char buffer_in[LEN];

/* Send buffer */
char buffer_out[LEN];

/* Função para separar dados das mensagens de acordo com delimitador definido */
char *param1, *param2, *param3, *param4;
void split(char str[100], char delim[]){
	int init_size = strlen(str), i=0;
	char *ptr = strtok(str, delim);
    char aux[LEN];

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

/* Trata as mensagens recebidas do servidor */
void handle() {
    /* Separa a mensagem e salva os paramentros */
    char delim[] = "|";
    split(buffer_in, delim);
    /* mostra a mensagem ao cliente */
    printf("%s\n", param3);
    /* Zera o buffer de entrada */
    memset(buffer_in, 0x0, LEN);
}

/* Thread responsavel por receber mensagens do servidor */
void* receive(void* arg){
    int slen;
    while(1){
        /* Recebe a mensagens do servidor */
        if ((slen = recv(sockfd, buffer_in, LEN, 0)) > 0) {
            buffer_in[slen + 1] = '\0';
            /* Realiza o tratamento da mensagem */
            handle();
            printf("\n\033[1;37m>> ");
            fflush(stdout);
        }
    }
}

/*
 * Main execution of the client program of our simple protocol
 */
int main(int argc, char *argv[]) {

    /* Server socket */
    struct sockaddr_in server;

    int len = sizeof(server);
    int slen;

    /* Receive buffer */
    char buffer_in[LEN];
    /* Send buffer */
    char buffer_out[LEN];

    fprintf(stdout, "\033[0;33mIniciando Cliente ...\n");

    /*
     * Creates a socket for the client
     */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error on client socket creation:");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "\033[0;33mClient socket created with fd: %d\n", sockfd);

    /* Variaveis de ip e porta do servidor */
    char SERVER_ADDR[30], PORTS[5];
    int PORT;
    /* Pega ip e porta digitadas pelo usuario */
    printf("\n\033[0;32mDigite o endereço do servidor: \033[0;37m");
    fgets(SERVER_ADDR, 30, stdin);
    printf("\n\033[0;32mDigite a porta do servidor: \033[0;37m");
    fgets(PORTS, 5, stdin); 
    getchar();
    PORT = atoi(PORTS);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    memset(server.sin_zero, 0x0, 8);

    /* Tenta se conectar ao servidor com os dados recebidos */
    if (connect(sockfd, (struct sockaddr*) &server, len) == -1) {
        perror("Can't connect to server");
        return EXIT_FAILURE;
    }

    /* Recebe a mensagem de apresentação do servidor */
    if ((slen = recv(sockfd, buffer_in, LEN, 0)) > 0) {   
        buffer_in[slen + 1] = '\0';
        char delim[] = "|";
        split(buffer_in, delim);
        fprintf(stdout, "\n\033[0;36mServer: %s\n", param3);
        memset(buffer_in, 0x0, LEN);
    }

    /* Pega o apelido e envia para enviar ao servidor */
    printf("\n\033[0;36mDigite um apelido: \033[0;37m");
    memset(buffer_in, 0x0, LEN);
    memset(buffer_out, 0x0, LEN);
    fgets(surname, SURNAME_MAX, stdin);

    /* Monta o mensagem para o servidor de entrada de novo usuario */
    strcpy(buffer_out, "bom|usuario_entra|");
    strcat(buffer_out, surname);
    buffer_out[strlen(buffer_out) - 1] = '\0';
    strcat(buffer_out, "|eom");

    /* Envia mensagem para o servidor tratar */
    send(sockfd, buffer_out, strlen(buffer_out), 0);

    /* Declaração da thread receive */
    pthread_t threadReceive;

    /* Inicializa a thread receive */
    if (pthread_create(&threadReceive, NULL, receive, NULL) != 0)
    {
        printf("Erro na Thread de receber!\n");
        return EXIT_FAILURE;
    }

    /* Buffer temporario para guarda mensagens digitadas */
    char msg[LEN];

    while (true) {
        /* Zera o buffer de saida e de mensagens */
        memset(buffer_out, 0x0, LEN);
        memset(msg, 0x0, LEN);

        /* Pega texto digitado pelo cliente */
        printf("\n\033[1;37m>> ");
        fgets(msg, LEN, stdin);
        printf("\033[1A");
        printf("\033[K");
        printf("\033[0;34myou: \033[0;37m%s", msg);

        /* "tchau" fecha a conexão com o servidor */
        if(strcmp(msg, "tchau\n") == 0){
            /* Cração do comando para o servidor */
            strcpy(buffer_out, "bom|usuario_sai|");
            strcat(buffer_out, msg); 
            strcat(buffer_out, "|eom");
            /* Envio da mensagem para o servidor */
            send(sockfd, buffer_out, strlen(buffer_out), 0);
            /* Fechamento da thread que recebe mensagens do servidor */
            pthread_cancel(threadReceive);
            break;
        }else {
            /* Cração do comando para o servidor */
            strcpy(buffer_out, "bom|msg_cliente|");
            strcat(buffer_out, msg);
            strcat(buffer_out, "|eom");
            /* Envio da mensagem para o servidor */
            send(sockfd, buffer_out, strlen(buffer_out), 0);
        }
    }
    /* Close the connection with the server */
    close(sockfd);

    fprintf(stdout, "\n\033[1;31mConexão fechada!\n\n");

    return EXIT_SUCCESS;
}
