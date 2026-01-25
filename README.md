# Screenshot Tool - Captura de Tela em JPG
<img width="1079" height="543" alt="ssw_-tela" src="https://github.com/user-attachments/assets/4a050181-8417-4453-8a24-e4b666add8ae" />

Ferramenta leve de captura de tela para Windows desenvolvida em C com SDL2, permitindo captura de tela completa ou seleção de região personalizada.

## Características

- ✅ Interface gráfica simples e intuitiva
- ✅ Captura de tela completa
- ✅ Seleção interativa de região com preview visual
- ✅ Salvamento em formato JPG de alta qualidade
- ✅ Controles de redimensionamento por handles nos cantos
- ✅ Movimentação da área selecionada por arrastar e soltar
- ✅ Overlay semi-transparente durante seleção
- ✅ Fonte bitmap customizada (sem dependências de TTF)

## Requisitos

### Bibliotecas Necessárias
- **SDL2** - Simple DirectMedia Layer 2
- **Windows API** (GDI) - Para captura de tela nativa
- **stb_image_write.h** - Para salvamento em JPG (header-only)

### Sistema Operacional
- Windows (utiliza GDI para captura de tela)

## Compilação

### MinGW/GCC
```bash
gcc main.c -o screenshot.exe -lSDL2 -lSDL2main -lgdi32 -mwindows
```

### Visual Studio
```bash
cl main.c /I"caminho\para\SDL2\include" /link SDL2.lib SDL2main.lib gdi32.lib /SUBSYSTEM:WINDOWS
```

## Uso

### Executar o Programa
```bash
screenshot.exe
```

### Interface Principal
Ao abrir o programa, você verá duas opções:

1. **TELA CHEIA** (botão azul)
   - Captura toda a tela instantaneamente
   - Salva como `captura_total.jpg`

2. **SELECIONAR REGIÃO** (botão verde)
   - Abre overlay de seleção interativa
   - Permite ajustar região manualmente

### Modo de Seleção de Região

**Controles:**
- **Arrastar dentro da região**: Move a área selecionada
- **Arrastar handles dos cantos**: Redimensiona a região
- **ENTER**: Confirma e captura a região selecionada
- **ESC**: Cancela a seleção

**Indicadores visuais:**
- Retângulo vermelho delimita a área de captura
- Handles brancos nos 4 cantos para redimensionamento
- Texto "ENTER PARA CAPTURAR" acima da região

## Arquivos de Saída

- `captura_total.jpg` - Captura de tela completa
- `captura_regiao.jpg` - Captura da região selecionada

Ambos salvos com qualidade JPG 90% no diretório de execução.

## Estrutura do Código

### Principais Funções

- `CaptureRegionGDI()` - Captura pixels da tela usando Windows GDI
- `SaveAsJpg()` - Converte surface SDL para JPG (com correção BGRA→RGBA)
- `SelectRegionAdvanced()` - Interface interativa de seleção de região
- `DrawChar()` / `DrawString()` - Renderização de texto bitmap customizado

### Font System

O programa inclui uma fonte bitmap 8x8 minimalista com suporte aos caracteres necessários para a interface (A, C, E, G, H, I, L, N, O, P, R, S, T, U e espaço).

## Dependências Externas

Certifique-se de ter o arquivo `stb_image_write.h` no diretório do projeto. Baixe em:
```
https://github.com/nothings/stb/blob/master/stb_image_write.h
```

## Configurações

### Ajustar Qualidade JPG
No arquivo fonte, linha da função `SaveAsJpg()`:
```c
stbi_write_jpg(filename, surface->w, surface->h, 4, surface->pixels, 90);
//                                                                    ^^ Altere este valor (1-100)
```

### Tamanho dos Handles
Defina no início do código:
```c
#define HANDLE_SIZE 10  // Pixels do handle de redimensionamento
```

## Limitações

- Apenas Windows (usa GDI para captura)
- Formato de saída fixo em JPG
- Nomes de arquivo fixos (captura_total.jpg e captura_regiao.jpg)

## Melhorias Futuras

- [ ] Suporte para PNG/BMP
- [ ] Diálogo para escolher local de salvamento
- [ ] Atalhos de teclado globais
- [ ] Histórico de capturas
- [ ] Anotações e edição básica
- [ ] Suporte multi-monitor

## Licença

Este projeto usa bibliotecas de terceiros:
- SDL2: [Zlib License](https://www.libsdl.org/license.php)
- stb_image_write: Public Domain

## Autor

Desenvolvido com SDL2 e Windows GDI.
