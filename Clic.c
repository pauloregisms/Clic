#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include "Clic.h"
#include <limits.h>
#include <wchar.h>

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

    int key = 0;
    do {
        Table_printHighlight(table, true);

        key = Clic_keyCapture();

        switch (key) {
            case Key_UP:   
                if(table->highlightedIndex) 
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
