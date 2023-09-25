#define _GNU_SOURCE

/*
  turing.c - The Turing's Machine Simulator for terminals
 
  Copyright (C) 2004: Marcelo Keese Albertini  albertini@ufu.br
  
 
  The Turing's Machine Simulator for terminals is Free Software GPL3. See LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#define TAM_FITA 80

/*********************************************************/
int  ler_regras(char *); // cada regra é uma tupla_t
int  inserir_regra(char *);
int  ler_fita(char *); // retorna positivo se sucesso, negativo se erro
int  interpretar_maquina(void );
int  interpretar_maquina( void );
void init();
void imprimir_fita(int, int );

typedef struct tuplas 
{
    int estado;   // em Q 
    char valor;   // em Lambda
    char escrever; // em Lambda
    int sentido; /*0: esquerda; 1: direita; -1 fim de execução*/
    int estado_seguinte; // em Q
} tupla_t;

/*********************************************************/
tupla_t E_tuplas[TAM_FITA];
/* definir estrutura da fita*/
char E_fita[TAM_FITA];

/* Variaveis da maquina*/
int E_numero_tuplas = 0,
    E_numero_posicoes = 0, 
    E_estado_inicial = 0,
    E_posicao_inicial = 0,
    PAUSA = 300000; // na visualizacao da Máquina

/*********************************************************/
/* ler de arquivo as tuplas das regras */
int 
ler_regras(arquivo)
char *arquivo;
{
    int retorno = 1;
    ssize_t lidos;
    size_t tam_buffer = 0;
    FILE *fd = NULL;
    char *buffer;
   
    if (( fd = fopen(arquivo,"r")) == NULL ) 
    {
       printf("Erro abrindo arquivo das regras.\n");
       return -1;
    }

    while ( (lidos = getline(&buffer,&tam_buffer, fd)) > 0 )
    {
       /* Limpa espaços iniciais */
       while ( isspace(buffer[0]) )
       {
          buffer++;
       }
       if ( buffer[0] != '\0' )
       {
          if ( inserir_regra(buffer) < 0 )
          {
              printf("Erro: regra inválida encontrada.\n");
              return -1;
          }
       }
    }
   
    return retorno;
}

/*********************************************************/
/* lê regra/tupla da entrada e a insere no array de tuplas*/
/* retorna maior que zero se a regra é válida e se foi possível carregá-la na memória. */
int
inserir_regra(linha)
   char *linha;
{
    int retorno = 1, i = 0, posicao_tupla;
    int estado;
    char valor;
    char escrever;
    int sentido;
    int estado_seguinte;
    
    posicao_tupla = E_numero_tuplas; 

    /* algoritmo de tratamento das linhas das tuplas */
    switch ( linha[i] )
    {
       case 'I': /* Tupla especial de início de execução. */
          { // sintaxe é I <estado> <simbolo> // estado que a MT está inicialmente e posicao inicial da cabeça de I/O da máquina

	        if (sscanf(linha, "I %d %d", &estado, &sentido) == 2) {
			printf("Tupla inicial: (I, %d, %d)\n", estado, sentido);
		} else {
			printf("erro na leitura da tupla inicial\n");
		}

		E_estado_inicial = estado;
                E_posicao_inicial = sentido;
    
             return 2;
             break;
          }

       case 'F': /* Tupla especial de fim de execução de aceitação. */
          {
            escrever = '!'; // símbolo reservado para tupla final
            sentido = -1; // para indicar o estado é final

	    if (sscanf(linha, "F %d %c", &estado, &valor) == 2) {
		printf("Tupla final: (F, %d, %c)\n", estado, valor);  // valor == 0 indica estado final de rejeição
								      // valor == 1 indica estado final de aceitação
		if (valor == '0') {
			estado_seguinte = -2; // rejeição
		} else if (valor == '1') {
			estado_seguinte = -1;// aceitação
		} else {
			printf("ERRO na definição de estado final\n");
		}
	    } else {
		    printf("ERRO: na leitura de tupla final.\n");
	    }
                
             break;
          }

       case '#': /* Somente um comentário.*/
          {
             return 2; 
             break;
          }
       
       default: /* Caso seja uma tupla regular. */
          {
	    if (sscanf(linha, "%3d %c %c %3d %d", &estado, &valor, &escrever, &sentido, &estado_seguinte)==5) {
		if (sentido == 0) {
			printf("Tupla regular: (%d, \'%c\', \'%c\', E, %d)\n", estado, valor, escrever, estado_seguinte); 
		} else if (sentido == 1) {
			printf("Tupla regular: (%d, \'%c\', \'%c\', D, %d)\n", estado, valor, escrever, estado_seguinte); 
		} else {
			printf("Tupla regular: (%d, \'%c\', \'%c\', %d, %d)\n", estado, valor, escrever, sentido, estado_seguinte); 
		}
	    } else {
		printf("A formatação da tupla está incorreta!\n");
	    }
	    break;
          }    
    }
    
    E_tuplas[posicao_tupla].estado = estado;
    E_tuplas[posicao_tupla].valor = valor;
    E_tuplas[posicao_tupla].escrever = escrever;
    E_tuplas[posicao_tupla].sentido = sentido;
    E_tuplas[posicao_tupla].estado_seguinte = estado_seguinte;

    E_numero_tuplas++;
    return retorno;
}


