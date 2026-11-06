# Referências visuais — ArtPro+ v24

Pasta central para as capturas de tela e evidências visuais usadas para
reproduzir o ArtPro+ v24 no ArtPlus.

Regra permanente associada: `AGENTS.md` § 16 — *Diretiva de fidelidade
absoluta*. Toda interface implementada no ArtPlus deve ser comparada com uma
captura guardada aqui (ver `AGENTS.md` § 8 — comparação visual obrigatória).

## Estrutura

| Pasta | Conteúdo esperado |
|-------|-------------------|
| `main_window/` | Janela principal, barra de título, área de trabalho, canvas, barra de status, divisores |
| `menus/` | Menu bar e submenus abertos (File, Edit, Object, Select, Prepress, View, Window, Help) |
| `toolbars/` | Barras de ferramentas, ícones, estados (normal/hover/ativo), tooltips |
| `panels/` | Document, Layers, Separations, Quality Control, Transform, Marks e demais painéis |
| `dialogs/` | Diálogos modais: Transform, Preflight, Export, e estados de erro/confirmação |
| `tools/` | Ferramentas individuais e o seu feedback visual |
| `workflows/` | Sequências de operação passo a passo com antes/depois |

## Convenção de nomes

```
<area>__<elemento>__<estado>__<versao>__<data>.png
```

Exemplos:

- `menus__object-menu__open__v24__2026-11-05.png`
- `panels__quality-control__default__v24__2026-11-05.png`
- `dialogs__transform__with-selection__v24__2026-11-05.png`

## Regras de uso

1. **Toda captura deve identificar a versão de referência** (v24 é a principal).
2. Capturas de versões posteriores (v25) só podem ser usadas quando a v24 não
   esclarecer o comportamento, e devem ser marcadas como tal.
3. Não armazenar capturas de terceiros sem indicar a origem.
4. Uma ferramenta **não** pode ser marcada como concluída sem uma captura de
   referência correspondente e a comparação registrada.

## Estado atual

As pastas foram criadas como a estrutura normativa de organização, mas ainda
**não contêm capturas**. Consequência prática, registrada para não gerar falsa
impressão de completude:

- Nenhuma ferramenta pode ser declarada "comparada com a referência" enquanto
  esta pasta estiver vazia.
- As capturas citadas em `AGENTS.md` (registro de 2026-11-03) vivem em
  `C:/Users/Evanei/AppData/Local/Temp/` e são temporárias; precisam ser
  copiadas para cá para servirem de evidência permanente.
