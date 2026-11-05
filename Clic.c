#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "Clic.h"
#include <limits.h>
#include <wchar.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/**
 * Calcula a largura em caracteres de uma string de texto dada, considerando caracteres multibyte.
 * Essa função converte cada caractere multibyte na entrada para sua representação correspondente em caracteres largos
 * e soma a largura de cada caractere largo usando wcwidth().
 * Retorna a largura total em colunas, ou -1 caso ocorra um erro durante a conversão.
 */
int Clic_textWidth(const char *text) {
    if (!text) return -1;

    mbstate_t state = {0};
    size_t remaining = strlen(text);
    int width = 0;

    while (remaining > 0) {
        wchar_t character;
        size_t bytes = mbrtowc(&character, text, remaining, &state);

        if (bytes == (size_t)-1 || bytes == (size_t)-2) return -1;
        if (bytes == 0) break;

        int columns = wcwidth(character);
        if (columns < 0 || width > INT_MAX - columns) return -1;

        width += columns;
        text += bytes;
        remaining -= bytes;
    }

    return width;
}

/** Mantém caracteres multibyte completos, incluindo marcas de zero largura que podem aparecer no final. */
static size_t Text_prefix(const char *text, int maxWidth, int *width) {
    mbstate_t state = {0};
    size_t remaining = strlen(text);
    size_t used = 0;
    *width = 0;

    while (remaining > 0) {
        wchar_t character;
        size_t bytes = mbrtowc(&character, text + used, remaining, &state);
        if (bytes == 0 || bytes == (size_t)-1 || bytes == (size_t)-2) break;

        int columns = wcwidth(character);
        if (columns < 0 || columns > maxWidth - *width) break;

        *width += columns;
        used += bytes;
        remaining -= bytes;
    }

    return used;
}

static void Text_printCell(const char *text, int width, bool leftAligned) {
    if (width <= 0) return;

    int textWidth = Clic_textWidth(text);
    if (textWidth < 0) {
        text = "?";
        textWidth = 1;
    }

    size_t bytes = strlen(text);
    const char *suffix = "";
    int suffixWidth = 0;

    if (textWidth > width) {
        suffix = Symbol_TREE_POINTS;
        suffixWidth = Clic_textWidth(suffix);
        if (suffixWidth <= 0 || suffixWidth > width) {
            suffix = ".";
            suffixWidth = 1;
        }
        bytes = Text_prefix(text, width - suffixWidth, &textWidth);
    }

    int padding = width - textWidth - suffixWidth;
    if (!leftAligned) printf("%*s", padding, "");
    fwrite(text, 1, bytes, stdout);
    fputs(suffix, stdout);
    if (leftAligned) printf("%*s", padding, "");
}

void Clic_resetColor(){ 
    printf("\e[m"); 
}

void Clic_setFontColor(Color color){ 
    printf("\e[38;5;%dm", color); 
}

void Clic_setBackgroundColor(Color color) {
    printf("\e[48;5;%dm", color);
}


void Clic_move(int line, int column){ 
    printf("\e[%d;%dH", line, column);
}

void Clic_moveUp(int lines){ 
    printf("\e[%dA", lines);
}

void Clic_moveDown(int lines){
    printf("\e[%dB", lines);
}

void Clic_moveRight(int columns){
    printf("\e[%dC", columns);
}

void Clic_moveLeft(int columns){
    printf("\e[%dD", columns);
}

void Clic_moveDownBegin(int lines){
    printf("\e[%dE", lines);
}

void Clic_moveUpBegin(int lines){
    printf("\e[%dF", lines);
}

void Clic_moveToColumn(int column){
    printf("\e[%dG", column);
}

void Clic_moveToBegin(){
    printf("\e[1G");
}


void Clic_clearLine(){
    printf("\e[2K");
}

void Clic_clearScreen(){
    printf("\e[2J\e[1;1H");
}

void Clic_breakLine(){
    printf("\n");
}


void Clic_printSymbol(char *symbol){
    printf("%s", symbol);
}

void Clic_printHLine(int width){
    for (int i = 0; i < width; i++) 
        Clic_printSymbol(Symbol_HLINE);
}

void Clic_printVLine(int height){
    for (int i = 0; i < height; i++) 
        printf(Symbol_VLINE "\e[1D\e[1B");
}

void Clic_printHBlockLine(int width){ 
    for (int i=0; i<width; i++) 
        printf(" ");
}

void Clic_printVBlockLine(int height){
    for (int i = 0; i < height; i++) 
        printf(" \e[1D\e[1B");
}

void Clic_printUpBorder(int width) {
    /** Desenha a linha horizontal superior ┏━━┓ */
	Clic_printSymbol(Symbol_CORNER_TL);
	Clic_printHLine(width - 2);
	Clic_printSymbol(Symbol_CORNER_TR);
}

void Clic_printVerticalBorder(int height) {
    /** Desenha uma linha verticai ┃ */
    for (int i = 0; i < height; i++) {
        printf(Symbol_VLINE);
        Clic_moveLeft(1);
        Clic_moveDown(1);
    }
}

void Clic_printBottomBorder(int width) {
    /** Desenha a linha horizontal inferiror ┗━━┛ */
    Clic_printSymbol(Symbol_CORNER_BL);
    Clic_printHLine(width - 2);
    Clic_printSymbol(Symbol_CORNER_BR);
}

void Clic_printBox(int width, int height) {
	if (width >= Clic_getScreenWidth())
		width = Clic_getScreenWidth();
    if (height >= Clic_getScreenHeight())
		height = Clic_getScreenHeight() - 1;

    Clic_saveCursorPosition();
	Clic_printUpBorder(width);
    Clic_moveLeft(1);
    Clic_moveDown(1);

    int i;
    for (i = 1; i <= height - 2; i++) {
        Clic_restoreCursorPosition();
        Clic_moveDown(i);
        printf(Symbol_VLINE);
        Clic_printHBlockLine(width - 2);
        printf(Symbol_VLINE);
    }

    Clic_restoreCursorPosition();
    Clic_moveDown(i);
    Clic_printBottomBorder(width);
}

int Clic_printCenter(char *text) {
    int textWidth = Clic_textWidth(text);
    if (textWidth < 0) return 0;

    int screenWidth = Clic_getScreenWidth();
    if (screenWidth <= 0) return 0;

    int visibleWidth = textWidth < screenWidth ? textWidth : screenWidth;
    int column = 1 + (screenWidth - visibleWidth) / 2;
    Clic_moveToColumn(column);
    Text_printCell(text, visibleWidth, true);
    return column;
}

void Clic_printScreenBox() {
    int width = Clic_getScreenWidth();
    int height = Clic_getScreenHeight();

    Clic_clearScreen();
    Clic_printBox(width, height);
}

