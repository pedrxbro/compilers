#ifndef CONSTANTS_H
#define CONSTANTS_H

enum TokenId 
{
    EPSILON  = 0,
    DOLLAR   = 1,
    t_INTEIRO_DECIMAL = 2,
    t_INTEIRO_BINARIO = 3,
    t_INTEIRO_HEXADECIMAL = 4,
    t_FLUTUANTE_LITERAL = 5,
    t_IDENTIFICADOR = 6,
    t_PROGRAMA = 7,
    t_INICIO = 8,
    t_FIM = 9,
    t_INTEIRO = 10,
    t_FLUTUANTE = 11,
    t_TEXTO = 12,
    t_LOGICO = 13,
    t_CARACTERE = 14,
    t_SE = 15,
    t_ENTAO = 16,
    t_SENAO = 17,
    t_FIMSE = 18,
    t_ENQUANTO = 19,
    t_FACA = 20,
    t_FIMENQUANTO = 21,
    t_PARA = 22,
    t_ATE = 23,
    t_PASSO = 24,
    t_FIMPARA = 25,
    t_FUNCAO = 26,
    t_RETORNE = 27,
    t_FIMFUNCAO = 28,
    t_LEIA = 29,
    t_MOSTRE = 30,
    t_RECEBE = 31,
    t_VERDADEIRO = 32,
    t_FALSO = 33,
    t_NULO = 34,
    t_MAIS = 35,
    t_MENOS = 36,
    t_VEZES = 37,
    t_DIVIDO = 38,
    t_RESTO = 39,
    t_MAIOR_QUE = 40,
    t_MENOR_QUE = 41,
    t_MAIOR_IGUAL_QUE = 42,
    t_MENOR_IGUAL_QUE = 43,
    t_IGUAL = 44,
    t_DIFERENTE = 45,
    t_E_LOGICO = 46,
    t_OU_LOGICO = 47,
    t_NAO_LOGICO = 48,
    t_DESLOCA_DIREITA = 49,
    t_DESLOCA_ESQUERDA = 50,
    t_E_BIT_BIT = 51,
    t_OU_BIT_BIT = 52,
    t_NAO_BIT_BIT = 53,
    t_OU_EXCLUSIVO_BIT_BIT = 54,
    t_CARACTERE_LITERAL = 55,
    t_TEXTO_LITERAL = 56,
    t_ABRE_PARENTESE = 57,
    t_FECHA_PARENTESE = 58,
    t_ABRE_COLCHETE = 59,
    t_FECHA_COLCHETE = 60,
    t_ABRE_CHAVE = 61,
    t_FECHA_CHAVE = 62,
    t_VIRGULA = 63,
    t_PONTO = 64,
    t_PONTO_VIRGULA = 65,
    t_DOIS_PONTOS = 66,
    t_PROCEDIMENTO = 67,
    t_FIMPROCEDIMENTO = 68
};

const int STATES_COUNT = 31;

extern int SCANNER_TABLE[STATES_COUNT][256];

extern int TOKEN_STATE[STATES_COUNT];

extern int SPECIAL_CASES_INDEXES[70];

extern const char *SPECIAL_CASES_KEYS[50];

extern int SPECIAL_CASES_VALUES[50];

extern const char *SCANNER_ERROR[STATES_COUNT];

const int FIRST_SEMANTIC_ACTION = 124;

const int SHIFT  = 0;
const int REDUCE = 1;
const int ACTION = 2;
const int ACCEPT = 3;
const int GO_TO  = 4;
const int ERROR  = 5;

extern const int PARSER_TABLE[191][124][2];

extern const int PRODUCTIONS[108][2];

extern const char *PARSER_ERROR[191];

#endif
