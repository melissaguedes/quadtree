## Descrição

O algoritmo de compressão Quadtree é amplamente utilizado em processamento de imagens e gráficos, principalmente para compressão sem perda de qualidade.
Ele funciona subdividindo a imagem em quatro quadrantes até que cada bloco seja considerado “uniforme” o suficiente para ser representado por um único valor médio.

Neste projeto:
	•	A imagem é lida em formato PGM (grayscale).
	•	É gerada uma versão comprimida no formato .qtb (Quadtree Binary).
	•	Em seguida, o programa decodifica o arquivo e reconstrói a imagem original.
	•	São calculadas as métricas:
	•	Taxa de compressão
	•	MSE (Mean Squared Error)

## Compilação

Para compilar o código, utilize o comando abaixo:

```bash
clang quadtreeSemb.c -lm -o quadtree
```
Após a compilação, execute o programa com:

```bash
./quadtree
```
Exemplo de saída:

```bash
Compressão Quadtree Lossless 
imagem carregada: 1080x1080

codificando...
arquivo bin/nasa_quadtree.qtb gerado! 
decodificando...
imagem reconstruída salva em bin/nasa_quadtree_reconstructed.pgm
Taxa de compressão: 56.92% (arquivo reduzido de 4075552 bytes para 1755741 bytes)

MSE (Mean Squared Error): 0.000000
compressão sem perda.
```