void Clic_setBufferOn() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= ICANON | ECHO;  // Ativa entrada canônica e eco
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // Aplica a nova configuração imediatamente
}

void Clic_setBufferOff() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);  // Desativa entrada canônica e eco
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void Clic_setNonBlockingInputOn() {
    Clic_setBufferOff();
    // Configura a entrada para NÃO-BLOQUEANTE (Usando fcntl)
    // O_NONBLOCK: Faz com que read() retorne -1 imediatamente se não houver dados.
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
}

void Clic_setNonBlockingInputOff() {
    Clic_setBufferOn();
    // Limpa o sinalizador de não-bloqueio (opcional, mas boa prática)
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK);
}

void Clic_saveCursorPosition(){
    printf("\e7");
}

void Clic_restoreCursorPosition(){
    printf("\e8");
}

void Clic_hideCursor() {
    printf("\e[?25l");
}

void Clic_showCursor() {
    printf("\e[?25h");
}

void Clic_getCursorPosition(int *row, int *col) {
    int tempRow, tempCol;

    Clic_setBufferOff();
    printf("\e[6n");
    fscanf(stdin, "\e[%d;%dR", &tempRow, &tempCol);
    Clic_setBufferOn();

    if (row) *row = tempRow;
    if (col) *col = tempCol;
}

int Clic_getScreenWidth() {
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) return 1;
	return w.ws_col;
}

int Clic_getScreenHeight() {
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) return 1;
	return w.ws_row;
}

void Clic_fixDraw() {
	Clic_move(Clic_getScreenHeight() - 1, Clic_getScreenWidth());
	Clic_breakLine();
}

int Clic_keyCapture() {
    int keyPressed = 0;
    int codeKey = 0;
    
    Clic_setBufferOff();

    keyPressed = getchar();

    if (keyPressed == '\e') {  // Se a tecla for uma sequencia de escape
        getchar(); // Ignora o caractere '['
        codeKey = getchar();
    }
    else
        codeKey = keyPressed;

    Clic_setBufferOn();

    return codeKey;
}

int Clic_keyCaptureNonBlocking() {
    int keyPressed = 0;
    int codeKey = 0;

    Clic_setNonBlockingInputOn();
    
    keyPressed = getchar();

    if (keyPressed == '\e') {  // Se a tecla for uma sequencia de escape
        getchar(); // Ignora o caractere '['
        codeKey = getchar();
    }
    else
        codeKey = keyPressed;

    Clic_setNonBlockingInputOff();

    return codeKey;
}

void Clic_pause(float seconds) {
    fflush(stdout);
    int microSeconds = seconds * 1000000;
    usleep(microSeconds);
}

int Format_extractWidth(char *format) {
    int width = 0;
    char *p = format;

    while (*p != '\0') {
        if (isdigit(*p)) {
            width = atoi(p);
            break;
        }
        p++;
    }

    return width;
}

int Format_isTypeSpecifier(char c) {
    if (strchr("diuoxXfFeEgGaAcspn%", c) != NULL) {
        return 1;
    }
    return 0;
}

Format *Format_formatSplit(char *sourceString) {
    
    char *p = sourceString;
    char *startOfFormat = NULL;
	
    int count = 0;
    Format *formats = calloc(Row_MAX_CELLS, sizeof(Format));

    while (*p != '\0') {
        if (*p == '%' && startOfFormat == NULL) {
            startOfFormat = p;
        }
        else if (startOfFormat != NULL) {

            if (Format_isTypeSpecifier(*p)) {
                int len = (p - startOfFormat) + 1;

                if (count < Row_MAX_CELLS && len <= 30) {
                    strncpy(formats[count].formatString, startOfFormat, len);

                    formats[count].formatString[len] = '\0';

                    if (strchr(formats[count].formatString, '-'))
                        formats[count].isLeftAligned = true;
                    else
                        formats[count].isLeftAligned = false;

                    count++;
                }

                if (startOfFormat[1] == '%') {
                    p++;
                }
                
                startOfFormat = NULL;
            } 
            else if (isdigit(*p)) {
                formats[count].isCustomWidth = true;
                formats[count].width = Format_extractWidth(startOfFormat);
            }
            else if (!isdigit(*p) && strchr("-+ #.*hlLzjt", *p) == NULL) {
                startOfFormat = NULL;
            }
        }

        if (*p == '\0') {
            break;
        }
        
        p++;
    }

	return formats;
}

Table *Table_create(char *format) {
	int width = Clic_getScreenWidth();
	
	if (!setlocale(LC_CTYPE, "") || Clic_textWidth("Ação") != 4) {
        fprintf(stderr, "Error: Table_create: Configure uma localidade UTF-8 antes de executar.\n");
        return NULL;
    }
	
	if (format == NULL) {
		perror("Error: Table_create: format string is null.\n");
		return NULL;
	}

    Table *table = (Table *) malloc(sizeof(Table));

    if (!table) {
        perror("Error: Table_create: Unable to allocate memory for Table.\n");
        return NULL;
    }

    table->firstSelectableIndex = 1;
    table->userData = NULL;
    table->userDataFree = NULL;
    table->highlightedIndex = 1;
    table->highlightColor = Color_YELLOW;
    
    table->formats = Format_formatSplit(format);
    
    int countFormats = 0;
    while (table->formats[countFormats].formatString[0] != '\0') {
        countFormats++;
    }
    
    table->nCols = countFormats;
    table->nRows = 0;
    table->width = width;
    table->defaultCellWidth = (width - 2) / table->nCols;

    table->firstRow = NULL;
    table->lastRow = NULL;
	
	return table;
}

void Table_setWidth(Table *table, int width) {
    if (table == NULL)
        return;
    if (width >= Clic_getScreenWidth())
		width = Clic_getScreenWidth();
    if (width < 3)
        width = 3;
        
    table->width = width;
    table->defaultCellWidth = (width - 2) / table->nCols;
}

int Format_getTotalCustomWidth(Table *table) {
    if (!table) return 0;

    int totalWidth = 0;

    for (int col = 0; col < table->nCols; col++) {
        if (table->formats[col].isCustomWidth) {
            totalWidth += table->formats[col].width;
        }
    }

    return totalWidth;
}

int Format_getCustomCellCount(Table *table) {
    if (!table) return 0;

    int count = 0;

    for (int col = 0; col < table->nCols; col++) {
        if (table->formats[col].isCustomWidth) {
            count++;
        }
    }

    return count;
}

