// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

#include <stdio.h>
#include <signal.h>
#include "common.h"
#include "contrib/mongoose.h"  

#include "query_processor.h"
#include "terminal.h"
#include "webserver_mg.h"

#define WEB_POST

//****************************************************************************************
//
//                                     KEY CONSTANTS
//
//****************************************************************************************
#define MIN_DATAGRAM_BUFFER_SIZE    576
#define MAX_DATAGRAM_SIZE           65536
#define WORK_BUFFER_SIZE            (2 * (MAX_DATAGRAM_SIZE))

#define HEADER_RESERVATION          64

#define LOCAL_SOCKET  1
#define REMOTE_SOCKET 0

#define DEFAULT_COLLECTOR_ID     "UPX_001"
#define DEFAULT_COLLECTOR_HOST   "localhost"
#define DEFAULT_COLLECTOR_PORT   "6001"
#define DEFAULT_MQTT_PORT        "3000"

#define DEFAULT_HEARTBEAT         60
#define DEFAULT_PORT             "5000"
#define DEFAULT_LOG_PORT         "6001"
#define DEFAULT_WEB_SITE         "https://http-echo-server-123.herokuapp.com/"

#define MAX_GAP   100000

#define SCHEMA1 "schema_rnp_1.json"
#define PREFIX1 "../rnp/BR_"
#define START_FILE1  "20190806"

#define PREFIX_2 "../covid19/"
#define SCHEMA_2 "schema_covid19_1.json"
#define START_FILE_2   "20200517"
#define START_HOUR_2    0
#define START_MINUTE_2  0


#if 0
#define PREFIX "../covid19/"
#define SCHEMA "schema_covid19_1.json"
#define START_FILE   "20200517"
#else
#define PREFIX          PREFIX_2
#define SCHEMA          SCHEMA_2
#define START_FILE      START_FILE_2
#define START_HOUR      START_HOUR_2
#define START_MINUTE    START_MINUTE_2
#endif



//****************************************************************************************
//
//                                 COMMOM DATA AREA
//
//****************************************************************************************
static  char                        io_buffer[MAX_DATAGRAM_SIZE + HEADER_RESERVATION];

static struct {
    int   port;
    char *schema_file;
    char *data_path;
    char *start_file;
    char *end_file;
    int web;
    int max_gap;
} options = {
    8001, 
    SCHEMA,
    PREFIX, 
    START_FILE,
    NULL,
    0,
	MAX_GAP
};


//========================================================================
static clock_t ctrl_c_time = 0;
static int ctrl_c_count = 0;

#define MAX_COUNT 5

void signal_ctrl_c() { 		 
    signal(SIGINT, signal_ctrl_c); /*  */

    ctrl_c_count ++;
    if (ctrl_c_count < MAX_COUNT) {
        fprintf(stderr,"Press Ctrl+C more %d times to Pause/Stop.\n",MAX_COUNT - ctrl_c_count);
    } else if (ctrl_c_count == MAX_COUNT) {
        exit(0);
        fprintf(stderr,"\n\nWeb Server PAUSED!\n\nPress ENTER to STOP or any other key + ENTER to RESUME .\n");
        if (getchar() == 13) exit(0);
        fprintf(stderr,"RESUMED.\n");
        ctrl_c_count = 0;
    } 

}
 
void quitproc() {
    printf("ctrl-\\ pressed to quit\n");
	exit(0); /* normal exit status */
}

//========================================================================

static char *s_http_port = "8001";
static struct mg_serve_http_opts s_http_server_opts;

static PMetaData pmd;
static PSchema   ps;
static PAddr     pa;
static PNode     root;
static void *    record;


static struct mg_connection *cur_nc;

#define RECV_BUF   120000
static char recv_buf[RECV_BUF];

//-----------------------
//
//
//
//
static int my_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int r = mg_vprintf(cur_nc, fmt, args);
    // vprintf(fmt, args);
    va_end(args);
    return r;
}