/*********************************************************/
/* ler de arquivo os valores da fita */
int 
ler_fita(arquivo)
char *arquivo;
{
    int retorno = 1;
    ssize_t lidos;
    size_t tam_buffer = 0;
    int i;
    FILE *fd = NULL;
    char *linha = NULL ;
    
    if (( fd = fopen(arquivo,"r")) == NULL ) 
    {
       printf("Erro abrindo arquivo da fita.\n");
       return -1;
    }

    if ((lidos = getline(&linha, &tam_buffer, fd)) < 0 )
    {
       printf("Erro na leitura da fita.\n");
       return -1;
    }
    
    if ( lidos <= 0 )
    {
       printf("Erro: número insuficiente de valores na fita ou erro na leitura.\n");
       return -1;
    }

    if ( lidos > TAM_FITA)
    {
       printf("Erro: número de valores na fita maior que o suportado (%d): %d .\n",(int)TAM_FITA,(int)lidos);
    }
    
    // ler cada simbolo da fita -- 1 simbolo == um char ASCII
    for ( i = 0; linha[i] != '\n' && linha[i] != ' '; ++i )
    {
       if ( linha[i] >= 35 || linha[i] <= 126 ) // entre # e ~
       {
           E_fita[i] = linha[i];
       }
       else // caracteres não visíveis
       {
            printf("Erro: valor da fita inválido.\n");
            return -1;
       }
       E_numero_posicoes++;
    }

    return retorno;
}

/*********************************************************/
/* Imprime fita na saída padrão. */
void 
imprimir_fita(int posicao_atual, int alterou)
{
    int i;
    printf("|  "); 
    for ( i = 0; i < E_numero_posicoes; i++)
    {
       /* colore de vermelho a posicao atual */
       if ( (i == posicao_atual) && (i != alterou) )
          printf(" \033[40;31;1m%c\033[m ", E_fita[i]);
       else
       /* colore de ciano se alterou a posicao anterior */
       if ( (alterou > 0) && (i == alterou) )
          printf(" \033[40;36;1m%c\033[m ", E_fita[i]);
       else
          printf(" %c ",E_fita[i]);
    }
}


/*********************************************************/
/* interpreta as regras para a dada fita */

