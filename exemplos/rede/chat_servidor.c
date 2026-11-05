#include "Clic.h"

int main() {
    Server *server = Server_create(5000);

    if (!server) {
        perror("Não foi possível iniciar o servidor");
        return 1;
    }

    printf("Servidor ativo na porta %d.\n", server->port);

    while (true) {
        char mensagem[Client_MESSAGE_SIZE];

        Client *origem = Server_receiveLine(server, mensagem);

        if (origem) {
            printf("%s: %s\n", Client_getIP(origem), mensagem);
            Server_sendAll(server, mensagem);
            Server_sendAll(server, "\n");
        }
    }

    Server_close(server);
    return 0;
}
