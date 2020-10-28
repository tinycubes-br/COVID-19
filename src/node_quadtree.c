#include <stdlib.h>
#include "schema.h"

//=======================================================================================
//
//
//
//=======================================================================================

typedef struct s_vertix {
    HEADER_INFO;
    struct {
        unsigned int  shared_content;
        unsigned int  beta_content;

        unsigned int deleted:4;
        unsigned int shared:4; 
    } info;
    PNode   content;
    PNode   nodes[4];
} Vertix, *PVertix; 

typedef struct s_arc {
    PNode pv;
    int   position;
} Arc, *PArc;

static int my_type = -1;

//-----------------------
//
//
//
//
static void _destructor(void * pn) {
    free(pn);
}

//-----------------------
//
//
//
//
static PNode _create_node(int beta) {
    PVertix pv = (PVertix) zcalloc(sizeof(Vertix),1);     
    
    #ifdef __STATS__
        stats.n_nodes++;
        if (beta) {
            stats.n_betas ++;
        }
    #endif   

    if (my_type == -1) {
        my_type = header_register_type(_destructor);
    }
    header_init(pv, beta, my_type);
    return (PNode) pv;
}

//-----------------------
//
//
//
//
static int _delete_node(PNode pn) {
    return header_deleted(pn, TRUE);
}

#define CONTENT_NODE 0
#define CHILD_NODE   1

#define CONTENT_EDGE 0 
#define CHILD_EDGE   1


//-----------------------
//
// Adiciona um edge CHILD entre os nodes <pn> e <pchild> com valor <v>
// e <shared> ou proper
//
static PNode _add_child_edge(PNode pn, PNode pchild, int shared, Value v) {

    if (v > 3) fatal("Quadtree: value out of range");

    #ifdef __STATS__
        if (shared) { 
            stats.n_shared_child_edges ++; 
        } else {
            stats.n_proper_child_edges ++;
        }
    #endif

    pv->info.shared |= shared << v;
    pv->nodes[v] = pchild;

    // incrementa a contagem de referencias
    header_adjust(pchild, CHILD, shared, 1);
    
    return pchild;
}

//-----------------------
//
// Remove o node indicado por <pb>, fazendo <pb> apontar para o primeiro node de <pn>
// e retornado o <child> associado ao node.
//
//
static PNode _remove_child_edge (PNode pn, Branch *pb, int *shared) {
    PVertix pv = (PVertix) pn;
    if (!pb) fatal("pb");

    int v = ((PArc) pb)->position;
    PNode pnc = pv->nodes[v];

    LOGIT_2("NODE_QUAD: REMOVE CHILD EDGE %03d RefCount %d",map_ptr(pnc),HEADER_COUNT(pnc)-1);
    
    // ajusta as variaveis de edge do node
    int must_free = header_adjust(pnc, TRUE, pa->shared, -1);
    if (must_free) {
        LOGIT_2("NODE_STD: MUST FREE NODE %03d RefCount %d",map_ptr(pnc),HEADER_COUNT(pnc));
    }           

    #ifdef __STATS__
        if (*shared) { 
            stats.n_shared_child_edges --; 
        } else {
            stats.n_proper_child_edges --;
        }
    #endif


    // ajusta o branch para apontar para o primeiro node
    pa->pv = pv;
    pa->v = 4;
    pnc = NULL;

    for (v=0;v<4;v++) { 
        if (!pv->nodes[v]) continue;
        pnc = pv->nodes[v];
        pa->v = v;
        break;
    }

    return pnc;
}

//-----------------------
//
//
//
//
static PNode _shallow_copy(PNode pn, int beta) {
    PVertix pv = (PVertix) pn;
    PVertix pnew = (PVertix) _create_node(beta);
    int v;
    
    // cria novos arcos shared apontando para os nodes já existentes
    for (v=0; v<4; v++) {
        PNode pnc = pv->nodes[v];
        // esta assumindo que o a funcao que adicona o filho é do mesmo tipo
        // também é quadtree  !!!
        if (pnc) _add_child_edge(pnew, pnc, SHARED, v); 
    }
    
    // Compartilha o content com o novo node
    pnew->content = pv->content;
    pnew->info.shared_content = 1;
    pnew->info.beta_content = pv->info.beta_content;

    // incrementa o contador de uso do content
    header_adjust(pv->content, CONTENT, TRUE, 1);

    return pnew;
}

//-----------------------
//
//
//
//
static PNode _replace_child_on_branch(Branch *branch, PNode pchild, int is_shared, PNodeFunctions pf) {
    PArc pa = (PArc) branch;
    int v = pa->position;
    PVertix pv = pa->pv;
    PNode pnc = pv->nodes[v];
    PNode next_node = pchild? pchild: pf->shallow_copy(pnc, FALSE);    
    pv->info.shared = is_shared;
    
    header_adjust(pnc, CHILD, SHARED, -1);
    pnc = pv->nodes[v] = next_node;
    header_adjust(pnc, CHILD, is_shared, 1);

#ifdef __STATS__
    if (!shared) {
        stats.n_shared_child_edges --;
        stats.n_proper_child_edges ++;
    }
#endif

    return pnc;
}



