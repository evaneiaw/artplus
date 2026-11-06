# ArtPlus — Regras permanentes de fidelidade ao ArtPro+

**Princípio absoluto:** o ArtPlus não deve ter mais, menos ou diferentes
funcionalidades do que o escopo funcional do ArtPro+. O trabalho é reproduzir
suas capacidades, não redesenhar o produto.

Este documento é normativo. Vale para todas as missões, sem exceção
(Missão 23 e seguintes, Geometry Engine, Color Engine, separações, trapping,
Step & Repeat, marks, preflight, automação, salvamento, edição, UI/UX).

## 1. Referência oficial

O **Esko ArtPro+ v24 (24.03.10017) instalado nesta máquina** é a referência
operacional principal para:

- estrutura da interface, menus, submenus, barras de ferramentas, painéis, abas;
- ferramentas, comandos, propriedades, atalhos;
- fluxos de trabalho, comportamento das operações e organização das funções.

A **v25 (25.03.116)** permanece como referência técnica complementar (pesquisa
já produzida). Diferenças entre versões ficam em `V24_V25_COMPARISON.md`;
nunca presuma equivalência por nome de módulo. Registre sempre a versão da
evidência.

Não são prova suficiente: nome de classe no binário, string isolada, hipótese da
documentação de engenharia reversa. **Prova é observação direta do programa ou
captura inequívoca.**

## 2. Proibição de invenção

É proibido criar, sem correspondência comprovada no ArtPro+:

- funcionalidades, ferramentas, comandos, menus, abas, painéis;
- atalhos de teclado e fluxos de trabalho;
- reorganizações de interface por preferência pessoal;
- recursos "úteis" ou "modernos" que o original não tem.

Exemplos diretos:

- se o ArtPro+ não tem uma aba "Objeto", o ArtPlus não cria essa aba;
- se uma propriedade aparece em outro painel no original, ela deve ir para o
  local correspondente, não para um painel novo;
- se uma ferramenta está em determinado menu do original, essa é a localização;
- se o original não tem o comando, ele não entra no ArtPlus por iniciativa
  própria.

Elementos técnicos mínimos para abrir documentos, diagnosticar e desenvolver o
software são permitidos, mas **não podem ser confundidos com funcionalidades do
produto final** e devem estar marcados como tal no mapeamento.

## 3. Mapeamento obrigatório antes de implementar

Antes de criar qualquer elemento visível, registrar em
`FEATURE_MAPPING_ARTPRO.md` / `UI_MAPPING_ARTPRO.md`:

1. Nome do recurso no ArtPro+.
2. Localização exata na interface original.
3. Menu, submenu, painel ou ferramenta a que pertence.
4. Comportamento observado.
5. Evidência que confirma a existência (arquivo de captura, caminho, versão).
6. Correspondência planejada no ArtPlus.

Classificações obrigatórias:

- **CONFIRMADO** — observado diretamente ou comprovado por evidência inequívoca.
- **FORTE INDÍCIO** — evidência técnica consistente, comportamento não confirmado.
- **NÃO CONFIRMADO** — evidência insuficiente.

Somente recursos **CONFIRMADOS** podem ser apresentados como funcionalidades
correspondentes ao ArtPro+. Não confirmados permanecem fora da interface final
até serem investigados.

## 4. Fidelidade estrutural da interface

A interface acompanha a organização do original: menus superiores, submenus,
barras, painéis laterais, abas, janelas de configuração, context menus,
ferramentas de seleção, propriedades, navegação e organização do workspace.

Não criar organização alternativa por conveniência de programação.

Função ainda não implementada pode constar do planejamento, mas **não** pode ser
apresentada como ferramenta inventada. Itens não implementados de menus
confirmados ficam desabilitados; submenus não capturados usam o texto
"Not implemented in ArtPlus yet" em vez de comandos inventados.

## 5. Diante de dúvida

1. Investigue o programa original (captura, inspeção, versão).
2. Consulte os documentos de pesquisa.
3. Examine recursos técnicos e referências disponíveis.
4. Compare versões quando necessário.
5. Registre a dúvida no mapeamento.

**Na dúvida, não invente. Investigue e aguarde confirmação.**

## 6. Correção da interface já criada

Auditoria concluída em 2026-11-03. Removidos por falta de correspondência:

| Elemento removido | Motivo | Substituto confirmado |
|---|---|---|
| Painel dock "Object" | não existe no ArtPro+; propriedades ficam em Inspector (não capturado) | `Object → Transform…` (Ctrl+Shift+M), comando confirmado |
| Atalho oculto Ctrl+setas (nudge) | atalho inventado, não existe no original | `Object → Transform…` |
| `Window → Reset Layout` | conteúdo do menu Window não confirmado | removido |
| `View → Rotate View → Rotate 90°` | submenu existe, conteúdo não capturado | submenu com placeholder desabilitado |
| `File → Save As Copy…` | comando não existe no original | API/teste interno apenas |

Mantidos como técnicos (não são funcionalidades do produto):

- barra de status com página/zoom/coordenadas (mapeamento pendente);
- `Help → About ArtPlus...` (conteúdo do menu Help não capturado);
- painel de mensagens de diagnóstico.

## 7. Definição de pronto

Uma entrega só é aceita quando:

1. Cada elemento visível tem linha no mapeamento com classificação e evidência.
2. Nenhum elemento **NÃO CONFIRMADO** aparece como funcionalidade do produto.
3. Itens confirmados mas não implementados estão desabilitados/placeholders.
4. Testes e build passam, e a documentação registra o que falta confirmar.
