# Teste externo — PS5 13.60 + Relapse

## Antes de instalar

Ambiente informado: firmware 13.60, Relapse, **etaHEN, ShadowMountPlus e kstuff lite**.
Ainda registrar modelo (Fat/Slim/Pro) e versões exatas desses componentes.
Confirmar se o loader executa pastas de títulos nativos e se permite acesso
a `/data` e memória executável. O protótipo não instala helpers de desbloqueio.

O ID local é `PPSA99361`. Confirmar que não existe outro aplicativo com esse ID.
Se existir, não substituir: pedir novo pacote com identidade diferente.
O projeto não requer atualizar/reinstalar firmware.

## Pacote

Conferir SHA-256 do ZIP e extrair. Instalação manual da pasta `PPSA99361`
em `/data/homebrew/`, conforme o loader que o testador usa. Os logs ficam em
`/data/x360ps5/logs`. Fechar o aplicativo antes de substituir arquivos.
O pacote não contém jogos, BIOS, chaves ou arquivos obtidos do console.

## Sequência

1. Abrir o aplicativo. Confirmar texto legível, cores e resposta a cima/baixo.
2. Selecionar cada teste com Cruz. Anotar qualquer falha antes de avançar.
3. Rodar `ALL TESTS X3` apenas depois dos testes individuais passarem.
4. Rodar `STRESS 10 MIN`. Círculo cancela; cancelamento não é aprovação.
5. Usar `EXIT` e confirmar retorno à interface do PS5. Se o sistema recusar,
   fechar pelo menu do PS5 e informar o retorno registrado.
6. Copiar todos os arquivos de `/data/x360ps5/logs`; enviar também foto da tela
   final e os dados do ambiente. Não enviar dumps de memória ou arquivos de jogos.

Um travamento: anotar teste selecionado e último `BEGIN`, fechar pelo sistema,
recolher o log; não insistir na sequência. Se a GPU reportar timeout, encerrar
o aplicativo antes de qualquer novo teste.

## O que não concluir

- X86 JIT aprovado não significa PowerPC/Xenia aprovado.
- Imagem no menu não prova apresentação Vulkan: a interface usa VideoOut.
- Compute/readback aprovado não significa GPU de Xbox 360 funcionando.
- Stress de memória/threads não equivale a dez minutos de emulação ou GPU.
- Resultados WSL/llvmpipe não são resultados PS5/RADV.
