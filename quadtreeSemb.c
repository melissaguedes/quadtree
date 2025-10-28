#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_IMAGE_SIZE 2048 // dimensão máxima da imagem
#define MAX_STACK_SIZE 4096 // tamanho máximo da pilha

typedef struct __attribute__((packed)) { // garante que não haja bytes extras de alinhamento na struct
    unsigned short x, y; // coordenadas
    unsigned short size; // tamanho do bloco
    unsigned char value; // valor do pixel
} Block;

typedef struct {
    int x, y;
    int size;
} Node; // nó da pilha

Node stack[MAX_STACK_SIZE]; // array da pilha
int top = -1; // índice do topo da pilha

int image[MAX_IMAGE_SIZE][MAX_IMAGE_SIZE];
int output[MAX_IMAGE_SIZE][MAX_IMAGE_SIZE];

// empilha um nó na pilha
void push(int x, int y, int size) {
    if (top < MAX_STACK_SIZE - 1) {
        top++;
        stack[top].x = x;
        stack[top].y = y;
        stack[top].size = size;
    }
}

// desempilha um nó da pilha
Node pop() { return stack[top--]; }

// verifica se a pilha está vazia
int isEmpty() { return top == -1; }

// lê arquivo PGM P2 simples para 'image'
void pgmRead(const char *filename, int image[MAX_IMAGE_SIZE][MAX_IMAGE_SIZE], int *width, int *height) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("Erro ao abrir %s\n", filename); exit(1); }

    char fileFormat[3];
    fscanf(fp, "%s", fileFormat);
    if (strcmp(fileFormat, "P2") != 0) {
        printf("Formato inválido. Use P2.\n");
        fclose(fp);
        exit(1);
    }

    int maxVal;
    fscanf(fp, "%d %d %d", width, height, &maxVal);

    for (int i = 0; i < *height; i++)
        for (int j = 0; j < *width; j++)
            fscanf(fp, "%d", &image[i][j]);

    fclose(fp);
}

// escreve matriz 'img' em arquivo PGM P2
void pgmWrite(const char *filename, int img[MAX_IMAGE_SIZE][MAX_IMAGE_SIZE], int width, int height) {
    FILE *fp = fopen(filename, "w");
    if (!fp) { printf("Erro ao salvar %s\n", filename); exit(1); }

    fprintf(fp, "P2\n%d %d\n255\n", width, height);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++)
            fprintf(fp, "%d ", img[i][j]);
        fprintf(fp, "\n");
    }
    fclose(fp);
}

// verifica se bloco é uniforme (lossless)
int isBlockUniform(int x, int y, int size, int width, int height) {
    int first = image[y][x];
    for (int i = y; i < y + size && i < height; i++) {
        for (int j = x; j < x + size && j < width; j++) {
            if (image[i][j] != first)
                return 0;
        }
    }
    return 1;
}

// preenche bloco em 'output' com valor
void fillOutputBlock(int x, int y, int size, int width, int height, int value) {
    for (int i = y; i < y + size && i < height; i++) {
        for (int j = x; j < x + size && j < width; j++) {
            output[i][j] = value;
        }
    }
}

// codifica imagem para .qtb usando quadtree lossless
void quadtreeEncode(int width, int height, const char *outfile) {
    FILE *fp = fopen(outfile, "wb");
    if (!fp) { printf("Erro ao criar arquivo %s\n", outfile); exit(1); }

    fwrite(&width, sizeof(int), 1, fp);
    fwrite(&height, sizeof(int), 1, fp);

    top = -1;
    int size = 1;
    while (size < width || size < height) size *= 2; // encontra tamanho potência de 2 para cobrir imagem
    push(0, 0, size); // empilha bloco inicial

    while (!isEmpty()) {
        Node current = pop(); // desempilha bloco atual

        if (isBlockUniform(current.x, current.y, current.size, width, height)) {
            Block b;
            b.x = current.x;
            b.y = current.y;
            b.size = current.size;
            b.value = (unsigned char)image[current.y][current.x];
            fwrite(&b, sizeof(Block), 1, fp); // salva bloco uniforme
        } else {
            int half = current.size / 2;
            if (half >= 1) {
                // subdivide bloco em quatro e empilha cada um
                push(current.x, current.y, half);
                push(current.x + half, current.y, half);
                push(current.x, current.y + half, half);
                push(current.x + half, current.y + half, half);
            }
        }
    }

    fclose(fp);
}

