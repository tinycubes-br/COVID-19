#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "stats.h"
#include "schema.h"
#include "terminal.h"
#include "node_std.h"

#include "logit.h"
#include "nodeset.h"

#include "dump1.h"
#include "webserver_mg.h"
#include "query_processor.h"
#include "webserver_mg.h"

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

#define SCHEMA_1_1 "schema_mestrado_1_1.json"
#define SCHEMA_2_2 "schema_mestrado_2_2.json"
#define SCHEMA_2_2_2 "schema_mestrado_2_2_2.json"
#define SCHEMA_8_1 "schema_mestrado_8_1.json"
#define SCHEMA_8_2 "schema_mestrado_8_2.json"

#define SCHEMA_RNP_1 "schema_rnp_1.json"
#define SCHEMA_TAXI_1  "schema_taxis_1.json"  // sem tempo
#define SCHEMA_TAXI_2  "schema_taxis_2.json"  // com tempo
#define SCHEMA_TAXI_3  "schema_taxis_3.json"  // mais alturas


#define SCHEMA_FILE SCHEMA_2_2

static char schema_file[1000];

long int count_rows = 0;
long int max_rows = 0;
long int del_rows = -1;

#if 1

void str_addr(PSchema ps, char *str, Values address) {
    int  i;
    for (i=0;i<ps->n_values;i++) {
        if (*str && (*str>='A' && *str<='Z')) {
            address[i] = *str - 64;
            str++;
        } else {
            address[i] = '-';
        }
    }    
}


//========================================================================

static PMetaData pmd;
static PSchema   ps;
static PAddr     pa;
static PNode     root;
static void *    record;



/**
 * 
 * 
 * 
 */ 
_PRIVATE(void) initialize(void) {
    printf("Initializing... ");
    map_reset();

    count_rows = 0;

    pmd = metadata_create();
    metadata_load(pmd, schema_file);

    ps = metadata_create_schema(pmd);
    terminal_set_schema(ps);

    root = schema_create_root(ps);
    
    pa = metadata_create_address(pmd);

    record = metadata_create_record(pmd);

#if 0
    printf("======================\n");
    int i;
    for (i = 0;i<ps->n_dims+1;i++){
        printf("%02d: n_values_of_dim[] = %2d, top_of_dim_level[] = %02d\n",i, ps->n_values_of_dim[i], ps->top_of_dim[i]);
    }
    for (i = 0; i<ps->n_values+1; i++) {
        printf("%02d: dim_of_value[] = %2d\n",i, ps->dim_of_value[i]);

    }
#endif    
    printf("Done\n");
}


/**
 * 
 * 
 * 
 * 
 */ 
void terminate(void) {
    metadata_release_record(record);
    metadata_release_address(pa);

    metadata_release_schema(ps);
    metadata_release(pmd);

    map_reset();
    
//    pointers_cleanup();
}


void make_title(char *title, char *s) {
    char buf[10];
    char *sep = "";
    *title = 0;
    sprintf(title, "%c( ",*s);
    s++;
    while (*s) {
        int v = *s - 65 + 1;    
        sprintf(buf,"%s%d",sep,v);
        strcat(title, buf);
        sep = ", ";
        s++;
    }
    strcat(title, ")");
}

/**
 * 
 * 
 * 
 * 
 **/ 
