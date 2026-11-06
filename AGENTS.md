# AGENTS.md — Regras permanentes do ArtPlus

## Objetivo
Reproduzir, com implementação própria e evidências verificáveis, as funcionalidades observáveis do ArtPro+ v24. Não copiar código, assets ou recursos proprietários da Esko.

## Sincronização obrigatória com GitHub
Repositório oficial: https://github.com/evaneiaw/artplus (branch padrão: master).

- Antes de cada tarefa, sincronize a branch com o GitHub e confira alterações locais/remotas. Nunca sobrescreva trabalho do usuário.
- Ao concluir cada tarefa que altere arquivos, faça commit e push para o repositório oficial. O trabalho não pode ficar apenas local.
- Nunca publique segredos, tokens, credenciais, dados pessoais, executáveis/DLLs proprietários do ArtPro+, dependências comerciais ou artefatos grandes de build. Use .gitignore.
- Não use comandos destrutivos como git reset --hard, git clean, checkout forçado ou rebase destrutivo.
- Se o push falhar, preserve as alterações e informe o erro exato. Não declare a tarefa concluída.
- Não crie commits vazios. Em tarefas sem alterações, informe que não houve mudança nem commit.

## Relatório obrigatório ao final de cada tarefa
Toda resposta final do harness/DSH deve conter as seções **Sincronização GitHub**, **Arquivos criados/modificados/removidos** e **Arquivos para revisão do ChatGPT**.

Informar obrigatoriamente:
1. Branch utilizada.
2. Commit SHA e link do commit, se criado.
3. Confirmação real do push (nunca presumir sucesso).
4. Lista completa de TODOS os arquivos criados, modificados, renomeados ou removidos, com caminho relativo e descrição da mudança. Não listar apenas o relatório: incluir fontes, headers, testes, configurações, scripts, documentação e recursos.
5. Build e testes executados, resultados e limitações; distinguir testes automatizados de validação visual.
6. Arquivos consultados que sejam relevantes, mas não foram alterados.

## Handoff para revisão pelo ChatGPT
- O ChatGPT revisará o estado publicado no GitHub; todo arquivo alterado necessário para entender/validar a tarefa deve estar no commit e push, respeitando segurança e propriedade intelectual.
- Liste caminhos exatos de TODOS os arquivos que o ChatGPT deve ler, separando **Ler obrigatoriamente** (todos os arquivos alterados relevantes: código, testes, configuração e docs) de **Contexto adicional** (arquivos existentes consultados).
- Forneça links diretos do commit e, quando possível, dos arquivos.
- Se um arquivo necessário não puder ser publicado (binário proprietário, segredo ou artefato excluído), informe caminho local, motivo e resumo técnico. Nunca finja que o ChatGPT pode acessá-lo.
- Não peça ZIP ou reenvio de arquivos já publicados no repositório.

## Qualidade técnica
- Não invente APIs, classes, assinaturas, atalhos, comportamentos, nomes de DLL ou resultados de engenharia reversa.
- Separe evidências confirmadas (código, testes, análise estática, execução dinâmica, comparação visual) de hipóteses e lacunas.
- Compilar ou passar testes unitários não comprova funcionamento visual. Valide a GUI quando aplicável.
- Preserve funcionalidades e testes existentes; não crie stubs/simulações nem enfraqueça testes.
- Evite auditorias/documentação repetidas; priorize execução concreta e atualize status com evidências.

## Regra para toda instrução de execução
Toda instrução que peça implementação ou alteração deve exigir antes de encerrar: build/testes pertinentes; atualização dos status afetados; commit e push; relatório completo de arquivos criados/modificados/removidos e lista dos arquivos que o ChatGPT deve ler.

Estas regras são permanentes para todos os agentes, harnesses e sessões do repositório. Preserve sempre a segurança, os dados do usuário e o histórico Git.