//-----------------------
//
//
//
//
static char * replace_quotes(char *c,char rep) {
    char *s = c;
    for (;*s; s++) {
        if (*s == '\n') *s = ' ';
        else if (*s == '\r') *s = ' ';
        else if (*s == '\"') *s = rep;
    }
    return c;
}

//-----------------------
//
//
//
//
static void on_query(struct mg_connection *nc, struct http_message *hm) {

    //printf("\nOn query\n");    
    int n = mg_url_decode(hm->body.p,(int) hm->body.len, recv_buf, RECV_BUF,1);
    //printf("Query: %s\n",recv_buf);

    mg_printf(nc,"\r\n");
#if 0
    mg_printf(nc,"\r\n<h1>Hello, </h1>\r\n"
            "You asked for (%s) %.*s\r\n",
            recv_buf, (int) hm->uri.len, hm->uri.p);
#endif

    if (n > 0) {
        clock_t start, stop;
        start = clock();

        PQuery pq = query_begin();

        // jump error
        if (setjmp(query_jmp)) {
            mg_printf(nc, "{\"id\":%d,  \"tp\": 0, \"msg\": \"%s\"",pq->id,replace_quotes(query_error_string,'\''));
        } else {
            if (!query_process(pq, recv_buf, ps, pmd)) {
                mg_printf(nc, "{\"id\":%d,  \"tp\": 0, \"msg\": \"%s\"",pq->id,replace_quotes(query_error_string,'\''));
            } else {
                // serializa o resultado da query
                mg_printf(nc, "{");        
                cur_nc = nc;
                query_out_as_json(pq, my_printf);
            }
        }

        query_end(pq);

        // computa o tempo total da query
        stop = clock();
        //double result = timediff(start, stop);

        double result = (double)(stop - start) / CLOCKS_PER_SEC;

        //printf("\nTime: %.f ms %ld %ld\n", result, start, stop);    
        
        mg_printf(nc,",\"ms\":%.f }", result);        
    }
}



#define CSV_TOKEN    "kzFXyp3fsnSyCmjk@xjVsTw!ef4se"

//-----------------------
//
// Maximo de MAX_LINES_INDEX registros por envio 
//
//
static void on_csv(struct mg_connection *nc, struct http_message *hm) {
    #define MAX_LINES_INDEX   (1000)
    static char *lines_index[MAX_LINES_INDEX];

    int n = mg_url_decode(hm->body.p,(int) hm->body.len, recv_buf, RECV_BUF,1);
    mg_printf(nc,"\r\n");

    // printf("N:%d\n",n);
    // recv_buf[n] = 0;
    // printf("N:%d\n%s\n",n,recv_buf);

    if (n == 0) {
        mg_printf(nc,"err empty!");
        return;
    }

    clock_t start, stop;
    start = clock();

    int wsize = 0;
    lines_index[0] =  recv_buf;
    char *c;    
    int i;
    int state = 0;
    int n_lines = 0;


    for (i=0, c = recv_buf; i < n; i++, c++) {
        // fprintf(stderr,"[%c%d]",c[0],c[0]);

        if (c[0] == '\n' || (c[0] == '\r' && c[1] == '\n')) {
            //fprintf(stderr,"wsize %d Line: %s\n",wsize, lines_index[n_lines]);
            if (!wsize) break; // sinaliza final de linha
            char *c0 = c;
            if (c[0] == '\r') { *c = 0; c++; } else { *c = 0; }
            
            n_lines ++;
            if (n_lines == 1 && state ==0) {
                if (strcmp(CSV_TOKEN, *lines_index) != 0) {
                    mg_printf(nc,"err invalid token\r\n");
                    return;
                }
                state = 1;      // token was validated
                n_lines = 0;
            }

            if (n_lines >= MAX_LINES_INDEX) {
                mg_printf(nc,"err too many lines: %d\r\n",n_lines);
                return;
            }
            lines_index[n_lines] =  c+1;
            // fprintf(stderr,"wsize %d Line: %s\n",wsize, lines_index[n_lines-1]);
            wsize = 0;
        } else {
            wsize ++;
        }
    }

    //fprintf(stderr,"N: %d\n",n_lines);
    for (i=0; i<n_lines; i++) {
        int err = metadata_process_line(pmd, record, lines_index[i], TRUE);
        metadata_record_to_address(pmd, record, pa);
        tinycubes_insert(ps, root, pa->values, record);
    }
    putc('.',stderr);

    // computa o tempo total da query
    stop = clock();
    double result = timediff(start, stop);
    mg_printf(nc,"ok lines=%d  time=%f\r\n",n_lines, result);
}


