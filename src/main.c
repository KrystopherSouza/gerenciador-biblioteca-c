/*
 * ============================================================================
 * SISTEMA DE GERENCIAMENTO DE BIBLIOTECA EM LINGUAGEM C
 * ============================================================================
 * Atividade MAPA - Engenharia de Software
 *
 * Funcionalidades:
 *   - Cadastro, listagem e pesquisa de livros e usuarios
 *   - Emprestimos (prazo de 7 dias), devolucoes e renovacoes
 *   - Persistencia em arquivos de texto (livros.txt, usuarios.txt,
 *     emprestimos.txt) com backup automatico (.bak) a cada gravacao
 *   - Relatorios: livros mais emprestados e usuarios com atraso
 *   - Busca avancada com multiplos criterios
 *
 * Organizacao do arquivo:
 *   1. Constantes e estruturas de dados
 *   2. Inicializacao
 *   3. Entrada de dados com validacao
 *   4. Funcoes de data
 *   5. Funcoes de busca e impressao
 *   6. Backup e gravacao em arquivos
 *   7. Carregamento dos arquivos
 *   8. Funcoes principais (cadastros, emprestimos, pesquisas, listagens)
 *   9. Funcoes avancadas (busca avancada, renovacao, relatorios)
 *  10. Menus e funcao main
 *
 * Formato dos arquivos: um registro por linha, campos separados por "|".
 * Por isso o caractere "|" e proibido nos textos digitados.
 * ============================================================================
 */

#include <ctype.h>   /* isspace, tolower */
#include <errno.h>   /* errno, ENOENT, ERANGE */
#include <limits.h>  /* INT_MAX */
#include <stdio.h>   /* printf, fopen, fgets, fprintf... */
#include <stdlib.h>  /* strtol */
#include <string.h>  /* strlen, strcmp, memcpy, memset... */
#include <time.h>    /* time, localtime (data atual do sistema) */

/* ========================================================================== */
/* 1. CONSTANTES E ESTRUTURAS DE DADOS                                        */
/* ========================================================================== */

/* Capacidade maxima dos vetores de registros. */
#define MAX_LIVROS 500
#define MAX_USUARIOS 500
#define MAX_EMPRESTIMOS 2000

/* Tamanhos maximos dos textos (sem contar o '\0', que e somado nas structs). */
#define TAM_TITULO 100
#define TAM_AUTOR 80
#define TAM_EDITORA 60
#define TAM_NOME 100
#define TAM_CURSO 50
#define TAM_TELEFONE 15

/* Tamanho do buffer usado para ler uma linha inteira do teclado ou arquivo. */
#define TAM_LINHA 512

/* Representa uma data do calendario. */
typedef struct {
    int dia;
    int mes;
    int ano;
} Data;

/* Representa um livro do acervo. */
typedef struct {
    int codigo;                      /* identificador unico do livro */
    char titulo[TAM_TITULO + 1];     /* +1 para o '\0' final */
    char autor[TAM_AUTOR + 1];
    char editora[TAM_EDITORA + 1];
    int anoPublicacao;
    int exemplaresTotal;             /* quantidade total de exemplares */
    int exemplaresDisponiveis;       /* exemplares que nao estao emprestados */
} Livro;

/* Representa um usuario cadastrado na biblioteca. */
typedef struct {
    int matricula;                   /* identificador unico do usuario */
    char nome[TAM_NOME + 1];
    char curso[TAM_CURSO + 1];
    char telefone[TAM_TELEFONE + 1];
    Data dataCadastro;               /* preenchida automaticamente */
} Usuario;

/* Representa um emprestimo de livro feito por um usuario. */
typedef struct {
    int codigo;                      /* identificador unico do emprestimo */
    int matriculaUsuario;            /* quem pegou o livro */
    int codigoLivro;                 /* qual livro foi emprestado */
    Data dataEmprestimo;
    Data dataPrevistaDevolucao;      /* 7 dias apos o emprestimo */
    int ativo;                       /* 1 = ativo, 0 = devolvido */
} Emprestimo;

/*
 * Agrupa todos os vetores e contadores do sistema em uma unica estrutura.
 * Assim, basta passar um ponteiro para Biblioteca para as funcoes terem
 * acesso a todos os dados.
 */
typedef struct {
    Livro livros[MAX_LIVROS];
    Usuario usuarios[MAX_USUARIOS];
    Emprestimo emprestimos[MAX_EMPRESTIMOS];
    int quantidadeLivros;
    int quantidadeUsuarios;
    int quantidadeEmprestimos;
} Biblioteca;

/* ========================================================================== */
/* 2. INICIALIZACAO                                                           */
/* ========================================================================== */

/* Inicializa vetores e contadores antes de carregar os arquivos. */
static void inicializarBiblioteca(Biblioteca *biblioteca)
{
    /* Zera tudo: contadores ficam em 0 e os vetores ficam limpos. */
    memset(biblioteca, 0, sizeof(*biblioteca));
}

/* ========================================================================== */
/* 3. ENTRADA DE DADOS COM VALIDACAO                                          */
/* ========================================================================== */

/*
 * Mostra a mensagem e le uma linha do teclado usando fgets, o que evita
 * estouro dos campos e descarta linhas excedentes.
 * Retorno: 1 = leitura ok, 0 = fim da entrada (EOF), -1 = linha longa demais.
 */
static int lerLinha(const char *mensagem, char *destino, size_t capacidade)
{
    size_t tamanho;
    int caractere;
    printf("%s", mensagem);
    if (fgets(destino, (int)capacidade, stdin) == NULL) {
        return 0;
    }
    tamanho = strlen(destino);
    if (tamanho > 0 && destino[tamanho - 1] == '\n') {
        /* Linha completa: remove o '\n' do final. */
        destino[--tamanho] = '\0';
    } else {
        /* Linha maior que o buffer: consome o que sobrou no teclado. */
        while ((caractere = getchar()) != '\n' && caractere != EOF) {
            /* Descarta o restante de uma linha maior que o buffer. */
        }
        if (tamanho == capacidade - 1) {
            return -1;
        }
    }
    /* Remove o '\r' final (quebra de linha no padrao Windows). */
    if (tamanho > 0 && destino[tamanho - 1] == '\r') {
        destino[--tamanho] = '\0';
    }
    return 1;
}

/*
 * Le um numero inteiro entre minimo e maximo. Repete a pergunta ate o
 * usuario digitar um valor valido.
 * Retorno: 1 = valor lido em *valor, 0 = fim da entrada (EOF).
 */
static int lerInteiro(const char *mensagem, int minimo, int maximo, int *valor)
{
    char linha[TAM_LINHA];
    char *fim;
    long numero;
    int resultado;
    for (;;) {
        resultado = lerLinha(mensagem, linha, sizeof(linha));
        if (resultado == 0) {
            return 0;
        }
        if (resultado < 0) {
            printf("Entrada muito longa. Tente novamente.\n");
            continue;
        }
        errno = 0;
        numero = strtol(linha, &fim, 10);
        /* Ignora espacos depois do numero. */
        while (isspace((unsigned char)*fim)) {
            fim++;
        }
        /* Valido se: leu digitos, nao sobrou lixo, nao estourou e esta na faixa. */
        if (fim != linha && *fim == '\0' && errno != ERANGE &&
            numero >= minimo && numero <= maximo) {
            *valor = (int)numero;
            return 1;
        }
        printf("Digite um numero inteiro entre %d e %d.\n", minimo, maximo);
    }
}

/*
 * Le um texto de 1 ate tamanhoMaximo caracteres, sem espacos nas pontas e
 * sem o caractere '|' (usado como separador nos arquivos).
 * Retorno: 1 = texto lido em destino, 0 = fim da entrada (EOF).
 */
