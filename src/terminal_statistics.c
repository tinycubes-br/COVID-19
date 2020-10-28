#include <math.h>
#include "metadata.h"

typedef struct {
    long   count;
    double sum;
    double sum_x2;
} Variance, *PVariance;

typedef struct {
    char *name;
    int   data_size;
    int   n_channels;
    int   initialize;
} TerminalModule, *PTerminalModule;

//*****************************************
//    RECORD FIELD TO ADDRESS
//*****************************************

void terminal_variance_register(void) {

}

#define TERMINAL_INSERT       1
#define TERMINAL_REMOVE       2

//---------------
//
//
//
void terminal_variance_op(int op,unsigned char *record, PRecordInfo pri, int *ids, void *data) {
    PVariance pv = data;

    PRecordField prf = pri->fields + ids[0];
    double v =  * (double *) (record + prf->offset);

     if (op == TERMINAL_INSERT) {
        pv->sum += v;
        pv->sum_x2 += v * v;
        pv->count ++;
    } else if (op == TERMINAL_REMOVE) {
        pv->sum -= v;
        pv->sum_x2 -= v * v;
        pv->count --;
    }
}

//---------------
//
//
//
int terminal_variance_get(void *data, int channel, double *result) {
    PVariance pv = data;

    switch (channel) {
        // variance
        case 0: 
            *result = (pv->count > 0) ? (pv->sum_x2 - pv->sum) / pv->count : 0;
            break;

        // count
        case 1: 
            *result = pv->count;
            break;

        // sum of x
        case 2: 
            *result = pv->sum;
            break;

        // std deviation
        case 3: 
            *result = sqrt((pv->count > 0) ? (pv->sum_x2 - pv->sum) / pv->count : 0);
            break;
    }
}

//---------------
//
//
//
void terminal_variance_getall(void *data, double *results) {
    PVariance pv = data;

    // variance
    *results = (pv->count > 0) ? (pv->sum_x2 - pv->sum) / pv->count : 0;
    results ++;

    // count
    *results = pv->count;
    results ++;

    // sum
    *results = pv->sum;
    results ++;

    // std dev
    *results = sqrt((pv->count > 0) ? (pv->sum_x2 - pv->sum) / pv->count : 0);
    results ++;
}

//---------------
// merge_begin
// merge_data
// merge_end