/** 
    numColsTerminal =     numColsTable + 2
    numRowsTerminal = 2 * numRowsTable + 1

    1+------------------+     +------------------+   
    2|12345678 112345678|     |                  |     12345678 112345678   
    3|------------------|     |                  |     ------------------   
    4|12345678 112345678| --> |                  |  +  12345678 112345678   
    5|------------------|     |                  |     ------------------
    6|12345678 112345678|     |                  |     12345678 112345678
    7+------------------+     +------------------+
*/
void Table_adjustVerticalSpace(Table *table) {
    if (!table) return;

    int currenLine;
    Clic_getCursorPosition(&currenLine, NULL);
    
    int height = Clic_getScreenHeight();
    int availableHeight = height - (currenLine - 1);
    int requiredHeight  = 2 * (table->nRows) + 1;

    if (requiredHeight > availableHeight) {
        
    }
}

void Table_printHighlight(Table *table, bool highlight) {
	if (!table) return;

    int width = table->width;
    if (table->nCols <= 0 || table->nCols > Row_MAX_CELLS ||
        width < 2 + table->nCols) return;

    int cellWidths[Row_MAX_CELLS] = {0};
    int remainingWidth = width - (2 + table->nCols);
    int defaultCellCount = 0;

    for (int col = 0; col < table->nCols; col++) {
        if (table->formats[col].isCustomWidth) {
            int requested = table->formats[col].width;
            if (requested < 0) requested = 0;
            cellWidths[col] = requested < remainingWidth ? requested : remainingWidth;
            remainingWidth -= cellWidths[col];
        } else {
            defaultCellCount++;
        }
    }

    int defaultCellWidth = defaultCellCount > 0 ? remainingWidth / defaultCellCount : 0;
    for (int col = 0; col < table->nCols; col++) {
        if (!table->formats[col].isCustomWidth) {
            cellWidths[col] = defaultCellWidth;
            remainingWidth -= defaultCellWidth;
        }
    }
    cellWidths[table->nCols - 1] += remainingWidth;

    Row *currentRow = table->firstRow;

    Clic_saveCursorPosition();
    Clic_printUpBorder(width);
    
    int linesDown = 1;
    Clic_restoreCursorPosition();
    Clic_moveDown(linesDown);

    while (currentRow != NULL) {
        Clic_printSymbol(Symbol_VLINE);

        for (int col = 0; col < table->nCols; col++) {
            Cell cell = currentRow->cell[col];

            int cellWidth = cellWidths[col];

            if (highlight && table->highlightedIndex == currentRow->index) {
                Clic_setBackgroundColor(table->highlightColor);
                Clic_setFontColor(Color_BLACK);
            }

            Text_printCell(cell.content, cellWidth, cell.format.isLeftAligned);
            printf(" ");

            Clic_resetColor();
        }

        Clic_printSymbol(Symbol_VLINE);
        Clic_restoreCursorPosition();
        Clic_moveDown(++linesDown);

        currentRow = currentRow->next;

        if (currentRow) {
            Clic_printSymbol(Symbol_VLINE);
            Clic_printHLine(width - 2);
            Clic_printSymbol(Symbol_VLINE);
            Clic_restoreCursorPosition();
            Clic_moveDown(++linesDown);
        }
    }

    Clic_printBottomBorder(width);

    if (highlight)
        Clic_restoreCursorPosition();
}

void Table_print(Table *table) {
    Table_printHighlight(table, false);
}

int Table_select(Table *table) {
	if (!table) return -1;

    if (table->firstSelectableIndex > 1) {
        if (table->nRows < table->firstSelectableIndex) return -1;
        if (table->highlightedIndex < table->firstSelectableIndex)
            table->highlightedIndex = table->firstSelectableIndex;
    }
    int key = 0;
    do {
        Table_printHighlight(table, true);

        key = Clic_keyCapture();

        switch (key) {
            case Key_UP:   
                if(table->highlightedIndex &&
                   (table->firstSelectableIndex <= 1 ||
                    table->highlightedIndex > table->firstSelectableIndex))
                    table->highlightedIndex--; 
                break;
            case Key_DOWN: 
                if(table->highlightedIndex < table->nRows) 
                    table->highlightedIndex++; 
                break;
        }
        
        if (key == Key_DELETE) {
            return (-1) * table->highlightedIndex;
        }
    }
    while (key != Key_ENTER);

    return table->highlightedIndex;
}

static void Row_destroy(Row *row) {
    if (!row) return;
    for (int col = 0; col < row->length; col++)
        free(row->cell[col].content);
    free(row);
}

static char *Cell_formatContent(const char *format, ...) {
    va_list args, sizeArgs;
    va_start(args, format);
    va_copy(sizeArgs, args);
    int length = vsnprintf(NULL, 0, format, sizeArgs);
    va_end(sizeArgs);

    if (length < 0) {
        va_end(args);
        return NULL;
    }

    size_t capacity = (size_t)length + 1;
    char *content = malloc(capacity);
    if (content) {
        int written = vsnprintf(content, capacity, format, args);
        if (written < 0 || (size_t)written >= capacity || Clic_textWidth(content) < 0) {
            free(content);
            content = NULL;
        }
    }

    va_end(args);
    return content;
}

void Table_free(Table *table) {
	if (!table) return;

	Row *currentRow = table->firstRow;
    Row *nextRow;

    while (currentRow != NULL) {
        nextRow = currentRow->next;
        Row_destroy(currentRow);
        currentRow = nextRow;
    }

    if (table->userDataFree && table->userData) {
        table->userDataFree(table->userData);
    }
    free(table->formats);
    free(table);
}

Row *Row_create(Table *table, va_list *args) {
    if (table == NULL) {
        perror("Error: Row_create: table is NULL.\n");
        return NULL;
    }

    Row *newRow = calloc(1, sizeof(Row));

    if (!newRow) {
        perror("Error: Row_create: Unable to allocate memory for new Row.\n");
        return NULL;
    }

    for (int col = 0; col < table->nCols; col++) {
        char *formatString = table->formats[col].formatString;
        char *buffer = NULL;

        /* Width/precision via * and length modifiers need different arguments. */
        if (strpbrk(formatString, "*hlLzjt") == NULL) {
            if (strchr(formatString, 's')) {
                char *strArg = va_arg(*args, char *);
                /* Apply string field width only when drawing, in columns. */
                char stringFormat[sizeof table->formats[col].formatString];
                const char *precision = strchr(formatString, '.');
                if (precision)
                    snprintf(stringFormat, sizeof stringFormat, "%%%s", precision);
                else
                    strcpy(stringFormat, "%s");
                if (strArg) buffer = Cell_formatContent(stringFormat, strArg);
            }
            else if (strchr(formatString, 'd') || strchr(formatString, 'i')) {
                int intArg = va_arg(*args, int);
                buffer = Cell_formatContent(formatString, intArg);
            }
            else if (strchr(formatString, 'f') || strchr(formatString, 'F')) {
                double doubleArg = va_arg(*args, double);
                buffer = Cell_formatContent(formatString, doubleArg);
            }
        }

        if (!buffer) {
            fprintf(stderr, "Error: Row_create: unable to format cell %d; check format, text and locale.\n", col + 1);
            Row_destroy(newRow);
            return NULL;
        }

        newRow->cell[col].content = buffer;
        newRow->cell[col].format = table->formats[col];
        newRow->length++;
    }

    return newRow;
}