//-----------------------
//
//
//
//
static void on_post(struct mg_connection *nc, struct http_message *hm) {
    struct mg_str s1 = mg_mk_str("/tc/query");
    struct mg_str s2 = mg_mk_str("/jsonquery");

    struct mg_str s3 = mg_mk_str("/tc/csv/v1");

    if (mg_strcmp(hm->uri,s1)==0 || mg_strcmp(hm->uri,s2)==0) {
        on_query(nc, hm);

    //
    } else if (mg_strcmp(hm->uri,s3)==0) {
        on_csv(nc, hm);

    } else {
        mg_printf(nc,
                "\r\nInvalid uri for post: %.*s\r\n",              
                (int) hm->uri.len, hm->uri.p);
    }
}

//-----------------------
//
//
//
//
static void on_get(struct mg_connection *nc,struct http_message *hm) {
    struct mg_serve_http_opts opts = { .document_root = "./www" }; 
    mg_serve_http(nc, hm, opts);
}


//-----------------------
//
//
//
//
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
 switch (ev) {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      //printf("%p: Connection from %s\r\n", nc, addr);
      break;
    }
    case MG_EV_HTTP_REQUEST: {
      struct http_message *hm = (struct http_message *) ev_data;
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      //printf("%p: %.*s %.*s\r\n", nc, (int) hm->method.len, hm->method.p, (int) hm->uri.len, hm->uri.p);


      if (mg_ncasecmp("POST",hm->method.p,hm->method.len) == 0) {
        mg_send_response_line(nc, 200,
                                "Content-Type: text/html\r\n"
                                "Connection: close"
                                );
        on_post(nc, hm);
      }

      if (mg_ncasecmp("GET",hm->method.p,hm->method.len) == 0) {
          on_get(nc, hm);
      }
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
    }
    case MG_EV_CLOSE: {
      // printf("%p: Connection closed\r4\n", nc);
      break;
    }
  }  
}

//========================================================================


//-----------------------
//
//
//
//
static void initialize(void) {
    map_reset();

    pmd = metadata_create();
    metadata_load(pmd, options.schema_file);

    ps = metadata_create_schema(pmd);
    terminal_set_schema(ps);
    
    root = ps->nfs[0].create_node(0);
    ps->root = root;

    pa = metadata_create_address(pmd);

    record = metadata_create_record(pmd);
}

//-----------------------
//
//
//
//
static void terminate(void) {
    free(record);
    metadata_close_input_file(pmd);
    metadata_release_address(pa);
    metadata_release(pmd);
}

//========================================================================

static int incrementa_dia(char *day) {
    char buf[10];
    
//    printf("Antes: %s\n",day);
    strncpy(buf,day,4); buf[4] =0;
    int yy = atoi(buf);
    strncpy(buf,day+4,2); buf[2] =0;
    int mm = atoi(buf);
    strncpy(buf,day+6,2); buf[2] =0;
    int dd = atoi(buf);
//    printf("%04d %02d %02d",yy,mm,dd);
    int inc_m = 0;

    dd ++;
    if (dd > 28) {
        if ((dd == 29) && (mm==2)) { // fevereiro?
            inc_m = (yy % 4 != 0);
        } else if (dd == 31) {
            inc_m = ((mm==4) || (mm==6) || (mm==9) || (mm==11));
        } else if (dd == 32) {
            inc_m = 1;
        }
    }

    if (inc_m) {
        dd = 1;
        mm ++;
        if (mm == 13) {
            mm = 1;
            yy ++;
        }
    }

    sprintf(day,"%04d%02d%02d",yy,mm,dd);
//    printf("Depois: %s\n",day);
}



