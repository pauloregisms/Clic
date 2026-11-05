/*** CLIC is a library for creating Command Line Interfaces in the C language. ***/
/**
 * Paulo Regis M. Sousa
 */
#ifndef CLIC_H
#define CLIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <locale.h>

#define Symbol_HLINE "\u2501"
#define Symbol_VLINE "\u2503"
#define Symbol_CORNER_TL "\u250f"
#define Symbol_CORNER_TR "\u2513"
#define Symbol_CORNER_BL "\u2517"
#define Symbol_CORNER_BR "\u251b"
#define Symbol_ARROW "\u2192"
#define Symbol_ARROW_RETURN "\u21b3"
#define Symbol_T_SIDE_L "\u2523"
#define Symbol_T_SIDE_R "\u252b"
#define Symbol_T_UP "\u2533"
#define Symbol_T_DOWN "\u253b"
#define Symbol_CROSS "\u254b"
#define Symbol_TREE_POINTS "\u2026"
#define Symbol_DIAG "\u2572"
#define Symbol_CDIAG "\u2571"
#define Symbol_BLOCK "\u2588"
#define Symbol_SAD "\U0001f641"
#define Symbol_SMILE "\U0001f642"

#define Row_MAX_CELLS 60

typedef enum Key Key;
typedef struct Format Format;
typedef struct Cell Cell;
typedef struct Row Row;
typedef struct Table Table;

typedef enum Color {
	Color_BLACK = 0,
	Color_GRAY = 8,
    Color_RED = 9,
	Color_GREEN = 10,
	Color_YELLOW = 11,
	Color_MAGENTA = 13,
	Color_CIAN = 14,
	Color_WHITE = 15,
	Color_BLUE = 39,
	Color_ORANGE = 208,
	Color_LIGHTGRAY = 248
} Color;

enum Key {
	Key_TAB = 9,   
	Key_ENTER = 10,
	Key_SPACE = 32,
	Key_UP = 65,
	Key_DOWN = 66,
	Key_RIGHT = 67,
	Key_LEFT = 68,
	Key_DELETE = 127,
	Key_0 = 48, Key_1 = 49,
	Key_2 = 50, Key_3 = 51,
	Key_4 = 52, Key_5 = 53,
	Key_6 = 54, Key_7 = 55,
	Key_8 = 56, Key_9 = 57,
	Key_A = 97, Key_B = 98, Key_C = 99,
	Key_D = 100, Key_E = 101, Key_F = 102,
	Key_G = 103, Key_H = 104, Key_I = 105,
	Key_J = 106, Key_K = 107, Key_L = 108,
	Key_M = 109, Key_N = 110, Key_O = 111,
	Key_P = 112, Key_Q = 113, Key_R = 114,
	Key_S = 115, Key_T = 116, Key_U = 117,
	Key_V = 118, Key_W = 119, Key_X = 120,
	Key_Y = 121, Key_Z = 122,
};

struct Format {
	char formatString[30];
	bool isLeftAligned;
	bool isCustomWidth;
	int width;
};

struct Cell {
	char *content;
	Format format;
};

struct Row {
    int index;
    int length;
    Cell cell[Row_MAX_CELLS];

    Row *next;
    Row *prev;
};

struct Table {
	int nRows;
	int nCols;
	int width;
	int defaultCellWidth;
    int highlightedIndex;
    Color highlightColor;
	Format *formats;

	void *(*selectCallback)(Row *row);
	void *(*updateCallback)(Row *row);
	void *(*deleteCallback)(Row *row);
    
    Row *firstRow;
    Row *lastRow;
};

// Cores -----------------------------------------------------------------------

/** @brief Restaura as cores e os atributos padrão do terminal. 
 * */
void Clic_resetColor();

/** @brief Define a cor usada na impressão dos próximos textos. 
 * */
void Clic_setFontColor(Color color);

/** @brief Define a cor de fundo usada nas próximas impressões. 
 * */
void Clic_setBackgroundColor(Color color);

// Manipulação do cursor -------------------------------------------------------

/** @brief Move o cursor para a linha e a coluna informadas, contadas a partir de 1. 
 * */
void Clic_move(int line, int column);

/** @brief Move o cursor para cima pelo número de linhas informado. 
 * */
void Clic_moveUp(int lines);

/** @brief Move o cursor para baixo pelo número de linhas informado. 
 * */
void Clic_moveDown(int lines);

/** @brief Move o cursor para a direita pelo número de colunas informado. 
 * */
void Clic_moveRight(int columns);

/** @brief Move o cursor para a esquerda pelo número de colunas informado. 
 * */