int process_it(PSchema ps, PNode root, PAddr address, char *prefix, 
                int i, char *line, char *title, int dump_it, int sum_check, int quiet) {
    char buf[1000];
    
    int remove = 0;
    
    if (dump_it) {
        make_title(title, line);
	    /// strcat(title,line); strcat(title,", ");
    }

    if (line[0] == '-') {
        remove = 1;
        line ++;
    } else if (line[0] == '+') {
        line ++;
    } 
    
    metadata_process_line(pmd, record, line, FALSE);
    metadata_record_to_address(pmd, record, address);
    
    if (remove) {
		LOGIT_2("REMOVE[%06d]: %s",i,line); 
	    tinycubes_remove(ps,root,address->values,record);
    } else {
		LOGIT_2("INSERT[%06d]: %s",i,line); 
		tinycubes_insert(ps,root,address->values,record);
    }
 
    int err = 0;    
    if (!quiet && (i % 100 == 0)) {
        fprintf(stderr,"%s",i%1000?".":"!");
    }

    if (sum_check && (i % sum_check == 0)) {
        ps->root = root;
        int  UNUSED_VAR(sum) = schema_compute_sum(ps,0,root, &err); 
        if (!quiet) fprintf(stderr,"#");

        LOGIT_1("Sum = %d",sum);
    }
    
    if (err) {
        printf("*** %2d %s [%s]\n",i, remove?"removing":"inserting",line);
    }

    if (!dump_it) {
        LOGIT("NO DUMP");
    } else {
        printf("*** DUMP ***");
    	sprintf(buf,"%s_%02d.dot",prefix,i);
        dump(ps,root,buf,title,0,0);

        LOGIT("AFTER DUMP");
        
        // gera o PDF
        sprintf(buf,"dot -Tpdf %s_%02d.dot -o %s_%02d.pdf -Gsize=320,240 -Gdpi=72",
                prefix,i, prefix,i);
                //printf("%s\n",buf)
        UNUSED_RESULT(system(buf));

        // gera o png
        sprintf(buf,"dot -Tpng %s_%02d.dot -o %s_%02d.png -Gsize=1024,768 -Gdpi=144",
                prefix,i, prefix,i);
                //printf("%s\n",buf)
        UNUSED_RESULT(system(buf));
    }    
    LOGIT("\n-------------------------------------------------");
    return err;
}



static int dirExists(const char *path)
{
    struct stat info;

    if(stat( path, &info ) != 0)
        return 0;
    else if(info.st_mode & __S_IFDIR)
        return 1;
    else
        return 0;
}

static void create_dir(const char *path) {

    if(!dirExists(path))
    {
        mode_t mask = umask(0);
        if(mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
            fatal("Error creating directory [%s]\n",path);
        }
        umask(mask);
    }

}

static  char prefix[1000];
static int tot_time_ns;
static int tot_mem_kb;

void load_test_file(char *fname, int sum_check, int quiet) {
    FILE *f;
    PNode root = NULL;

    struct rusage r_usage, r_usage2;
    clock_t start, stop;

    stats_reset();

    getrusage(RUSAGE_SELF,&r_usage);
    start = clock();

    f = fopen(fname,"rt");
    if (!f) {
        fatal("Error loading test file %s",fname);
    }

    char dir[1000];
    char subdir[1000];
    UNUSED_RESULT(getcwd(dir,sizeof(dir)));
    
    char line[1024];
    char title[1000];
    char filename[400];

    title[0] = 0;
    int i = 1;
    int plot = 0;
    int dump_all = 0;
    
    logit_set_filename("log.txt");
    
    fprintf(stdout, "File: %s\n", fname);
//    fprintf(stdout, "Start: Xallocations = %d  Xmemory = %ld \n", 
//        zalloc_n_alloc(), zalloc_total_allocated());
    fflush(stdout);

    while (fgets(line,1000,f) != NULL) {
        char *s = line;
        int n = strlen(line);

        if (n>0 && (line[n-1] == '\n')) {
            line[n-1] = 0;
            n --;
        }

        if (*s == '#') continue;

        if (*s == '>') {
            s ++;
            n--;
            dump_all = 1;
        }

        if (*s == '<') {
            s ++;
            n--;
            dump_all = 0;
        }

        int dump_it = dump_all;
        if (*s == '!') {
            s ++;
            n--;
            dump_it = 1;
        }
        
        if ((*s == '=') || (*s == '*')) {
            plot = (*s == '=');
            title[0] = 0;
            dump_all = 0;

            // don't reuse to avoi mixing structures in case of 
            // existence of "remains" from previous execution
            if (root) schema_release_root(root);

            root = schema_create_root(ps);
            
            strcpy(prefix, s+2);

            sprintf(subdir,"%s/logs/%s/",dir,prefix);
            create_dir(subdir);
            strcat(subdir,prefix);

            sprintf(filename,"%s/logs/%s.log",dir,prefix);
            //puts(filename);
            logit_set_filename(filename);
            
            i = 1;
            
        } else if ((*s == '+') || (*s == '-')) {
            //puts(subdir);
            if (!root) {
                fatal("Load Test File: Missing root initialization.");
            }
            if (process_it(ps, root, pa, subdir, i, s, title, 
                            plot || dump_it, sum_check, quiet))
                return;
            i ++;

            
        }
    }
    if (root) {
        schema_release_root(root);
    }

    node_std_cleanup();

    stop = clock();
    double result = timediff(start, stop);
    getrusage(RUSAGE_SELF,&r_usage2);
    printf("\nTime: %.f ms   Mem Before = %ld KB  Mem After=%ld KB\n",
        result, r_usage.ru_maxrss, r_usage2.ru_maxrss);
        
        tot_mem_kb = r_usage2.ru_maxrss - r_usage.ru_maxrss;
        tot_time_ns = result * 1000;


    fprintf(stdout, "\n");
//    fprintf(stdout, "End: Xallocations = %d  Xmemory = %ld \n", 
//        zalloc_n_alloc(), zalloc_total_allocated());
    fflush(stdout);

    fclose(f);    
}


