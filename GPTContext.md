# GPTContext.md
## Foco da sessão
Implementar X360PS5 v0.1.0: protótipo nativo baseado em Xenia Canary para PS5 13.60 com Relapse.
## Tasks em andamento
- Publicação inicial solicitada em `polarco/X360PS5`: preparando commit local;
  GitHub CLI instalado no WSL, aguardando autenticação via device flow.
  Manter v0.1.0, pois é a publicação da entrega existente, sem mudança de código.
- Builds WSL e PS5 preparados; dependências fixadas em dependencies.lock.json.
- Diagnóstico e núcleo Xenia vinculados; validação local em docs/evidence.
- Pendente: execução no PS5, apresentação Vulkan/pipeline gráfico e suporte
  completo à plataforma. Não chamar o port de emulador funcional de jogos.
## Observações / hipóteses
Console disponível através do amigo do usuário: firmware 13.60, Relapse,
etaHEN, ShadowMountPlus e kstuff lite informados. Modelo e versões exatas dos
payloads pendentes. “Fala pra ele” era destinado ao Codex, sem encaminhamento.
## Descobertas no código
Xenia não tem plataforma PS5; memória POSIX consulta /proc/self/maps. PS5CEMU distingue acesso a /data de permissão JIT. RADV tem receita de link e SDK próprios.
O adaptador próprio não consulta /proc. Overlays ficam em build/ps5/xenia-overlay
e são gerados por tools/xenia-overlay.py; upstream permanece identificável.
Contexto PS5 usa mc_rip e mc_fpstate; ucontext do SDK difere de FreeBSD genérico.
Teste real Xenia em src/xenia_core_probe.cpp; src/xenia_probe.cpp é apenas o
marcador explícito do diagnóstico host sem núcleo, que tem teste separado.
O SDK exportado usado é o pin 95c08f2, mesmo que o checkout auxiliar tenha HEAD
diferente. Arquivo de fontes usa o pin. Não tratar checkout auxiliar como build.
## Rascunhos pendentes (não promovidos)
Resultados no hardware: não testados. Não declarar jogos ou recompilador funcionais sem evidência.
### Validação local final
tools/validate.py passou: 2 testes CTest, três programas PPC em três ciclos,
inspeção de eboot/libc e dois builds com hashes iguais no mesmo ambiente.
Eboot SHA-256: 1ed541e48d24b3e0e64c39d24471b6a5d89bfa4565093ee49e11da0464efa380.
Artefatos locais esperados: dist/X360PS5-0.1.0-diagnostic.zip,
dist/X360PS5-0.1.0-sources.tar.gz e dist/SHA256SUMS. Sem publicação ou instalação.
## Bugs / atritos
Toolchain instalada no Ubuntu 26.04. Git Linux/DrvFS foi lento para conferir
Mesa; prepare.py usa git.exe nessa conferência quando disponível. Erros de
inclusão local de platform.h resolvidos com -include e -iquote.
Sem suporte a callbacks de thread/mutex robusto: falha explícita. Memória
física antecipada e decommit sem devolução são limitações registradas.
## Ideias para discutir com Claude
Nenhuma colaboração solicitada.
