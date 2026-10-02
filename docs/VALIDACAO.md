# Validação da 0.1.1

## Evidências locais

Consulte `evidence/results.json`, `evidence/host.txt`, `evidence/inspection.txt`
e os dois logs de build nativo. `tools/validate.py` interrompe no primeiro erro;
o relatório final só é escrito se testes e comparação de binários passarem.

| Critério | Estado | Alcance |
|---|---|---|
| Diagnóstico no computador | Aprovado | Memória, pthreads, JIT x86, sinal, persistência e Vulkan no WSL |
| Adaptador de memória | Aprovado no modelo host | Aliases reais via memfd; não é execução das syscalls PS5 |
| PowerPC pelo Xenia | Aprovado no computador | 3 programas, resultados 42/4/0; 3 ciclos de criação e destruição |
| Build PS5 com núcleo e RADV | Aprovado | Vínculo nativo e inspeção de eboot/libc; não prova inicialização |
| Repetição do build | Ver evidence/results.json | Comparação SHA-256 no mesmo ambiente; não é build limpo independente |
| Abrir/fechar no PS5, DualSense e logs recuperados | Não testado | Precisa do testador |
| Memória, exceções e PowerPC no PS5 | Não testado | Precisa do testador |
| Compute e readback PS5/RADV | Não testado | Precisa do testador; WSL usou llvmpipe |
| Apresentação Vulkan / pipeline gráfico | Não testado | Ainda não implementados; o menu usa VideoOut |
| 3 sequências e carga por 10 minutos no PS5 | Não testado | Modo implementado; métricas parciais, carga de memória/threads |
| XEX, kernel Xbox, Xenos ou jogos | Não testado | Fora do protótipo atual |

**O plano ainda não está integralmente concluído.** A compilação e os testes
locais estabelecem uma base verificável; a aceitação no console e a integração
gráfica continuam pendentes. Nenhum resultado do WSL foi promovido a resultado PS5.

## Ambiente e próximos testes

Alvo informado: PS5 13.60 com Relapse, etaHEN, ShadowMountPlus e kstuff lite.
Ainda faltam modelo e versões exatas dos componentes. A pasta de aplicativo foi
gerada localmente para a pré-release no GitHub; não houve instalação ou envio ao console.

Antes de testar, seguir `TESTADOR.md`. Recolher todos os logs, último teste
iniciado, foto da tela e identificação do ambiente. Se algum teste falhar,
investigar antes de repetir a sequência. Ver `ARQUITETURA.md` para limitações
de threads, backing físico e métricas de memória.