void Clic_moveLeft(int columns);

/** @brief Move o cursor para baixo e para o início da linha de destino. 
 * */
void Clic_moveDownBegin(int lines);

/** @brief Move o cursor para cima e para o início da linha de destino. 
 * */
void Clic_moveUpBegin(int lines);

/** @brief Move o cursor para a coluna informada, contada a partir de 1. 
 * */
void Clic_moveToColumn(int column);

/** @brief Move o cursor para o início da linha atual. 
 * */
void Clic_moveToBegin();

/** @brief Salva a posição atual do cursor para restauração posterior. 
 * */
void Clic_saveCursorPosition();

/** @brief Restaura a posição do cursor salva anteriormente. 
 * */
void Clic_restoreCursorPosition();

/** @brief Oculta o cursor do terminal. 
 * */
void Clic_hideCursor();

/** @brief Torna o cursor do terminal visível. 
 * */
void Clic_showCursor();

/** @brief Consulta a posição do cursor e preenche row e col quando não nulos. 
 * */
void Clic_getCursorPosition(int *row, int *col);

/** @brief Posiciona o cursor para que todo o desenho apareça. 
 * */
void Clic_fixDraw();

// Operações com linhas e símbolos ---------------------------------------------

/** @brief Apaga o conteúdo da linha atual mantendo a posição do cursor. 
 * */
void Clic_clearLine();

/** @brief Imprime uma quebra de linha. 
 * */
void Clic_breakLine();

/** @brief Desenha uma linha horizontal de símbolos com a largura informada. 
 * */
void Clic_printHLine(int width);

/** @brief Desenha uma linha vertical de símbolos com a altura informada. 
 * */
void Clic_printVLine(int height);

/** @brief Desenha uma faixa horizontal de espaços com a cor de fundo atual. 
 * */
void Clic_printHBlockLine(int width);

/** @brief Desenha uma faixa vertical de espaços com a cor de fundo atual. 
 * */
void Clic_printVBlockLine(int height);

/** @brief Imprime o símbolo informado na posição atual do cursor. 
 * */
void Clic_printSymbol(char *symbol);

// Operações auxiliares de posicionamento de texto -----------------------------

/** @brief Retorna a largura do texto em colunas do terminal, ou -1 se o texto for nulo ou inválido na localidade atual.
 * */
int Clic_textWidth(const char *text);

/** @brief Centraliza o texto na linha atual e retorna a coluna inicial, ou 0 em caso de falha. 
 * */
int Clic_printCenter(char *text);

// Operações auxiliares para a tela --------------------------------------------

/** @brief Limpa a tela e posiciona o cursor na primeira linha e coluna. 
 * */
void Clic_clearScreen();

/** @brief Retorna a largura do terminal em colunas, ou 1 se a consulta falhar. 
 * */
int Clic_getScreenWidth();

/** @brief Retorna a altura do terminal em linhas, ou 1 se a consulta falhar. 
 * */
int Clic_getScreenHeight();

// Criação de caixas -----------------------------------------------------------

/** @brief Desenha uma caixa a partir do cursor, incluindo as bordas nas dimensões. 
 * */
void Clic_printBox(int width, int height);

/** @brief Desenha uma caixa do tamanho da tela. 
 * */
void Clic_printScreenBox();

// Captura de teclas e manipulação de execução ---------------------------------

/** @brief Aguarda uma tecla e retorna seu código, interpretando as setas. 
 * */
int Clic_keyCapture();

/** @brief Retorna o código de uma tecla sem esperar, ou EOF quando não há entrada. 
 * */
int Clic_keyCaptureNonBlocking();

/** @brief Envia a saída pendente ao terminal e pausa pelo tempo informado em segundos. 
 * */
void Clic_pause(float seconds);

// Criação de tabelas ----------------------------------------------------------

/** @brief Cria uma tabela a partir dos formatos. 
 * */
Table *Table_create(char *format);

/** @brief Define a largura total da tabela em colunas, incluindo as bordas. 
 * */
void Table_setWidth(Table *table, int width);

/** @brief Libera a tabela, suas linhas, os textos das células e os formatos. 
 * */
void Table_free(Table *table);

/** @brief Adiciona uma linha com um valor por coluna, conforme os formatos da tabela. 
 * */
void Table_addRow(Table *table, ...);

/** @brief Desenha a tabela na posição atual do cursor. 
 * */
void Table_print(Table *table);

/** @brief Permite selecionar uma linha com as setas e retorna seu índice, negado em Key_DELETE. 
 * */
int Table_select(Table *table);


#endif /* CLIC_H */
