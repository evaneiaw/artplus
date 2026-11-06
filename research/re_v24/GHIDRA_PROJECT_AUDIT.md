# GHIDRA_PROJECT_AUDIT — estado real do projeto ArtProV24

Data: 2026-11-03 (horário local)
Projeto: `research/re_v24/ghidra/ArtProV24`
Binário: `C:\Esko\bg_prog_artpro+_v240\ArtPro+.app\Contents\Windows\ArtPro+.exe`
SHA-256: `9D33166126305F25C50349BB881F7D9ACDC996F3C0C9181D0BE61295D70B0F03`
Versão: 24.03.10017 — **Authenticode HashMismatch** (ver V24_REFERENCE.md)

## 1. Estado do projeto

| Item | Estado |
|---|---|
| Projeto criado | sim (`ArtProV24.gpr`, `ArtProV24.rep`) em 2026-11-03 00:33 |
| Executável importado | sim, loader PE, linguagem `x86:LE:64:default:windows` |
| Base de imagem | `140000000` (confirmado no log do loader) |
| Análise automática | **em andamento** (processo `java` ativo; log em crescimento) |
| Fase atual do log | processamento de RTTI/TypeDescriptors e decompilação |
| Lock do projeto | presente (`ArtProV24.lock`) — impede abrir em paralelo |
| DLLs importadas no projeto | **não** (apenas o executável foi importado; bibliotecas foram resolvidas como exports files, não salvas) |
| Exportação de funções | ainda não executada |

## 2. O que já foi comprovado

- O binário é PE64, MSVC 19.38.33134, Qt 6.3.1, e carrega **95 DLLs** no
  import table (seção 3 de `MAIN_EXECUTABLE_ANALYSIS.md`).
- Muitas funções têm nomes de RTTI/símbolos ricos (o log mostra tipos como
  `ArtProPlus::UploadFile::TransmissionStatus`, `Localization::Quantity`,
  `GMGOpenColorModel::Exception`, `QString`, `QHeaderView`), o que indica
  binário com RTTI e classes C++ exportadas — útil para nomear estruturas.
- O exe importa `LoadLibraryA`, `LoadLibraryW`, `LoadLibraryExA` e
  `GetProcAddress`, portanto parte dos módulos é carregada dinamicamente e
  **não** aparece no import table (ex.: `PDFObjectActions.dll` não está entre
  as 95 dependências diretas).

## 3. O que ainda precisa ser feito

1. Aguardar o término da análise automática (sem reiniciar o projeto).
2. Contar funções definidas/indefinidas e exportar lista
   (`FunctionManager`), separando `FUN_*` de funções nomeadas.
3. Importar e analisar as DLLs prioritárias **em projetos próprios**
   (`ArtProV24_PDFObjectActions`, etc.), sem tocar neste projeto.
4. Mapear XREFs das APIs de UI (Qt) e de documento para os fluxos
   abrir/selecionar/editar/salvar.
5. Comparar funções com a documentação da v25 (`14_FUNCTION_CATALOG.md`).

## 4. Observações de método

- O log contém milhares de warnings de decompilação em endereços pseudo
  (`ram:20ccccccc`, `ram:1645c8948`). Isso é esperado em binários grandes com
  RTTI/vtables e não indica falha da análise; as funções continuam sendo
  criadas.
- Nenhuma função foi renomeada manualmente até agora.
- A análise foi iniciada com `-analysisTimeoutPerFile 2400`; se o timeout for
  atingido, o Ghidra salva o que já foi analisado (estado parcial, documentado).

## 5. Conclusão honesta

A análise **não terminou**; não é possível afirmar contagem de funções nem
cobertura. O projeto existe, o executável está importado e a análise automática
está ativa. Este relatório será atualizado com números reais quando o processo
concluir.
