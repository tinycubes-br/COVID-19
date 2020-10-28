#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "teste_block.h"

// #define ADDRESS_SIZE     10
#define BLOCK_ADDRESSES  2
#define BLOCK_SIZE       (ADDRESS_SIZE * BLOCK_ADDRESSES) 
#define MIN_BLOCKS       5
// typedef int TTestBlock[BLOCK_SIZE];

//typedef struct {
//    int n;
//    TTestBlock blocks[MAX_BLOCKS];
//    int removal_order[MAX_BLOCKS];
//} TTest, *PTest;

#define RAND_INT(RMIN, RMAX)   ((rand() % (RMAX-RMIN)) + RMIN)

char fname[260];


#include <string.h>
#include <errno.h>
#include <sys/stat.h>    /* Fix up for Windows - inc mode_t */

typedef struct stat Stat;

#ifndef lint
/* Prevent over-aggressive optimizers from eliminating ID string */
const char jlss_id_mkpath_c[] = "@(#)$Id: mkpath.c,v 1.13 2012/07/15 00:40:37 jleffler Exp $";
#endif /* lint */

static int do_mkdir(const char *path, mode_t mode)
{
    Stat            st;
    int             status = 0;

    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = -1;
    }

    return(status);
}

/**
** mkpath - ensure all directories in path exist
** Algorithm takes the pessimistic view and works top-down to ensure
** each directory in path exists, rather than optimistically creating
** the last element and working backwards.
*/
int mkpath(const char *path, mode_t mode)
{
    char           *pp;
    char           *sp;
    int             status;
    char           *copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = do_mkdir(path, mode);
    free(copypath);
    return (status);
}


#if 0

char *generate_testset_filename(int max_n, int seq) {
    PTest ptest;

    if (max_n<MIN_BLOCKS) max_n = MIN_BLOCKS;
    if (max_n>MAX_BLOCKS) max_n = MAX_BLOCKS;
    
    int n = RAND_INT(MIN_BLOCKS,max_n);            
 
    int i, j;
    for (i = 0; i < n; i++) {
        TTestBlock pb = ptest->blocks + i;
        for (j=0; j < BLOCK_SIZE; j ++) {
            pb[j] = RAND_INT(65, 65+12); 
        }
    }   

    //  gera a ordem de remocao normal
    for (i = 0; i < n; i++) {
        ptest->removal_order[i] = i;
    }

    // permuta dois elementos 100 x o numero máximo de blocos
    for (j = 0; j < 100 * MAX_BLOCKS; j++) {
        int a = RAND_INT(0, n);
        int b = RAND_INT(0, n);
        int t = ptest->removal_order[a];
        ptest->removal_order[a] = ptest->removal_order[b];
        ptest->removal_order[b] = t;
    }

    // prepara para gerar o arquivo de saida
    time_t current_time;
    struct tm * time_info;
    char timeString[20];  

    time(&current_time);
    time_info = localtime(&current_time);

    strftime(timeString, sizeof(timeString), "%y%m%d_%H%M%S", time_info);    
    sprintf(fname,"./tests/%s/test_%d",timeString,seq);

    // cria o arquivo de saida
    FILE *f;
    f = fopen(fname,"wt");


    fclose(f);
}   

#endif

#define MIN_FIRST_RUN  5
#define MAX_FIRST_RUN  (MIN_FIRST_RUN + 10)

// int blocks[ADDRESS_SIZE * MAX_BLOCKS];

// #define ADDRESS_RANGE 25

/**
 * 
 * 
 *
 **/
void create_address(unsigned char *a, int n, int range) {
    for (int j=0; j < n; j ++) {
        a[j] = RAND_INT(65, 65 + range); 
    }
}


    int presentes[2*MAX_BLOCKS];
    int action[2*MAX_BLOCKS];
    // int addresses[MAX_BLOCKS * ADDRESS_SIZE];


/**
 * 
 * 
 *
 **/
