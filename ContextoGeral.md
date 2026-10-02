# ContextoGeral.md
## Visão — nome, descrição, problema, público
X360PS5: adaptação experimental de Xbox 360 para PS5; primeiro público é o testador técnico.
## Objetivo atual
Protótipo técnico nativo v0.1.0, alvo firmware 13.60 + Relapse.
## Estado — status, versão
0.1.1 experimental: aplicativo diagnóstico com núcleo Xenia e RADV compilado.
Nenhuma validação no PS5 realizada; critérios completos da v0.1.0 ainda pendentes.
## Stack — front, back, db, infra, integrações
C/C++, Xenia Canary, PS5 payload SDK público, Mesa/RADV via PS5_Vulkan; Ubuntu 26.04 no WSL.
## Features — ✅ feitas / 🚧 em progresso / 📅 planejadas
✅ Código de diagnóstico, builds separados, núcleo PowerPC→x64 vinculado,
adaptador de memória e overlays de plataforma. 🚧 Validação PS5, apresentação
Vulkan e consolidação da plataforma. 📅 Executável homebrew 360; depois jogos,
áudio e saves. Detalhes e evidências: docs/VALIDACAO.md.
## Decisões — decisão / motivo / impacto / quem decidiu
2026-10-02: usuário autorizou enviar o projeto ao GitHub pessoal `polarco`.
Repositório criado: https://github.com/polarco/X360PS5 (privado).
Publicação inicial concluída como v0.1.1, revisão documental da v0.1.0.
Essa autorização substitui a restrição anterior de publicação para este envio;
instalação no console continua fora do escopo. Autenticação polarco confirmada.
Usuário aprovou port nativo com primeiro marco técnico. Revisões fixas; resultados locais separados de hardware. Publicação no GitHub autorizada posteriormente; sem instalação automática no console.
## Estrutura de pastas
src/ aplicação; tools/ build; patches/ adaptações; tests/ testes; docs/ instruções; .deps/ fontes fixadas; dist/ artefatos.
## Fluxos principais
Compilar -> testar no computador -> pacote -> testador executa no PS5 -> recolher logs.
## Dívidas técnicas
Compatibilidade e desempenho de jogos ainda desconhecidos. Sem pipeline gráfico
ou renderizador Xenos integrado. Callbacks assíncronos de threads e recuperação
de mutex robusto não suportados. Backing de memória é antecipado; decommit não
devolve memória física. Port deve ser revisado com métricas do hardware.
## Pontos de atenção
Usuário informou Relapse + etaHEN + ShadowMountPlus + kstuff lite no firmware
13.60. Modelo e versões exatas pendentes. O pacote local não foi enviado nem
instalado; confirmar fluxo e identidade PPSA99361 antes do primeiro teste.
## Convenções — versionamento, branches, commits, organização
SemVer; feature minor, correção patch, quebra major; atualizar CHANGELOG e contextos a cada entrega.
## Histórico
2026-10-02: plano aprovado e implementação iniciada.
2026-10-02: três programas PPC executados no Xenia no WSL (42, 4 e 0);
aplicativo PS5 vinculado e inspecionado. Ainda não comprova execução no console.
Validação local final: dois CTests aprovados, três ciclos do teste Xenia e hashes
idênticos em dois builds nativos consecutivos. Evidências em docs/evidence.
## Próximos passos
Usar docs/TESTADOR.md para obter modelo/versões e validar abertura, DualSense,
logs, memória, recompilador e GPU no console. Implementar apresentação Vulkan
e pipeline gráfico antes de afirmar atendimento completo ao primeiro marco.
## Backlog
Carregar homebrew 360, integrar GPU Xenia, áudio, controles e saves.
## Divisão de trabalho — Codex: / Claude:
Codex: implementação atual. Claude: nenhuma task atribuída; não editar ClaudeContext.md.

### Apresentação no GitHub — 2026-10-02
Usuário solicitou página detalhada e bonita, com uso de IA explícito. README
com banner SVG, status, arquitetura, roadmap, créditos e declaração de IA.
Codex identificado como assistente usado; sem alegação de auditoria independente.

### Publicação concluída — 2026-10-02
https://github.com/polarco/X360PS5 (privado), pré-release v0.1.1 com aplicativo,
fontes e SHA256SUMS. Hashes dos três assets remotos conferidos.
Sem instalação no PS5; todas as pendências de hardware permanecem.

### Visibilidade pública — 2026-10-02
Usuário solicitou tornar o repositório público. Alteração aplicada e verificada
como PUBLIC em https://github.com/polarco/X360PS5. Referências anteriores a
privado descrevem o estado inicial. Alteração administrativa; versão do aplicativo
mantida em 0.1.1, sem novos binários ou mudanças na validação de hardware.
