# Clic: interfaces de terminal em C

A **Clic** é uma biblioteca em C para construir interfaces de terminal com cores, posicionamento do cursor, linhas, caixas, leitura de teclas e tabelas. Pode ser usada em menus, pequenos jogos, exercícios de programação e programas que precisam apresentar informações em uma posição específica da tela.

**Autor da biblioteca:** Paulo Regis M. Sousa.  
**Repositório:** [pauloregismsuva/Clic](https://github.com/pauloregismsuva/Clic).  
**Licença da biblioteca:** [MIT](LICENSE).

Esta documentação descreve as **40 funções públicas declaradas em `Clic.h`**, incluindo `Clic_textWidth()`, e o suporte a largura visual UTF-8 atualizado em **21/09/2026**. A atualização foi preparada sobre o commit `14a8befe0a03283e87e91603e8ced70e9a537888`. O guia acompanha os arquivos desta revisão; os exemplos estão em `exemplos/`.

## Sumário

- [Requisitos e organização](#requisitos)
- [Primeiro programa e compilação](#inicio)
- [Convenções de uso](#convencoes)
- [Exemplos completos](#exemplos)
- [Cores e símbolos](#constantes)
- [Tela, cursor e desenho](#tela)
- [Teclado e temporização](#teclado)
- [Tabelas e seleção de linhas](#tabelas)
- [Tipos e estruturas](#tipos)
- [Limitações da versão documentada](#limitacoes)
- [Dúvidas frequentes](#duvidas)
- [Fontes](#fontes)

<a id="requisitos"></a>
## 1. Requisitos e organização

A implementação usa sequências de escape ANSI/VT e funções de ambiente POSIX, incluindo `termios`, `ioctl`, `fcntl` e `usleep`.

| Requisito | Orientação |
| --- | --- |
| Compilador | Use GCC em modo GNU C11 nos comandos apresentados. |
| Sistema | A implementação é voltada a terminais POSIX. Os exemplos foram verificados em Linux. |
| Terminal | Execute em um terminal interativo com suporte a sequências ANSI/VT e cores de 256 índices. |
| Caracteres | Use terminal e fonte compatíveis com UTF-8 para visualizar bordas e símbolos. |
| Arquivos | Inclua `Clic.h` e compile também `Clic.c`. A biblioteca não é composta apenas pelo cabeçalho. |
| Bibliotecas adicionais | Os comandos apresentados não exigem `ncurses` nem opções extras de vinculação. |

No Windows, o código deste commit **não compila diretamente no ambiente nativo de MSVC/MinGW**, pois inclui cabeçalhos POSIX sem uma implementação alternativa para Windows. Um ambiente Linux no WSL é uma possibilidade de uso. Compatibilidade nativa com Windows e execução em macOS não foram testadas nesta documentação.

Execute os comandos apresentados a partir da raiz do repositório, onde ficam `Clic.h`, `Clic.c`, `README.md` e a pasta `exemplos/`.

| Caminho | Conteúdo |
| --- | --- |
| `Clic.h` | Declarações públicas, tipos e constantes da biblioteca. |
| `Clic.c` | Implementação das funções. |
| `LICENSE` | Licença original da biblioteca. |
| `README.md` | Este guia e a referência da API. |
| `exemplos/` | Oito programas pequenos e independentes. |
| `Makefile` | Compilação dos exemplos. |

<a id="inicio"></a>
## 2. Primeiro programa e compilação

Salve o programa abaixo como `main.c`, na mesma pasta de `Clic.h` e `Clic.c`:

```c
#include "Clic.h"

int main(void) {
    Clic_clearScreen();
    Clic_move(2, 4);
    Clic_setFontColor(Color_GREEN);
    printf("Ola! Esta interface usa a Clic.\n");
    Clic_resetColor();
    return 0;
}
```

Compile e execute:

```bash
gcc -std=gnu11 -Wall -Wextra main.c Clic.c -o programa
./programa
```

O programa limpa a tela, posiciona o cursor na **linha 2, coluna 4**, imprime uma mensagem em verde e restaura os atributos padrão do terminal.

Use `-std=gnu11` porque esta implementação contém extensões como `\e` e usa funções POSIX. A opção `-I.` nos exemplos seguintes informa ao compilador que deve procurar `Clic.h` também na pasta atual.

Para obter a biblioteca:

```bash
git clone https://github.com/pauloregismsuva/Clic.git
cd Clic
```

Os comandos de clonagem obtêm a versão publicada do repositório. Se a atualização UTF-8 estiver em uma branch separada, selecione essa branch antes de compilar. O exemplo `08_tabela_utf8.c` requer a função `Clic_textWidth()` adicionada por esta atualização.

Também é possível compilar a biblioteca separadamente e reutilizar o objeto em vários programas:

```bash
gcc -std=gnu11 -Wall -Wextra -c Clic.c -o Clic.o
gcc -std=gnu11 -Wall -Wextra -I. exemplos/01_cores.c Clic.o -o cores
./cores
```

<a id="convencoes"></a>
## 3. Convenções de uso

### Localidade UTF-8

As strings continuam sendo `char *` terminadas em `\0`. O armazenamento é medido em bytes; o desenho passa a ser medido em colunas visuais usando `mbrtowc()` e `wcwidth()`.

Antes de adicionar linhas com acentos ou outros caracteres Unicode, configure `LC_CTYPE` uma vez no início da aplicação:

```c
#include <locale.h>

/* No inicio de main(), antes das operacoes com texto. */
if (setlocale(LC_CTYPE, "") == NULL) {
    fprintf(stderr, "Nao foi possivel configurar a localidade.\n");
    return 1;
}
```

O ambiente precisa selecionar uma localidade UTF-8 instalada. Apenas o retorno não nulo de `setlocale` não garante UTF-8; o exemplo 08 também confere `Clic_textWidth("Ação") == 4`. A biblioteca não altera a localidade global por conta própria. Usar apenas `LC_CTYPE` mantém a configuração de separador decimal existente.

Na localidade `C` padrão, os textos ASCII continuam funcionando. Textos de células precisam ser imprimíveis: tabulações, quebras de linha, sequências ANSI embutidas e sequências inválidas são rejeitadas na inserção. Aplique cores pelas funções da Clic, fora do conteúdo da string. Não altere `LC_CTYPE` enquanto outra thread estiver medindo ou imprimindo texto.

Para converter um programa existente, acrescente a inicialização da localidade, recompile `Clic.c` e preserve as chamadas `Table_create`, `Table_addRow`, `Table_print` e `Table_select`.

### Coordenadas e dimensões

- As coordenadas do terminal começam em **1**. A posição `(1, 1)` é o canto superior esquerdo.
- `Clic_move(linha, coluna)` recebe primeiro a linha e depois a coluna.
- Largura é medida em colunas; altura, em linhas.
- Os tamanhos de uma caixa incluem suas bordas.
- O cursor avança ao imprimir texto. Posicione-o explicitamente antes de desenhar outro componente.

### Atualização da saída

Grande parte das funções apenas escreve sequências de controle em `stdout`. Quando o desenho não termina com uma quebra de linha, descarregue o buffer antes de esperar entrada:

```c
Clic_move(3, 5);
printf("Pressione uma tecla...");
fflush(stdout);
int tecla = Clic_keyCapture();
```

Para uma aplicação que redesenha a tela frequentemente, outra opção é desabilitar o buffer de saída **antes de qualquer escrita em `stdout`**:

```c
setvbuf(stdout, NULL, _IONBF, 0);
```

Esse cuidado é útil em `Table_select()`, pois a função desenha e espera uma tecla internamente. `Clic_pause()` já chama `fflush(stdout)` antes da espera.

### Encerramento

Ao terminar uma interface que alterou cores ou ocultou o cursor, restaure sua apresentação:

```c
Clic_resetColor();
Clic_showCursor();
Clic_move(12, 1);  /* Escolha uma linha livre no seu layout. */
fflush(stdout);
```

A biblioteca não possui funções públicas de inicialização ou encerramento global. A captura de teclas ajusta o modo do terminal em cada chamada. Uma interrupção abrupta pode impedir a restauração desse modo; consulte as [dúvidas frequentes](#duvidas).

<a id="exemplos"></a>
## 4. Exemplos completos

Os arquivos são independentes: **compile um exemplo por vez**, pois cada um possui sua própria função `main`. Os exemplos de interface verificam um terminal de pelo menos 60 colunas por 18 linhas. Para testá-los confortavelmente, use 80 colunas por 24 linhas.

| Arquivo | Demonstração | Interação |
| --- | --- | --- |
| [01_cores.c](exemplos/01_cores.c) | Cor do texto, cor do fundo e restauração dos atributos. | Termina após imprimir. |
| [02_caixa.c](exemplos/02_caixa.c) | Caixa, coordenadas, símbolo e faixa colorida. | Termina após desenhar. |
| [03_teclado.c](exemplos/03_teclado.c) | Captura bloqueante e identificação de setas. | Setas; `q` minúsculo para sair. |
| [04_menu.c](exemplos/04_menu.c) | Menu feito com vetor de strings e função de desenho. | Cima/baixo, Enter; `q` cancela. |
| [05_tabela.c](exemplos/05_tabela.c) | Tabela com texto, inteiro e ponto flutuante. | Termina após desenhar. |
| [06_selecao_tabela.c](exemplos/06_selecao_tabela.c) | Menu usando `Table_select()` e validação do retorno. | Cima/baixo e Enter. |
| [07_animacao.c](exemplos/07_animacao.c) | Bloco em movimento, pausa e entrada não bloqueante. | `q` minúsculo para sair. |
| [08_tabela_utf8.c](exemplos/08_tabela_utf8.c) | Acentos, cedilha, marcas combinantes, caracteres largos e truncamento. | Termina após desenhar. |

Exemplo de compilação a partir da raiz do repositório:

```bash
gcc -std=gnu11 -Wall -Wextra -I. exemplos/04_menu.c Clic.c -o menu
./menu
```

Para a animação:

```bash
gcc -std=gnu11 -Wall -Wextra -I. exemplos/07_animacao.c Clic.c -o animacao
./animacao
```

Os exemplos anteriores continuam funcionando com ASCII. O exemplo 08 inicializa a localidade e demonstra texto UTF-8. O exemplo de menu manual usa índices de vetor iniciados em **0**; as linhas de `Table` usam índices iniciados em **1**.

Para compilar todos os exemplos e executar a demonstração UTF-8:

```bash
make examples
./build/08_tabela_utf8
```

<a id="constantes"></a>
## 5. Cores e símbolos

### Tipo `Color`

As constantes representam índices da paleta de 256 cores enviada ao terminal. A aparência efetiva depende da paleta e do suporte do terminal.

| Constante | Índice | Cor sugerida pelo nome |
| --- | ---: | --- |
| `Color_BLACK` | 0 | Preto |
| `Color_GRAY` | 8 | Cinza |
| `Color_RED` | 9 | Vermelho |
| `Color_GREEN` | 10 | Verde |
| `Color_YELLOW` | 11 | Amarelo |
| `Color_MAGENTA` | 13 | Magenta |
| `Color_CIAN` | 14 | Ciano |
| `Color_WHITE` | 15 | Branco |
| `Color_BLUE` | 39 | Azul |
| `Color_ORANGE` | 208 | Laranja |
| `Color_LIGHTGRAY` | 248 | Cinza claro |

A grafia definida no cabeçalho é **`Color_CIAN`**.

```c
void Clic_setFontColor(Color color);
void Clic_setBackgroundColor(Color color);
void Clic_resetColor();
```

| Função | Parâmetro e efeito |
| --- | --- |
| `Clic_setFontColor` | `color` define a cor dos próximos caracteres impressos. |
| `Clic_setBackgroundColor` | `color` define o fundo das próximas células impressas. Não recolore automaticamente a tela inteira. |
| `Clic_resetColor` | Envia a sequência de restauração dos atributos gráficos do terminal, incluindo cores e outros estilos ativos. |

Todas retornam `void`. O estado permanece ativo até outra mudança ou restauração.

```c
Clic_setFontColor(Color_BLACK);
Clic_setBackgroundColor(Color_YELLOW);
printf(" Opcao selecionada ");
Clic_resetColor();
printf("\n");
```

### Constantes `Symbol_*`

Cada símbolo é uma **string**, e não um único `char`. Use `Clic_printSymbol()` ou `printf("%s", simbolo)`.

| Constante | Símbolo | Uso típico |
| --- | --- | --- |
| `Symbol_HLINE` | ━ | Linha horizontal |
| `Symbol_VLINE` | ┃ | Linha vertical |
| `Symbol_CORNER_TL` | ┏ | Canto superior esquerdo |
| `Symbol_CORNER_TR` | ┓ | Canto superior direito |
| `Symbol_CORNER_BL` | ┗ | Canto inferior esquerdo |
| `Symbol_CORNER_BR` | ┛ | Canto inferior direito |
| `Symbol_ARROW` | → | Seta para a direita |
| `Symbol_ARROW_RETURN` | ↳ | Seta de continuação |
| `Symbol_T_SIDE_L` | ┣ | Junção lateral esquerda |
| `Symbol_T_SIDE_R` | ┫ | Junção lateral direita |
| `Symbol_T_UP` | ┳ | Junção superior |
| `Symbol_T_DOWN` | ┻ | Junção inferior |
| `Symbol_CROSS` | ╋ | Cruzamento |
| `Symbol_TREE_POINTS` | … | Reticências; nome mantido como no cabeçalho |
| `Symbol_DIAG` | ╲ | Diagonal descendente para a direita |
| `Symbol_CDIAG` | ╱ | Diagonal ascendente para a direita |
| `Symbol_BLOCK` | █ | Bloco cheio |
| `Symbol_SAD` | 🙁 | Rosto triste |
| `Symbol_SMILE` | 🙂 | Rosto sorridente |

Emojis podem ocupar duas colunas ou variar conforme o terminal. Não os use para calcular alinhamentos rígidos.

<a id="tela"></a>
## 6. Tela, cursor e desenho

### 6.1. Limpeza e tamanho da tela

```c
void Clic_clearScreen();
void Clic_clearLine();
void Clic_breakLine();
int  Clic_getScreenWidth();
int  Clic_getScreenHeight();
```

| Função | Comportamento e retorno |
| --- | --- |
| `Clic_clearScreen` | Limpa a área da tela e posiciona o cursor em `(1, 1)`. Não implementa limpeza explícita do histórico de rolagem. Retorno `void`. |
| `Clic_clearLine` | Apaga toda a linha atual, mantendo a posição do cursor. Retorno `void`. |
| `Clic_breakLine` | Imprime `\n`. Retorno `void`. |
| `Clic_getScreenWidth` | Retorna o número de colunas informado por `ioctl` sobre `stdout`; retorna `1` quando a consulta falha. |
| `Clic_getScreenHeight` | Retorna o número de linhas informado por `ioctl` sobre `stdout`; retorna `1` quando a consulta falha. |

Uma consulta bem-sucedida ainda pode informar dimensão zero em um terminal sem tamanho configurado. Antes de desenhar, confirme que as dimensões comportam o layout. Redirecionar a saída para um arquivo não cria uma tela de terminal.

```c
int largura = Clic_getScreenWidth();
int altura = Clic_getScreenHeight();
if (largura < 60 || altura < 18) {
    fprintf(stderr, "Terminal pequeno para esta interface.\n");
    return 1;
}

Clic_move(4, 1);
Clic_clearLine();
printf("Linha atualizada");
```

### 6.2. Movimentação do cursor

```c
void Clic_move(int line, int column);
void Clic_moveUp(int lines);
void Clic_moveDown(int lines);
void Clic_moveRight(int columns);
void Clic_moveLeft(int columns);
void Clic_moveDownBegin(int lines);
void Clic_moveUpBegin(int lines);
void Clic_moveToColumn(int column);
void Clic_moveToBegin();
```

| Função | Efeito |
| --- | --- |
| `Clic_move(line, column)` | Vai para a posição absoluta indicada por linha e coluna. |
| `Clic_moveUp(lines)` | Sobe `lines` linhas. |
| `Clic_moveDown(lines)` | Desce `lines` linhas. |
| `Clic_moveRight(columns)` | Avança `columns` colunas. |
| `Clic_moveLeft(columns)` | Recua `columns` colunas. |
| `Clic_moveDownBegin(lines)` | Desce `lines` linhas e vai para o início da linha de destino. |
| `Clic_moveUpBegin(lines)` | Sobe `lines` linhas e vai para o início da linha de destino. |
| `Clic_moveToColumn(column)` | Vai para a coluna indicada, preservando a linha atual. |
| `Clic_moveToBegin()` | Vai para a coluna 1 da linha atual. |

Todas retornam `void`. Use posições e deslocamentos positivos. A biblioteca envia os valores ao terminal sem validar se estão dentro da tela; o tratamento de valores fora da área depende do terminal.

```c
Clic_move(2, 4);
printf("Nome:");
Clic_move(3, 4);
printf("Idade:");
Clic_moveToColumn(15);
printf("21");
```

### 6.3. Visibilidade, salvamento e consulta

```c
void Clic_saveCursorPosition();
void Clic_restoreCursorPosition();
void Clic_hideCursor();
void Clic_showCursor();
void Clic_getCursorPosition(int *row, int *col);
```

| Função | Comportamento |
| --- | --- |
| `Clic_saveCursorPosition` | Envia a sequência para salvar a posição atual. |
| `Clic_restoreCursorPosition` | Restaura a posição salva pelo terminal. |
| `Clic_hideCursor` | Oculta o cursor visível. |
| `Clic_showCursor` | Volta a exibir o cursor. |
| `Clic_getCursorPosition` | Solicita ao terminal sua posição e lê a resposta em `stdin`; grava linha em `*row` e coluna em `*col`. Cada ponteiro pode ser `NULL` para descartar aquele resultado. |

Todas retornam `void`. O salvamento não funciona como uma pilha de posições: chamadas internas de desenho de caixas e tabelas podem substituir a posição salva pela aplicação.

`Clic_getCursorPosition()` depende de o terminal responder à consulta ANSI de posição. A implementação não estabelece prazo máximo de espera nem verifica o resultado de `fscanf`. Assim, a função pode bloquear em um ambiente sem resposta adequada ou produzir valores inválidos se a resposta não corresponder ao esperado. Ela também não descarrega explicitamente a consulta em `stdout`; se precisar usá-la, configure a saída sem buffer antes das escritas. Não a use com entrada redirecionada ou durante outro protocolo de leitura de `stdin`.

Exemplo de salvamento e restauração sem consulta ao terminal:

```c
Clic_move(2, 2);
Clic_saveCursorPosition();
Clic_move(10, 2);
printf("Mensagem temporaria em outra area");
Clic_restoreCursorPosition();
printf("De volta a linha 2");
```

### 6.4. Símbolos, linhas e blocos

```c
void Clic_printSymbol(char *symbol);
void Clic_printHLine(int width);
void Clic_printVLine(int height);
void Clic_printHBlockLine(int width);
void Clic_printVBlockLine(int height);
```

| Função | Parâmetro e comportamento |
| --- | --- |
| `Clic_printSymbol(symbol)` | Imprime a string terminada em `\0` apontada por `symbol`. Forneça um ponteiro válido. |
| `Clic_printHLine(width)` | Imprime `width` símbolos `Symbol_HLINE` a partir do cursor. |
| `Clic_printVLine(height)` | Imprime `height` símbolos `Symbol_VLINE` na mesma coluna, descendo uma linha após cada impressão. |
| `Clic_printHBlockLine(width)` | Imprime `width` espaços. Use cor de fundo para formar uma faixa preenchida. |
| `Clic_printVBlockLine(height)` | Imprime espaços na mesma coluna, descendo uma linha após cada espaço. Use cor de fundo para visualizá-los. |

Todas retornam `void`. As versões verticais terminam uma linha abaixo do último símbolo ou espaço impresso, se não houver rolagem ou restrição do terminal. As funções de bloco **não imprimem `Symbol_BLOCK`**: o preenchimento depende de espaços com fundo colorido.

```c
Clic_move(4, 3);
Clic_printHLine(20);
Clic_move(6, 3);
Clic_setBackgroundColor(Color_BLUE);
Clic_printHBlockLine(12);
Clic_resetColor();
```

### 6.5. Largura visual, caixas, centralização e ajuste final

```c
void Clic_printBox(int width, int height);
int  Clic_textWidth(const char *text);
int  Clic_printCenter(char *text);
void Clic_fixDraw();
```

**`Clic_textWidth(text)`** soma as larguras em colunas fornecidas por `wcwidth()` para os caracteres decodificados com `mbrtowc()` na localidade atual. Retorna `0` para a string vazia e `-1` para `NULL`, sequência inválida/incompleta, caractere não imprimível ou soma que não cabe em `int`.

Em uma localidade UTF-8, `Clic_textWidth("João")` retorna `4`, embora `strlen("João")` retorne `5`. Um ideograma como `界` normalmente ocupa duas colunas; um acento combinante pode acrescentar zero. `strlen` continua apropriado para calcular espaço de armazenamento em bytes.

A soma por caractere não implementa segmentação completa de grafemas. Emojis compostos, seletores de apresentação e caracteres de largura ambígua podem divergir da apresentação do terminal. Não inclua códigos ANSI na string medida.

**`Clic_printBox(width, height)`** desenha uma caixa a partir do cursor atual. A largura e a altura incluem bordas; use pelo menos `2 × 2`, preferencialmente `3 × 3` ou maior quando houver conteúdo interno. O interior é preenchido com espaços, de modo que texto já existente nessa área será sobrescrito.

A função limita a largura ao total de colunas da tela; quando a altura é maior ou igual à altura da tela, usa a altura da tela menos um. Esses limites não consideram a posição inicial. Calcule também o espaço restante à direita e abaixo. A função não valida tamanhos mínimos e não mantém o cursor no ponto inicial ao terminar. Retorno `void`.

```c
/* Execute em um terminal que comporte este desenho. */
Clic_move(2, 3);
Clic_printBox(40, 6);
Clic_move(3, 5);
printf("Cadastro de participantes");
```

**`Clic_printCenter(text)`** usa a largura visual, imprime na linha atual e retorna a coluna inicial, contada a partir de 1. Para um texto que cabe, usa `1 + (larguraDaTela - larguraVisual) / 2`. Textos maiores que a tela são truncados com reticências e começam na coluna 1. Se `text == NULL`, o texto for inválido/não imprimível ou a largura consultada for não positiva, não imprime e retorna `0`.

Essa função passou a retornar a coluna efetivamente utilizada, corrigindo o deslocamento anterior. Se sua aplicação utiliza esse retorno em cálculos próprios, considere a origem em 1.

```c
Clic_move(2, 1);
Clic_printCenter("MENU PRINCIPAL");
```

**`Clic_fixDraw()`** move o cursor para a última coluna da penúltima linha da tela e imprime uma quebra de linha. Retorno `void`. É um posicionamento específico; não recalcula componentes nem corrige automaticamente um layout. Nos exemplos, a linha final do cursor é escolhida explicitamente com `Clic_move()`.

<a id="teclado"></a>
## 7. Teclado e temporização

### 7.1. Constantes `Key`

As funções de captura retornam `int`. Use esse tipo também na variável que recebe o retorno, para preservar valores como `EOF`.

| Constante ou grupo | Valor | Observação |
| --- | ---: | --- |
| `Key_TAB` | 9 | Tabulação. |
| `Key_ENTER` | 10 | Quebra de linha usual recebida ao pressionar Enter. |
| `Key_SPACE` | 32 | Espaço. |
| `Key_0` a `Key_9` | 48 a 57 | Dígitos `0` a `9`. |
| `Key_UP` | 65 | Último byte da sequência usual da seta para cima. |
| `Key_DOWN` | 66 | Último byte da sequência usual da seta para baixo. |
| `Key_RIGHT` | 67 | Último byte da sequência usual da seta para a direita. |
| `Key_LEFT` | 68 | Último byte da sequência usual da seta para a esquerda. |
| `Key_A` a `Key_Z` | 97 a 122 | Correspondem às letras **minúsculas** `a` a `z`. |
| `Key_DELETE` | 127 | Byte DEL, frequentemente enviado pela tecla Backspace. |

`Key_A` representa `'a'`, apesar do nome em maiúscula. As letras maiúsculas `'A'`, `'B'`, `'C'` e `'D'` coincidem numericamente com `Key_UP`, `Key_DOWN`, `Key_RIGHT` e `Key_LEFT`. A implementação não distingue essas entradas após retornar o código.

Não há constante `Key_ESC`. O byte 127 não identifica universalmente a tecla física Delete: muitos terminais enviam uma sequência de vários bytes para essa tecla, que não é tratada por completo pela biblioteca.

### 7.2. Captura bloqueante

```c
int Clic_keyCapture();
```

Espera a chegada de uma tecla sem exigir Enter e sem ecoar a entrada durante a captura. Retorna o código recebido. Se o primeiro byte for Escape, consome mais dois bytes e retorna o terceiro, permitindo reconhecer as sequências usuais das setas.

```c
printf("Pressione q para encerrar.\n");
for (;;) {
    int tecla = Clic_keyCapture();
    if (tecla == Key_Q || tecla == EOF) {
        break;
    }
    if (tecla == Key_UP) {
        printf("Seta para cima\n");
    }
}
```

O retorno pode ser `EOF` quando a leitura falha ou chega ao fim. Essa função não é um interpretador completo de teclado: Escape isolado pode deixá-la esperando os bytes seguintes, e teclas de função ou sequências longas podem produzir resultados parciais.

### 7.3. Captura não bloqueante

```c
int Clic_keyCaptureNonBlocking();
```

Consulta a entrada sem esperar uma tecla, ativando temporariamente `O_NONBLOCK` no descritor de `stdin`. Retorna o código lido; na ausência de entrada, `getchar()` normalmente retorna `EOF`, que também pode indicar fim da entrada ou erro. O retorno de ausência de tecla **não é zero**.

Como a implementação usa a interface de fluxos da biblioteca padrão, uma leitura sem dados pode deixar indicadores de erro ou fim de arquivo ativos em `stdin`. Para repetir a consulta, limpe esses indicadores ao receber `EOF`:

```c
int tecla = Clic_keyCaptureNonBlocking();
if (tecla == EOF) {
    clearerr(stdin);
} else if (tecla == Key_Q) {
    /* Encerrar a animacao. */
}
```

O exemplo [07_animacao.c](exemplos/07_animacao.c) mostra esse uso em um laço. Ele usa uma tecla de um único byte para encerrar. Se os bytes de uma seta chegarem separados, a captura não bloqueante pode consumir uma sequência incompleta.

As duas funções de captura reativam `ICANON` e `ECHO` no fim da chamada; elas não guardam uma cópia completa da configuração anterior. A variante não bloqueante também remove `O_NONBLOCK` ao terminar. Evite misturá-las com outro componente que controle o modo do terminal ou os mesmos descritores.

### 7.4. Pausa

```c
void Clic_pause(float seconds);
```

Descarrega `stdout` e espera aproximadamente `seconds` **segundos**, convertidos internamente para microssegundos e repassados a `usleep`. Retorno `void`.

```c
Clic_pause(0.05f);  /* Aproximadamente 50 milissegundos. */
```

Use valores pequenos e não negativos para animações. A função não valida o intervalo, não retorna erros de espera e não compensa o tempo gasto no desenho; portanto, não estabelece uma taxa exata de quadros.

<a id="tabelas"></a>
## 8. Tabelas e seleção de linhas

Uma `Table` reúne linhas com o mesmo conjunto de formatos. Os valores são convertidos em strings quando a linha é adicionada. A biblioteca desenha a borda externa e separadores horizontais entre as linhas.

### 8.1. Fluxo básico

```c
Table *tabela = Table_create("%-s %d");
if (tabela == NULL) {
    return 1;
}

Table_setWidth(tabela, 40);
Table_addRow(tabela, "Ana", 20);
Table_addRow(tabela, "Bruno", 22);
Clic_move(3, 1);
Table_print(tabela);

Table_free(tabela);
```

Este trecho pressupõe um terminal que comporte o desenho. Para textos Unicode, configure a localidade antes de adicionar as linhas. `Table_free()` libera também os textos das células e o vetor de formatos.

### 8.2. Formatos aceitos

O argumento de `Table_create()` se parece com uma string de `printf`, mas o suporte efetivo é mais restrito. Cada especificador reconhecido define uma coluna. Espaços e outros textos entre especificadores não viram conteúdo de cabeçalho.

| Formato | Argumento de `Table_addRow` | Comportamento |
| --- | --- | --- |
| `%s` | `char *` válido | Texto, alinhamento à direita por padrão. |
| `%-s` | `char *` válido | Texto alinhado à esquerda; largura automática da coluna. |
| `%d` ou `%i` | `int` | Inteiro com largura automática. |
| `%f` ou `%F` | `double` | Ponto flutuante com a precisão padrão de `printf`. Um `float` é promovido a `double` em chamada variádica. |
| `%-12s` | `char *` válido | Texto à esquerda com largura personalizada de 12. |
| `%8.2f` | `double` | Número com duas casas decimais e largura personalizada de 8. |

Use somente as famílias `%s`, `%d`, `%i`, `%f` e `%F`, sem modificadores de tamanho ou largura/precisão via `*`. Outros formatos, como `%u`, `%x`, `%c`, `%p`, `%e`, `%g` e `%n`, fazem a adição da linha ser rejeitada. A linha parcial é liberada e as linhas anteriores são preservadas.

Não use `%ld`, `%lld`, `%zu`, `%Lf`, `%ls`, `%*s`, `%.*f` ou `%%` como formatos de coluna. Eles não têm um contrato de uso correto nesta implementação.

**Largura e precisão:** o analisador ainda considera dígitos da precisão como largura personalizada. Por isso, `%.2f` também configura largura de coluna `2`. Prefira `%8.2f` para definir uma largura suficiente.

Para `%s`, o preenchimento de largura é aplicado somente no desenho, em colunas visuais. A precisão explícita, como em `%.3s`, conserva a semântica de bytes do `printf`; se ela cortar uma sequência multibyte, a linha é rejeitada. Prefira `%s` ou `%-12s` e deixe a tabela truncar o texto visualmente.

### 8.3. Criação

```c
Table *Table_create(char *format);
```

`format` deve ser uma string terminada em `\0`, válida e com pelo menos uma coluna suportada. Retorna um ponteiro para a tabela alocada, ou `NULL` quando o formato é `NULL` ou a alocação principal falha. Nessas falhas, imprime uma mensagem em `stderr`.

Estado inicial:

- `nRows = 0` e `nCols` igual ao número de formatos reconhecidos.
- Largura inicial igual à largura consultada do terminal.
- `highlightedIndex = 1` e `highlightColor = Color_YELLOW`.
- `firstRow = NULL` e `lastRow = NULL`.

A função não verifica todas as alocações internas. Formato vazio ou sem coluna reconhecida pode causar divisão por zero. Embora `Row_MAX_CELLS` seja 60, a busca do terminador dos formatos não é protegida quando todas as posições são ocupadas. Nesta versão, use **de 1 a 59 colunas**, com formatos individuais de no máximo **29 bytes**, e prefira poucas colunas para facilitar a leitura.

### 8.4. Largura da tabela

```c
void Table_setWidth(Table *table, int width);
```

`table` é a tabela criada e `width` é a largura total pretendida em colunas, incluindo bordas. Retorno `void`. Se `table == NULL`, não faz nada. A função limita a largura ao total de colunas da tela e força um mínimo de 3; não verifica o espaço restante a partir do cursor nem se cada coluna terá espaço suficiente.

Colunas automáticas e personalizadas podem ser combinadas. Tabelas em que todas as colunas têm largura explícita também são aceitas, sem divisão por zero.

Exemplo apropriado para a implementação atual:

```c
Table *tabela = Table_create("%-s %d %8.2f");
```

As duas primeiras colunas têm largura automática e a última reserva oito posições para o valor formatado.

O desenho reserva o espaço de conteúdo assim:

```text
espacoDisponivel = larguraTotal - (2 + numeroDeColunas)
largurasPersonalizadas = reservadas da esquerda para a direita, limitadas ao espaco restante
larguraAutomatica = espacoRestante / numeroDeColunasAutomaticas, quando esse numero e positivo
```

São reservadas duas posições para as bordas externas e um espaço após cada célula. O espaço final que sobrar é acrescentado à última coluna, inclusive quando todas são personalizadas. Larguras solicitadas que não couberem são reduzidas; colunas sem espaço não imprimem conteúdo. Se a largura total não comportar nem as bordas e um separador por coluna (`width < 2 + nCols`), a impressão retorna sem desenhar.

Não há mais buffer fixo de 300 bytes no desenho ou nas células. Ainda é necessário escolher uma largura que caiba no terminal e permita ler os dados.

`Table_setWidth()` não altera os formatos já associados às colunas. Os valores explícitos em especificadores como `%8.2f` continuam influenciando o desenho.

### 8.5. Adição de linhas

```c
void Table_addRow(Table *table, ...);
```

Depois de `table`, forneça **um valor para cada coluna, na mesma ordem e com o tipo correspondente ao formato**. Retorno `void`. Se `table == NULL`, informa erro e retorna. A função não fornece um código de sucesso. Se houver falha ao construir a linha, formato não suportado, string nula, sequência inválida ou caractere não imprimível, informa erro, libera a linha parcial e preserva a tabela. A construção da própria tabela ainda possui limitações de validação descritas abaixo.

```c
Table *tabela = Table_create("%-s %d %8.2f");
if (tabela != NULL) {
    Table_addRow(tabela, "Caneta", 3, 2.50);
    Table_addRow(tabela, "Caderno", 2, 18.90);
    /* Desenhar e liberar ao terminar. */
}
```

Cada célula recebe um buffer dimensionado para o resultado completo da formatação, incluindo `\0`. Uma string longa não é cortada por um limite fixo de armazenamento; apenas sua apresentação é limitada à largura da coluna. Uma precisão explícita no formato continua podendo limitar o conteúdo antes do armazenamento. A linha guarda sua própria cópia: alterar uma variável de origem depois da inserção não atualiza a tabela.

Os índices das linhas começam em **1**. Todas as linhas são linhas de dados: não há uma função específica para cabeçalho. Em uma coluna `%d`, não passe uma string como `"Idade"` para simular um título. Imprima o título fora da tabela, como em [05_tabela.c](exemplos/05_tabela.c), ou crie uma tabela somente de strings e converta os valores antes de adicioná-los.

### 8.6. Impressão

```c
void Table_print(Table *table);
```

Desenha a tabela na posição atual do cursor, sem destacar uma seleção. Retorno `void`. Para `table == NULL`, não faz nada.

Uma tabela não vazia com `nRows` linhas ocupa **`2 * nRows + 1` linhas de terminal**: borda superior, linhas de conteúdo, separadores e borda inferior. Não há paginação nem ajuste vertical automático implementado. Posicione a tabela de modo que ela caiba inteira na tela.

Quando o conteúdo ultrapassa a coluna, o desenho reserva a largura visual de `Symbol_TREE_POINTS` e conserva somente caracteres multibyte completos que cabem no restante. Marcas combinantes de largura zero após o último caractere mantido são preservadas. Na localidade `C`, ou se as reticências não couberem, usa `.` como marcador. O preenchimento à esquerda/direita usa espaços calculados pela largura visual.

Esse procedimento suporta acentos e caracteres largos de acordo com a localidade e o terminal; não implementa agrupamento completo de emojis compostos. Se os campos da estrutura forem alterados diretamente para conter texto inválido, o desenho substitui a célula por `?`. As inserções normais já rejeitam esse texto.

A função usa internamente a posição salva do cursor e restaura os atributos de cor durante o desenho. Ela termina após a borda inferior; mova o cursor para uma linha livre antes de imprimir uma mensagem seguinte.

### 8.7. Seleção com setas

```c
int Table_select(Table *table);
```

Desenha a tabela repetidamente, destaca a linha corrente e espera comandos. Use uma tabela não vazia e que caiba na tela. A cor de fundo vem de `table->highlightColor`; a cor do texto destacado é preta.

| Entrada | Comportamento atual |
| --- | --- |
| `Key_UP` | Diminui `highlightedIndex`; a implementação permite chegar a **0**. |
| `Key_DOWN` | Aumenta o índice até `nRows`. |
| `Key_ENTER` | Encerra e retorna o índice corrente. |
| `Key_DELETE` | Encerra e retorna o negativo do índice corrente. Não remove a linha. |
| Outras teclas | A seleção continua. Não há tratamento próprio de `q` ou Escape. |

Retornos:

- **`1` a `nRows`**: índice de uma linha confirmado por Enter.
- **`0`**: índice sem linha correspondente, alcançável ao subir antes da primeira linha; também pode ocorrer ao solicitar exclusão nesse índice.
- **Valor negativo**: solicitação de exclusão; o módulo do valor identifica a linha.
- **`-1` para `table == NULL`**: coincide com o retorno de exclusão da primeira linha. Verifique o ponteiro antes da chamada.

Valide o retorno antes de indexar um vetor:

```c
/* menu deve apontar para uma Table valida e nao vazia. */
int indice = Table_select(menu);
if (indice >= 1 && indice <= menu->nRows) {
    int indiceNoVetor = indice - 1;
    printf("Linha %d; posicao no vetor %d\n", indice, indiceNoVetor);
} else if (indice < 0) {
    printf("Solicitacao de exclusao da linha %d\n", -indice);
} else {
    printf("Selecao sem linha valida\n");
}
```

Esse trecho ilustra a validação. Em uma interface completa, posicione o cursor abaixo da tabela antes da mensagem, como em [06_selecao_tabela.c](exemplos/06_selecao_tabela.c).

É possível configurar a seleção inicial e a cor antes da chamada:

```c
menu->highlightedIndex = 1;
menu->highlightColor = Color_CIAN;
```

Os campos de callback existentes em `Table` não são chamados pela implementação atual. Ações como iniciar um jogo, alterar um registro ou excluir um item devem ser realizadas pela aplicação após receber e validar o retorno.

### 8.8. Liberação

```c
void Table_free(Table *table);
```

Libera os buffers `Cell.content`, os nós `Row`, o vetor `table->formats` e a estrutura `Table`. Para `NULL`, não faz nada. Retorno `void`. Após a chamada, não acesse a tabela nem suas linhas; atribua `NULL` à sua variável se for mantê-la no escopo. A mesma liberação de células é usada para desfazer a construção de uma linha que falhou.

<a id="tipos"></a>
## 9. Tipos e estruturas

Os tipos são expostos por `typedef` em `Clic.h`: `Color`, `Key`, `Format`, `Cell`, `Row` e `Table`. Para o uso cotidiano, trabalhe com `Table *` e as funções `Table_*`. Os campos abaixo ajudam a compreender a representação, mas alterar ligações, contagens ou buffers manualmente pode invalidar a tabela.

### `Format`

| Campo | Tipo | Significado |
| --- | --- | --- |
| `formatString` | `char[30]` | Especificador armazenado, incluindo o terminador `\0`. |
| `isLeftAligned` | `bool` | Indica a presença de `-` no formato. |
| `isCustomWidth` | `bool` | Indica largura identificada pelo analisador. |
| `width` | `int` | Largura personalizada encontrada. |

### `Cell`

| Campo | Tipo | Significado |
| --- | --- | --- |
| `content` | `char *` | Texto já formatado e alocado para a célula. |
| `format` | `Format` | Cópia do formato de sua coluna. |

### `Row`

| Campo | Tipo | Significado |
| --- | --- | --- |
| `index` | `int` | Índice da linha, iniciado em 1. |
| `length` | `int` | Quantidade de colunas usadas. |
| `cell` | `Cell[Row_MAX_CELLS]` | Armazenamento das células; a constante vale 60. |
| `next` | `Row *` | Próxima linha, ou `NULL`. |
| `prev` | `Row *` | Linha anterior, ou `NULL`. |

As linhas formam uma lista duplamente encadeada. Não há função pública para remover ou reordenar linhas nesta versão.

### `Table`

| Campo | Tipo | Significado e cuidado de uso |
| --- | --- | --- |
| `nRows` | `int` | Quantidade de linhas adicionadas. |
| `nCols` | `int` | Quantidade de formatos/colunas. |
| `width` | `int` | Largura total configurada. Use `Table_setWidth()` para alterá-la. |
| `defaultCellWidth` | `int` | Valor calculado na criação e no ajuste de largura; o desenho atual recalcula localmente a distribuição das colunas. |
| `highlightedIndex` | `int` | Índice destacado; escolha um valor entre 1 e `nRows` antes de selecionar. |
| `highlightColor` | `Color` | Fundo da linha destacada. |
| `formats` | `Format *` | Vetor de formatos alocado internamente. |
| `selectCallback` | `void *(*)(Row *)` | Campo declarado, não inicializado nem utilizado pelas operações atuais. |
| `updateCallback` | `void *(*)(Row *)` | Campo declarado, não inicializado nem utilizado pelas operações atuais. |
| `deleteCallback` | `void *(*)(Row *)` | Campo declarado, não inicializado nem utilizado pelas operações atuais. |
| `firstRow` | `Row *` | Primeira linha; `NULL` na tabela vazia. |
| `lastRow` | `Row *` | Última linha; `NULL` na tabela vazia. |

Não leia nem execute os callbacks como se tivessem sido configurados por `Table_create()`. Apenas declarar esses campos não implementa os eventos correspondentes.

### Funções auxiliares internas

`Clic.c` também define funções não declaradas no cabeçalho, como `Format_formatSplit`, `Row_create`, `Table_printHighlight`, `Table_adjustVerticalSpace`, `Clic_setBufferOn` e `Clic_setBufferOff`. Elas são detalhes da implementação e não compõem a API pública documentada. Não as invoque por meio de declarações manuais nos programas de exemplo.

<a id="limitacoes"></a>
## 10. Limitações da versão documentada

Esta seção reúne os pontos que afetam diretamente a utilização. Eles descrevem o commit consultado e não significam que uma versão posterior mantenha o mesmo comportamento.

| Situação | Consequência atual | Como proceder no uso desta versão |
| --- | --- | --- |
| Ambiente Windows nativo | Há dependências POSIX sem alternativa no código. | Use ambiente POSIX; no Windows, considere Linux no WSL. |
| Terminal sem suporte ou entrada/saída redirecionada | Consultas podem falhar; sequências aparecem como texto; a captura depende de `termios`. | Execute em terminal interativo e confira as dimensões. |
| Escape isolado ou sequências especiais longas | A captura interpreta apenas um padrão curto de três bytes. | Nos exemplos, use setas usuais e letras minúsculas; use `q` apenas onde a aplicação o implementa. |
| Maiúsculas `A` a `D` | São indistinguíveis dos códigos retornados para as setas. | Não use essas letras como comandos diferentes das setas. |
| Captura não bloqueante sem limpar `stdin` | Indicadores de erro/fim podem interferir em consultas seguintes. | Use `clearerr(stdin)` ao receber `EOF`, como no exemplo de animação. |
| Configuração prévia especial do terminal | As funções de captura reativam flags, sem restaurar uma cópia integral do estado anterior. | Evite compartilhar o controle do terminal com outra biblioteca. |
| `Clic_getCursorPosition` sem resposta válida | Pode bloquear ou gravar valores sem uma leitura válida. | Evite a consulta quando a aplicação já conhece suas coordenadas. |
| Formato de tabela vazio | Pode ocorrer divisão por zero na criação. | Forneça pelo menos uma coluna suportada. |
| `%.2f` em uma tabela | A precisão ainda pode ser interpretada como largura de coluna 2. | Use largura explícita suficiente, como `%8.2f`. |
| Formatos extensos ou número máximo de colunas | O analisador original ainda possui limites sem validação completa. | Use formatos de até 29 bytes e de 1 a 59 colunas. |
| Texto UTF-8 sem localidade compatível | A validação pode rejeitar a linha. | Configure `LC_CTYPE` para uma localidade UTF-8 antes de usar texto Unicode. |
| Precisão em `%s` | Continua contando bytes e pode produzir sequência incompleta. | Prefira truncar pela largura visual da tabela; sequências incompletas são rejeitadas. |
| Emojis compostos e largura ambígua | A soma de `wcwidth` pode diferir da apresentação do terminal. | Verifique o terminal de destino; o suporte não inclui segmentação completa de grafemas. |
| Tabulações, quebras de linha ou códigos ANSI no conteúdo | Não são texto imprimível de uma única célula; a inserção é rejeitada. | Use células de uma linha e aplique cores pelas funções da biblioteca. |
| Tabela maior que a tela | Não existe paginação ou compensação vertical automática. | Limite a quantidade de linhas exibidas e calcule `2 * nRows + 1`. |
| Seta para cima na primeira linha | `Table_select()` pode retornar 0. | Valide o intervalo antes de usar `indice - 1`. |
| Retorno negativo da seleção | A biblioteca não exclui dados nem chama callbacks. | Trate a ação explicitamente na aplicação. |
| Falhas de alocação e de leitura | Algumas verificações internas e retornos de erro não estão implementados. | Não interprete a API atual como uma garantia de tratamento completo dessas falhas. |

A atualização UTF-8 corrige medição, armazenamento, truncamento, preenchimento e liberação das células. Permanecem pontos independentes para evolução, principalmente a validação completa do analisador de formatos, os limites da seleção e a robustez da captura de teclado.

<a id="duvidas"></a>
## 11. Dúvidas frequentes

### Incluí `Clic.h`, mas recebo “undefined reference”.

O cabeçalho declara as funções; a implementação está em `Clic.c`. Compile os dois arquivos, ou vincule o objeto `Clic.o`:

```bash
gcc -std=gnu11 -Wall -Wextra -I. main.c Clic.c -o programa
```

### Aparece “Clic.h: No such file or directory”.

Confira o caminho e a capitalização do nome. Se o programa estiver em `exemplos/` e o cabeçalho na raiz, execute a compilação na raiz com `-I.`. Para outro local, passe esse diretório na opção `-I`.

### As cores e os desenhos aparecem como códigos ou caracteres incorretos.

Use um terminal interativo que interprete sequências ANSI/VT e uma fonte com os símbolos UTF-8. Um painel simples de saída de IDE pode não oferecer esse suporte; execute no terminal integrado ou em um terminal do sistema.

### A tela só atualiza depois que pressiono uma tecla.

Chame `fflush(stdout)` após o desenho e antes da espera. Para `Table_select()` e interfaces com atualizações frequentes, configure `setvbuf(stdout, NULL, _IONBF, 0)` antes das escritas em `stdout`.

### A linha foi apagada, mas o texto seguinte não começa à esquerda.

`Clic_clearLine()` não reposiciona o cursor. Use `Clic_moveToBegin()` ou `Clic_move(linha, 1)` antes de reescrever.

### A função de blocos não desenha nada visível.

`Clic_printHBlockLine()` e `Clic_printVBlockLine()` imprimem espaços. Defina uma cor de fundo diferente antes de chamá-las. Para imprimir o caractere cheio, use `Clic_printSymbol(Symbol_BLOCK)`.

### O terminal ficou sem eco ou com o cursor oculto após interromper o programa.

Depois de encerrar o programa, execute no shell:

```bash
stty sane
printf '\033[0m\033[?25h\n'
```

Esses comandos restauram modos usuais de entrada, atributos gráficos e visibilidade do cursor. Os exemplos fazem a limpeza no encerramento normal; não instalam tratamento de sinais.

### Quero um menu com uma opção “Sair”.

Use o exemplo de [menu manual](exemplos/04_menu.c) ou o de [seleção de tabela](exemplos/06_selecao_tabela.c). O primeiro controla navegação e cancelamento diretamente no programa. O segundo retorna o índice de uma linha, que deve ser validado e associado à ação desejada.

<a id="fontes"></a>
## 12. Fontes

Fontes principais:

- [Clic.h: API pública, tipos e constantes](Clic.h).
- [Clic.c: implementação dos comportamentos descritos](Clic.c).
- [LICENSE: licença MIT da biblioteca](LICENSE).
- [mbrtowc: conversão de caracteres na GNU C Library](https://sourceware.org/glibc/manual/latest/html_node/Converting-a-Character.html).
- [wcwidth: largura em colunas na documentação Linux](https://man7.org/linux/man-pages/man3/wcwidth.3.html).

Ao atualizar a biblioteca, revise especialmente a assinatura das funções, o tratamento das teclas e as observações sobre tabelas antes de reutilizar esta documentação como referência de uma nova versão.

## Comunicação em rede e testes

A extensão TCP/IPv4, descoberta UDP, exemplos e contratos de recebimento estão em
[README_REDE.md](README_REDE.md). Use `make test` para a suíte unitária e de integração
local, e `make test-sanitize` para executar a mesma suíte com ASan e UBSan.
