# Evidência — Rotate View (renderizações do ArtPlus)

Artefactos produzidos pelo **ArtPlus**, não pelo ArtPro+. Guardados fora de
`research/artpro_analysis/visual_reference/` para essa árvore conter apenas
capturas do aplicativo de referência.

## Como reproduzir

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd H:\Desktop\artplus-lite\build-msvc-nmake
cmake --build . --parallel 8
cd tests\runtime
..\..\bin\artplus_pdf_smoke.exe artplus_edit_multipage_fixture.pdf ^
    H:\Desktop\artplus-lite\research\validation_evidence\rotate_view
```

## Ficheiros

| Ficheiro | Dimensões | O que mostra |
|----------|-----------|--------------|
| `render_page01.png` | 918 x 1188 | Página 1, sem rotação de visualização |
| `render_page01_rot000.png` | 918 x 1188 | 0° — orientação original (retrato) |
| `render_page01_rot090.png` | 1188 x 918 | 90° horário — paisagem, texto vertical, retângulo vermelho passa da esquerda para a direita |
| `render_page01_rot180.png` | 918 x 1188 | 180° — texto invertido, vermelho em baixo à direita |
| `render_page01_rot270.png` | 1188 x 918 | 270° — paisagem no sentido inverso |
| `render_page02.png` | — | Segunda página |

As dimensões confirmam a troca largura/altura em 90° e 270° e a manutenção em
0°/180°; a inspeção visual confirma o **sentido horário** da rotação.

## Verificações automáticas correspondentes

Em `tests/pdf_smoke.cpp`, secção *View rotation rendering*:

- 90° e 270° trocam as dimensões da imagem renderizada;
- 180° mantém as dimensões;
- uma volta completa repõe as dimensões originais;
- as imagens de 90°/180°/270° **diferem** da imagem a 0° (a rotação chega aos
  pixels, não apenas ao estado);
- o ficheiro em disco mantém o SHA-256 original.

## Limitação

Estas imagens provam que a rotação de visualização funciona sobre o conteúdo
renderizado. **Não** provam paridade visual com o ArtPro+ v24: para isso é
necessário capturar a ferramenta no original e guardar em
`research/artpro_analysis/visual_reference/` (atualmente vazia).