void Table_addRow(Table *table, ...) {
    if (table == NULL) {
        perror("Error: Table_addRow: table is NULL.\n");
        return;
    }
    
    va_list args;
    va_start(args, table);

    Row *newRow = Row_create(table, &args);
    va_end(args);

    if (!newRow) {
        return;
    }

    table->nRows++;
    newRow->index = table->nRows;
    newRow->length = table->nCols;
    newRow->next = NULL;
    newRow->prev = table->lastRow;

    if (table->lastRow) {
        table->lastRow->next = newRow;
    } 
    else {
        table->firstRow = newRow;
    }

    table->lastRow = newRow;
}

/* ------------------------------------------------------------------------- */
/*                         Comunicação em rede                               */
/* ------------------------------------------------------------------------- */

#define Clic_INVALID_SOCKET (-1)
#define Clic_DISCOVERY_QUERY "CLIC_DISCOVER"
#define Clic_DISCOVERY_RESPONSE "CLIC_SERVER"
#define Clic_DISCOVERY_MAGIC 1129075780

typedef struct NetworkDiscoveryData {
    int magic;
    int count;

    struct {
        char name[Client_NAME_SIZE];
        char ip[40];
        int port;
    } server[Server_MAX_CLIENTS];
} NetworkDiscoveryData;

/* SIGPIPE suppression is local to sockets/sends, never process-wide. */
static int Network_prepareSocket(int fd) {
#ifdef SO_NOSIGPIPE
    int enabled = 1;
    return setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
#else
    (void)fd;
    return 0;
#endif
}

/* Recompute timeouts after EINTR using a monotonic deadline. */
static long long Network_milliseconds(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) return -1;
    return (long long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

static int Network_poll(struct pollfd *fds, nfds_t count, int timeout) {
    long long deadline = timeout >= 0 ? Network_milliseconds() + timeout : 0;
    for (;;) {
        int result = poll(fds, count, timeout);
        if (result >= 0 || errno != EINTR) return result;
        if (timeout >= 0) {
            long long remaining = deadline - Network_milliseconds();
            timeout = remaining > 0 ? (int)remaining : 0;
        }
    }
}

static void Network_copyString(char *destination, size_t destinationSize,
                               const char *source) {
    if (!destination || destinationSize == 0) {
        return;
    }

    if (!source) {
        destination[0] = '\0';
        return;
    }

    snprintf(destination, destinationSize, "%s", source);
}

static int Network_setNonBlocking(int socketFD) {
    int flags = fcntl(socketFD, F_GETFL, 0);

    if (flags == -1) {
        return -1;
    }

    return fcntl(socketFD, F_SETFL, flags | O_NONBLOCK);
}

static void Network_closeSocket(int *socketFD) {
    if (socketFD && *socketFD != Clic_INVALID_SOCKET) {
        int savedError = errno;
        shutdown(*socketFD, SHUT_RDWR);
        close(*socketFD);
        *socketFD = Clic_INVALID_SOCKET;
        errno = savedError;
    }
}

static bool Network_isValidPort(int port) {
    return port > 0 && port <= 65535;
}

static Client *Client_createFromSocket(int socketFD,
                                       const struct sockaddr_in *address) {
    Client *client = calloc(1, sizeof(Client));

    if (!client) {
        close(socketFD);
        return NULL;
    }

    if (Network_prepareSocket(socketFD) == -1) {
        int savedError = errno;
        close(socketFD);
        free(client);
        errno = savedError;
        return NULL;
    }
    client->socket = socketFD;
    client->connected = true;
    client->lineBufferLength = 0;
    client->name[0] = '\0';

    if (address) {
        if (!inet_ntop(AF_INET, &address->sin_addr, client->ip,
                       sizeof(client->ip))) {
            Network_copyString(client->ip, sizeof(client->ip), "0.0.0.0");
        }

        client->port = ntohs(address->sin_port);
    } else {
        struct sockaddr_in peer;
        socklen_t peerLength = sizeof(peer);

        memset(&peer, 0, sizeof(peer));

        if (getpeername(socketFD, (struct sockaddr *)&peer, &peerLength) == 0) {
            if (!inet_ntop(AF_INET, &peer.sin_addr, client->ip,
                           sizeof(client->ip))) {
                Network_copyString(client->ip, sizeof(client->ip), "0.0.0.0");
            }
            client->port = ntohs(peer.sin_port);
        } else {
            Network_copyString(client->ip, sizeof(client->ip), "0.0.0.0");
            client->port = 0;
        }
    }

    return client;
}

static int Network_connectSocket(const char *host, int port,
                                 int timeoutMilliseconds) {
    if (!host || !Network_isValidPort(port)) {
        errno = EINVAL;
        return Clic_INVALID_SOCKET;
    }

    char portText[6];
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *current;
    int socketFD = Clic_INVALID_SOCKET;

    snprintf(portText, sizeof(portText), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, portText, &hints, &result) != 0) {
        errno = EHOSTUNREACH;
        return Clic_INVALID_SOCKET;
    }

    for (current = result; current; current = current->ai_next) {
        socketFD = socket(current->ai_family, current->ai_socktype,
                          current->ai_protocol);

        if (socketFD == Clic_INVALID_SOCKET) {
            continue;
        }

        if (timeoutMilliseconds < 0) {
            if (connect(socketFD, current->ai_addr, current->ai_addrlen) == 0) {
                break;
            }

            close(socketFD);
            socketFD = Clic_INVALID_SOCKET;
            continue;
        }

        if (Network_setNonBlocking(socketFD) == -1) {
            close(socketFD);
            socketFD = Clic_INVALID_SOCKET;
            continue;
        }

        if (connect(socketFD, current->ai_addr, current->ai_addrlen) == 0) {
            break;
        }

        if (errno == EINPROGRESS) {
            struct pollfd fd = { .fd = socketFD, .events = POLLOUT };
            int ready = Network_poll(&fd, 1, timeoutMilliseconds);
            if (ready > 0) {
                int connectError = 0;
                socklen_t errorLength = sizeof(connectError);
                if (getsockopt(socketFD, SOL_SOCKET, SO_ERROR, &connectError,
                               &errorLength) == 0 && connectError == 0) break;
                if (connectError) errno = connectError;
            } else if (ready == 0) {
                errno = ETIMEDOUT;
            }
        }

        close(socketFD);
        socketFD = Clic_INVALID_SOCKET;
    }

    freeaddrinfo(result);
    return socketFD;
}