//-----------------------
//
//
//
//
static PNode _locate_child(PNode pn, Value v,int *shared, Branch *pb, 
                           struct s_node_api *pf) {
    PVertix pv = (PVertix) pn;

    PNode pnc = pv->nodes[v];
    if (!pnc) return NULL;

    int shared_mask = (1 << v);

    // ajusta vertices sem dono
    if ((pv->info.shared & shared_mask) && header_make_proper_child(pnc) ) {
        LOGIT("LOCATE CHILD: MAKED PROPER CHILD");
        LOGIT_2("SHARED->PROPER   Node:%03d -> Child:%03d",map_ptr(pn),map_ptr(pnc));
        pv->info.shared &= ~shared_mask;

        #ifdef __STATS__
            stats.n_shared_child_edges --;
            stats.n_proper_child_edges ++;
        #endif
    }
    // fim NOVO

    // retorna a informacao
    *shared = pv->info.shared & shared_mask;
    if (pb) {
        PArc pa = (PArc) pb;

        pa->pv = pv;
        pb->position = v;
    }
    
    return pnc;
}


//-----------------------
//
//
//
//
void _remove_content_edge (PNode pn, PNodeFunctions pf) {
    PVertix pv = (PVertix) pn;

    // deveria limpar o campo content????
    pv->content = NULL; //?????????

    // ajusta as variaveis de edge do node
    header_adjust(pv->content, CONTENT, FALSE, -1);
}

//-----------------------
// 
// returns
// 0 -> No children
// 1 -> One child
// 2 -> Two or more children
//
static int _get_children_class(PNode pn) {
    PVertix pv = (PVertix) pn;
    int v, c;
    for (v = 0, c=0; v < 4; v++) {
        if (pv->nodes[v]) c++;
    }
    if (c > 2) c = 2;
    return c;
}


//-----------------------
//
//
//
//
static PNode _get_content_info(PNode pn, int *shared) {
    PVertix pv = (PVertix) pn;
    
    if (shared) *shared = pv->info.shared_content;
    return pv->content;
}

//-----------------------
//
//
//
//
static PNode _set_content_info(PNode pn, PNode pcontent, int beta, int is_shared, int ignore) { //, struct s_node_api *pf) {
    PVertix pv = (PVertix) pn;

    // It didn't change anything? Just return
    if (pv->content == pcontent) {
        LOGIT_1("SET CONTENT INFO: NO CHANGES %p",pcontent);
        return pcontent;
    }

    // se existe content, entao decrementa a contagem para ele
    if (!ignore && pv->content /* && pv->content != pcontent */) {
        LOGIT_DECL(int rc = HEADER_COUNT(pv->content) - 1);
        LOGIT_2("ADJUST %03d. RC = %d",map_ptr(pv->content), rc);

        header_adjust(pv->content, CONTENT, pv->shared_content, -1);
    }


    pv->info.shared_content = is_shared;
    pv->content = pcontent;
    pv->info.beta_content = beta;

    // se existe um novo content, entao incrementa a contagem
    if (pcontent) {
        header_adjust(pcontent, CONTENT, is_shared, 1);
    }
    
    return pcontent;    
}

//-----------------------
//
//
//
//
static PNode _get_child(PNode pn, Value *v, int *shared, Branch *ref, int change_proper) {
    PVertix pv = (PVertix) pn;
    PArc pa = (PArc) ref;
    int idx;
    PNode pnc;

    // primeira chamada
    if (pn) {
        for (idx = 0; idx<4; idx++) if (pv->nodes[idx]) break;
        if (idx == 4) return NULL;

        pnc = pv->nodes[idx];
        if (v) *v = idx;
        int mask = 1 << idx;
        if (change_proper && header_make_proper_child(pnc)) {
            LOGIT("GET CHILD: MAKED PROPER CHILD 1");
            pv->info.shared &= ~mask;
        }
        if (shared) *shared = pv->info.shared;
        if (ref) { pa->pv = pv; pa->position = idx; }
        return pnc;

    // chamadas seguintes
    } else if (ref) {
        for (idx = pa->position+1; idx<4; idx++) if (pv->nodes[idx]) break;
        if (idx == 4) return NULL;

        pnc = pv->nodes[idx];
        if (v) *v = idx;
        int mask = 1 << idx;
        if (change_proper && header_make_proper_child(pnc)) {
            LOGIT("GET CHILD: MAKED PROPER CHILD 2");
            pv->info.shared &= ~mask;
        }
        if (shared) *shared = pv->info.shared;
        if (ref) { pa->pv = pv; pa->position = idx; }
        return pnc;
    } else {
        return NULL;
    }
}

//-----------------------
//
//
//
//
static int _has_single_child (PNode pn) {
    return _get_children_class(pn) == 2;
}


//-----------------------
//
//
//
//
void node_quadtree_init_functions(NodeFunctions *pfs) {
    pfs->create_node = _create_node;
    pfs->delete_node = _delete_node;
    
    pfs->add_child_edge = _add_child_edge;
    pfs->remove_child_edge = _remove_child_edge;
    
    pfs->get_children_class = _get_children_class;
    
    pfs->locate_child = _locate_child;
    pfs->shallow_copy = _shallow_copy;
    pfs->replace_child_on_branch = _replace_child_on_branch;
    pfs->get_child = _get_child;

    pfs->get_content_info = _get_content_info;
    pfs->set_content_info = _set_content_info;

    pfs->has_single_child = _has_single_child;
    
}