static int lerTexto(const char *mensagem, char *destino, size_t tamanhoMaximo)
{
    char linha[TAM_LINHA];
    size_t inicio;
    size_t fim;
    size_t tamanho;
    int resultado;
    for (;;) {
        resultado = lerLinha(mensagem, linha, sizeof(linha));
        if (resultado == 0) {
            return 0;
        }
        if (resultado < 0) {
            printf("Texto muito longo. Tente novamente.\n");
            continue;
        }
        /* Remove espacos do inicio e do fim do texto. */
        inicio = 0;
        while (isspace((unsigned char)linha[inicio])) {
            inicio++;
        }
        fim = strlen(linha);
        while (fim > inicio && isspace((unsigned char)linha[fim - 1])) {
            fim--;
        }
        tamanho = fim - inicio;
        /* Rejeita texto vazio, grande demais ou com o separador '|'. */
        if (tamanho == 0 || tamanho > tamanhoMaximo ||
            memchr(linha + inicio, '|', tamanho) != NULL) {
            printf("Informe um texto de 1 a %u caracteres, sem o caractere |.\n",
                   (unsigned)tamanhoMaximo);
            continue;
        }
        memcpy(destino, linha + inicio, tamanho);
        destino[tamanho] = '\0';
        return 1;
    }
}

/* ========================================================================== */
/* 4. FUNCOES DE DATA                                                         */
/* ========================================================================== */

/* Retorna 1 se o ano e bissexto, 0 caso contrario. */
static int anoBissexto(int ano)
{
    return ano % 400 == 0 || (ano % 4 == 0 && ano % 100 != 0);
}

