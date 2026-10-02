# Arquitetura do protótipo 0.1.0

O aplicativo PS5 vincula RADV e um subconjunto real do Xenia: memória, frontend
PowerPC, IR/otimizadores e backend x64. `src/xenia_core_probe.cpp` cria uma arena,
carrega três programas próprios em big-endian e executa as funções recompiladas.
Não contém substituto por interpretador. Não carrega XEX, kernel Xbox ou jogos.

## Camada de plataforma

- `patches/xenia/platform.patch` define `XE_PLATFORM_PS5` para `__PROSPERO__`.
- `port/xenia/memory_ps5.cc` usa reservas virtuais e objetos de memória direta
  do SDK, acompanha aliases e permissões por página de 16 KiB. Não consulta `/proc`.
- `tools/xenia-overlay.py` exporta adaptações identificáveis sem editar o Xenia:
  relógio monotônico, contexto de sinais PS5, threads e caminhos de arquivos.
  Cabeçalhos locais do upstream recebem a plataforma com `-include`.
- Arquivo executável: `/app0/eboot.bin`. Dados graváveis: `/data/x360ps5`.
  Nenhuma busca de HOME Linux ou `/proc/self/exe` no adaptador de arquivos.
- Fila de callbacks para thread e recuperação de mutex com dono morto não foram
  implementadas: produzem exceção explícita. Não são usadas pelo teste aritmético.
- Objetos compartilhados alocam backing físico antecipadamente, mesmo com
  `commit=false`. Decommit zera e protege; não devolve o backing ao sistema.
  Essa diferença exige medição e revisão antes de executar cargas reais.

## Diagnóstico e limites

O menu usa o Canvas/VideoOut do boilerplate. Os testes Vulkan criam recursos três
vezes, executam compute com 256 resultados conhecidos e fazem clear/cópia/readback
de uma imagem vermelha 8×8. Não há pipeline gráfico com triângulos, swapchain
Vulkan ou integração do renderizador Xenos. `presentation` permanece não testado.

O teste de dez minutos exercita memória e pthreads. Registra heap do título,
objetos/aliases do SDK e `ru_maxrss` a cada 30 segundos. São métricas parciais;
não representam toda a memória de GPU nem dez minutos de emulação.

Cada log tem nome exclusivo. O último BEGIN é descarregado antes de começar o
teste. Uma interrupção não é aprovação. A sequência para após falha; timeout
de GPU bloqueia novas execuções de GPU até reiniciar o aplicativo.

## Reproduzir e revisar

`dependencies.lock.json` fixa fontes; submódulos usam as revisões do Xenia.
`tools/prepare.py` rejeita revisão incorreta ou alterações relevantes nas fontes
consumidas. O SDK e o boilerplate são exportados pelas receitas de suas revisões.
Use `make deps`, `make test`, `make xenia-host`, `make ps5`, `make package`.
As versões dos pacotes Ubuntu ainda não estão congeladas numa imagem de container;
reprodução entre toolchains diferentes não está garantida.

O arquivo de fontes contém a árvore própria e arquivos upstream com suas licenças.
Para reproduzir com os scripts Git, use a árvore própria e `make deps` para
recriar os checkouts nos pins. O arquivo também permite inspecionar as fontes
sem buscar os repositórios. Nenhuma publicação automática faz parte da receita.
