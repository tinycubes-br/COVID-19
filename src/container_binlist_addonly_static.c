#include <stdlib.h>
#include "metadata.h"
#include "register.h"
#include "query.h"

#define __USE_POOL__NO

//=============================================================================================
//
//    
//
//=============================================================================================

#ifdef __USE_POOL__
#define POOL_STEP 4096

typedef struct s_free_item {
    void *next;
} FreeNode, *PFreeItem;

typedef struct s_block {
    struct s_block *next;
    int   offset_next_free;
    int   free_tail;
} Block, *PBlock;

typedef struct s_pool {
    PFreeItem *freelist;

    int item_size;
    int nodes_per_block;

    int n_blocks;
    PBlock blocks;
} Pool, *PPool;

// Inicializa o Pool
static Pool pool =  { NULL, 0, 0, 0, NULL};

//-----------------------
//
//
//
//
static void release_pool(void) {
    PBlock p, q;
    for (p = pool.blocks; p; p = q) {
        q = p->next;
        free(p);
    }
    memset(&pool, 0, sizeof(Pool));
}

//-----------------------
//
//
//
//
static PBlock alloc_block(PPool pp) {
    int sz = sizeof(Block) + pp->item_size * pp->nodes_per_block;
    PBlock pb = malloc(sz);
    
    content_total_alocado += sz;

    if (!pb) fatal("BinList: allocation");

    pb->free_tail = pp->nodes_per_block;
    pb->offset_next_free = sizeof(Block);
    //printf("Size: %d, Free: %d Offset: %d\n",sz,pb->free_tail, pb->offset_next_free); fflush(stdout);
    return pb;
}

//-----------------------
//
//
//
//
static void *pool_alloc(int item_size) {
    PPool pp = &pool;
    void *res;

    if (!pp->item_size) {
        pp->item_size = item_size;
        pp->nodes_per_block = POOL_STEP;
        pp->blocks = alloc_block(pp);

    } else if (pp->item_size != item_size) {
        fatal("pool item size");
    }

    PBlock pb = NULL;

    // aloca da lista livre primeiro
    res = pp->freelist;
    if (res) {
        pp->freelist = ((PFreeItem) res)->next;

    // nao tem espaco no final 
    } else {
        if (!pp->blocks->free_tail) {
            // aloca um novo bloco 
            pb = alloc_block(pp); 
            pb->next = pp->blocks;
            pp->blocks = pb;
            //printf("New ");
        } else {
            pb  = pp->blocks;
            //printf("Current ");

        }
        res = ((char *) pb) + pb->offset_next_free;
        pb->offset_next_free += pp->item_size;
        pb->free_tail --;
        //printf("Item size: %d, Free: %d Offset: %d\n",pp->item_size,pb->free_tail, pb->offset_next_free); fflush(stdout);
    }

    // limpa
    memset(res,0,pp->item_size);
    return res;
}

#endif

//=============================================================================================
//
//    
//
//=============================================================================================


typedef struct s_bnode *PBinNode;
#define SHORT_BIN

#ifdef SHORT_BIN
typedef unsigned short bin_t;
#else
typedef unsigned int bin_t;
#endif

#define __INSERT_ONLY__


typedef struct __attribute__((__packed__))  s_bitem {
    bin_t id_bin;
    Byte data [];
} BinItem, *PBinItem;

typedef struct __attribute__((__packed__))  s_bnode {
    struct s_bnode  *next_bin;
    BinItem   items[0];
} BinNode;

typedef struct __attribute__((__packed__)) {
    PBinNode first; 
    PBinNode last;
    struct {
        __uint64_t   count    :37; // up to 128G-1
        unsigned int bin_size :17; // up to 128K-1
        unsigned int data_size :9; // up to 512-1
        unsigned int shallow   :1; // up to 2-1
    } data;
//    unsigned char items_per_node;
#ifdef __HIST__    
    int n_nodes;
#endif    
} BinList, *PBinList;

typedef struct {
    unsigned int list_bin_size;
} BParams, *PBinListParams;

#ifdef __HIST__    
int total_n_nodes = 0;
int max_blist_n_nodes = 0;
#endif

#ifdef __HIST__
int hist[25001];
#endif

static Byte shallow_data[1024];
int count = 0;

#define ITEMS_PER_NODE 4

__int64_t count_items[ITEMS_PER_NODE] = { 0, 0 };