/* Retorna quantos dias o mes tem, considerando fevereiro em ano bissexto. */
static int diasNoMes(int mes, int ano)
{
    static const int dias[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return dias[mes - 1] + (mes == 2 && anoBissexto(ano));
}

/* Retorna 1 se a data existe no calendario, 0 caso contrario. */
static int dataValida(Data data)
{
    return data.ano >= 1 && data.ano <= 9999 && data.mes >= 1 &&
           data.mes <= 12 && data.dia >= 1 &&
           data.dia <= diasNoMes(data.mes, data.ano);
}

/* Preenche *data com a data atual do sistema. Retorna 0 em caso de erro. */
static int obterDataAtual(Data *data)
{
    time_t agora;
    struct tm *dataLocal;
    agora = time(NULL);
    dataLocal = localtime(&agora);
    if (dataLocal == NULL) {
        fprintf(stderr, "Erro: nao foi possivel obter a data atual do sistema.\n");
        return 0;
    }
    data->dia = dataLocal->tm_mday;
    data->mes = dataLocal->tm_mon + 1;       /* tm_mon vai de 0 a 11 */
    data->ano = dataLocal->tm_year + 1900;   /* tm_year conta desde 1900 */
    return 1;
}

/* Soma "quantidade" dias a uma data, avancando mes e ano quando necessario. */
static Data adicionarDias(Data data, int quantidade)
{
    while (quantidade-- > 0) {
        if (++data.dia > diasNoMes(data.mes, data.ano)) {
            data.dia = 1;
            if (++data.mes > 12) {
                data.mes = 1;
                data.ano++;
            }
        }
    }
    return data;
}

/* Compara duas datas. Retorna -1 se a < b, 1 se a > b e 0 se forem iguais. */
static int compararDatas(Data a, Data b)
{
    if (a.ano != b.ano) return a.ano < b.ano ? -1 : 1;
    if (a.mes != b.mes) return a.mes < b.mes ? -1 : 1;
    if (a.dia != b.dia) return a.dia < b.dia ? -1 : 1;
    return 0;
}

/* Imprime a data no formato dd/mm/aaaa. */
static void imprimirData(Data data)
{
    printf("%02d/%02d/%04d", data.dia, data.mes, data.ano);
}

/* ========================================================================== */
/* 5. FUNCOES DE BUSCA E IMPRESSAO                                            */
/* ========================================================================== */

/* Procura um livro pelo codigo. Retorna o ponteiro ou NULL se nao achar. */
static Livro *buscarLivroPorCodigo(Biblioteca *biblioteca, int codigo)
{
    int i;
    for (i = 0; i < biblioteca->quantidadeLivros; i++) {
        if (biblioteca->livros[i].codigo == codigo) {
            return &biblioteca->livros[i];
        }
    }
    return NULL;
}

/* Procura um usuario pela matricula. Retorna o ponteiro ou NULL. */
static Usuario *buscarUsuarioPorMatricula(Biblioteca *biblioteca, int matricula)
{
    int i;
    for (i = 0; i < biblioteca->quantidadeUsuarios; i++) {
        if (biblioteca->usuarios[i].matricula == matricula) {
            return &biblioteca->usuarios[i];
        }
    }
    return NULL;
}

/* Procura um emprestimo pelo codigo. Retorna o ponteiro ou NULL. */
static Emprestimo *buscarEmprestimoPorCodigo(Biblioteca *biblioteca, int codigo)
{
    int i;
    for (i = 0; i < biblioteca->quantidadeEmprestimos; i++) {
        if (biblioteca->emprestimos[i].codigo == codigo) {
            return &biblioteca->emprestimos[i];
        }
    }
    return NULL;
}

/*
 * Verifica se "texto" contem "termo", sem diferenciar maiusculas de
 * minusculas. Usada nas pesquisas por parte do titulo, autor ou nome.
 */
static int textoContem(const char *texto, const char *termo)
{
    size_t i;
    size_t j;
    size_t tamanhoTexto;
    size_t tamanhoTermo;
    tamanhoTexto = strlen(texto);
    tamanhoTermo = strlen(termo);
    if (tamanhoTermo == 0) {
        return 1;
    }
    if (tamanhoTermo > tamanhoTexto) {
        return 0;
    }
    /* Tenta casar o termo em cada posicao possivel do texto. */
    for (i = 0; i <= tamanhoTexto - tamanhoTermo; i++) {
        for (j = 0; j < tamanhoTermo; j++) {
            if (tolower((unsigned char)texto[i + j]) !=
                tolower((unsigned char)termo[j])) {
                break;
            }
        }
        if (j == tamanhoTermo) {
            return 1;
        }
    }
    return 0;
}

/* Imprime os dados de um livro, incluindo o status (disponivel/emprestado). */
static void imprimirLivro(const Livro *livro)
{
    printf("Codigo: %d | Titulo: %s | Autor: %s | Editora: %s\n",
           livro->codigo, livro->titulo, livro->autor, livro->editora);
    printf("Ano: %d | Disponiveis: %d de %d | Status: %s\n",
           livro->anoPublicacao, livro->exemplaresDisponiveis,
           livro->exemplaresTotal,
           livro->exemplaresDisponiveis > 0 ? "disponivel" : "emprestado");
}

/* Imprime os dados de um usuario em uma linha. */
static void imprimirUsuario(const Usuario *usuario)
{
    printf("Matricula: %d | Nome: %s | Curso: %s | Telefone: %s | Cadastro: ",
           usuario->matricula, usuario->nome, usuario->curso, usuario->telefone);
    imprimirData(usuario->dataCadastro);
    printf("\n");
}

/*
 * Imprime um emprestimo mostrando tambem o nome do usuario e o titulo do
 * livro (buscados pelos codigos guardados no emprestimo).
 */
static void imprimirEmprestimo(const Biblioteca *biblioteca,
                               const Emprestimo *emprestimo)
{
    const Livro *livro;
    const Usuario *usuario;
    livro = NULL;
    usuario = NULL;
    /* Procura o livro do emprestimo. */
    if (emprestimo->codigoLivro > 0) {
        int i;
        for (i = 0; i < biblioteca->quantidadeLivros; i++) {
            if (biblioteca->livros[i].codigo == emprestimo->codigoLivro) {
                livro = &biblioteca->livros[i];
                break;
            }
        }
    }
    /* Procura o usuario do emprestimo. */
    if (emprestimo->matriculaUsuario > 0) {
        int i;
        for (i = 0; i < biblioteca->quantidadeUsuarios; i++) {
            if (biblioteca->usuarios[i].matricula == emprestimo->matriculaUsuario) {
                usuario = &biblioteca->usuarios[i];
                break;
            }
        }
    }
    printf("Emprestimo %d | Usuario: %d (%s) | Livro: %d (%s)\n",
           emprestimo->codigo, emprestimo->matriculaUsuario,
           usuario != NULL ? usuario->nome : "nao encontrado",
           emprestimo->codigoLivro,
           livro != NULL ? livro->titulo : "nao encontrado");
    printf("Retirada: ");
    imprimirData(emprestimo->dataEmprestimo);
    printf(" | Prevista: ");
    imprimirData(emprestimo->dataPrevistaDevolucao);
    printf(" | Status: %s\n", emprestimo->ativo ? "ativo" : "devolvido");
}

/* ========================================================================== */
/* 6. BACKUP E GRAVACAO EM ARQUIVOS                                           */
/* ========================================================================== */

/*
 * Copia um arquivo byte a byte, sem depender de comandos ou bibliotecas
 * fora do C padrao. Se a origem nao existir, considera sucesso (nao ha o
 * que copiar ainda).
 */
static int copiarArquivo(const char *origem, const char *destino)
{
    FILE *entrada = fopen(origem, "rb");
    FILE *saida;
    char buffer[4096];
    size_t bytes;
    int sucesso = 1;
    if (entrada == NULL) return errno == ENOENT;
    saida = fopen(destino, "wb");
    if (saida == NULL) {
        fclose(entrada);
        return 0;
    }
    /* Le em blocos de 4 KB e grava no destino. */
    while ((bytes = fread(buffer, 1, sizeof(buffer), entrada)) > 0) {
        if (fwrite(buffer, 1, bytes, saida) != bytes) {
            sucesso = 0;
            break;
        }
    }
    if (ferror(entrada)) sucesso = 0;
    if (fclose(entrada) != 0) sucesso = 0;
    if (fclose(saida) != 0) sucesso = 0;
    return sucesso;
}

/* Cria o backup: apaga o backup antigo e copia o arquivo atual para ele. */
static int criarBackup(const char *origem, const char *backup)
{
    if (remove(backup) != 0 && errno != ENOENT) return 0;
    return copiarArquivo(origem, backup);
}

/*
 * Restaura o arquivo de destino a partir do backup. Se nao existir backup,
 * significa que o arquivo original tambem nao existia, entao apaga o destino.
 */
static int restaurarBackup(const char *backup, const char *destino)
{
    FILE *arquivo = fopen(backup, "rb");
    if (arquivo == NULL) {
        if (errno != ENOENT) return 0;
        return remove(destino) == 0 || errno == ENOENT;
    }
    fclose(arquivo);
    if (remove(destino) != 0 && errno != ENOENT) return 0;
    return copiarArquivo(backup, destino);
}

/*
 * Grava os dados em um arquivo temporario e so depois troca pelo arquivo
 * definitivo. Assim, se algo falhar no meio da gravacao, o arquivo original
 * nao fica corrompido. O parametro "gravar" e um ponteiro para a funcao
 * que sabe escrever o tipo de dado (livros, usuarios ou emprestimos).
 */
static int salvarArquivoTemporario(const char *temporario, const char *destino,
                                   const char *backup,
                                   int (*gravar)(FILE *, const Biblioteca *),
                                   const Biblioteca *biblioteca)
{
    FILE *arquivo = fopen(temporario, "w");
    if (arquivo == NULL) {
        fprintf(stderr, "Erro ao criar %s.\n", temporario);
        return 0;
    }
    if (!gravar(arquivo, biblioteca) || ferror(arquivo)) {
        fprintf(stderr, "Erro ao gravar %s.\n", temporario);
        fclose(arquivo);
        remove(temporario);
        return 0;
    }
    if (fclose(arquivo) != 0) {
        fprintf(stderr, "Erro ao finalizar %s.\n", temporario);
        remove(temporario);
        return 0;
    }
    /* Remove o arquivo antigo antes de renomear (necessario no Windows). */
    if (remove(destino) != 0 && errno != ENOENT) {
        fprintf(stderr, "Erro ao substituir %s.\n", destino);
        remove(temporario);
        return 0;
    }
    if (rename(temporario, destino) != 0) {
        fprintf(stderr, "Erro ao renomear %s para %s.\n", temporario, destino);
        restaurarBackup(backup, destino);
        return 0;
    }
    return 1;
}

/* Grava todos os livros, um por linha, com campos separados por '|'. */
static int gravarLivros(FILE *arquivo, const Biblioteca *biblioteca)
{
    int i;
    for (i = 0; i < biblioteca->quantidadeLivros; i++) {
        const Livro *livro = &biblioteca->livros[i];
        if (fprintf(arquivo, "%d|%s|%s|%s|%d|%d|%d\n",
                    livro->codigo, livro->titulo, livro->autor, livro->editora,
                    livro->anoPublicacao, livro->exemplaresTotal,
                    livro->exemplaresDisponiveis) < 0) {
            return 0;
        }
    }
    return 1;
}

/* Grava todos os usuarios, um por linha, com campos separados por '|'. */
static int gravarUsuarios(FILE *arquivo, const Biblioteca *biblioteca)
{
    int i;
    for (i = 0; i < biblioteca->quantidadeUsuarios; i++) {
        const Usuario *usuario = &biblioteca->usuarios[i];
        if (fprintf(arquivo, "%d|%s|%s|%s|%d|%d|%d\n",
                    usuario->matricula, usuario->nome, usuario->curso,
                    usuario->telefone, usuario->dataCadastro.dia,
                    usuario->dataCadastro.mes, usuario->dataCadastro.ano) < 0) {
            return 0;
        }
    }
    return 1;
}

/* Grava todos os emprestimos, um por linha, com campos separados por '|'. */
static int gravarEmprestimos(FILE *arquivo, const Biblioteca *biblioteca)
{
    int i;
    for (i = 0; i < biblioteca->quantidadeEmprestimos; i++) {
        const Emprestimo *emprestimo = &biblioteca->emprestimos[i];
        if (fprintf(arquivo, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
                    emprestimo->codigo, emprestimo->matriculaUsuario,
                    emprestimo->codigoLivro, emprestimo->dataEmprestimo.dia,
                    emprestimo->dataEmprestimo.mes, emprestimo->dataEmprestimo.ano,
                    emprestimo->dataPrevistaDevolucao.dia,
                    emprestimo->dataPrevistaDevolucao.mes,
                    emprestimo->dataPrevistaDevolucao.ano, emprestimo->ativo) < 0) {
            return 0;
        }
    }
    return 1;
}

/*
 * Salva os tres arquivos de dados. Antes de gravar, cria o backup (.bak)
 * de cada um (backup automatico). Se qualquer gravacao falhar, restaura os
 * tres arquivos a partir dos backups.
 */
static int salvarDados(const Biblioteca *biblioteca)
{
    if (!criarBackup("livros.txt", "livros.txt.bak") ||
        !criarBackup("usuarios.txt", "usuarios.txt.bak") ||
        !criarBackup("emprestimos.txt", "emprestimos.txt.bak")) {
        fprintf(stderr, "Erro ao criar backup. Dados nao alterados.\n");
        return 0;
    }
    if (!salvarArquivoTemporario("livros.txt.tmp", "livros.txt", "livros.txt.bak",
                                 gravarLivros, biblioteca) ||
        !salvarArquivoTemporario("usuarios.txt.tmp", "usuarios.txt", "usuarios.txt.bak",
                                 gravarUsuarios, biblioteca) ||
        !salvarArquivoTemporario("emprestimos.txt.tmp", "emprestimos.txt",
                                 "emprestimos.txt.bak", gravarEmprestimos, biblioteca)) {
        restaurarBackup("livros.txt.bak", "livros.txt");
        restaurarBackup("usuarios.txt.bak", "usuarios.txt");
        restaurarBackup("emprestimos.txt.bak", "emprestimos.txt");
        fprintf(stderr, "Falha ao salvar. Os arquivos anteriores foram restaurados quando possivel.\n");
        return 0;
    }
    return 1;
}

/* ========================================================================== */
/* 7. CARREGAMENTO DOS ARQUIVOS                                               */
/* ========================================================================== */

/*
 * Divide uma linha em "quantidade" campos usando '|' como separador.
 * Troca cada '|' por '\0' e guarda o inicio de cada campo em campos[].
 * Retorna 0 se a linha tiver campos a mais ou a menos.
 * Os registros usam |; esse caractere e proibido nos campos cadastrados.
 */
static int separarCampos(char *linha, char **campos, int quantidade)
{
    int i;
    char *inicio;
    char *separador;
    inicio = linha;
    for (i = 0; i < quantidade; i++) {
        campos[i] = inicio;
        separador = strchr(inicio, '|');
        if (i == quantidade - 1) {
            /* Ultimo campo: nao pode haver mais nenhum '|' depois dele. */
            if (separador != NULL) {
                return 0;
            }
        } else {
            /* Campos do meio: precisa existir um '|' para separar. */
            if (separador == NULL) {
                return 0;
            }
            *separador = '\0';
            inicio = separador + 1;
        }
    }
    return 1;
}

/* Converte um campo de texto em inteiro dentro de [minimo, maximo]. */
static int converterCampoInt(const char *texto, int minimo, int maximo, int *valor)
{
    char *fim;
    long numero;
    errno = 0;
    numero = strtol(texto, &fim, 10);
    if (texto[0] == '\0' || *fim != '\0' || errno == ERANGE ||
        numero < minimo || numero > maximo) {
        return 0;
    }
    *valor = (int)numero;
    return 1;
}

/* Copia um campo de texto para o destino, validando o tamanho (1 a max). */
static int copiarCampo(const char *origem, char *destino, size_t tamanhoMaximo)
{
    size_t tamanho;
    tamanho = strlen(origem);
    if (tamanho == 0 || tamanho > tamanhoMaximo) {
        return 0;
    }
    memcpy(destino, origem, tamanho + 1);
    return 1;
}

/*
 * Carrega livros.txt para a memoria. Linhas invalidas sao ignoradas com um
 * aviso. Se o arquivo ainda nao existir, nao e erro (primeira execucao).
 * Retorna 1 se todos os registros eram validos.
 */
static int carregarLivros(Biblioteca *biblioteca)
{
    FILE *arquivo;
    char linha[TAM_LINHA];
    char *campos[7];
    Livro livro;
    int validos;
    int numeroLinha;
    arquivo = fopen("livros.txt", "r");
    if (arquivo == NULL) {
        if (errno != ENOENT) {
            fprintf(stderr, "Erro ao abrir livros.txt.\n");
            return 0;
        }
        return 1;
    }
    validos = 1;
    numeroLinha = 0;
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        numeroLinha++;
        /* Linha maior que o buffer: descarta o restante e ignora o registro. */
        if (strchr(linha, '\n') == NULL && !feof(arquivo)) {
            int caractere;
            while ((caractere = fgetc(arquivo)) != '\n' && caractere != EOF) {
                /* Descarta registro maior que o buffer. */
            }
            fprintf(stderr, "Aviso: registro longo ignorado em livros.txt:%d.\n",
                    numeroLinha);
            validos = 0;
            continue;
        }
        linha[strcspn(linha, "\r\n")] = '\0';
        /* Valida cada campo; qualquer falha faz o registro ser ignorado. */
        if (biblioteca->quantidadeLivros >= MAX_LIVROS ||
            !separarCampos(linha, campos, 7) ||
            !converterCampoInt(campos[0], 1, INT_MAX, &livro.codigo) ||
            !copiarCampo(campos[1], livro.titulo, TAM_TITULO) ||
            !copiarCampo(campos[2], livro.autor, TAM_AUTOR) ||
            !copiarCampo(campos[3], livro.editora, TAM_EDITORA) ||
            !converterCampoInt(campos[4], 1, 9999, &livro.anoPublicacao) ||
            !converterCampoInt(campos[5], 1, INT_MAX, &livro.exemplaresTotal) ||
            !converterCampoInt(campos[6], 0, livro.exemplaresTotal,
                               &livro.exemplaresDisponiveis) ||
            buscarLivroPorCodigo(biblioteca, livro.codigo) != NULL) {
            fprintf(stderr, "Aviso: registro invalido ignorado em livros.txt:%d.\n",
                    numeroLinha);
            validos = 0;
            continue;
        }
        biblioteca->livros[biblioteca->quantidadeLivros++] = livro;
    }
    if (ferror(arquivo)) {
        fprintf(stderr, "Erro de leitura em livros.txt.\n");
        validos = 0;
    }
    fclose(arquivo);
    return validos;
}

