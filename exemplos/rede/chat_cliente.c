#include "Clic.h"

int main(int argc, char *argv[]) {
    char *ip = argc > 1 ? argv[1] : "127.0.0.1";
    Client *client = Client_connect(ip, 5000);

    if (!client) {
        perror("Não foi possível conectar");
        return 1;
    }

    printf("Conectado a %s:%d.\n", Client_getIP(client), Client_getPort(client));

    while (true) {
        char mensagem[Client_MESSAGE_SIZE];

        printf("> ");

        if (!fgets(mensagem, sizeof(mensagem), stdin)) {
            break;
        }

        if (Client_sendLine(client, mensagem) < 0) {
            break;
        }

        if (Client_receiveLine(client, mensagem) < 0) {
            break;
        }

        printf("Servidor: %s\n", mensagem);
    }

    Client_free(client);
    return 0;
}