#include "teste_block.h"

//=========================================================================================

/**
 * @brief 
 * 
 */
typedef struct {
    char path[500]; //!< documentation 
    char fname[500];
    char what; 
    int  n_rep;
    int  run_sz;
    int  sum_check;
    int  show_options;
    int  first_run; // -i
    int  only_insertions; 
    int  quiet;
    int  range;
} Options;

/**
 * @brief 
 * 
 */
static void show_usage(void) {
    fprintf(stderr,"Usage: tc what options\n");
    fprintf(stderr,"  what:\n");
    fprintf(stderr,"    -f <fname> run test with specified test file.\n");
    fprintf(stderr,"    -a [num] automatic generation of <num> test file(s). See options below\n");
    fprintf(stderr,"  options:\n");
    fprintf(stderr,"    -I performs only INSERTIONS (don't remove) \n");
    fprintf(stderr,"    -c <num> check integrity at each <num> samples \n");
    fprintf(stderr,"    -i <num>: insert <num> nodes before first remove.\n");
    fprintf(stderr,"    -n <num>: number of insertions in each run (-a): 1 .. %d \n",MAX_BLOCKS);
    fprintf(stderr,"    -o: show option values \n");
    fprintf(stderr,"    -r <num> number of repetitions \n");
    fprintf(stderr,"    -d <num> Distinct  values/letters (max 25) for level\n");
    fprintf(stderr,"    -p <path> path/prefix to store results\n");
    fprintf(stderr,"    -s <schema_file> specify the schema file\n");
    fprintf(stderr,"    -M maximum number of rows to test\n");
    fprintf(stderr,"    -R maximum number of rows to remove\n");

    exit(1);
}

/**
 * @brief 
 * 
 * @param s 
 * @param num 
 * @param max 
 * @param optional 
 * @return int 
 */


static int parse_number(char *s, int *num, int min, int max, int optional) {
    *num = atoi(s);
    if (*num < min) {
        if (!optional) fatal("Invalid number: %s\n", s);
        return 0;
    } 
    if (*num > max) {
        fatal("limit exceeded %s > %d\n",s, max);
    }
    return 1;
}

static int parse_long(char *s, long *num, long min, long max, int optional) {
    *num = atoi(s);
    if (*num < min) {
        if (!optional) fatal("Invalid number: %s\n", s);
        return 0;
    } 
    if (*num > max) {
        fatal("limit exceeded %s > %d\n",s, max);
    }
    return 1;
}


/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @param po 
 */
