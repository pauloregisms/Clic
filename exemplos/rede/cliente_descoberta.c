#include "Clic.h"

int main() {
    Table *lista = Network_discoverServers();

    if (!lista) {
        return 1;
    }

    if (Network_discoveredCount(lista) == 0) {
        printf("Nenhum servidor encontrado.\n");
        Table_free(lista);
        return 0;
    }

    Table_print(lista);

    int escolha = Table_select(lista);
    Client *client = Network_connectDiscovered(lista, escolha);

    Table_free(lista);

    if (!client) {
        return 1;
    }

    printf("Conectado a %s:%d\n",
           Client_getIP(client),
           Client_getPort(client));

    Client_free(client);
    return 0;
}