// decodifica .qtb e preenche 'output'
void quadtreeDecode(const char *infile) {
    FILE *fp = fopen(infile, "rb");
    if (!fp) { printf("Erro ao abrir arquivo %s\n", infile); exit(1); }

    int width, height;
    fread(&width, sizeof(int), 1, fp);
    fread(&height, sizeof(int), 1, fp);

    Block b;
    while (fread(&b, sizeof(Block), 1, fp)) {
        fillOutputBlock(b.x, b.y, b.size, width, height, b.value); // preenche bloco decodificado
    }

    fclose(fp);
}

// calcula MSE entre imagens original e reconstruída
double calcMSE(int original[MAX_IMAGE_SIZE][MAX_IMAGE_SIZE], int reconstructed[MAX_IMAGE_SIZE][MAX_IMAGE_SIZE], int width, int height) {
    double mse = 0.0;
    long total = width * height;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
            double diff = original[i][j] - reconstructed[i][j];
            mse += diff * diff; // acumula erro quadrático
        }
    return mse / total; // retorna média do erro
}

// calcula e imprime taxa de compressão (%)
void calcCompressionRate(const char *originalFile, const char *compressedFile) {
    FILE *f1 = fopen(originalFile, "rb");
    FILE *f2 = fopen(compressedFile, "rb");
    if (!f1 || !f2) return;

    fseek(f1, 0L, SEEK_END);
    fseek(f2, 0L, SEEK_END);
    long s1 = ftell(f1);
    long s2 = ftell(f2);

    double rate = (1.0 - ((double)s2 / s1)) * 100.0;
    printf("Taxa de compressão: %.2f%% (arquivo reduzido de %ld bytes para %ld bytes)\n", rate, s1, s2);

    fclose(f1);
    fclose(f2);
}

// programa principal: lê, codifica, decodifica e avalia
int main(void) {
    printf("Compressão Quadtree Lossless \n");

    // char inputFile[100] = "img/logotipo-da-apple.pgm";
    // char qtFile[100] = "bin/logotipo-da-apple_quadtree.qtb";
    // char outputFile[100] = "bin/logotipo-da-apple_quadtree_reconstructed.pgm";

    // char inputFile[100] = "img/smpte.pgm";
    // char qtFile[100] = "bin/smpte_quadtree.qtb";
    // char outputFile[100] = "bin/smpte_quadtree_reconstructed.pgm";

    // char inputFile[100] = "img/nasa.pgm";
    // char qtFile[100] = "bin/nasa_quadtree.qtb";
    // char outputFile[100] = "bin/nasa_quadtree_reconstructed.pgm";

    char inputFile[100] = "img/coolcat.pgm";
    char qtFile[100] = "bin/coolcat_quadtree.qtb";
    char outputFile[100] = "bin/coolcat_quadtree_reconstructed.pgm";

    int width, height;
    pgmRead(inputFile, image, &width, &height); // lê imagem PGM
    printf("imagem carregada: %dx%d\n\n", width, height);

    printf("codificando...\n");
    quadtreeEncode(width, height, qtFile); // codifica imagem em quadtree
    printf("arquivo %s gerado! \n", qtFile);

    printf("decodificando...\n");
    quadtreeDecode(qtFile); // decodifica arquivo quadtree

    pgmWrite(outputFile, output, width, height); // salva imagem reconstruída
    printf("imagem reconstruída salva em %s\n", outputFile);

    double loss = calcMSE(image, output, width, height); // calcula erro quadrático médio
    calcCompressionRate(inputFile, qtFile); // calcula taxa de compressão

    printf("\nMSE (Mean Squared Error): %.6f\n", loss);
    if (loss == 0.0)
        printf("compressão sem perda.\n");
    else
        printf("compressão com perda (MSE > 0).\n");

    return 0;
}