static void parse_options(int argc, char *argv[], Options *po, int no_fail) {

    if (argc == 1) show_usage();

    po->run_sz = 1000;
    po->n_rep = 1;
    po->what = 0;
    po->sum_check = 0;
    po->fname[0] = '\0';
    po->show_options = 0;
    po->first_run = 0;
    po->quiet = 0;
    po->only_insertions = 0;
    po->range = 5;

    strcpy(po->path,argv[0]);
    *prefix = 0;

    char *o;
    int idx = 1;
    while (idx < argc) {
        o = argv[idx];
        idx ++;
        if (strcmp(o,"-a") == 0) {
            po->what = 'a';
            if (idx < argc) {
                if (parse_number(argv[idx], &po->n_rep, 1, 100, 1)) {
                    idx ++;
                }
            }

        } else if (strcmp(o,"-f") == 0) {
            po->what = 'f';
            if (idx <= argc) {
                strcpy(po->fname, argv[idx]);
                idx ++;
            } else {
                fatal("Missing filename.");
            }

        } else if (strcmp(o,"-p") == 0) {
            if (idx <= argc) {
                strcpy(prefix, argv[idx]);
                idx ++;
            } else {
                fatal("Missing filename.");
            }

        } else if (strcmp(o,"-s") == 0) {
            if (idx <= argc) {
                strcpy(schema_file, argv[idx]);
                idx ++;
            } else {
                fatal("Missing filename.");
            }

        } else if (strcmp(o,"-n")==0) {
            if (idx < argc) {
                if (po->first_run) {
                    fatal("First run must be settled after -n");
                }
                parse_number(argv[idx], &po->run_sz, 1, MAX_BLOCKS, 0);
                idx ++;
            }

        } else if (strcmp(o,"-r")==0) {
            if (idx < argc) {
                parse_number(argv[idx], &po->n_rep, 1, 100, 0);
                idx ++;
            }

        } else if (strcmp(o,"-M")==0) {
            if (idx < argc) {
                parse_long(argv[idx], &max_rows, 1, 1000000000000L, 0);
                idx ++;
            }

        } else if (strcmp(o,"-R")==0) {
            if (idx < argc) {
                parse_long(argv[idx], &del_rows, 1, 1000000000000L, 0);
                idx ++;
            }

        } else if (strcmp(o,"-c")==0) {
            if (idx < argc) {
                parse_number(argv[idx], &po->sum_check, 0, 10000, 0);
                idx ++;
            }

        } else if (strcmp(o,"-i")==0) {
            if (idx < argc) {
                parse_number(argv[idx], &po->first_run, 1, po->run_sz, 0);
                idx ++;
            }

        } else if (strcmp(o,"-d")==0) {
            if (idx < argc) {
                parse_number(argv[idx], &po->range, 1, 25, 0);
                idx ++;
            }

        } else if (strcmp(o,"-q")==0) {
            po->quiet = 1;

        } else if (strcmp(o,"-I")==0) {
            po->only_insertions = 1;

        } else if (strcmp(o,"-o")==0) {
            po->show_options = 1;

        } else if (!no_fail) {
            show_usage();
        }
    }

}

//=========================================================================================

#include "metadata.h"


void dump_stats(char *fname) {
    char outname[1000];
    FILE *fout;

    strcpy(outname, fname);
    strcat(outname, ".log");
    fout = fopen(outname,"wt");
    stats_dump_to_file(fout);
    fclose(fout);

    strcpy(outname, schema_file);
    strcat(outname, ".log");
    printf("%s\n",outname);
    fout = fopen(outname,"at");
    stats_line_to_file(fout, tot_mem_kb, tot_time_ns);
    fclose(fout);

    stats_dump_to_file(stdout);

}

int test01(int argc, char *argv[]) {    
    Options opt;

    
    parse_options(argc, argv, &opt, 0);
    if (opt.show_options) {
        printf("Run size = %d\n",opt.run_sz);
        printf("Filename: %s\n",opt.fname);
    }

    initialize_randomizer();
    initialize();

    if (opt.what == 'a') {
        chdir("./1_tests");

        int n = opt.n_rep;

        while (n) {
            fprintf(stderr,"------------------------\n");
            
            int address_size = ps->n_values;
            char *fname = create_test_sequence("test",address_size,
                opt.run_sz,opt.first_run, opt.only_insertions, opt.range);
            if (fname) {
                printf("Autotest: %s\n",fname);
                load_test_file(fname, opt.sum_check, opt.quiet);
                dump_stats(fname);
            }
            n--;
        }

    } else if (opt.what == 'f') {
        int n = opt.n_rep;
        while (n) {
            fprintf(stderr,"------------------------\n");
            load_test_file(opt.fname, opt.sum_check, opt.quiet);
            dump_stats(opt.fname);
            n--;
        }
    }

    node_std_cleanup();

    terminate();

    
    return 0;
        
}
#endif