//-----------------------
//
//
//
//
static void *binlist_insert(PRecord record, void *data, PContentData pcd) {
    PBinList pbl = (PBinList) data;
    PBinListParams pblp = (PBinListParams) pcd->params;
    PBinItem pbi;

    pbl->data.count ++;
    
    if (pbl->data.bin_size != pblp->list_bin_size) 
        fatal("BIN_SIZE: [%p %d] != [%p %d]", pbl, pbl->data.bin_size, pblp, pblp->list_bin_size);

    // sinaliza que os dados foram compartilhados
    if (pbl->data.shallow) return &shallow_data;

    __int64_t value = get_record_value(record, pcd, 0);
    value /= pblp->list_bin_size;
    if (value > __INT16_MAX__) fatal("BINLINST: overflow\n");
    bin_t id_bin = (bin_t) value;

    id_bin ++; // reserva o valor zero

    PBinNode p = pbl->last;
    int item_size = pbl->data.data_size + sizeof(bin_t);

    //printf("pbl = %p  l = %p\n", pbl, p);
    // Primeiro node?
    if (!p) {
        //printf("BINLIST: Primeiro\n");
        // configura a lista
        pbl->data.data_size = pcd->total_data_size;
        //ITEMS_PER_NODE = 1;
    
    // ja existem nodes na lista
    } else {
        int i,j=0;

        //printf("BINLIST: Nao Primeiro\n");
        //printf("BINLIST: Found\n");
        for (i=ITEMS_PER_NODE, pbi = p->items; i; i--, j++) {
            
            //printf("BINLIST: %p found id_bin %d %d \n", p, pbi->id_bin, id_bin);
            //printf("Epoch %ld  Bin = %d Last = %p\n",  value, id_bin, p );

            if (id_bin == pbi->id_bin) {
                //printf("EXACT\n");
                return pbi->data;

            } else if (id_bin < pbi->id_bin) {
                fatal("Backwarding is not allowed in container binlist_addonly");
            
            // acabou a lista
            } else if (pbi->id_bin == 0) {
                // printf("NEW\n");
                count_items[j-1]--;
                count_items[j]++;
                pbi->id_bin = id_bin;
                return pbi->data;
            }
            pbi = (PBinItem) ((char *) pbi +  item_size); 
        }

        // nao encontrou no bloco. 
        // E nao tem espaco vazio Precisa alocar.
    }
    // printf("BINLIST: new id_bin %d  %d  %p\n", id_bin, value, pbl);

    // allocates space to store subchannel data
    int node_size = sizeof(BinNode) + (item_size * ITEMS_PER_NODE);

#ifdef __USE_POOL__    
    p = (PBinNode) pool_alloc(node_size);        
#else
    p = (PBinNode) calloc(1, node_size);   
    //printf("itemsize: %d, node_size:%d * %d\n",item_size,node_size,ITEMS_PER_NODE);     
#endif
    pbi = p->items;
    count_items[0] ++;

    pbi->id_bin = id_bin;
    //printf("BINLIST: %p New Node id_bin %d %d \n", p, pbi->id_bin, id_bin);

    // encadeia no final da lista
    if (!pbl->last) { 
        pbl->first = p; 
    } else { 
        pbl->last->next_bin = p; 
    } 
    pbl->last = p;
    count ++;
    //if (count > 2000) fatal("finished");

    return pbi->data;
}


//-----------------------
//
//
//
//
static void *binlist_remove(PRecord record, void *data, PContentData pcd) {
    return NULL;
}


//----------------------
//
//
//
//
static void _shallow_copy(PByte dst, PByte src, int n_bytes) {    
    PBinList s = (PBinList) src;
    PBinList d = (PBinList) dst;
    PBinNode p, q;

    //d->data.shallow = 1;
    //return;

    memset(d,0,sizeof(BinList));
    d->data.bin_size = s->data.bin_size;
    d->data.data_size = s->data.data_size;

    // printf(" SHALLOW %d ",d->data.bin_size); fflush(stdout);
    
    int item_size = s->data.data_size + sizeof(bin_t);
    int items_size = item_size * ITEMS_PER_NODE;
    int node_size = sizeof(BinNode) + items_size;

    for (p = s->first; p; p = p->next_bin) {
#ifdef __USE_POOL__
        q = pool_alloc(dnode_size);
#else
        q = calloc(1, node_size);
#endif

        //printf("SHALLOW %p  Items size:%d\n",q,items_size);

        // deveria fazer um shallow copy recursivo
        memcpy(q->items,p->items,items_size);
        
        // ja foi copiado no memcpy
        // q->id_bin = p->id_bin;

        if (!d->last) { 
            d->first = q; 
        } else { 
            d->last->next_bin = q; 
        }
        d->last = q;

    }
}


//----------------------
//
//
//
//
static void _prepare(int op, void *content,  PContentData pcd) {
    PBinList pbl = (PBinList) content;
    PBinListParams pblp = (PBinListParams) pcd->params;

    if (op == TERMINAL_OP_INIT) {
        pbl->data.bin_size = pblp->list_bin_size;
        // printf(" [PREPARE %p %d] ", pbl, pbl->data.bin_size); fflush(stdout);
    }
}