static int Client_sendBytes(Client *client, const char *data, size_t length) {
    size_t totalSent = 0;

    if (!Client_isConnected(client) || (!data && length > 0)) {
        errno = ENOTCONN;
        return -1;
    }


    while (totalSent < length) {
        ssize_t sent = send(client->socket, data + totalSent,
                            length - totalSent,
#ifdef MSG_NOSIGNAL
                            MSG_NOSIGNAL
#else
                            0
#endif
        );

        if (sent > 0) {
            totalSent += (size_t)sent;
            continue;
        }

        if (sent == -1 && errno == EINTR) {
            continue;
        }

        Client_disconnect(client);
        return -1;
    }

    return (int)totalSent;
}

static int Client_copyBufferedData(Client *client, char *buffer) {
    int amount;

    if (!client || !buffer || client->lineBufferLength <= 0) {
        return 0;
    }

    amount = client->lineBufferLength;

    if (amount > Client_MESSAGE_SIZE - 1) {
        amount = Client_MESSAGE_SIZE - 1;
    }

    memcpy(buffer, client->lineBuffer, (size_t)amount);
    buffer[amount] = '\0';

    memmove(client->lineBuffer, client->lineBuffer + amount,
            (size_t)(client->lineBufferLength - amount));
    client->lineBufferLength -= amount;

    return amount;
}

static void Client_appendBufferedData(Client *client, const char *data,
                                      int dataLength) {
    if (!client || !data || dataLength <= 0) return;
    assert(dataLength <= Client_LINE_BUFFER_SIZE - 1 - client->lineBufferLength);
    memcpy(client->lineBuffer + client->lineBufferLength, data, (size_t)dataLength);
    client->lineBufferLength += dataLength;
    client->lineBuffer[client->lineBufferLength] = '\0';
}

static int Client_extractLine(Client *client, char *buffer, bool *found) {
    int lineEnd = -1;
    int amount;

    if (found) {
        *found = false;
    }

    if (!client || !buffer) {
        return -1;
    }

    for (int index = 0; index < client->lineBufferLength; index++) {
        if (client->lineBuffer[index] == '\n') {
            lineEnd = index;
            break;
        }
    }

    if (lineEnd < 0) {
        return 0;
    }

    if (found) {
        *found = true;
    }

    amount = lineEnd;

    if (amount > Client_MESSAGE_SIZE - 1) {
        amount = Client_MESSAGE_SIZE - 1;
    }

    memcpy(buffer, client->lineBuffer, (size_t)amount);
    buffer[amount] = '\0';

    if (amount > 0 && buffer[amount - 1] == '\r') {
        buffer[amount - 1] = '\0';
        amount--;
    }

    memmove(client->lineBuffer, client->lineBuffer + lineEnd + 1,
            (size_t)(client->lineBufferLength - lineEnd - 1));
    client->lineBufferLength -= lineEnd + 1;
    client->lineBuffer[client->lineBufferLength] = '\0';

    return amount;
}

static int Client_waitReadable(Client *client, bool nonBlocking) {
    if (!Client_isConnected(client)) {
        errno = ENOTCONN;
        return -1;
    }
    struct pollfd fd = { .fd = client->socket, .events = POLLIN };
    return Network_poll(&fd, 1, nonBlocking ? 0 : -1);
}

static int Client_receiveRaw(Client *client, char *buffer, bool nonBlocking) {
    int buffered;

    if (buffer) {
        buffer[0] = '\0';
    }

    if (!buffer || !Client_isConnected(client)) {
        errno = ENOTCONN;
        return -1;
    }

    buffered = Client_copyBufferedData(client, buffer);

    if (buffered > 0) {
        return buffered;
    }

    int ready = Client_waitReadable(client, nonBlocking);

    if (ready == 0) {
        return Client_NO_DATA;
    }

    if (ready < 0) {
        Client_disconnect(client);
        return -1;
    }

    while (true) {
        ssize_t received = recv(client->socket, buffer,
                                Client_MESSAGE_SIZE - 1,
#ifdef MSG_DONTWAIT
                                nonBlocking ? MSG_DONTWAIT : 0
#else
                                0
#endif
        );

        if (received > 0) {
            buffer[received] = '\0';
            return (int)received;
        }

        if (received == 0) {
            Client_disconnect(client);
            return -1;
        }

        if (errno == EINTR) {
            continue;
        }

        if (nonBlocking && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return Client_NO_DATA;
        }

        Client_disconnect(client);
        return -1;
    }
}

static int Client_receiveLineInternal(Client *client, char *buffer,
                                      bool nonBlocking) {
    char temporary[1024];

    if (buffer) {
        buffer[0] = '\0';
    }

    if (!buffer || !Client_isConnected(client)) {
        errno = ENOTCONN;
        return -1;
    }

    while (true) {
        bool found = false;
        int lineLength = Client_extractLine(client, buffer, &found);

        if (found) {
            return lineLength;
        }

        int ready = Client_waitReadable(client, nonBlocking);

        if (ready == 0) {
            return Client_NO_DATA;
        }

        if (ready < 0) {
            Client_disconnect(client);
            return -1;
        }

        while (true) {
            int available = Client_LINE_BUFFER_SIZE - 1 - client->lineBufferLength;
            if (available == 0) {
                Client_disconnect(client);
                errno = EMSGSIZE;
                return -1;
            }
            size_t capacity = (size_t)available < sizeof(temporary)
                ? (size_t)available : sizeof(temporary);
            ssize_t received = recv(client->socket, temporary, capacity,
#ifdef MSG_DONTWAIT
                                    nonBlocking ? MSG_DONTWAIT : 0
#else
                                    0
#endif
            );

            if (received > 0) {
                Client_appendBufferedData(client, temporary, (int)received);
                break;
            }

            if (received == 0) {
                Client_disconnect(client);
                return -1;
            }

            if (errno == EINTR) {
                continue;
            }

            if (nonBlocking && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                return Client_NO_DATA;
            }

            Client_disconnect(client);
            return -1;
        }
    }
}

Client *Client_connect(char *ip, int port) {
    int socketFD = Network_connectSocket(ip, port, -1);

    if (socketFD == Clic_INVALID_SOCKET) {
        return NULL;
    }

    return Client_createFromSocket(socketFD, NULL);
}

void Client_disconnect(Client *client) {
    if (!client) {
        return;
    }

    Network_closeSocket(&client->socket);
    client->connected = false;
    client->lineBufferLength = 0;
}

void Client_free(Client *client) {
    if (!client) {
        return;
    }

    Client_disconnect(client);
    free(client);
}

bool Client_isConnected(Client *client) {
    return client && client->connected &&
           client->socket != Clic_INVALID_SOCKET;
}

