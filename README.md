# Socket-Cliente-Servidor
Trabalho de Sistemas Operacionais. Implementação de socket para receber e enviar comandos de mensagens, utilizando exclusão mutua e realizando a sincronização de threads produtora e consumidora com semáforos.

# Compilar e executar Servidor 
g++ -pthread servidor.cpp -o servidor && ./servidor

# Somente executar servidor
./servidor

# Compilar e executar Cliente
g++ -pthread cliente.cpp -o cliente && ./cliente

# Somente executar cliente
./cliente

# Endereço e Porta do servidor
Endereço servidor: 127.0.0.1
Porta: 4242

# Descrição
O servidor suporta até 100 clientes conectados, podendo ser alterado a quantidade maxima no em "MAX". Assim como a quantidade de mensagens que podem ser armazenadas no buffer na definição "MAX_MESSAGE".

# Cliente
O cliente envia comandos estruturados da seguinte forma:

    # 1 Usuario entra
    "bom|usuario_entra|{APELIDO}|eom" -> Sendo o priemiro comando enviado do cliente ao servidor, definindo seu nome de usuario na thread produtora aberta para este cliente.

    # 2 Mensagem cliente
    "bom|msg_cliente|{Ex.: Ola!}|eom" -> Este comando é enviado ao servidor para ser replicado aos demais clientes, apos ser processado, ficando: "Cliente diz: Ola".

    # 3 Usuario Sai
    "bom|usuario_sai|tchau|eom" -> Comando que sinaliza a saida do cliente do servidor, finalizando sua thread produtora e o retirando da lista de clientes conectados.

# Servidor
O servidor além de tratar os comandos do cliente, envia apenas um tipo de comando estruturado da seguinte forma:

    # 1 Mensagem servidor
    "bom|msg_servidor|Cliente diz: Ola!|eom" -> Este comando é sempre enviado para os clientes no final do processamento de um comando. Sendo feito pela thread consumidora.

# Aviso: Se usar é não der star vai reprovar em SO ;D