/**
 * 
 * 
 * 
 */ 
_PRIVATE(int) load_textfile_to_memory(const char *filename, char **result) { 
	int size = 0;
	FILE *f = fopen(filename, "rt");
	if (f == NULL) 	{ 
		*result = NULL;
		return -1; // -1 means file opening fail 
	} 
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	*result = (char *)malloc(size+1);
	if (size != fread(*result, sizeof(char), size, f)) 
	{ 
		free(*result);
		return -2; // -2 means file reading fail 
	} 
	fclose(f);
	(*result)[size] = 0;
	return size;
}

// EXTERNAL PROTOTYPES
void register_ops(void);


//-----------------------
//
//
//
//
void init_tinycubes(char *fname, PMetaData *p_pmd, PSchema *p_ps,PAddr *p_pa) {
    map_reset();

    PMetaData pmd = metadata_create();
    metadata_load(pmd, SCHEMA_FILE);
    *p_pmd = pmd;

    PSchema ps = metadata_create_schema(pmd);
    terminal_set_schema(ps);
    *p_ps = ps;
    
    PNode root = ps->nfs[0].create_node(0);
    ps->root = root;

    PAddr pa = metadata_create_address(pmd);
    *p_pa = pa;

    void *record = metadata_create_record(pmd);

    printf("Loading initial data ..."); fflush(stdout);

    char prefix[] = "1_tests/teste_1";
    char suffix[] = ".txt";
    char buf[1024];

    int i;
    for (i = 0; i < 1; i++) {
        sprintf(buf, "%s%s",prefix,suffix);
        printf("[%s]",buf);  fflush(stdout);
        metadata_open_input_file(pmd, buf);

#if 0
        int c = 0;
        //metadata_open_input_file(pmd, "crime50k_000001.txt");
    //    metadata_open_input_file(pmd, "../rnp/MXRJ/20190701_b.txt");
    //    metadata_open_input_file(pmd, "../data/taxis.txt");

        clock_t start, stop;
        start = clock();
        struct rusage r_usage, r_usage2;

        getrusage(RUSAGE_SELF,&r_usage);
#endif
#define DISCARD 0
        int discard = 0;
        while (metadata_read(pmd, record)) {
            if (discard) {
                discard --;
                continue;
            } else {
                discard = DISCARD;
            }

#if 0
            printf("read:\n");
            metadata_dump_record(pmd, record);

            printf("Address: ");
            for (int i = 0; i<pa->n_values; i++) {
                printf("%d ", pa->values[i]);
            }
            printf("\n");
            printf("Root = %p\n", root);
#endif
            metadata_record_to_address(pmd, record, pa);

            tinycubes_insert(ps, root, pa->values, record);
#if 0
            c ++;
            if (c % 10000 == 0) {
                if (c % 100000 == 0) {
                    stop = clock();
                    double result = timediff(start, stop);
                    printf("[ %d K: %.2f s ]",c/1000, result/1000.);
                } else {
                    printf(".");
                }
                fflush(stdout);
            }
    #endif        
        }
        metadata_close_input_file(pmd);
    }
    printf(" done.\n"); fflush(stdout);

#if 0
    stop = clock();

    double result = timediff(start, stop);

    getrusage(RUSAGE_SELF,&r_usage2);
    printf("\nTime: %.f ms   Mem  = %ld KB  KB\n",
        result, r_usage2.ru_maxrss - r_usage.ru_maxrss );

    stats_dump();
#endif
}

//-----------------------
//
//
//
//
void term_tinycubes(PMetaData pmd, PAddr pa) {
    metadata_close_input_file(pmd);

    metadata_release_address(pa);

    metadata_release(pmd);

#if 0
extern int max_blist_n_nodes;
extern int total_n_nodes;
    printf("MAX N: %d\n",max_blist_n_nodes);
    printf("TOTAL N: %d\n",total_n_nodes);
extern int hist[25001];

    for (int i = 25000; i>0; i--) {
        for (int j = 0; j < i; j++) {
            hist[j] -= hist[i];
        }
    }    
#if 0
    #define X  8
    int n_32 = 0;
    for (int i = X; i<25000; i++) {
        n_32 += hist[i] * (i / X);
        hist[i%X] += hist[i] * (i%X);
        hist[i] = 0;
    }
    printf("N %d: %d  Save Bytes: %d \n",X,n_32, 8 * (X - 1) * n_32 );
#endif

    for (int i = 0; i<25001; i++) {
        if (hist[i]) printf("%05d => %d\n", i, hist[i]);
    }
#endif     
    fflush(stdout);
    // iserver_run();
}

