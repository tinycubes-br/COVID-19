#include <stdlib.h>

#include "stats.h"
#include "schema.h"

typedef struct s_counter {
    Header rc;    
    int counter;
    struct s_counter * shared_edges_first;
} Counter, *PCounter;

//-----------------------
//
//
//
//
void *alloc(void) {
    return zcalloc(sizeof(Counter),1);
}

static int my_type = -1;

//-----------------------
//
//
//
//
static void _destructor(void * pn) {
    #ifdef __STATS__
        stats.n_nodes --;
        stats.n_terminals --;
        if (HEADER_GET_BETA(pn)) {
            stats.n_betas --;
            stats.n_beta_terminals --;
        }
    #endif
    LOGIT_1("TERMINAL COUNT: DESTRUCTING %03d",map_ptr(pn));
    #ifdef __LOGIT
    map_free_ptr(pn);
    #endif
    free(pn);
}


//=============================================================================

//-----------------------
//
//
//
//
static PNode _create_node(int beta) {

    PCounter pc = (PCounter) alloc();

    if (my_type == -1) {
        my_type = header_register_type(_destructor);
    }

    header_init(pc, beta, my_type);
    HEADER_SET_TERMINAL(pc,1);

    #ifdef __STATS__
        stats.n_nodes ++;
        stats.n_terminals ++;
        if (beta) {
            stats.n_betas ++;
            stats.n_beta_terminals ++;
        }
    #endif

    return pc;
}

//-----------------------
//
//
//
//
static int _delete_node(PNode pn) {
    header_deleted(pn, TRUE);

    return 1;
}

//-----------------------
//
//
//
//
static PNode _shallow_copy(PNode pn, int beta, struct s_node_api *pf) {
    LOGIT_1("TERMINAL: SHALLOW COPY %03d", map_ptr(pn));

    PCounter pc = _create_node(beta);

    // copia o content (contagem) para o novo node
    pc->counter = ((PCounter) pn)->counter;
        
    return (PNode) pc;
}



//=============================================================================

//-----------------------
//
//
//
//
static PNode _get_content_info(PNode pn, int *shared) {
    if (shared) *shared = 0;
    LOGIT("TERMINAL: ABNORMAL CALL TO GET CONTENT INFO FOR TERMINAL");
    return pn;
}

//-----------------------
//
//
//
//
static PNode _set_content_info(PNode pn, PNode pcontent, int beta, int shared, int ignore) { //zstruct s_node_api *pf) {
    // Nao faz testes para ver se está "resetando" o content info 
    // por achar que nao sera chamada duas vezes para um terminal
    // porem nao esta provado
    LOGIT("TERMINAL: ABNORMAL CALL TO SET CONTENT INFO FOR TERMINAL");
    
    header_adjust((PHeader) pn, beta, shared,1);
    return pn;    
}

//=============================================================================

//-----------------------
//
//
//
//
static int _insert_object(PNode pn, PObject o) {
    PCounter pc = (PCounter) pn;
    pc->counter ++;
    return pc->counter;
}

//-----------------------
//
//
//
//
static void _remove_object(PNode pn, PObject o) {
    PCounter pc = (PCounter) pn;
    
	if (pc->counter > 0) pc->counter --;
}


//-----------------------
//
//
//
//
static int _get_content_data(PNode pn, int *data) {
    *data = ((PCounter) pn)->counter;
    return 1;
}


//-----------------------
//
//
//
//
static int _is_empty(PNode pn) {
	return ((PCounter) pn)->counter == 0;
}

//=============================================================================

//-----------------------
//
//
//
//
void terminal_count_init_functions(TerminalFunctions *tfs) {
    tfs->create_node = _create_node;
	tfs->delete_node = _delete_node;
	
    tfs->get_content_info = _get_content_info;
    tfs->set_content_info = _set_content_info;

    tfs->shallow_copy = _shallow_copy;
    
    tfs->insert_object = _insert_object;
	tfs->remove_object = _remove_object;
	
    tfs->get_content_data = _get_content_data;
	tfs->is_empty = _is_empty;
}