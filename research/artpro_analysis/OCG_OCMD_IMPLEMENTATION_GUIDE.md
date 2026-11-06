# ARTPLUS — IMPLEMENTAÇÃO DE OCG/OCMD EM PDFs

## 📊 STATUS ATUAL (2026-11-05)

### ✅ O QUE JÁ EXISTE
- Painel Layers: `QStringLiteral("Layer 1")` fixo — não reflete camadas reais
- Painel Separations: 4 CMYK hardcodadas com label `(profile-based)` + aviso de limitação
- `updateDocumentInfo()` — exibe metadados (pages, size, title, author, etc.)
- `PdfSnapshot::capture(document)` — captura snapshot do documento
- `snapshot.objectCount()`, `snapshot.countOf(kind)` — contagem de objetos por tipo

### ❌ O QUE NÃO ESTÁ DISPONÍVEL (PDFium 158 deste projeto)
- `FPDFDoc_GetOCG()` — extração de OCGs ✗
- `FPDFDoc_GetOCMD()` — extração de dicionários OCMD ✗
- `FPDFPage_GetOCGVisibilité()` — visibilidade de OCG ✗
- Qualquer função pública para OCG/OCMD nos headers deste projeto ✗

### 📋 GHIDRA EVIDENCE (2026-11-05)
- `PDFObjectActions.dll`: `GOVCheckOCVisibility` — verificação de visibilidade de OCG/OCMD
- `PreflightProfileLayer.dll`: `GpetInkCoverage`, `ConvertToCheckActionList()`
- **CRÍTICO**: Essas DLLs **não estão carregadas** no ArtPlus atual

### 🚫 RESTRIÇÕES OBRIGATÓRIAS (da diretiva)
1. **Não simular camadas com contagem de objetos** — "Layer 1" fixo não conta como camada real
2. **Implementar alteração de visibilidade somente se houver suporte seguro** — deve testar renderização
3. **Testar com PDFs reais** — ter PDFs com camadas visíveis e ocultas
4. **Não inventar estruturas** — usar apenas o que a API fornece

---

## 🎯 OBJETIVO DA IMPLEMENTAÇÃO

Ler estrutura OCG/OCMD real em PDFs usando apenas APIs PDFium 158 disponíveis, sendo **100% honesto** sobre limitações, e integrar informações ao painel Layers como **dados adicionais** (NÃO substituindo "Layer 1").

### ⚠️ PRINCÍPIO DIRETOR
- **Não simular camadas** com contagem de objetos
- **Não oferecer controle de visibilidade** ao usuário sem renderização testada
- **Integrar informações** ao painel Layers como dados adicionais
- **Documentar limitações** claramente

---

## 📋 ARQUITETURA DE LEITURA OCG/OCMD

### Estrutura OCG no PDF (especificação padrão)

```
PDF Catalog
  └── /D — Catálogo de documentos
       └── /OC / — Opções de conteúdo
            ├── /OCProperties — Propriedades de grupos de conteúdo opcional
            │   ├── /D — Dicionário de OCGs
            │   │   ├── /Name — Nome do grupo de conteúdo
            │   │   ├── /P — pais (herança de visibilidade)
            │   │   └── /V — Visibilidade (ON / OFF)
            │   └── /Ordering — Ordem de renderização
            │   └── /ContentOrder — Ordem de conteúdo
            └── /RB — Fator de rendering brightness
                
Objeto PDF individual
  └── /OCMD / — Dicionário de Membership Opcional Content
       ├── /OC — Lista de IDs de OCGs (ocIDs) aos quais este objeto pertence
       └── /V — Visibilidade override (Always / Never / Inverted / Auto)
```

### Estados de Visibilidade
- **/ON** — OCG visível
- **/OFF** — OCG oculto
- **/Auto** — Herda do pai / propriedades do documento
- **/Inverted** — Inverte a visibilidade do pai

### Dicionário /OCProperties
- /D — Dicionário de definição de OCG
  - /Name — Nome identificador do OCG
  - /P — Padre (herança)
  - /V — Visibilidade corrente
  - /A — Visibilidade alterada pelo usuário
  - /C — Se alterações são permitidas

---

## 🛠️ IMPLEMENTAÇÃO CONCRETA

### 1. Ler estrutura OCG no `openDocument()`

Vou adicionar código no `openDocument()` para extrair informações OCG do catálogo do documento.

### 2. Ler estados ON/OFF

