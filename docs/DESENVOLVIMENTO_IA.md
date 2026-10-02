# Desenvolvimento com IA: declaração de transparência

**O X360PS5 está sendo desenvolvido com assistência ativa de inteligência artificial.**
Na implementação inicial, o Codex participou de pesquisa técnica, leitura de fontes,
escrita de código C/C++, adaptações de plataforma, scripts, execução de ferramentas
de build/teste e documentação. Uma parte substancial das alterações próprias foi
produzida nessa colaboração entre o responsável humano e a IA.

## Responsabilidade e autoria

- O responsável humano define o objetivo, fornece o contexto do console e autoriza ações como a publicação.
- A IA auxilia na implementação e análise. Suas conclusões podem conter erros e exigem revisão.
- O código de Xenia, Mesa, SDK e demais dependências pertence aos respectivos autores; o uso de IA não altera créditos ou licenças.
- Não há alegação de auditoria humana independente ou de validação no PS5.
- Nenhuma participação de outro agente é atribuída sem registro; esta entrega identifica o trabalho feito com Codex.

## O que conta como evidência

Texto produzido por IA não é prova de funcionamento. Este projeto distingue:

| Evidência | O que demonstra | O que não demonstra |
| --- | --- | --- |
| Compilação e vínculo | O conjunto de fontes gera um executável com a toolchain usada | Inicialização no console |
| Inspeção do contêiner | Integridade estrutural verificada pela ferramenta | Compatibilidade com o loader do testador |
| Testes host/WSL | Resultado dos casos executados naquele ambiente | Resultado no PS5 |
| Hashes iguais | Dois builds consecutivos coincidiram no mesmo ambiente | Reprodução universal ou correção do emulador |
| Teste de hardware | Somente o comportamento efetivamente observado e registrado | Compatibilidade automática com outros jogos/modelos |

Logs, versões e limitações ficam em [VALIDACAO.md](VALIDACAO.md) e [evidence/](evidence/).
Falhas são registradas como falhas; testes não executados permanecem não testados.
Os resultados no console continuam pendentes.

## Como colaborar

Declare se usou IA numa contribuição e descreva como revisou e verificou a mudança.
Preserve atribuições upstream. Não invente resultados, screenshots, benchmarks ou
compatibilidade. Inclua comandos e logs reproduzíveis quando houver testes.

Não envie credenciais, chaves de console ou material privado para issues ou prompts.
O objetivo é manter o processo verificável, inclusive quando uma hipótese da IA falha.
