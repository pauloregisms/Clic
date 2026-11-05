#include "Clic.h"

void quandoConectar(Client *client) {
    printf("Novo participante: %s:%d\n",
           Client_getIP(client),
           Client_getPort(client));
}

void quandoDesconectar(Client *client) {
    printf("Saiu: %s\n", Client_getIP(client));
}

void quandoReceber(Client *client, char *mensagem) {
    printf("[%s] %s\n", Client_getIP(client), mensagem);
}

int main() {
    Server *server = Server_create(5000);

    if (!server) {
        return 1;
    }

    Server_onConnect(server, quandoConectar);
    Server_onDisconnect(server, quandoDesconectar);
    Server_onMessage(server, quandoReceber);

    while (true) {
        char mensagem[Client_MESSAGE_SIZE];
        Client *origem = Server_receiveLine(server, mensagem);

        if (origem) {
            Server_sendOthers(server, origem, mensagem);
            Server_sendOthers(server, origem, "\n");
        }
    }
}
