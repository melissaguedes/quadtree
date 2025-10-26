#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_SIZE 2048
#define MAX_STACK 4096

// Estrutura compacta para blocos quadtree
typedef struct __attribute__((packed)) {
    unsigned short x, y;  // posição
    unsigned short size;  // tamanho do bloco
    unsigned char value;  // valor do pixel
} Block;

// estrutura pilha
typedef struct {
    int x, y;
    int size;
} Node;

Node stack[MAX_STACK];
int top = -1;

// matrizes globais
int image[MAX_SIZE][MAX_SIZE];
int output[MAX_SIZE][MAX_SIZE];

// pilha
void push(int x, int y, int size) {
    if (top < MAX_STACK - 1) {
        top++;
        stack[top].x = x;
        stack[top].y = y;
        stack[top].size = size;
    }
}

Node pop() { return stack[top--]; }

int isEmpty() { return top == -1; }

// leitura e escrita de imagens pgm
void readPGM(const char *filename, int image[MAX_SIZE][MAX_SIZE], int *width, int *height) {
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("Erro ao abrir %s\n", filename); exit(1); }

    char magic[3];
    fscanf(fp, "%s", magic);
    if (strcmp(magic, "P2") != 0) {
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

void writePGM(const char *filename, int img[MAX_SIZE][MAX_SIZE], int width, int height) {
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
// funções auxiliares
int isUniformLossless(int x, int y, int size, int width, int height) {
    int first = image[y][x];
    for (int i = y; i < y + size && i < height; i++) {
        for (int j = x; j < x + size && j < width; j++) {
            if (image[i][j] != first)
                return 0;
        }
    }
    return 1;
}

void fillBlock(int x, int y, int size, int width, int height, int value) {
    for (int i = y; i < y + size && i < height; i++) {
        for (int j = x; j < x + size && j < width; j++) {
            output[i][j] = value;
        }
    }
}
// compressão Quadtree lossless
void quadtree_encode_lossless(int width, int height, const char *outfile) {
    FILE *fp = fopen(outfile, "wb");
    if (!fp) { printf("Erro ao criar arquivo %s\n", outfile); exit(1); }

    fwrite(&width, sizeof(int), 1, fp);
    fwrite(&height, sizeof(int), 1, fp);

    top = -1;
    int size = 1;
    while (size < width || size < height) size *= 2; // potência de 2
    push(0, 0, size);

    while (!isEmpty()) {
        Node current = pop();

        if (isUniformLossless(current.x, current.y, current.size, width, height)) {
            Block b;
            b.x = current.x;
            b.y = current.y;
            b.size = current.size;
            b.value = (unsigned char)image[current.y][current.x];
            fwrite(&b, sizeof(Block), 1, fp);
        } else {
            int half = current.size / 2;
            if (half >= 1) {
                push(current.x, current.y, half);
                push(current.x + half, current.y, half);
                push(current.x, current.y + half, half);
                push(current.x + half, current.y + half, half);
            }
        }
    }

    fclose(fp);
}
// decodificação quadtree
void quadtree_decode_lossless(const char *infile) {
    FILE *fp = fopen(infile, "rb");
    if (!fp) { printf("Erro ao abrir arquivo %s\n", infile); exit(1); }

    int width, height;
    fread(&width, sizeof(int), 1, fp);
    fread(&height, sizeof(int), 1, fp);

    Block b;
    while (fread(&b, sizeof(Block), 1, fp)) {
        fillBlock(b.x, b.y, b.size, width, height, b.value);
    }

    fclose(fp);
}
// métricas loss
double calculateLoss(int original[MAX_SIZE][MAX_SIZE], int reconstructed[MAX_SIZE][MAX_SIZE], int width, int height) {
    double mse = 0.0;
    long total = width * height;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
            double diff = original[i][j] - reconstructed[i][j];
            mse += diff * diff;
        }
    return mse / total;
}

void compressionRate(const char *originalFile, const char *compressedFile) {
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
// main
int main(void) {
    printf("Compressão Quadtree Lossless \n");

    char inputFile[100] = "img/nasa.pgm";
    char qtFile[100] = "bin/nasa_quadtree.qtb";
    char outputFile[100] = "bin/nasa_quadtree_reconstructed.pgm";

    int width, height;
    readPGM(inputFile, image, &width, &height);
    printf("imagem carregada: %dx%d\n\n", width, height);

    printf("codificando...\n");
    quadtree_encode_lossless(width, height, qtFile);
    printf("arquivo %s gerado! \n", qtFile);

    printf("decodificando...\n");
    quadtree_decode_lossless(qtFile);

    writePGM(outputFile, output, width, height);
    printf("imagem reconstruída salva em %s\n", outputFile);

    double loss = calculateLoss(image, output, width, height);
    compressionRate(inputFile, qtFile);

    printf("\nMSE (Mean Squared Error): %.6f\n", loss);
    if (loss == 0.0)
        printf("compressão sem perda.\n");
    else
        printf("compressão com perda (MSE > 0).\n");

    return 0;
}