int
interpretar_maquina( void )
{
    int retorno = 1;
    int continuar = 1;
    int estado_atual, i, posicao_atual, tupla_atual, achou = 0, alterou=-1;
    
    estado_atual  = E_estado_inicial;
    posicao_atual = E_posicao_inicial;
    
    /*int estado;
    int valor;
    int escrever;
    int sentido; 0: esquerda; 1: direita; -1 fim de execução
    int estado_seguinte; */
    
    printf("\nEstado inicial: %d\n", estado_atual);
    printf("Posição inicial: %d\n\n", posicao_atual +1);
    imprimir_fita(posicao_atual, alterou);
    printf("\n");
    
    while (continuar == 1)
    {
       
       /* Procurar tupla para a configuração atual */
       achou = 0;
       for ( i = 0; i < E_numero_tuplas && achou == 0; i++)
       {
          if ( (E_tuplas[i].estado == estado_atual ) && 
              	(( E_tuplas[i].valor == E_fita[posicao_atual]) 
		 ||
		 ( E_tuplas[i].sentido < 0))
	     )
          {
             achou = 1;
             tupla_atual = i;
          }
       }
       
       /* Para parar é necessário:
       * - não haver tuplas para a atual configuração;
       * - ou, chegar numa tupla de estado final.
       * */ 
       if ( (achou == 0) || (E_tuplas[tupla_atual].sentido < 0 ))
       {
          continuar = 0;
       }

       /* Executando a tupla.*/
       if ( (achou == 1) && (continuar == 1) )
       {
           /* guarda qual alterou para colorir */
           if ( E_fita[posicao_atual] != E_tuplas[tupla_atual].escrever )
              alterou = posicao_atual;
           
           /* escreve novo valor na fita*/
           E_fita[posicao_atual] = E_tuplas[tupla_atual].escrever;
           /* guarda novo estado da maquina */
           estado_atual = E_tuplas[tupla_atual].estado_seguinte;
	   
           switch (E_tuplas[tupla_atual].sentido)
           {
              case 0:/* esquerda */ 
                 {
                    posicao_atual--;
                    if ( posicao_atual < 0 )
                    {
			posicao_atual = 0;
                    }
                    break;
                 }
              case 1: /* direita */
                 {
                    posicao_atual++;
                    if ( posicao_atual > TAM_FITA -1)
                    {
                       posicao_atual--;
                    }
                    break;
                 }
              default:/* Se fosse -1 deveria ter saido antes, qualquer outro é erro*/
                 {
                    printf("Erro: sentido inválido.\n");
                    return -1;
                 }
           }
       
       }
       
       imprimir_fita(posicao_atual, alterou);
       
       /* Exibe execução.*/  
       printf("  | %d | (%d,%c,%c,%d,%d)\n", estado_atual, E_tuplas[tupla_atual].estado,
                                                   E_tuplas[tupla_atual].valor,
                                                   E_tuplas[tupla_atual].escrever,
                                                   E_tuplas[tupla_atual].sentido,
                                                   E_tuplas[tupla_atual].estado_seguinte);
       
       usleep(PAUSA);
    }
    
    printf("\n");
    return retorno;
}



/*********************************************************/
/* Prepara variáveis para a execução do simulador. */
void
init()
{
   int i = 0;
   
    for ( i = 0; i < TAM_FITA; ++i)
    {
        E_fita[i] = -1;
        
        E_tuplas[i].estado = -1;
        E_tuplas[i].valor = ' ';
        E_tuplas[i].escrever = ' ';
        E_tuplas[i].sentido = -1;
        E_tuplas[i].estado_seguinte = -1;
    }
}

/*********************************************************/
/* usuario deverá entrar primeiro caminho do arquivo de regras e depois da fita*/
int
main(argc, argv)
int argc;
char **argv;
{

  extern FILE *stdout;
    

  /* se não foram passados todos os argumentos (tuplas/fita) */
  if ( argc < 3 )
  {
     printf("\n\033[40;37;1mSimulador da máquina de turing v0.1.\033[m\n\n");
     printf("Modo de uso:  turing \033[40;33marquivo_de_tuplas\033[m \033[40;32marquivo_da_fita\033[m \033[40;36m[saida]\033[m\n\n");
     printf("Onde \033[40;33marquivo_de_tuplas\033[m é o endereço do arquivo que contém as regras de\n");
     printf("execução da máquina de turing e \033[40;32marquivo_da_fita\033[m é o arquivo que contém\n");
     printf("o estado inicial da fita para execução da máquina.\n");
     printf("O argumento opcional \033[40;36m[saida]\033[m serve para gravar a saída do simulador no\narquivo de endereço \033[40;36msaida\033[m.\n\n");
     exit(0);
  }

  /* se for modo nao iterativo */
  if ( argc == 4 )
  {
     fclose(stdout);
     if (( stdout = fopen(argv[3],"w")) == NULL ) 
     {
        printf("Erro abrindo arquivo de gravação.\n");
        return 0;
     }
     PAUSA = 0;
  }
     
  /* inicializa estruturas de dados */ 
  init();
  
  if ( ler_regras(argv[1]) < 0 )
  {
     printf("Erro carregando regras.\n");
     exit(0);
  }
  
  if ( ler_fita(argv[2]) < 0 )
  {
     printf("Erro carregando fita.\n");
     exit(0);
  }
  
  if (interpretar_maquina() < 0 )
  {
    printf("Erro interpretando a máquina.\n");
    exit(0);
  }

  return 1;

}
/*********************************************************/