int Client_send(Client *client, char *message) {
    if (!message) {
        errno = EINVAL;
        return -1;
    }

    return Client_sendBytes(client, message, strlen(message));
}

int Client_sendLine(Client *client, char *message) {
    int messageLength;
    int sent;

    if (!message) {
        errno = EINVAL;
        return -1;
    }

    messageLength = Client_send(client, message);

    if (messageLength < 0) {
        return -1;
    }

    if (messageLength > 0 && message[messageLength - 1] == '\n') {
        return messageLength;
    }

    sent = Client_sendBytes(client, "\n", 1);

    if (sent < 0) {
        return -1;
    }

    return messageLength + sent;
}

int Client_receive(Client *client, char *buffer) {
    return Client_receiveRaw(client, buffer, false);
}

int Client_receiveLine(Client *client, char *buffer) {
    return Client_receiveLineInternal(client, buffer, false);
}

int Client_receiveNonBlocking(Client *client, char *buffer) {
    return Client_receiveRaw(client, buffer, true);
}

int Client_receiveLineNonBlocking(Client *client, char *buffer) {
    return Client_receiveLineInternal(client, buffer, true);
}

char *Client_getIP(Client *client) {
    static char empty[] = "";

    if (!client) {
        return empty;
    }

    return client->ip;
}

int Client_getPort(Client *client) {
    return client ? client->port : 0;
}

void Client_setName(Client *client, char *name) {
    if (!client) {
        return;
    }

    Network_copyString(client->name, sizeof(client->name), name);
}

char *Client_getName(Client *client) {
    static char empty[] = "";

    if (!client) {
        return empty;
    }

    return client->name;
}

int Client_ping(Client *client) {
    long long start;
    long long end;
    int testSocket;
    long elapsed;

    if (!Client_isConnected(client) || !Network_isValidPort(client->port)) {
        errno = ENOTCONN;
        return -1;
    }

    start = Network_milliseconds();
    testSocket = Network_connectSocket(client->ip, client->port, 2000);
    end = Network_milliseconds();

    if (testSocket == Clic_INVALID_SOCKET) {
        return -1;
    }

    close(testSocket);

    elapsed = (long)(end - start);

    if (elapsed < 0) {
        elapsed = 0;
    }

    return (int)elapsed;
}

static void Server_removeClientAt(Server *server, int index,
                                  bool notifyDisconnect) {
    Client *client;

    if (!server || index < 0 || index >= server->nClients) {
        return;
    }

    client = server->clients[index];

    /* Remove before notification: callbacks may query/count other clients. */
    for (int current = index; current < server->nClients - 1; current++) {
        server->clients[current] = server->clients[current + 1];
    }
    server->clients[server->nClients - 1] = NULL;
    server->nClients--;
    if (notifyDisconnect && server->disconnectCallback && client) {
        server->disconnectCallback(client);
    }
    Client_free(client);
}

static void Server_cleanupDisconnectedClients(Server *server) {
    if (!server) {
        return;
    }

    for (int index = 0; index < server->nClients; index++) {
        Client *client = server->clients[index];

        if (!Client_isConnected(client)) {
            Server_removeClientAt(server, index, true);
            index--;
        }
    }
}

static Client *Server_addClient(Server *server, int socketFD,
                                const struct sockaddr_in *address) {
    Client *client;

    if (!server || server->nClients >= Server_MAX_CLIENTS) {
        close(socketFD);
        errno = ENOSPC;
        return NULL;
    }

    client = Client_createFromSocket(socketFD, address);

    if (!client) {
        return NULL;
    }

    server->clients[server->nClients++] = client;

    if (server->connectCallback) {
        server->connectCallback(client);
    }

    return client;
}

static Client *Server_acceptOne(Server *server) {
    struct sockaddr_in address;
    socklen_t addressLength = sizeof(address);
    int clientSocket;

    if (!server || server->socket == Clic_INVALID_SOCKET) {
        errno = ENOTCONN;
        return NULL;
    }

    memset(&address, 0, sizeof(address));

    do {
        clientSocket = accept(server->socket, (struct sockaddr *)&address,
                              &addressLength);
    } while (clientSocket == -1 && errno == EINTR);

    if (clientSocket == -1) {
        return NULL;
    }

    return Server_addClient(server, clientSocket, &address);
}

static void Server_processDiscovery(Server *server) {
    char request[256];
    struct sockaddr_in sender;
    socklen_t senderLength = sizeof(sender);

    if (!server || !server->discoveryEnabled ||
        server->discoverySocket == Clic_INVALID_SOCKET) {
        return;
    }

    while (true) {
        ssize_t received;
        char response[Client_NAME_SIZE + 32];

        memset(&sender, 0, sizeof(sender));
        senderLength = sizeof(sender);

        received = recvfrom(server->discoverySocket, request,
                            sizeof(request) - 1, 0,
                            (struct sockaddr *)&sender, &senderLength);

        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }

            if (errno == EINTR) {
                continue;
            }

            return;
        }

        request[received] = '\0';

        if (strcmp(request, Clic_DISCOVERY_QUERY) != 0) {
            continue;
        }

        snprintf(response, sizeof(response), "%s|%s|%d",
                 Clic_DISCOVERY_RESPONSE, server->name, server->port);

        sendto(server->discoverySocket, response, strlen(response), 0,
               (struct sockaddr *)&sender, senderLength);
    }
}

Server *Server_create(int port) {
    Server *server;
    struct sockaddr_in address;
    int reuseAddress = 1;
    char hostName[Client_NAME_SIZE] = {0};

    if (port < 0 || port > 65535) {
        errno = EINVAL;
        return NULL;
    }


    server = calloc(1, sizeof(Server));

    if (!server) {
        return NULL;
    }

    server->socket = socket(AF_INET, SOCK_STREAM, 0);
    server->discoverySocket = Clic_INVALID_SOCKET;

    if (server->socket == Clic_INVALID_SOCKET) {
        free(server);
        return NULL;
    }

    setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &reuseAddress,
               sizeof(reuseAddress));

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);

    if (bind(server->socket, (struct sockaddr *)&address, sizeof(address)) == -1 ||
        listen(server->socket, Server_MAX_CLIENTS) == -1) {
        close(server->socket);
        free(server);
        return NULL;
    }

    if (port == 0) {
        socklen_t addressLength = sizeof(address);

        if (getsockname(server->socket, (struct sockaddr *)&address,
                        &addressLength) == 0) {
            server->port = ntohs(address.sin_port);
        }
    } else {
        server->port = port;
    }

    if (gethostname(hostName, sizeof(hostName)) == 0) {
        hostName[sizeof(hostName) - 1] = '\0';
        Network_copyString(server->name, sizeof(server->name), hostName);
    } else {
        Network_copyString(server->name, sizeof(server->name), "Clic Server");
    }

    return server;
}