/* Carrega usuarios.txt para a memoria (mesma logica de carregarLivros). */
static int carregarUsuarios(Biblioteca *biblioteca)
{
    FILE *arquivo;
    char linha[TAM_LINHA];
    char *campos[7];
    Usuario usuario;
    int validos;
    int numeroLinha;
    arquivo = fopen("usuarios.txt", "r");
    if (arquivo == NULL) {
        if (errno != ENOENT) {
            fprintf(stderr, "Erro ao abrir usuarios.txt.\n");
            return 0;
        }
        return 1;
    }
    validos = 1;
    numeroLinha = 0;
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        numeroLinha++;
        if (strchr(linha, '\n') == NULL && !feof(arquivo)) {
            int caractere;
            while ((caractere = fgetc(arquivo)) != '\n' && caractere != EOF) {
                /* Descarta registro maior que o buffer. */
            }
            fprintf(stderr, "Aviso: registro longo ignorado em usuarios.txt:%d.\n",
                    numeroLinha);
            validos = 0;
            continue;
        }
        linha[strcspn(linha, "\r\n")] = '\0';
        if (biblioteca->quantidadeUsuarios >= MAX_USUARIOS ||
            !separarCampos(linha, campos, 7) ||
            !converterCampoInt(campos[0], 1, INT_MAX, &usuario.matricula) ||
            !copiarCampo(campos[1], usuario.nome, TAM_NOME) ||
            !copiarCampo(campos[2], usuario.curso, TAM_CURSO) ||
            !copiarCampo(campos[3], usuario.telefone, TAM_TELEFONE) ||
            !converterCampoInt(campos[4], 1, 31, &usuario.dataCadastro.dia) ||
            !converterCampoInt(campos[5], 1, 12, &usuario.dataCadastro.mes) ||
            !converterCampoInt(campos[6], 1, 9999, &usuario.dataCadastro.ano) ||
            !dataValida(usuario.dataCadastro) ||
            buscarUsuarioPorMatricula(biblioteca, usuario.matricula) != NULL) {
            fprintf(stderr, "Aviso: registro invalido ignorado em usuarios.txt:%d.\n",
                    numeroLinha);
            validos = 0;
            continue;
        }
        biblioteca->usuarios[biblioteca->quantidadeUsuarios++] = usuario;
    }
    if (ferror(arquivo)) {
        fprintf(stderr, "Erro de leitura em usuarios.txt.\n");
        validos = 0;
    }
    fclose(arquivo);
    return validos;
}