//----------------------
//
//
//
//
static void _parse_param(int id_param, char *s, PByte params) {
    // printf("BINLIST ParseParam:\n");
    PBinListParams pblp = (PBinListParams) params;
    if (id_param == 1) {
        pblp->list_bin_size = atoi(s);
        // printf("BIN LIST: BinSize %d\n", pblp->list_bin_size); fflush(stdout);
        if (!pblp->list_bin_size) fatal("BINLIST: BinSize Error. ");
    } else {
        fatal("BINLIST: Extra parameter 1 %d\n",id_param);        
    }
}

/*-----------------
*
*
*
*/
static int _find_bounds(PByte pdata, int *bounds) {
    PBinList pbl = (PBinList) pdata;

    PBinNode p = pbl->first;
    if (!p) return 0; 

    bounds[0] = (p->items[0].id_bin -1) * pbl->data.bin_size;

    while (p->next_bin) p = p->next_bin;

    // deve achar o ultimo 
    PBinItem pbi = p->items;
    do {
        bounds[1] = pbi->id_bin;
        pbi ++;
    } while (pbi->id_bin);
    bounds[1] = (bounds[1] - 1) * pbl->data.bin_size;

    return 1;
}


//----------------------
//
//
//
//
static void register_content_binlist(PClass pclass) {

    static ContentFunctions cfs;
    cfs.parse_param = _parse_param;
    cfs.prepare = _prepare;
    cfs.shallow_copy = _shallow_copy;
    cfs.insert = &binlist_insert;
    cfs.remove = &binlist_remove;
    cfs.find_bounds = &_find_bounds;
    // cfs.remove

    static ContentInfo ci;
    memset(&ci, 0, sizeof(ContentInfo));

    ci.n_param_fields = 1;
    ci.min_params = 1;
    ci.max_params = 1;
    ci.params_size = sizeof(BParams);
    ci.private_size = sizeof(BinList);
    ci.pclass = pclass;

    ci.pcfs = &cfs;

    register_content("binlist_addonly", &ci);
}

//=============================================================================================
//
//    
//
//=============================================================================================

static int end_of_data;
#define END_OF_DATA (&end_of_data)


typedef struct {
    PBinNode pbn;
    PBinItem pbi;
    int cur_item;
} State, *PState;

static State state;

/*-----------------
*
*
*
*/
static void *cop_between(PQArgs pa, PQOpData pdata, void **ppstate) {
#if 0    
    //printf("COP BETWEEN: First %p\n",pdata);
     if (*ppstate == END_OF_DATA) return NULL;

    PBinList pbl = (PBinList) pdata;

    PState p = *ppstate;
    if (!p) {
        // printf("BETWEEN: First\n");
        p = &state;
        p->pbn = pbl->first;
        p->pbi = p->pbn->items;
        p->cur_item = ITEMS_PER_NODE;
    } else {
        p->cur_item ++;
        if (p->cur_item < ITEMS_PER_NODE) {
            p->pbi ++;
            if (!p->pbi->id_bin) { *ppstate = END_OF_DATA; return NULL; }
        } else {
            p->pbn = p->pbn->next_bin;
            if (!p->pbn) { *ppstate = END_OF_DATA; return NULL; }
            p->pbi = p->pbn->items;
            p->cur_item = 0;
        }
    }

    //printf("1"); fflush(stdout);
    bin_t t0 = pa->args[0].i/pbl->data.bin_size + 1;
    bin_t t1 = pa->args[1].i/pbl->data.bin_size + 1;
    if (t0 == t1) t1 ++;
//    printf("t0 = %ld\n",t0);

    //printf("X"); fflush(stdout);
    // printf("BETWEEN: %d %d %p BIN: %d COunt: %d PBL Count %d  %d\n", t0, t1, p, p->id_bin, p->count, pbl->count, pbl->n_nodes);

    // localiza o inicio do between
    if (p->id_bin < t0) {
        do {
            // printf("P.BIN %d\n",p->id_bin);
            p = p->next_bin;
        } while (p && (p->id_bin < t0));
        if (!p) { *ppstate = END_OF_DATA; return NULL; }
    }

    // enquanto estiver 
    if ( (p->id_bin >= t0) && (p->id_bin <= t1)) {
        // printf("P.BIN FOUND %d\n",p->id_bin);
        *ppstate = p;
        return p->data;
    }

    // printf("END OF DATA\n");
    *ppstate = END_OF_DATA;
    return NULL;
#endif
}