//-----------------------
//
// retorna 0 
// retorna 0 se n
//
static int perform_increment(char *day, int *hour, int *minute, int inc){

    (*minute) += inc;
    if (*minute <= 59) return 0;
    *minute = 0;

    (*hour) ++;
    if (*hour <= 23) return 1;
    *hour = 0;

    incrementa_dia(day);
    printf("\n[*** %s ****]",day);
    return 1;
}


//-----------------------
//
// Versão para protótipo
//
//
static int incremental_load_from_file(char *day, int *hour, int *minute) {
    static int load_state = 0;
//    static char day[1024];
//    strcpy(day, day0);

    clock_t start, stop;
    start = clock();

    static char *suffix = ".csv";
    static char buf[1024];

#define INCREMENT 5

    if (load_state == 0) {
        printf("Ready to Load data from files: "); 
        printf("\n[*** %s ****]",day);
        fflush(stdout);
        load_state = 1;
    }

    if (load_state == 1) {
        int max_gap = options.max_gap;
        int gap = 0;
        while (1) {
            sprintf(buf, "%s%s_%02d_%02d%s",options.data_path, day, *hour, *minute, suffix);
            FILE *f;
            if ((f = fopen(buf,"r")) != NULL) {
                fclose(f);
                break;
            } 
            if (!options.end_file) return 0;
            if (strcmp(options.end_file,day)==0) return 0;
            if (gap >= max_gap) return 0;
            printf("{%02d:%02d}", *hour, *minute);  fflush(stdout);
            perform_increment(day, hour, minute, INCREMENT);
            gap ++;
        } 
        printf("[%02d:%02d", *hour, *minute);  fflush(stdout);
        metadata_open_input_file(pmd, buf);
        load_state = 2;
    }

    if (load_state == 2) {
        while (metadata_read(pmd, record)) {
            metadata_record_to_address(pmd, record, pa);
            tinycubes_insert(ps, root, pa->values, record);
            stop = clock();
            double diff = timediff(start,stop);
            if (diff > 40) {
                // printf("."); fflush(stdout);
                return 1;
            } 
        }
        load_state = 3;
    }

    if (load_state == 3) {
        printf("]");  fflush(stdout);
        metadata_close_input_file(pmd);
        load_state = 1;
        perform_increment(day,hour,minute,INCREMENT);
#if 0
        (*minute) += INCREMENT;
        if (*minute > 59) {
            (*hour) ++;
            if (*hour > 23) {
                incrementa_dia(day);
#if 0
                char *p = day + strlen(day) - 2;
                int d = atoi(p);
                char buf[10];
                sprintf(buf,"%02d",d+1);
                //printf("\n%s %s %d %s\n",day,p,d,buf);
                p[0] = buf[0];
                p[1] = buf[1];
#endif
                printf("\n[*** %s ****]",day);
                *hour = 0;
            }
            *minute = 0;
            load_state = 1;
        } 
#endif        
        return 1;
    }
    return 0;
}