/* Carrega emprestimos.txt para a memoria (mesma logica de carregarLivros). */
static int carregarEmprestimos(Biblioteca *biblioteca)
{
    FILE *arquivo;
    char linha[TAM_LINHA];
    char *campos[10];
    Emprestimo emprestimo;
    int validos;
    int numeroLinha;
    arquivo = fopen("emprestimos.txt", "r");
    if (arquivo == NULL) {
        if (errno != ENOENT) {
            fprintf(stderr, "Erro ao abrir emprestimos.txt.\n");
            return 0;
        }
        return 1;
    }
    validos = 1;
    numeroLinha = 0;
    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        numeroLinha++;
        if (strchr(linha, '\n') == NULL && !feof(arquivo)) {
            int caractere;
            while ((caractere = fgetc(arquivo)) != '\n' && caractere != EOF) {
                /* Descarta registro maior que o buffer. */
            }
            fprintf(stderr, "Aviso: registro longo ignorado em emprestimos.txt:%d.\n",
                    numeroLinha);
            validos = 0;
            continue;
        }
        linha[strcspn(linha, "\r\n")] = '\0';
        /* Alem dos campos, confere se as datas existem e se a devolucao
           prevista nao e anterior a data do emprestimo. */
        if (biblioteca->quantidadeEmprestimos >= MAX_EMPRESTIMOS ||
            !separarCampos(linha, campos, 10) ||
            !converterCampoInt(campos[0], 1, INT_MAX, &emprestimo.codigo) ||
            !converterCampoInt(campos[1], 1, INT_MAX, &emprestimo.matriculaUsuario) ||
            !converterCampoInt(campos[2], 1, INT_MAX, &emprestimo.codigoLivro) ||
            !converterCampoInt(campos[3], 1, 31, &emprestimo.dataEmprestimo.dia) ||
            !converterCampoInt(campos[4], 1, 12, &emprestimo.dataEmprestimo.mes) ||
            !converterCampoInt(campos[5], 1, 9999, &emprestimo.dataEmprestimo.ano) ||
            !dataValida(emprestimo.dataEmprestimo) ||
            !converterCampoInt(campos[6], 1, 31,
                               &emprestimo.dataPrevistaDevolucao.dia) ||
            !converterCampoInt(campos[7], 1, 12,
                               &emprestimo.dataPrevistaDevolucao.mes) ||
            !converterCampoInt(campos[8], 1, 9999,
                               &emprestimo.dataPrevistaDevolucao.ano) ||
            !dataValida(emprestimo.dataPrevistaDevolucao) ||
            compararDatas(emprestimo.dataPrevistaDevolucao,
                          emprestimo.dataEmprestimo) < 0 ||
            !converterCampoInt(campos[9], 0, 1, &emprestimo.ativo) ||
            buscarEmprestimoPorCodigo(biblioteca, emprestimo.codigo) != NULL) {
            fprintf(stderr, "Aviso: registro invalido ignorado em emprestimos.txt:%d.\n",
                    numeroLinha);
            validos = 0;
            continue;
        }
        biblioteca->emprestimos[biblioteca->quantidadeEmprestimos++] = emprestimo;
    }
    if (ferror(arquivo)) {
        fprintf(stderr, "Erro de leitura em emprestimos.txt.\n");
        validos = 0;
    }
    fclose(arquivo);
    return validos;
}

/*
 * Confere a consistencia entre os tres arquivos depois de carregados:
 *   - recalcula os exemplares disponiveis a partir dos emprestimos ativos;
 *   - descarta emprestimos que apontam para livro ou usuario inexistente.
 * Assim, o numero de disponiveis nunca fica inconsistente com os emprestimos.
 */
static int reconciliarDados(Biblioteca *biblioteca)
{
    int i, quantidade = 0, sucesso = 1;
    /* Comeca assumindo que todos os exemplares estao disponiveis. */
    for (i = 0; i < biblioteca->quantidadeLivros; i++)
        biblioteca->livros[i].exemplaresDisponiveis =
            biblioteca->livros[i].exemplaresTotal;
    /* Cada emprestimo ativo valido retira um exemplar. */
    for (i = 0; i < biblioteca->quantidadeEmprestimos; i++) {
        Emprestimo emprestimo = biblioteca->emprestimos[i];
        Livro *livro = buscarLivroPorCodigo(biblioteca, emprestimo.codigoLivro);
        Usuario *usuario = buscarUsuarioPorMatricula(biblioteca, emprestimo.matriculaUsuario);
        if (livro == NULL || usuario == NULL ||
            (emprestimo.ativo && livro->exemplaresDisponiveis == 0)) {
            fprintf(stderr, "Aviso: emprestimo %d ignorado por inconsistencia nos dados.\n",
                    emprestimo.codigo);
            sucesso = 0;
            continue;
        }
        if (emprestimo.ativo) livro->exemplaresDisponiveis--;
        /* Compacta o vetor mantendo apenas os emprestimos validos. */
        biblioteca->emprestimos[quantidade++] = emprestimo;
    }
    biblioteca->quantidadeEmprestimos = quantidade;
    return sucesso;
}

/* Carrega os tres arquivos e depois reconcilia os dados. */
static int carregarDados(Biblioteca *biblioteca)
{
    int sucessoLivros = carregarLivros(biblioteca);
    int sucessoUsuarios = carregarUsuarios(biblioteca);
    int sucessoEmprestimos = carregarEmprestimos(biblioteca);
    return reconciliarDados(biblioteca) && sucessoLivros &&
           sucessoUsuarios && sucessoEmprestimos;
}

/* ========================================================================== */
/* 8. FUNCOES PRINCIPAIS (CADASTROS, EMPRESTIMOS, PESQUISAS, LISTAGENS)       */
/* ========================================================================== */

/*
 * Aplica uma alteracao de forma segura: primeiro salva a copia atualizada
 * nos arquivos e so entao substitui os dados em memoria. Se o salvamento
 * falhar, a memoria continua igual aos arquivos.
 */
static int aplicarAlteracao(Biblioteca *biblioteca, const Biblioteca *atualizada)
{
    if (!salvarDados(atualizada)) {
        printf("Alteracao cancelada: nao foi possivel salvar os dados.\n");
        return 0;
    }
    *biblioteca = *atualizada;
    return 1;
}

/* Cadastra um novo livro, validando os dados e impedindo codigo repetido. */
static void cadastrarLivro(Biblioteca *biblioteca)
{
    Livro livro;
    Biblioteca atualizada;
    if (biblioteca->quantidadeLivros >= MAX_LIVROS) {
        printf("Limite de livros atingido.\n");
        return;
    }
    if (!lerInteiro("Codigo do livro: ", 1, INT_MAX, &livro.codigo) ||
        !lerTexto("Titulo: ", livro.titulo, TAM_TITULO) ||
        !lerTexto("Autor: ", livro.autor, TAM_AUTOR) ||
        !lerTexto("Editora: ", livro.editora, TAM_EDITORA) ||
        !lerInteiro("Ano de publicacao: ", 1, 9999, &livro.anoPublicacao) ||
        !lerInteiro("Numero de exemplares: ", 1, INT_MAX,
                    &livro.exemplaresTotal)) {
        return;
    }
    if (buscarLivroPorCodigo(biblioteca, livro.codigo) != NULL) {
        printf("Ja existe um livro com esse codigo.\n");
        return;
    }
    /* Trabalha em uma copia e so confirma se o salvamento der certo. */
    atualizada = *biblioteca;
    livro.exemplaresDisponiveis = livro.exemplaresTotal;
    atualizada.livros[atualizada.quantidadeLivros++] = livro;
    if (aplicarAlteracao(biblioteca, &atualizada))
        printf("Livro cadastrado e dados salvos.\n");
}

