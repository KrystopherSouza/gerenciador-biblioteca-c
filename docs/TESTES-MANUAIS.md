# Roteiro de testes manuais

Use somente dados fictícios. Execute os testes na pasta principal do projeto para que os arquivos de persistência sejam criados no local esperado.

## Preparação

1. Compile o programa seguindo as instruções do `README.md`.
2. Execute o programa.
3. Para começar com uma base vazia, feche o programa e remova apenas os arquivos de dados de teste (`livros.txt`, `usuarios.txt`, `emprestimos.txt` e os respectivos `.bak`). Não faça isso em uma pasta que contenha dados que deseja preservar.

## Cenários

| ID | Cenário | Resultado esperado |
|---|---|---|
| CT-01 | Cadastrar um livro com dados válidos | Livro cadastrado e mensagem de confirmação |
| CT-02 | Cadastrar novamente um código de livro existente | Cadastro rejeitado por código duplicado |
| CT-03 | Listar livros cadastrados | Dados do livro e quantidade disponível exibidos |
| CT-04 | Pesquisar livro por código, título e autor | Livro correspondente encontrado |
| CT-05 | Cadastrar usuário com matrícula válida | Usuário cadastrado com data atual |
| CT-06 | Cadastrar novamente uma matrícula existente | Cadastro rejeitado por matrícula duplicada |
| CT-07 | Realizar empréstimo com usuário e livro existentes | Empréstimo criado com devolução prevista para 7 dias depois; quantidade disponível reduzida |
| CT-08 | Tentar emprestar um livro sem exemplares disponíveis | Operação rejeitada |
| CT-09 | Registrar devolução de empréstimo ativo | Empréstimo marcado como devolvido e exemplar liberado |
| CT-10 | Tentar devolver novamente um empréstimo já devolvido | Operação rejeitada com mensagem informativa |
| CT-11 | Renovar um empréstimo ativo | Data prevista prorrogada em mais 7 dias |
| CT-12 | Fechar e abrir novamente o programa | Registros anteriormente salvos carregados |
| CT-13 | Executar relatórios | Relatórios exibem os dados correspondentes aos registros cadastrados |
| CT-14 | Informar texto vazio, muito longo ou contendo `|` | Entrada inválida solicitada novamente |

## Registro dos resultados

Para cada cenário, anote a data, o resultado observado e se passou ou falhou. Se encontrar um problema, registre também os passos para reproduzi-lo e a mensagem exibida pelo programa.

> Este documento é um roteiro para execução manual. Os cenários não são executados automaticamente pelo repositório.
