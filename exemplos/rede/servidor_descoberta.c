#include "Clic.h"

int main() {
    Server *server = Server_create(5000);

    if (!server) {
        return 1;
    }

    Server_setName(server, "Laboratório 01");
    Server_enableDiscovery(server);

    while (true) {
        char mensagem[Client_MESSAGE_SIZE];
        Client *origem = Server_receiveLine(server, mensagem);

        if (origem) {
            Server_sendAll(server, mensagem);
            Server_sendAll(server, "\n");
        }
    }
}