/*-----------------
*
*
*
*/
static void *cog_between(PQArgs pa, PQOpData pdata, int *indexes, void **ppstate) {
#if 0    
    if (*ppstate == END_OF_DATA) return NULL;

    PBinList pbl = (PBinList) pdata;
    // printf("BETWEEN 0: %d %d %p %d\n", pa->args[0].i, pa->args[1].i,pdata, pbl->n_nodes);

    PBinNode p = *ppstate;
    if (!p) {
        // printf("BETWEEN: First\n");
        p =  pbl->first;
        // printf("Count: %d Size: %d\n", pbl->count, pbl->data.bin_size);
    } else {
        p = p->next_bin;
    }
    
    if (!p) { 
        // printf("X\n");
        *ppstate = END_OF_DATA; 
        return NULL; 
    }
  
    //printf(" G %d ",pbl->data.bin_size);fflush(stdout);
    bin_t t0 = pa->args[0].i/pbl->data.bin_size;
    bin_t t1 = pa->args[1].i/pbl->data.bin_size;
    if (t0 == t1) t1 ++;

    // printf("COG t0 = %ld\n",t0);
    //printf("X");fflush(stdout);
    
    // printf("BETWEEN: %d %d\n", t0, t1);

    // localiza o inicio do between
    while (p) {
        // printf("%d\n",p->id_bin);
        if ((p->id_bin >= t0)) break;
        p = p->next_bin;
    }
    if (!p) { *ppstate = END_OF_DATA; return NULL; }

    // printf("BETWEEN: %d %d\n", pa->args[0].i, pa->args[1].i);

    // enquanto estiver 
    if ( (p->id_bin >= t0) && (p->id_bin < t1)) {
        *ppstate = p;
        if (indexes) { 
            //indexes[0] = (p->id_bin   - t0) * pbl->data.bin_size; 
            indexes[0] = (p->id_bin   - t0);  // Posicao relativa aos bins
  //          printf("BINLIST: Index %d \n", indexes[0]);
        }
        // printf("OFFS: %d \n", p->id_bin);
        return p->data;
    }

    *ppstate = END_OF_DATA;
    return NULL;
#endif    
}

#define REGISTER_BINLIST_OP(X,S,Y)  \
    static QOpInfo qoi_##X;\
    qoi_##X.op = cop_##X;\
    qoi_##X.op_group_by = cog_##X;\
    qoi_##X.min_args = Y;\
    qoi_##X.max_args = MAX_ARGS;\
    qoi_##X.fmt = "II"   ;\
    register_op("binlist2", S, &qoi_##X)

/*-----------------
*
*
*
*/
static void register_ops(void) {
    REGISTER_BINLIST_OP(between,"between2",2);
    qoi_between.max_args = 2;
}


//=============================================================================================
//
//    CLASS 
//
//=============================================================================================

/*--------------------------
*
*
*
*/
static int get_distinct_info(void * cop, PQArgs pa, PByte params, int *distinct_count, int *distinct_base) {
    //printf("Z1\n"); fflush(stdout);
    PBinListParams pblp = (PBinListParams) params;

    if (cop == cop_between) {
        int begin = pa->args[0].i;
        int end   = pa->args[1].i;
        // printf("Z1 begin %d end %d\n", begin, end); fflush(stdout);
        
        //printf("BINLIST: %d\n", pblp->list_bin_size); fflush(stdout);

        *distinct_count = ((end - begin) / pblp->list_bin_size); 
        if (!*distinct_count) *distinct_count = 1;
        *distinct_base = begin;
        // printf("Z1\n"); fflush(stdout);

        return 1;
    }
    return 0;
}


/*--------------------------
*
*
*
*/
static void *create_op_data(void * cop, PQArgs pa) {

    if (cop == cop_between) {
        return calloc(1,sizeof(BinList));
    } 
    return NULL;
}

/*--------------------------
*
*
*
*/
static void destroy_op_data(void *cop, void *op_data) {
    // assume que o conteudo foi liberado
    if (op_data) free(op_data);
}

/*--------------------------
*
*
*
*/
void register_container_binlist_addonly(void) {

    // prevention against reinitialization during execution
    static int done = 0; 
    if (!done) done = 1; else return;
    
    static Class c;
    memset(&c, 0, sizeof(c));

    c.name = "binlist_addonly";
    c.class_type = CONTAINER_CLASS;

    c.n_group_by_indexes = 1;
    c.group_by_requires_rule = 1;  // was 0

    c.get_distinct_info = &get_distinct_info;

    c.create_op_data = &create_op_data;
    c.destroy_op_data = &destroy_op_data;

#if CLASS_PARSE
    c.n_fields = 1;
    c.n_params = 1;
    c.parse_param = _parse_param;
#endif

    register_class(&c);
    register_content_binlist(&c);
    register_ops();
}