/* Cadastra um novo usuario; a data de cadastro e a data atual do sistema. */
static void cadastrarUsuario(Biblioteca *biblioteca)
{
    Usuario usuario;
    Biblioteca atualizada;
    if (biblioteca->quantidadeUsuarios >= MAX_USUARIOS) {
        printf("Limite de usuarios atingido.\n");
        return;
    }
    if (!lerInteiro("Matricula: ", 1, INT_MAX, &usuario.matricula) ||
        !lerTexto("Nome completo: ", usuario.nome, TAM_NOME) ||
        !lerTexto("Curso: ", usuario.curso, TAM_CURSO) ||
        !lerTexto("Telefone: ", usuario.telefone, TAM_TELEFONE)) {
        return;
    }
    if (buscarUsuarioPorMatricula(biblioteca, usuario.matricula) != NULL) {
        printf("Ja existe um usuario com essa matricula.\n");
        return;
    }
    if (!obterDataAtual(&usuario.dataCadastro)) {
        return;
    }
    atualizada = *biblioteca;
    atualizada.usuarios[atualizada.quantidadeUsuarios++] = usuario;
    if (aplicarAlteracao(biblioteca, &atualizada))
        printf("Usuario cadastrado e dados salvos.\n");
}

/*
 * Gera o proximo codigo de emprestimo (maior codigo existente + 1).
 * Retorna 0 se nao for possivel gerar outro codigo (limite do int).
 */
static int proximoCodigoEmprestimo(const Biblioteca *biblioteca, int *codigo)
{
    int i;
    int maior;
    maior = 0;
    for (i = 0; i < biblioteca->quantidadeEmprestimos; i++) {
        if (biblioteca->emprestimos[i].codigo > maior) {
            maior = biblioteca->emprestimos[i].codigo;
        }
    }
    if (maior == INT_MAX) {
        return 0;
    }
    *codigo = maior + 1;
    return 1;
}

/*
 * Realiza um emprestimo: confere se usuario e livro existem e se ha
 * exemplar disponivel, define a devolucao para 7 dias depois e diminui o
 * numero de exemplares disponiveis.
 */
static void realizarEmprestimo(Biblioteca *biblioteca)
{
    Emprestimo emprestimo;
    Usuario *usuario;
    Livro *livro;
    Biblioteca atualizada;
    if (biblioteca->quantidadeEmprestimos >= MAX_EMPRESTIMOS) {
        printf("Limite de emprestimos atingido.\n");
        return;
    }
    if (!lerInteiro("Matricula do usuario: ", 1, INT_MAX,
                    &emprestimo.matriculaUsuario) ||
        !lerInteiro("Codigo do livro: ", 1, INT_MAX, &emprestimo.codigoLivro)) {
        return;
    }
    usuario = buscarUsuarioPorMatricula(biblioteca, emprestimo.matriculaUsuario);
    livro = buscarLivroPorCodigo(biblioteca, emprestimo.codigoLivro);
    if (usuario == NULL) {
        printf("Usuario nao cadastrado.\n");
        return;
    }
    if (livro == NULL) {
        printf("Livro nao cadastrado.\n");
        return;
    }
    if (livro->exemplaresDisponiveis == 0) {
        printf("Nao ha exemplares disponiveis.\n");
        return;
    }
    if (!proximoCodigoEmprestimo(biblioteca, &emprestimo.codigo)) {
        printf("Nao foi possivel gerar outro codigo de emprestimo.\n");
        return;
    }
    if (!obterDataAtual(&emprestimo.dataEmprestimo)) {
        return;
    }
    /* Prazo de devolucao: 7 dias apos a data do emprestimo. */
    emprestimo.dataPrevistaDevolucao = adicionarDias(emprestimo.dataEmprestimo, 7);
    if (!dataValida(emprestimo.dataPrevistaDevolucao)) {
        printf("O prazo excede o intervalo de datas suportado.\n");
        return;
    }
    emprestimo.ativo = 1;
    /* Altera a copia: tira um exemplar disponivel e registra o emprestimo. */
    atualizada = *biblioteca;
    livro = buscarLivroPorCodigo(&atualizada, emprestimo.codigoLivro);
    livro->exemplaresDisponiveis--;
    atualizada.emprestimos[atualizada.quantidadeEmprestimos++] = emprestimo;
    if (aplicarAlteracao(biblioteca, &atualizada)) {
        printf("Emprestimo %d realizado. Devolucao prevista para ", emprestimo.codigo);
        imprimirData(emprestimo.dataPrevistaDevolucao);
        printf(".\n");
    }
}

/*
 * Registra a devolucao de um livro: marca o emprestimo como devolvido e
 * devolve o exemplar ao acervo (aumenta os disponiveis).
 */
static void realizarDevolucao(Biblioteca *biblioteca)
{
    int codigo;
    Emprestimo *emprestimo;
    Livro *livro;
    Biblioteca atualizada;
    if (!lerInteiro("Codigo do emprestimo: ", 1, INT_MAX, &codigo)) {
        return;
    }
    emprestimo = buscarEmprestimoPorCodigo(biblioteca, codigo);
    if (emprestimo == NULL) {
        printf("Emprestimo nao encontrado.\n");
        return;
    }
    if (!emprestimo->ativo) {
        printf("Esse emprestimo ja foi devolvido.\n");
        return;
    }
    livro = buscarLivroPorCodigo(biblioteca, emprestimo->codigoLivro);
    if (livro == NULL || livro->exemplaresDisponiveis >= livro->exemplaresTotal) {
        printf("Erro de consistencia: nao foi possivel atualizar o acervo.\n");
        return;
    }
    /* Altera a copia e so confirma se o salvamento der certo. */
    atualizada = *biblioteca;
    emprestimo = buscarEmprestimoPorCodigo(&atualizada, codigo);
    livro = buscarLivroPorCodigo(&atualizada, emprestimo->codigoLivro);
    emprestimo->ativo = 0;
    livro->exemplaresDisponiveis++;
    if (aplicarAlteracao(biblioteca, &atualizada))
        printf("Devolucao registrada.\n");
}

/* Pesquisa livros por codigo, por parte do titulo ou por parte do autor. */
static void pesquisarLivros(Biblioteca *biblioteca)
{
    int opcao;
    int codigo;
    char termo[TAM_LINHA];
    int encontrados;
    int i;
    printf("\n1. Por codigo\n2. Por titulo\n3. Por autor\n0. Voltar\n");
    if (!lerInteiro("Opcao: ", 0, 3, &opcao) || opcao == 0) {
        return;
    }
    encontrados = 0;
    if (opcao == 1) {
        /* Busca exata pelo codigo. */
        if (!lerInteiro("Codigo: ", 1, INT_MAX, &codigo)) {
            return;
        }
        {
            Livro *livro = buscarLivroPorCodigo(biblioteca, codigo);
            if (livro != NULL) {
                imprimirLivro(livro);
                encontrados++;
            }
        }
    } else {
        /* Busca por trecho do titulo (opcao 2) ou do autor (opcao 3). */
        if (!lerTexto(opcao == 2 ? "Titulo (ou parte): " : "Autor (ou parte): ",
                      termo, sizeof(termo) - 1)) {
            return;
        }
        for (i = 0; i < biblioteca->quantidadeLivros; i++) {
            const char *campo = opcao == 2 ? biblioteca->livros[i].titulo
                                           : biblioteca->livros[i].autor;
            if (textoContem(campo, termo)) {
                imprimirLivro(&biblioteca->livros[i]);
                encontrados++;
            }
        }
    }
    if (encontrados == 0) {
        printf("Nenhum livro encontrado.\n");
    }
}