void Server_close(Server *server) {
    if (!server) {
        return;
    }

    Server_disableDiscovery(server);

    while (server->nClients > 0) {
        Server_removeClientAt(server, 0, false);
    }

    Network_closeSocket(&server->socket);
    free(server);
}

int Server_clientCount(Server *server) {
    if (!server) {
        return 0;
    }

    Server_cleanupDisconnectedClients(server);
    return server->nClients;
}

Client *Server_getClient(Server *server, int index) {
    if (!server) {
        return NULL;
    }

    Server_cleanupDisconnectedClients(server);

    if (index < 0 || index >= server->nClients) {
        errno = EINVAL;
        return NULL;
    }

    return server->clients[index];
}

Client *Server_accept(Server *server) {
    if (!server || server->socket == Clic_INVALID_SOCKET) {
        errno = ENOTCONN;
        return NULL;
    }
    Server_cleanupDisconnectedClients(server);
    for (;;) {
        struct pollfd fds[2] = {
            { .fd = server->socket, .events = POLLIN },
            { .fd = server->discoverySocket, .events = POLLIN }
        };
        if (Network_poll(fds, 2, -1) < 0) return NULL;
        if (fds[1].revents & POLLIN) Server_processDiscovery(server);
        if (fds[0].revents & POLLIN) return Server_acceptOne(server);
        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            errno = EIO;
            return NULL;
        }
    }
}

Client *Server_acceptNonBlocking(Server *server) {
    if (!server || server->socket == Clic_INVALID_SOCKET) {
        errno = ENOTCONN;
        return NULL;
    }
    Server_update(server);
    struct pollfd fd = { .fd = server->socket, .events = POLLIN };
    if (Network_poll(&fd, 1, 0) > 0 && (fd.revents & POLLIN))
        return Server_acceptOne(server);
    return NULL;
}

void Server_send(Server *server, Client *client, char *message) {
    (void)server;

    if (!client || !message) {
        return;
    }

    Client_send(client, message);
}

void Server_sendAll(Server *server, char *message) {
    if (!server || !message) {
        return;
    }

    for (int index = 0; index < server->nClients; index++) {
        Client_send(server->clients[index], message);
    }

    Server_cleanupDisconnectedClients(server);
}

void Server_sendOthers(Server *server, Client *origin, char *message) {
    if (!server || !message) {
        return;
    }

    for (int index = 0; index < server->nClients; index++) {
        Client *client = server->clients[index];

        if (client != origin) {
            Client_send(client, message);
        }
    }

    Server_cleanupDisconnectedClients(server);
}

/* Callback clients are borrowed. A callback may disconnect a client, but
 * must not free it or close its owning Server. */
static Client *Server_deliver(Server *server, Client *client, char *buffer) {
    if (server->messageCallback) server->messageCallback(client, buffer);
    /* A broadcast in a callback may have removed a disconnected client. */
    for (int i = 0; i < server->nClients; i++)
        if (server->clients[i] == client) return client;
    return NULL;
}

static Client *Server_receiveInternal(Server *server, char *buffer,
                                      bool nonBlocking, bool receiveLine) {
    if (buffer) buffer[0] = '\0';
    if (!server || !buffer || server->socket == Clic_INVALID_SOCKET) {
        errno = EINVAL;
        return NULL;
    }
    Server_cleanupDisconnectedClients(server);
    for (;;) {
        /* recv() may already have read several messages in one packet. */
        for (int i = 0; i < server->nClients; i++) {
            Client *client = server->clients[i];
            bool found = false;
            int length = receiveLine ? Client_extractLine(client, buffer, &found)
                                     : Client_copyBufferedData(client, buffer);
            if (found || (!receiveLine && length > 0))
                return Server_deliver(server, client, buffer);
        }
        struct pollfd fds[Server_MAX_CLIENTS + 2] = {
            { .fd = server->socket, .events = POLLIN },
            { .fd = server->discoverySocket, .events = POLLIN }
        };
        int clientCount = server->nClients;
        for (int i = 0; i < clientCount; i++) {
            fds[i + 2].fd = server->clients[i]->socket;
            fds[i + 2].events = POLLIN;
        }
        if (Network_poll(fds, (nfds_t)clientCount + 2, nonBlocking ? 0 : -1) <= 0)
            return NULL;
        if (fds[1].revents & POLLIN) Server_processDiscovery(server);
        /* Finish the snapshot before accepting or removing clients. */
        for (int i = 0; i < clientCount; i++) {
            if (!fds[i + 2].revents) continue;
            Client *client = server->clients[i];
            int received = receiveLine ? Client_receiveLineInternal(client, buffer, true)
                                       : Client_receiveRaw(client, buffer, true);
            if ((receiveLine && received >= 0) || (!receiveLine && received > 0))
                return Server_deliver(server, client, buffer);
        }
        Server_cleanupDisconnectedClients(server);
        if (fds[0].revents & POLLIN) Server_acceptOne(server);
        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            errno = EIO;
            return NULL;
        }
        if (nonBlocking) return NULL;
    }
}

Client *Server_receive(Server *server, char *buffer) {
    return Server_receiveInternal(server, buffer, false, false);
}

Client *Server_receiveLine(Server *server, char *buffer) {
    return Server_receiveInternal(server, buffer, false, true);
}

Client *Server_receiveNonBlocking(Server *server, char *buffer) {
    return Server_receiveInternal(server, buffer, true, false);
}

Client *Server_receiveLineNonBlocking(Server *server, char *buffer) {
    return Server_receiveInternal(server, buffer, true, true);
}

void Server_onConnect(Server *server, ServerConnectCallback callback) {
    if (server) {
        server->connectCallback = callback;
    }
}

void Server_onDisconnect(Server *server, ServerDisconnectCallback callback) {
    if (server) {
        server->disconnectCallback = callback;
    }
}

void Server_onMessage(Server *server, ServerMessageCallback callback) {
    if (server) {
        server->messageCallback = callback;
    }
}

bool Server_enableDiscovery(Server *server) {
    struct sockaddr_in address;
    int reuseAddress = 1;
    int discoverySocket;

    if (!server) {
        errno = EINVAL;
        return false;
    }

    if (server->discoveryEnabled) {
        return true;
    }

    discoverySocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (discoverySocket == Clic_INVALID_SOCKET) {
        return false;
    }

    setsockopt(discoverySocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddress,
               sizeof(reuseAddress));

#ifdef SO_REUSEPORT
    setsockopt(discoverySocket, SOL_SOCKET, SO_REUSEPORT, &reuseAddress,
               sizeof(reuseAddress));
#endif

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(Network_DISCOVERY_PORT);

    if (bind(discoverySocket, (struct sockaddr *)&address, sizeof(address)) == -1 ||
        Network_setNonBlocking(discoverySocket) == -1) {
        close(discoverySocket);
        return false;
    }

    server->discoverySocket = discoverySocket;
    server->discoveryEnabled = true;
    return true;
}