void test_metadata(int argc, char *argv[]) {
    PMetaData pmd;
    PAddr pa;
    PSchema ps;
    
    init_tinycubes("../rnp/BR_20190806_14_30.csv", &pmd, &ps, &pa);
    
#if 1    
    char *src;
    load_textfile_to_memory("query.json", &src);
    PQuery pq = query_begin();
    if (!setjmp(query_jmp)) {
        query_process(pq, src, ps, pmd);
        query_out_as_json(pq,printf);
    }
    query_end(pq);
#endif

    term_tinycubes(pmd, pa);
}

static char files_prefix[1000];
static char files_suffix[1000];


//-----------------------
//
// Versão para protótipo
//
//
static int std_run_file(char *fname,int remove) {
    printf("File: %s\n",fname);
    metadata_open_input_file(pmd, fname);

    while (metadata_read(pmd, record)) {

        if (max_rows && (count_rows >=max_rows)) break;
        count_rows ++;

        //metadata_dump_record(pmd, record);
        
        metadata_record_to_address(pmd, record, pa);
        //int i;for (i=0;i<ps->n_values;i++) {printf("%d,",pa->values[i]);}printf("\n"); fflush(stdout);

        //printf("[insert]");fflush(stdout);
        //printf("[read]");fflush(stdout);

        if (remove) { 
            //printf("remove\n");
            tinycubes_remove(ps, root, pa->values, record);
        } else {
            //printf("insert\n");
            tinycubes_insert(ps, root, pa->values, record);
        }

        //stop = clock();
        //double diff = timediff(start,stop);
    }
    metadata_close_input_file(pmd);
    return 1;
}

static char files_fmt[200] = "%s%02d%s";

//-----------------------
//
// 
//
//
static int std_run_step(int base, int n,int remove,struct rusage*r, clock_t start0) {
    struct rusage r_usage2;
    clock_t start, stop;

    //start0 = clock();

    static char buf[1024];

    int id;

    for (id=0; id < n; id++) {
        if (n==1) {
            sprintf(buf, "%s%s",files_prefix, files_suffix);
        } else {
            sprintf(buf, files_fmt,files_prefix, base+id+1, files_suffix);
        }

        start = clock();

        std_run_file(buf, remove);
        
        stats_dump();

        stop = clock();
        double result = timediff(start, stop);
        double result_total = timediff(start0, stop);

        getrusage(RUSAGE_SELF,&r_usage2);
    
        printf("\nTime (total): %.f ms   Time (run): %.f ms Mem Before = %ld KB  Mem After=%ld KB\n",
            result_total, result, r->ru_maxrss, r_usage2.ru_maxrss);    
    }

    return 0;
}

//-----------------------
//
// 
//
//
static int std_run(int base, int n) {
    struct rusage r_usage, r_usage2;
    clock_t start0, stop;

    getrusage(RUSAGE_SELF,&r_usage);

    start0 = clock();
    initialize_randomizer();
    
    initialize();
        
    stats_reset();
//        stats_dump();
    
    std_run_step(base, n, 0, &r_usage, start0);
    if (del_rows != -1) {
        if (max_rows) {
            if (del_rows > max_rows) 
                del_rows = max_rows;
        } else {
            max_rows = del_rows;
        }
        printf("|Max rows: %ld\n", max_rows);
        count_rows = 0;
        std_run_step(base, n, 1, &r_usage, start0);

    }
}