/* Pesquisa usuarios por matricula ou por parte do nome. */
static void pesquisarUsuarios(Biblioteca *biblioteca)
{
    int opcao;
    int matricula;
    char termo[TAM_LINHA];
    int encontrados;
    int i;
    printf("\n1. Por matricula\n2. Por nome\n0. Voltar\n");
    if (!lerInteiro("Opcao: ", 0, 2, &opcao) || opcao == 0) {
        return;
    }
    encontrados = 0;
    if (opcao == 1) {
        /* Busca exata pela matricula. */
        if (!lerInteiro("Matricula: ", 1, INT_MAX, &matricula)) {
            return;
        }
        {
            Usuario *usuario = buscarUsuarioPorMatricula(biblioteca, matricula);
            if (usuario != NULL) {
                imprimirUsuario(usuario);
                encontrados++;
            }
        }
    } else {
        /* Busca por trecho do nome. */
        if (!lerTexto("Nome (ou parte): ", termo, sizeof(termo) - 1)) {
            return;
        }
        for (i = 0; i < biblioteca->quantidadeUsuarios; i++) {
            if (textoContem(biblioteca->usuarios[i].nome, termo)) {
                imprimirUsuario(&biblioteca->usuarios[i]);
                encontrados++;
            }
        }
    }
    if (encontrados == 0) {
        printf("Nenhum usuario encontrado.\n");
    }
}

/* Lista todos os livros cadastrados. */
static void listarLivros(const Biblioteca *biblioteca)
{
    int i;
    if (biblioteca->quantidadeLivros == 0) {
        printf("Nenhum livro cadastrado.\n");
        return;
    }
    for (i = 0; i < biblioteca->quantidadeLivros; i++) {
        imprimirLivro(&biblioteca->livros[i]);
        printf("----------------------------------------\n");
    }
}

/* Lista todos os usuarios cadastrados. */
static void listarUsuarios(const Biblioteca *biblioteca)
{
    int i;
    if (biblioteca->quantidadeUsuarios == 0) {
        printf("Nenhum usuario cadastrado.\n");
        return;
    }
    for (i = 0; i < biblioteca->quantidadeUsuarios; i++) {
        imprimirUsuario(&biblioteca->usuarios[i]);
    }
}

/* Lista apenas os emprestimos que ainda nao foram devolvidos. */
static void listarEmprestimosAtivos(const Biblioteca *biblioteca)
{
    int i;
    int encontrados;
    encontrados = 0;
    for (i = 0; i < biblioteca->quantidadeEmprestimos; i++) {
        if (biblioteca->emprestimos[i].ativo) {
            imprimirEmprestimo(biblioteca, &biblioteca->emprestimos[i]);
            encontrados++;
        }
    }
    if (encontrados == 0) {
        printf("Nao ha emprestimos ativos.\n");
    }
}

/* ========================================================================== */
/* 9. FUNCOES AVANCADAS (BUSCA AVANCADA, RENOVACAO, RELATORIOS)               */
/* ========================================================================== */

/*
 * Busca avancada com multiplos criterios combinados (todos precisam ser
 * atendidos): titulo, autor, editora, faixa de ano e disponibilidade.
 * Digitando * em um criterio de texto, ele e ignorado.
 */
static void buscaAvancada(Biblioteca *biblioteca)
{
    char titulo[TAM_TITULO + 1];
    char autor[TAM_AUTOR + 1];
    char editora[TAM_EDITORA + 1];
    int anoMinimo;
    int anoMaximo;
    int somenteDisponiveis;
    int i;
    int encontrados;
    printf("Use * para ignorar um criterio de texto.\n");
    if (!lerTexto("Titulo: ", titulo, TAM_TITULO) ||
        !lerTexto("Autor: ", autor, TAM_AUTOR) ||
        !lerTexto("Editora: ", editora, TAM_EDITORA) ||
        !lerInteiro("Ano minimo (1-9999): ", 1, 9999, &anoMinimo) ||
        !lerInteiro("Ano maximo (1-9999): ", 1, 9999, &anoMaximo) ||
        !lerInteiro("Somente disponiveis? (1=sim, 0=nao): ",
                    0, 1, &somenteDisponiveis)) {
        return;
    }
    if (anoMinimo > anoMaximo) {
        printf("O ano minimo nao pode ser maior que o ano maximo.\n");
        return;
    }
    encontrados = 0;
    for (i = 0; i < biblioteca->quantidadeLivros; i++) {
        Livro *livro = &biblioteca->livros[i];
        /* O livro so aparece se atender a todos os criterios informados. */
        if ((strcmp(titulo, "*") == 0 || textoContem(livro->titulo, titulo)) &&
            (strcmp(autor, "*") == 0 || textoContem(livro->autor, autor)) &&
            (strcmp(editora, "*") == 0 || textoContem(livro->editora, editora)) &&
            livro->anoPublicacao >= anoMinimo &&
            livro->anoPublicacao <= anoMaximo &&
            (!somenteDisponiveis || livro->exemplaresDisponiveis > 0)) {
            imprimirLivro(livro);
            encontrados++;
        }
    }
    if (encontrados == 0) {
        printf("Nenhum livro corresponde aos criterios.\n");
    }
}

/* Renova um emprestimo ativo, somando mais 7 dias a data prevista. */
static void renovarEmprestimo(Biblioteca *biblioteca)
{
    int codigo;
    Emprestimo *emprestimo;
    Data novaData;
    Biblioteca atualizada;
    if (!lerInteiro("Codigo do emprestimo ativo: ", 1, INT_MAX, &codigo)) {
        return;
    }
    emprestimo = buscarEmprestimoPorCodigo(biblioteca, codigo);
    if (emprestimo == NULL || !emprestimo->ativo) {
        printf("Emprestimo ativo nao encontrado.\n");
        return;
    }
    novaData = adicionarDias(emprestimo->dataPrevistaDevolucao, 7);
    if (!dataValida(novaData)) {
        printf("A renovacao excede o intervalo de datas suportado.\n");
        return;
    }
    atualizada = *biblioteca;
    emprestimo = buscarEmprestimoPorCodigo(&atualizada, codigo);
    emprestimo->dataPrevistaDevolucao = novaData;
    if (aplicarAlteracao(biblioteca, &atualizada)) {
        printf("Emprestimo renovado. Nova data prevista: ");
        imprimirData(novaData);
        printf(".\n");
    }
}

/*
 * Relatorio dos livros mais emprestados. Conta quantos emprestimos (ativos
 * e devolvidos) cada livro teve e exibe em ordem decrescente. Livros com a
 * mesma quantidade aparecem juntos.
 */
static void relatorioMaisEmprestados(const Biblioteca *biblioteca)
{
    int contagens[MAX_LIVROS];
    int i;
    int j;
    int maior;
    int exibiu;
    if (biblioteca->quantidadeLivros == 0) {
        printf("Nenhum livro cadastrado.\n");
        return;
    }
    /* Passo 1: conta os emprestimos de cada livro. */
    for (i = 0; i < biblioteca->quantidadeLivros; i++) {
        contagens[i] = 0;
        for (j = 0; j < biblioteca->quantidadeEmprestimos; j++) {
            if (biblioteca->emprestimos[j].codigoLivro ==
                biblioteca->livros[i].codigo) {
                contagens[i]++;
            }
        }
    }
    printf("Livros mais emprestados (historico completo):\n");
    exibiu = 0;
    /* Passo 2: a cada rodada, exibe os livros com a maior contagem restante
       e marca como -1 para nao serem exibidos de novo. */
    while (exibiu < biblioteca->quantidadeLivros) {
        maior = -1;
        for (i = 0; i < biblioteca->quantidadeLivros; i++) {
            if (contagens[i] > maior) {
                maior = contagens[i];
            }
        }
        if (maior <= 0) {
            break;
        }
        for (i = 0; i < biblioteca->quantidadeLivros; i++) {
            if (contagens[i] == maior) {
                printf("%d emprestimo(s) - %s (codigo %d)\n", maior,
                       biblioteca->livros[i].titulo, biblioteca->livros[i].codigo);
                contagens[i] = -1;
                exibiu++;
            }
        }
    }
    if (exibiu == 0) {
        printf("Ainda nao ha emprestimos registrados.\n");
    }
}

