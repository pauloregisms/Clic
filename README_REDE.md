# Comunicação em rede

A API TCP/IPv4 usa o mesmo modelo de funções e estruturas da Clic e depende
somente de POSIX. Os exemplos ficam em `exemplos/rede/`.

```sh
make examples
# ou, na raiz do repositório:
gcc -std=gnu11 -Wall -Wextra -I. Clic.c exemplos/rede/chat_servidor.c -o servidor
gcc -std=gnu11 -Wall -Wextra -I. Clic.c exemplos/rede/chat_cliente.c -o cliente
```

Execute servidor e cliente em terminais separados. Os exemplos de chat usam a
porta 5000. `Server_create(0)` escolhe uma porta livre, disponível em `server->port`.
A conexão e os envios são síncronos; um destinatário lento pode bloquear um envio.
`Client_ping` abre outra conexão TCP: não mede ICMP e pode gerar um novo evento de
conexão no servidor. O limite de dois segundos cobre a conexão TCP de cada endereço,
não a resolução de nomes. Objetos e callbacks devem ser usados por uma única thread.

## Recebimento e propriedade

Use sempre `char mensagem[Client_MESSAGE_SIZE]`, com 256 bytes. As funções de
recebimento terminam a saída com NUL. `Client_send` envia uma string, não dados
binários com NUL embutido. `Client_receive` retorna a quantidade de bytes recebidos;
TCP não preserva limites entre envios. Prefira `sendLine`/`receiveLine` para texto.

| Retorno de `Client_receive*` | Significado |
| --- | --- |
| Positivo | Quantidade de bytes entregues |
| `0` | Linha vazia válida em `receiveLine*` |
| `Client_NO_DATA` (`-2`) | Sem dados ou linha incompleta, apenas nas variantes não bloqueantes |
| `-1` | Erro ou desconexão; consulte `errno` quando aplicável |

Este contrato corrige a ambiguidade do ZIP original, que usava zero para ausência
de dados e para linha vazia. Uma linha remove LF/CRLF; até 255 bytes são entregues.
Linhas maiores são consumidas inteiras e truncadas na saída. Uma linha que complete
4095 bytes sem LF excede a capacidade interna: retorna erro `EMSGSIZE` e desconecta,
em vez de descartar silenciosamente o começo da mensagem. O limite de entrega é em
bytes; a API não garante que truncamento de mensagens preserve caracteres UTF-8.
O desenho de tabelas continua usando a implementação Unicode existente da biblioteca.

```c
char mensagem[Client_MESSAGE_SIZE];
int n = Client_receiveLineNonBlocking(client, mensagem);
if (n >= 0) {
    printf("Recebida (%d bytes): %s\n", n, mensagem);
} else if (n == -1) {
    /* Encerrar ou tratar o erro. Client_NO_DATA apenas pede outra consulta. */
}
```

`Server_receive*` retorna a origem da mensagem ou `NULL`. Uma linha vazia também
retorna a origem e aciona o callback. As variantes bloqueantes aceitam conexões
automaticamente e esperam uma mensagem; as outras voltam sem esperar.
Mensagens completas já no buffer são processadas antes de consultar os sockets.

Clientes criados por `Client_connect`/`Network_connectDiscovered` devem ser liberados
com `Client_free`. Clientes aceitos pertencem ao servidor: **não os libere**.
`Client_disconnect` pode desconectá-los; a próxima limpeza do servidor os libera.
Ponteiros retornados pelo servidor e pelos getters são emprestados e não devem ser
usados depois da remoção do cliente ou do fechamento do servidor.

Callbacks podem consultar o servidor e desconectar clientes. Não devem chamar
`Client_free` em clientes aceitos nem fechar o próprio servidor. O buffer do callback
vale durante a chamada. Os broadcasts não acrescentam `\n`: envie o delimitador.
`Server_send*` é uma conveniência sem retorno de status; use `Client_send` quando
precisar verificar o resultado de envio de um destinatário.

## Descoberta

`Server_enableDiscovery` habilita respostas UDP na porta 39000. O servidor precisa
executar `Server_update`, `Server_accept*` ou `Server_receive*` para responder.
`Network_discoverServers` consulta broadcast e loopback por até um segundo e retorna
uma tabela que deve ser liberada com `Table_free`; `NULL` indica falha de operação.
Uma tabela sem endpoints representa ausência de respostas.

O cabeçalho não é selecionável; `Network_connectDiscovered` recebe o índice de linha
retornado por `Table_select`. Endpoints são identificados pelo par IP/porta: nomes
iguais em máquinas diferentes não são descartados. As tabelas mantêm suporte UTF-8,
larguras fixas e recorte Unicode. A descoberta não fornece autenticação; respostas
anunciam endpoints e o aplicativo decide a qual se conectar.

## Testes

```sh
make test
make test-sanitize
```

A suíte inclui testes unitários dos buffers, formatação Unicode e metadados, além
de integração TCP/UDP em loopback. Cada caso executa em processo separado com limite
de cinco segundos e as falhas propagam um status diferente de zero. Não requer
acesso à Internet. A descoberta UDP é exercitada diretamente em loopback, sem
broadcast na rede local. Não execute outra instância usando a porta UDP 39000
simultaneamente. Os testes de descritores elevados exigem limite de arquivos acima
de 1024 (`ulimit -n`); o runner falha explicitamente se essa capacidade faltar.

O alvo de sanitizadores usa GCC/Clang com ASan e UBSan, em diretório de saída separado.
Foi validado em Linux; macOS e descoberta entre máquinas não foram validados aqui.

`make test-legacy` compara os oito exemplos originais com o commit `41c095e`,
anterior à extensão de rede, em pseudoterminais 80×24. Requer Python 3 e esse commit
no histórico Git local (`CLIC_BASELINE_REF` permite selecionar outra referência).
Sete exemplos devem produzir saída idêntica; na animação, cuja quantidade de frames
varia com o escalonamento, verifica o encerramento por teclado. A compilação de
ambas as versões usa `-Werror`.
