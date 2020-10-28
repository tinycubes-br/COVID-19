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

static NodeFunctions nfs;
static TerminalNodeFunctions tnfs;



//========================================================

/**
 * 
 * 
 * 
 */ 
_PRIVATE(void) prepare_schema(PSchema *ref_ps, Values *ref_address) {
    PSchema ps;
    //PObject pobject;

    map_reset();
    
    node_std_init_functions(&nfs);
    terminal_init_functions(&tnfs);

    // Create Schema
    ps = schema_begin();
    *ref_ps = ps;
    
    // nivel 1
    schema_new_level(ps);
    schema_new_value(ps, 8, &nfs);
    
    // nivel 2
    schema_new_level(ps);
    schema_new_value(ps, 1, &nfs);

#if 0
    // nivel 3
    schema_new_level(ps);
    schema_new_value(ps, 2, &fs);

    // nivel 4
    schema_new_level(ps);
    schema_new_value(ps, 3, &fs);
#endif

    *ref_address = (Values) schema_create_address(ps);

    schema_end(ps, &tnfs);

    terminal_set_schema(ps);
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
}


/**
 * 
 * 
 * 
 * 
 */ 
void release_schema(PSchema ps, Values address) {
    map_reset();
        
    schema_release_address(address);    
    schema_release_all(ps);

    pointers_cleanup();
    node_std_cleanup();
}

/**
 * 
 * 
 * 
 * 
 **/ 
int process_it(PSchema ps, PNode root, Values address, char *prefix, 
                int i, char *s, char *title, int dump_it, int sum_check, int quiet) {
    char buf[1000];
    
    int remove = 0;
    
    if (s[0] == '-') {
        remove = 1;
        s ++;
    } else if (s[0] == '+') {
        s ++;
    } 
    
    if (dump_it) {
	    strcat(title,s); strcat(title,", ");
    }

    str_addr(ps,s,address);
    
    if (remove) {
		LOGIT_2("REMOVE[%06d]: %s",i,s); 
	    tinycubes_remove(ps,root,address,0);
    } else {
		LOGIT_2("INSERT[%06d]: %s",i,s); 
		tinycubes_insert(ps,root,address,0);
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
        printf("*** %2d %s [%s]\n",i, remove?"removing":"inserting",s);
    }

    if (!dump_it) {
        LOGIT("NO DUMP");
    } else {
    	sprintf(buf,"%s_%02d.dot",prefix,i);
        dump(ps,root,buf,title,0,0);
        LOGIT("AFTER DUMP");
        sprintf(buf,"dot -Tpng %s_%02d.dot -o %s_%02d.png -Gsize=3dot0,24! -Gdpi=72",
                prefix,i, prefix,i);
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

void load_test_file(char *fname, PSchema ps, Values address, int sum_check, int quiet) {
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
    char prefix[1000];
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
                fatal("Load Test File: Missing root inistalizadion.");
            }
            if (process_it(ps, root, address, subdir, i, s, title, 
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

static int parse_number(char *s, int *num, int max, int optional) {
    *num = atoi(s);
    if (*num <= 0) {
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
static void parse_options(int argc, char *argv[], Options *po) {

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

    char *o;
    int idx = 1;
    while (idx < argc) {
        o = argv[idx];
        idx ++;
        if (strcmp(o,"-a") == 0) {
            po->what = 'a';
            if (idx < argc) {
                if (parse_number(argv[idx], &po->n_rep, 100, 1)) {
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

        } else if (strcmp(o,"-n")==0) {
            if (idx < argc) {
                if (po->first_run) {
                    fatal("First run must be settled after -n");
                }
                parse_number(argv[idx], &po->run_sz, MAX_BLOCKS, 0);
                idx ++;
            }

        } else if (strcmp(o,"-r")==0) {
            if (idx < argc) {
                parse_number(argv[idx], &po->n_rep, 100, 0);
                idx ++;
            }

        } else if (strcmp(o,"-c")==0) {
            if (idx < argc) {
                parse_number(argv[idx], &po->sum_check, 5000, 0);
                idx ++;
            }

        } else if (strcmp(o,"-i")==0) {
            if (idx < argc) {
                parse_number(argv[idx], &po->first_run, po->run_sz, 0);
                idx ++;
            }

        } else if (strcmp(o,"-d")==0) {
            if (idx < argc) {
                parse_number(argv[idx], &po->range, 25, 0);
                idx ++;
            }

        } else if (strcmp(o,"-q")==0) {
            po->quiet = 1;

        } else if (strcmp(o,"-I")==0) {
            po->only_insertions = 1;

        } else if (strcmp(o,"-o")==0) {
            po->show_options = 1;

        } else {
            show_usage();
        }
    }
}

//=========================================================================================

#include "metadata.h"


int test_01(int argc, char *argv[]) {    
    PSchema ps;
    Values address;
    Options opt;

    initialize_randomizer();
    
    prepare_schema(&ps, &address);

    parse_options(argc, argv, &opt);
    if (opt.show_options) {
        printf("Run size = %d\n",opt.run_sz);
        printf("Filename: %s\n",opt.fname);
    }


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
                load_test_file(fname, ps, address, opt.sum_check, opt.quiet);
                stats_dump();
            }
            n--;
        }

    } else if (opt.what == 'f') {
        int n = opt.n_rep;
        while (n) {
            fprintf(stderr,"------------------------\n");
            load_test_file(opt.fname, ps, address, opt.sum_check, opt.quiet);
            stats_dump();
            n--;
        }
    }

    node_std_cleanup();
    release_schema(ps, address);
 
    
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
    metadata_load(pmd, "schema_rnp_1.json");
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

    char prefix[] = "../rnp/BR_20190806_14_";
    char suffix[] = ".csv";
    char buf[1024];

    int i;
    for (i = 0; i < 1; i++) {
        sprintf(buf, "%s%02d%s",prefix,i+30,suffix);
        printf("[%d]",i);  fflush(stdout);
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


//-----------------------
//
//
//
//
int main_test01(int argc, char *argv[]) {

    app_init();
    
    if  (argc >= 2) {
        if (strcmp(argv[1],"-m")==0) {
            test_metadata(argc, argv);
            return 0;
        } 
    }
    return test_01(argc, argv);
}