//-----------------------
//
//
//
//
int webserver_run(int port) {
    struct mg_mgr mgr;
    struct mg_connection *nc;

    mg_mgr_init(&mgr, NULL);

    char port_s [10];
    if (port) {
        sprintf(port_s,"%d",port);
        s_http_port = port_s;
    }

    nc = mg_bind(&mgr, s_http_port, ev_handler);
    if (nc == NULL) {
        printf("Failed to create listener\n");
        return 1;
    }
    printf("Listening on port %s...\n", s_http_port);

    // c
    signal(SIGINT, signal_ctrl_c);
    ctrl_c_time = clock();
    //signal(SIGQUIT, quitproc);

    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = ".";  // Serve current directory
    s_http_server_opts.enable_directory_listing = "no";

    int hour = START_HOUR;
    int minute = START_MINUTE;

#define READING_TIMEOUT 20
#define WAITING_TIMEOUT 1000

    int timeout = READING_TIMEOUT;
   
    // application loop
    for (;;) {
        mg_mgr_poll(&mgr, timeout);
        if (!options.web) {
            if (!incremental_load_from_file(options.start_file, &hour, &minute)) {
                timeout = WAITING_TIMEOUT;   
            } else {
                timeout = READING_TIMEOUT;     // reading file
            }
        }
    }
    mg_mgr_free(&mgr);

    return 0;
}





/************************
 * 
 * 
 *
 */
static void help(char *program) {
    fprintf(stderr,"Usage: %s [ options ] \n",program);
    fprintf(stderr,"Options:\n");
    fprintf(stderr,"  -l port                  - local port. Default = %d\n",options.port);
    fprintf(stderr,"  -d initial_date          - format: YYYYMMDD. Default = %s\n", options.start_file);
    fprintf(stderr,"  -s schema_file           - Default = %s\n",options.schema_file);
    fprintf(stderr,"  -p data_path             - Default = %s\n",options.data_path);
}

/************************
 * 
 * 
 *
 */
static int parse_int_option(char *s, int *res, char *err) {
    *res = atoi(s);
    if (!*res) {

    }
}

/************************
 * 
 * 
 *
 */
static void parse_command_line(int argc, char *argv[]) {
    int i;
    char **it;

    // parse_options options
    for (i=1, it=argv+1; i < argc; i++, it++) {
        if (*it[0] != '-') break;

        if (strcmp(*it,"-l") == 0) {
            i ++;
            it ++;
            options.port = atoi(*it);
        } 
        else
        if (strcmp(*it,"-h") == 0) {
            help(argv[0]);
            exit(EXIT_FAILURE);
        }
        else
        if (strcmp(*it,"-d") == 0) {
            i ++;
            it ++;
            options.start_file = *it;
        }
        else
        if (strcmp(*it,"-s") == 0) {
            i ++;
            it ++;
            options.schema_file = *it;
        }
        else
        if (strcmp(*it,"-e") == 0) {
            i ++;
            it ++;
            options.end_file = *it;
        }
        else
        if (strcmp(*it,"-w") == 0) {
            options.web = 1;
        }
        else
        if (strcmp(*it,"-m") == 0) {
            options.max_gap = atoi(*it);
        }
        else
        if (strcmp(*it,"-p") == 0) {
            i ++;
            it ++;
            options.data_path = *it;
        }
        else {
            fprintf(stderr,"Opcão desconhecida: %s", *it);
            exit(EXIT_FAILURE);
        }
    }

    if (i == argc) {
    }
}



//-----------------------
//
//
//
//
int webserver_main(int argc, char *argv[]) {
    printf("-------------------------------------------------\n");
    printf("Tinycubes Server                 Version 0.2.0001\n");
    printf("Universidade Federal Fluminense - UFF, 2019, 2020\n");
    printf("-------------------------------------------------\n");

    parse_command_line(argc, argv);


    printf("Options:\n");
    printf(" -l Listen port:         %d\n", options.port);
    printf(" -p Data path:           %s\n", options.data_path);
    printf(" -s Schema:              %s\n", options.schema_file);
    printf(" -d Initial Date         %s\n", options.start_file);
    printf(" -e End Date (optional): %s\n", options.end_file?options.end_file:"");
    printf(" -w load from web        %s\n", options.web?"Y":"N");
    printf(" -m max missing files    %d\n", options.max_gap);

    app_init();

    initialize();
    webserver_run(options.port);
    terminate();
    return 0;
}