//-----------------------
//
// Versão para protótipo
//
//
static void load_taxis(int taxi_id) {

    strcpy(files_prefix, "../data/taxis/taxis");
    strcpy(files_suffix, ".txt");

    if (taxi_id == 1) {
        strcpy(schema_file,SCHEMA_TAXI_1);
    } else if (taxi_id == 2) {
        strcpy(schema_file,SCHEMA_TAXI_2);
    } else if (taxi_id == 3) {
        strcpy(schema_file,SCHEMA_TAXI_3);
    } else {
        fatal("no taxi schema");
    }

    std_run(0, 1);

}

//-----------------------
//
// Versão para protótipo
//
//
static void load_rnp(void) {
    strcpy(files_prefix, "../rnp/novo/BR_20190806_14_");
    strcpy(files_suffix, ".csv");

    strcpy(schema_file,SCHEMA_RNP_1);
    std_run(-1, 60);
}

//-----------------------
//
// Brightkite 
//
//
static void load_bright_kite(void) {
    strcpy(files_prefix, "../data/Brightkite_ok");
    strcpy(files_suffix, ".txt");

    strcpy(schema_file,"schema_brightkite.json");
    std_run(0, 1);
}

//-----------------------
//
// Brightkite 
//
//
static void load_flight(void) {
    strcpy(files_prefix, "../data/flightdb/full/out/fb_delays_");
    strcpy(files_suffix, "");
    strcpy(schema_file,"schema_flight.json");
    strcpy(files_fmt, "%s%03d%s");

    std_run(0, 487);
}
//-----------------------
//
// Flight db_2
//
//
static void load_flight_2(void) {
    int y, base,n;

    struct rusage r_usage, r_usage2;
    clock_t start, start0, stop;

    getrusage(RUSAGE_SELF,&r_usage);

    start0 = clock();
    initialize_randomizer();
    
    initialize();
        
    stats_reset();
    strcpy(files_suffix, ".csv");
    strcpy(schema_file,"schema_flight.json");
//        stats_dump();
    
    for (y = 1987, base=9, n=3; y<2009; y++, base=0, n=12) {
        sprintf(files_prefix, "../data/flightdb/full/%04d_",y);

        std_run_step(base, n, 0, &r_usage, start);
        if (del_rows != -1) {
            if (max_rows) {
                if (del_rows > max_rows) 
                    del_rows = max_rows;
            } else {
                max_rows = del_rows;
            }
            printf("|Max rows: %ld\n", max_rows);
            count_rows = 0;
            std_run_step(base, n, 1, &r_usage, start);
        }  
    }
    // std_run(0, 12);
}


extern __int64_t content_total_alocado;
#define N_ITEMS 4
extern __int64_t count_items[4];
            void compact_bin_list(void);

//-----------------------
//
//
//
//
int mestrado_main(int argc, char *argv[]) {
Options opt;

    content_total_alocado = 0;
    app_init();
    strcpy(schema_file, SCHEMA_FILE);

    parse_options(argc, argv, &opt, 1);
    if (opt.show_options) { 
        printf("Run size = %d\n",opt.run_sz);
        printf("Filename: %s\n",opt.fname);
    }
    
    if  (argc >= 2) {
        if (strcmp(argv[1],"-m")==0) {
            test_metadata(argc, argv);
            return 0;
        } 
        if ( strcmp(argv[1],"-taxis1")==0)  {
            load_taxis(1);
            return 0;
        }
        if ( strcmp(argv[1],"-taxis2")==0)  {
            load_taxis(2);
            return 0;
        }
        if ( strcmp(argv[1],"-taxis3")==0)  {
            load_taxis(3);
            return 0;
        }
        if ( strcmp(argv[1],"-rnp")==0)  {
            load_rnp();
            return 0;
        }
        if ( strcmp(argv[1],"-fb")==0)  {
            load_flight();
            compact_bin_list();

            printf("Alocado: %ld\n",content_total_alocado);
            return 0;
        }
        if ( strcmp(argv[1],"-bk")==0)  {

            load_bright_kite();

#if 0
            int i;
            printf("----------------------\n");
            for (i=0;i<N_ITEMS;i++) {
                printf("Count[%d] = %ld\n",i,count_items[i]);
            }
#endif
            compact_bin_list();

            printf("Alocado: %ld\n",content_total_alocado);
            return 0;
        }
    }
    return test01(argc, argv);
}