Adicionar código para ler /ON e /OFF no conteúdo do PDF.

### 3. Ler dicionários /OCMD em objetos

Verificar quais OCGs cada objeto pertence via /OCMD.

### 5. Atualizar painel Layers com informação OCG

Adicionar informação ao painel Layers como dado adicional.

### 5. Documentar limitações

Garantir que o usuário saiba o que foi lido e o que não foi.

---

## 📋 PRÓXIMOS PASSOS TÉCNICOS

### 1. Investigar estrutura real de PDFs de teste
- Abrir PDFs com OCGs conhecidos no ArtPlus
- Verificar catálogo e dicionários internos
- Identificar o que PDFium 158 expõe efetivamente

### 2. Implementar leitura no código C++
- Adicionar funções para extrair /OCProperties do catálogo
- Extrair /OCMD de objetos individuais
- Extrair estados /ON, /OFF, /Auto

### 3. Integrar ao painel Layers
- Adicionar informação OCG como dado adicional
- Manter "Layer 1" fixo (princípio da regra)
- Exibir estados de visibilidade como informação adicional

### 5. Testar com PDFs reais
- PDFs com camadas visíveis e ocultas
- Verificar renderização
- Documentar resultados

### 6. Documentar limitações
- O que foi lido com sucesso
- O que não foi possível extrair
- Caminho para futura integração com DLLs ArtPro+ (GOVCheckOCVisibility)

---

## 📋 GUIA RÁPIDO DE IMPLEMENTAÇÃO

### Código a ser adicionado em `src/ui/src/MainWindow.cpp`:

```cpp
// No openDocument(), após capturar o snapshot:
if (m_doc && m_doc->isValid()) {
    // Verificar catálogo para /OCProperties
    // Exemplo pseudo-código:
    // QByteArray catalog = m_doc->getCatalog();
    // Extrair /OCProperties, /OCGs, /D, /ON, /OFF
    
    // Atualizar painel Layers com informação OCG
    // APENAS como dado adicional, NÃO substituindo "Layer 1"
}
```

### Informações a exibir no painel Layers:

```
Layer 1  (fixo)  [OCG info: 3 grupos encontrados]
  - Objeto 1: Path - Fill:Sim Stroke:Não [OCG: "Imagens"]
  - Objeto 2: Text - Fill:Não Stroke:Sim [OCG: "Texto"]
  - Objeto 3: Image - Fill:Sim Stroke:Não
```

### Limitações a documentar:

```
IMPORTANTE: 
- As APIs PDFium 158 deste projeto não fornecem acesso direto a OCGs/OCMD
- Estas informações foram extraídas da estrutura interna do PDF
- Para controle completo de visibilidade: necessário carregar DLLs ArtPro+ v24
- "Layer 1" permanece fixo e não reflete estrutura OCG real
- Estados de visibilidade (/ON, /OFF) são exibidos como informação adicional
```

---

## ✅ CRITÉRIO DE CONCLUSÃO (Regra 12)

Uma funcionalidade de OCG/OCMD só pode ser marcada como concluída quando:

1. ✅ O código existe e está integrado ao aplicativo
2. ✅ A interface exibe informação sobre OCGs encontrados (como dado adicional)
3. ✅ O comportamento foi testado com PDFs reais
4. ✅ Não há declaração de paridade com ArtPro+ sem evidências
5. ✅ Limitações são claramente documentadas
6. ✅ Não se simula camadas com contagem de objetos

---

## 📝 REGISTRO DE ALTERAÇÕES

### 2026-11-05 — Implementação Inicial

- Criado guia de implementação OCG/OCMD
- Documentadas limitações do PDFium 158
- Planejada integração no painel Layers como dado adicional
- Mantido "Layer 1" fixo (princípio da regra)
- Não oferecido controle de visibilidade ao usuário (sem renderização testada)

### Próximos passos:
1. Investigar estruturas reais de PDFs de teste
2. Implementar leitura no código C++
3. Testar com PDFs reais (visível/oculto)
4. Integrar informação ao painel Layers
5. Documentar resultados finais

---

*Esta implementação segue rigorosamente a Regra 12 do ArtPlus Master Development Directive:
"Uma funcionalidade só pode ser marcada como concluída quando: O código existe, Está integrado ao aplicativo, A interface exibe a ação, O comportamento foi testado, Não há regressões, O resultado foi comparado com a referência quando aplicável, Nunca declarar paridade com ArtPro+ sem evidências."*