void Server_disableDiscovery(Server *server) {
    if (!server) {
        return;
    }

    Network_closeSocket(&server->discoverySocket);
    server->discoveryEnabled = false;
}

void Server_setName(Server *server, char *name) {
    if (!server) {
        return;
    }

    Network_copyString(server->name, sizeof(server->name), name);
    for (char *p = server->name; *p; p++)
        if (*p == '|' || (unsigned char)*p < 32 || *p == 127) *p = ' ';
}

char *Server_getName(Server *server) {
    static char empty[] = "";

    if (!server) {
        return empty;
    }

    return server->name;
}

void Server_update(Server *server) {
    if (!server) {
        return;
    }

    Server_processDiscovery(server);
    Server_cleanupDisconnectedClients(server);
}

static void Network_freeDiscoveryData(void *data) {
    free(data);
}

static void Network_addDiscoveredServer(NetworkDiscoveryData *data,
                                        const char *name, const char *ip,
                                        int port) {
    if (!data || !name || !*name || strlen(name) >= Client_NAME_SIZE ||
        !ip || !Network_isValidPort(port) || Clic_textWidth(name) < 0) {
        return;
    }

    for (int index = 0; index < data->count; index++) {
        if (data->server[index].port == port &&
            (strcmp(data->server[index].ip, ip) == 0)) {
            return;
        }
    }

    if (data->count >= Server_MAX_CLIENTS) {
        return;
    }

    Network_copyString(data->server[data->count].name,
                       sizeof(data->server[data->count].name), name);
    Network_copyString(data->server[data->count].ip,
                       sizeof(data->server[data->count].ip), ip);
    data->server[data->count].port = port;
    data->count++;
}

static Table *Network_createDiscoveryTable(void) {
    Table *servers;
    NetworkDiscoveryData *data;
    servers = Table_create("%-20s %-15s %-5s");

    if (!servers) {
        return NULL;
    }

    Table_addRow(servers, "Servidor", "IP", "Porta");
    servers->firstSelectableIndex = 2;
    servers->highlightedIndex = 2;

    data = calloc(1, sizeof(NetworkDiscoveryData));

    if (!data) {
        Table_free(servers);
        return NULL;
    }

    data->magic = Clic_DISCOVERY_MAGIC;
    servers->userData = data;
    servers->userDataFree = Network_freeDiscoveryData;

    return servers;
}

Table *Network_discoverServers() {
    Table *servers;
    NetworkDiscoveryData *data;
    int discoverySocket;
    int broadcast = 1;
    struct sockaddr_in address;
    struct sockaddr_in sender;
    long long start;
    char query[] = Clic_DISCOVERY_QUERY;

    servers = Network_createDiscoveryTable();
    if (!servers) return NULL;
    data = servers->userData;

    discoverySocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (discoverySocket == Clic_INVALID_SOCKET) {
        int savedError = errno;
        Table_free(servers);
        errno = savedError;
        return NULL;
    }

    setsockopt(discoverySocket, SOL_SOCKET, SO_BROADCAST, &broadcast,
               sizeof(broadcast));

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(Network_DISCOVERY_PORT);
    address.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    ssize_t broadcastSent = sendto(discoverySocket, query, strlen(query), 0,
           (struct sockaddr *)&address, sizeof(address));

    /*
     * Também consulta localhost: isso torna a descoberta utilizável em
     * atividades e testes feitos em uma única máquina.
     */
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    ssize_t localSent = sendto(discoverySocket, query, strlen(query), 0,
           (struct sockaddr *)&address, sizeof(address));
    if (broadcastSent < 0 && localSent < 0) {
        int savedError = errno;
        close(discoverySocket);
        Table_free(servers);
        errno = savedError;
        return NULL;
    }

    start = Network_milliseconds();

    while (true) {
        long long remaining = Network_DISCOVERY_TIME - (Network_milliseconds() - start);
        if (remaining <= 0) break;
        struct pollfd fd = { .fd = discoverySocket, .events = POLLIN };
        int result = Network_poll(&fd, 1, (int)remaining);
        if (result < 0) {
            int savedError = errno;
            close(discoverySocket);
            Table_free(servers);
            errno = savedError;
            return NULL;
        }
        if (result == 0) break;
        if (fd.revents & POLLIN) {
            char response[Client_NAME_SIZE + 40];
            socklen_t senderLength = sizeof(sender);
            ssize_t received = recvfrom(discoverySocket, response,
                                        sizeof(response) - 1, 0,
                                        (struct sockaddr *)&sender,
                                        &senderLength);

            if (received <= 0) {
                continue;
            }

            response[received] = '\0';

            char *kind = strtok(response, "|");
            char *name = strtok(NULL, "|");
            char *portText = strtok(NULL, "|");
            char senderIP[40];

            if (!kind || !name || !portText ||
                strcmp(kind, Clic_DISCOVERY_RESPONSE) != 0) {
                continue;
            }

            if (!inet_ntop(AF_INET, &sender.sin_addr, senderIP,
                           sizeof(senderIP))) {
                continue;
            }

            char *end;
            errno = 0;
            long port = strtol(portText, &end, 10);
            if (errno || *end || port <= 0 || port > 65535 || Clic_textWidth(name) < 0)
                continue;
            Network_addDiscoveredServer(data, name, senderIP, (int)port);
        }
    }

    close(discoverySocket);

    for (int index = 0; index < data->count; index++) {
        char portText[6];
        snprintf(portText, sizeof(portText), "%d", data->server[index].port);
        Table_addRow(servers, data->server[index].name,
                     data->server[index].ip, portText);
    }

    return servers;
}

int Network_discoveredCount(Table *servers) {
    NetworkDiscoveryData *data;

    if (!servers || !servers->userData) {
        return 0;
    }

    data = servers->userData;

    if (data->magic != Clic_DISCOVERY_MAGIC) {
        return 0;
    }

    return data->count;
}

Client *Network_connectDiscovered(Table *servers, int selectedIndex) {
    NetworkDiscoveryData *data;
    int index;

    if (!servers || !servers->userData) {
        errno = EINVAL;
        return NULL;
    }

    data = servers->userData;

    if (data->magic != Clic_DISCOVERY_MAGIC) {
        errno = EINVAL;
        return NULL;
    }

    index = selectedIndex - servers->firstSelectableIndex;

    if (index < 0 || index >= data->count) {
        errno = EINVAL;
        return NULL;
    }

    return Client_connect(data->server[index].ip, data->server[index].port);
}