char *create_test_sequence(char *filename, int address_size, int n, int _first_run, int only_insertions, int range) {
    int next=0;
    int n_presentes = 0;
    unsigned char *addresses = calloc(address_size * MAX_BLOCKS,  sizeof(unsigned char));

    // if (n < MIN_FIRST_RUN) n = MIN_FIRST_RUN;

    printf("n:%d, first_run:%d, range:%d\n", n, _first_run, range);
    int first_run;
    if (_first_run) {
        first_run = _first_run;
    } else {
        first_run = n / 10;
        if (first_run < 2 * MIN_FIRST_RUN) first_run = 2 * MIN_FIRST_RUN;
        first_run = MIN_FIRST_RUN + RAND_INT(0, first_run);
    }

    if (n < first_run) n = first_run;
    if (n > MAX_BLOCKS) n = MAX_BLOCKS;

    fprintf(stderr, "Generating test set ...");
    
    // printf("----------------[0]-----------\n");

    // first_run
    int idx = 0;
    while (idx<first_run) {
        create_address(addresses+(next*address_size),address_size, range); 
        action[idx] = next+1;
        idx ++;

        presentes[n_presentes] = next;
        n_presentes ++;
        next ++;
    }

    //printf("----------------[1]-----------\n");
    
    // gera a sequencia de acoes
    while (next < n) {
        if (idx > 2 * n) {
            printf("ERROR \n");
            return NULL;
        }

        int bias;
        bias = (idx < n)? 50: 50;

        int coloca = only_insertions? 0: RAND_INT(0,100);

        if ((coloca < bias) || (n_presentes < 2*first_run)) {
            create_address(addresses+(next*address_size),address_size, range); 
            action[idx] = next+1;
            idx ++;

            presentes[n_presentes] = next;
            n_presentes ++;
            next ++;

        } else if (n_presentes > 0) {
            int idel = RAND_INT(0,n_presentes);
            int sel = presentes[idel];
            action[idx] = -(sel+1);
            idx ++;

            // desloca todos os demais presentes uma posicao para baixo 
            idel++;
            while (idel < n_presentes) {
                presentes[idel-1] = presentes[idel];
                idel ++;
            }
            n_presentes--;
        }
    }

    // printf("----------------[9]-----------\n");
    //printf("Idx = %d  Next=%d  N_Presentes=%d\n", idx, next, n_presentes);
    if (!only_insertions) {
        while (n_presentes > 0) {
            int idel = RAND_INT(0,n_presentes);
            if (idel <0) {
                fprintf(stderr,"Fatal 1\n");
            }
            if (idel >= 2*MAX_BLOCKS) {
                fprintf(stderr,"Fatal 10\n");
            }
            int sel = presentes[idel];
            action[idx] = -(sel+1);
            if (idx >= 2*MAX_BLOCKS) {
                fprintf(stderr,"Fatal 20. Idx = %d\n",idx);
            }
            idx ++;

            // desloca todos os demais presentes uma posicao para baixo 
            idel++;
            while (idel < n_presentes) {
                if (idel <1) {
                    fprintf(stderr,"Fatal 2\n");
                }
                presentes[idel-1] = presentes[idel];
                idel ++;
            }
            n_presentes--;
        }
    }

    int total_elements = idx;
    // printf("----------------[10]-----------\n");

    time_t current_time;
    struct tm * time_info;
    char timeString[20];  
    static char testset_fname[260];

    time(&current_time);
    time_info = localtime(&current_time);

    strftime(timeString, sizeof(timeString), "%y%m%d_%H%M%S", time_info);    
    sprintf(testset_fname,"test_%s_%02d.txt",timeString,address_size);


    FILE *f;
    f = fopen(testset_fname,"wt");
    if (!f) { printf("ERROR FILE\n"); return NULL;}
    fprintf(f,"* %s_%s\n",filename,timeString);

    int i;
    for (i=0; i< total_elements; i++) {

        idx = action[i];
        if (idx < 0) idx = -idx;
        idx --;

        // transforma o ADDRESS em string
        char addr[200+1];
        unsigned char *c = addresses + (idx * address_size);
        for (int j=0; j<address_size;j++)
            addr[j] = (char) c[j];
        addr[address_size] = '\0';
        
        // printf("%05d => %+6d  '%s' \n",i+1, action[i], addr);

        char *sinal = (action[i]<0) ? "-" : "+";
        fprintf(f,"%s%s\n",sinal,addr);
    }

    fclose(f);
    fprintf(stderr," done!\n");

    free(addresses);
    return testset_fname;
}