/*
 * Relatorio de usuarios com emprestimos em atraso: emprestimos ativos cuja
 * data prevista ja passou. Os dados do usuario sao exibidos uma unica vez,
 * seguidos de cada emprestimo atrasado dele.
 */
static void relatorioAtrasos(const Biblioteca *biblioteca)
{
    Data hoje;
    int i;
    int j;
    int k;
    int encontrados;
    int usuarioJaExibido;
    if (!obterDataAtual(&hoje)) {
        return;
    }
    encontrados = 0;
    printf("Usuarios com emprestimos em atraso (data atual: ");
    imprimirData(hoje);
    printf("):\n");
    for (i = 0; i < biblioteca->quantidadeEmprestimos; i++) {
        const Emprestimo *emprestimo = &biblioteca->emprestimos[i];
        /* Ignora emprestimos devolvidos ou ainda dentro do prazo. */
        if (!emprestimo->ativo ||
            compararDatas(emprestimo->dataPrevistaDevolucao, hoje) >= 0) {
            continue;
        }
        /* Verifica se o usuario ja teve outro atraso exibido antes. */
        usuarioJaExibido = 0;
        for (j = 0; j < i; j++) {
            if (biblioteca->emprestimos[j].ativo &&
                biblioteca->emprestimos[j].matriculaUsuario ==
                    emprestimo->matriculaUsuario &&
                compararDatas(biblioteca->emprestimos[j].dataPrevistaDevolucao,
                              hoje) < 0) {
                usuarioJaExibido = 1;
                break;
            }
        }
        if (!usuarioJaExibido) {
            const Usuario *usuario = NULL;
            for (k = 0; k < biblioteca->quantidadeUsuarios; k++) {
                if (biblioteca->usuarios[k].matricula ==
                    emprestimo->matriculaUsuario) {
                    usuario = &biblioteca->usuarios[k];
                    break;
                }
            }
            if (usuario != NULL) {
                imprimirUsuario(usuario);
            } else {
                printf("Matricula %d (usuario nao encontrado)\n",
                       emprestimo->matriculaUsuario);
            }
        }
        printf("  ");
        imprimirEmprestimo(biblioteca, emprestimo);
        encontrados++;
    }
    if (encontrados == 0) {
        printf("Nenhum atraso registrado.\n");
    }
}

/* ========================================================================== */
/* 10. MENUS E FUNCAO MAIN                                                    */
/* ========================================================================== */

/* Submenu de livros: repete ate o usuario escolher 0 (Voltar). */
static void menuLivros(Biblioteca *biblioteca)
{
    int opcao;
    do {
        printf("\n--- LIVROS ---\n");
        printf("1. Cadastrar livro\n2. Listar livros\n3. Pesquisar livro\n");
        printf("4. Busca avancada\n0. Voltar\n");
        if (!lerInteiro("Opcao: ", 0, 4, &opcao)) {
            return;
        }
        switch (opcao) {
        case 1:
            cadastrarLivro(biblioteca);
            break;
        case 2:
            listarLivros(biblioteca);
            break;
        case 3:
            pesquisarLivros(biblioteca);
            break;
        case 4:
            buscaAvancada(biblioteca);
            break;
        }
    } while (opcao != 0);
}

/* Submenu de usuarios: repete ate o usuario escolher 0 (Voltar). */
static void menuUsuarios(Biblioteca *biblioteca)
{
    int opcao;
    do {
        printf("\n--- USUARIOS ---\n");
        printf("1. Cadastrar usuario\n2. Listar usuarios\n3. Pesquisar usuario\n");
        printf("0. Voltar\n");
        if (!lerInteiro("Opcao: ", 0, 3, &opcao)) {
            return;
        }
        switch (opcao) {
        case 1:
            cadastrarUsuario(biblioteca);
            break;
        case 2:
            listarUsuarios(biblioteca);
            break;
        case 3:
            pesquisarUsuarios(biblioteca);
            break;
        }
    } while (opcao != 0);
}

/* Submenu de emprestimos: repete ate o usuario escolher 0 (Voltar). */
static void menuEmprestimos(Biblioteca *biblioteca)
{
    int opcao;
    do {
        printf("\n--- EMPRESTIMOS ---\n");
        printf("1. Realizar emprestimo\n2. Registrar devolucao\n");
        printf("3. Listar emprestimos ativos\n4. Renovar emprestimo\n0. Voltar\n");
        if (!lerInteiro("Opcao: ", 0, 4, &opcao)) {
            return;
        }
        switch (opcao) {
        case 1:
            realizarEmprestimo(biblioteca);
            break;
        case 2:
            realizarDevolucao(biblioteca);
            break;
        case 3:
            listarEmprestimosAtivos(biblioteca);
            break;
        case 4:
            renovarEmprestimo(biblioteca);
            break;
        }
    } while (opcao != 0);
}

/* Submenu de relatorios: repete ate o usuario escolher 0 (Voltar). */
static void menuRelatorios(Biblioteca *biblioteca)
{
    int opcao;
    do {
        printf("\n--- RELATORIOS ---\n");
        printf("1. Livros mais emprestados\n");
        printf("2. Usuarios com emprestimos em atraso\n0. Voltar\n");
        if (!lerInteiro("Opcao: ", 0, 2, &opcao)) {
            return;
        }
        switch (opcao) {
        case 1:
            relatorioMaisEmprestados(biblioteca);
            break;
        case 2:
            relatorioAtrasos(biblioteca);
            break;
        }
    } while (opcao != 0);
}

/*
 * Funcao principal: inicializa a biblioteca, carrega os dados salvos e
 * exibe o menu principal ate o usuario escolher sair.
 */
int main(void)
{
    Biblioteca biblioteca;
    int opcao;
    inicializarBiblioteca(&biblioteca);
    if (!carregarDados(&biblioteca)) {
        printf("Aviso: alguns registros podem estar ausentes ou invalidos.\n");
    }
    printf("Sistema de Gerenciamento de Biblioteca\n");
    printf("%d livro(s), %d usuario(s) e %d emprestimo(s) carregado(s).\n",
           biblioteca.quantidadeLivros, biblioteca.quantidadeUsuarios,
           biblioteca.quantidadeEmprestimos);
    do {
        printf("\n=== MENU PRINCIPAL ===\n");
        printf("1. Livros\n2. Usuarios\n3. Emprestimos\n4. Relatorios\n0. Sair\n");
        if (!lerInteiro("Opcao: ", 0, 4, &opcao)) {
            /* Entrada encerrada (EOF): sai do programa sem travar. */
            printf("\nEncerrando o sistema.\n");
            break;
        }
        switch (opcao) {
        case 1:
            menuLivros(&biblioteca);
            break;
        case 2:
            menuUsuarios(&biblioteca);
            break;
        case 3:
            menuEmprestimos(&biblioteca);
            break;
        case 4:
            menuRelatorios(&biblioteca);
            break;
        case 0:
            printf("Encerrando o sistema.\n");
            break;
        }
    } while (opcao != 0);
    return 0;
}