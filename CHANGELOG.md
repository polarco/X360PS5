# Changelog
## 0.1.0 — 2026-10-02 — feature (protótipo experimental)
- Preparação da publicação inicial no GitHub pessoal `polarco`, autorizada pelo usuário; mesma versão do protótipo, sem alteração do binário.
- Início do protótipo técnico X360PS5, alvo PS5 13.60 + Relapse.
- Dependências fixadas e referências técnicas registradas.
- Validação em hardware pendente; esta versão não afirma compatibilidade com jogos.
- Build separado WSL/PS5; RADV e núcleo real do Xenia vinculados ao aplicativo.
- Adaptador PS5 de memória compartilhada, proteção por página, relógios,
  contexto de exceções, threads e arquivos; overlays preservam o upstream.
- Menu DualSense/VideoOut, logs persistentes com readback, testes de memória,
  pthreads, JIT x86, exceções, três programas PowerPC e compute/readback Vulkan.
- Sequência repetida três vezes, carga controlada de memória/threads por dez
  minutos e métricas parciais de memória. Execução desses modos no PS5 pendente.
- Timeout de GPU bloqueia novas submissões; falhas interrompem a sequência.
- Pacote de fontes, aplicativo e hashes; roteiro para Relapse + etaHEN,
  ShadowMountPlus e kstuff lite. Nenhuma instalação/publicação realizada.
- Limitações: sem apresentação Vulkan, pipeline gráfico/Xenos ou carregador de
  jogos; callbacks de thread e recuperação de mutex robusto ainda não suportados.
