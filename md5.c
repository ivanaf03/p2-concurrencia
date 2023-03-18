#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <openssl/evp.h>
#include <threads.h>

#include "options.h"
#include "queue.h"


#define MAX_PATH 1024
#define BLOCK_SIZE (10*1024*1024)
#define MAX_LINE_LENGTH (MAX_PATH * 2)


struct file_md5 {
    char *file;
    unsigned char *hash;
    unsigned int hash_size;
};

struct param1{
    queue *in_q;
    char *dir;
};

struct param2{
    queue *in_q;
    queue *out_q;
};

struct param3{
    int dirname_len;
    queue *out_q;
    FILE *out;
};

struct param4{
    char *dir;
    char *file;
    queue *in_q;
};

struct param5{
    queue *in_q;
};


int get_entries(void *args);


void print_hash(struct file_md5 *md5) {
    for(int i = 0; i < md5->hash_size; i++) {
        printf("%02hhx", md5->hash[i]);
    }
}


void read_hash_file(char *file, char *dir, queue q) {
    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char *file_name, *hash;
    int hash_len;

    if((fp = fopen(file, "r")) == NULL) {
        printf("Could not open %s : %s\n", file, strerror(errno));
        exit(0);
    }

    while(fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        char *field_break;
        struct file_md5 *md5 = malloc(sizeof(struct file_md5));

        if((field_break = strstr(line, ": ")) == NULL) {
            printf("Malformed md5 file\n");
            exit(0);
        }
        *field_break = '\0';

        file_name = line;
        hash      = field_break + 2;
        hash_len  = strlen(hash);

        md5->file      = malloc(strlen(file_name) + strlen(dir) + 2);
        sprintf(md5->file, "%s/%s", dir, file_name);
        md5->hash      = malloc(hash_len / 2);
        md5->hash_size = hash_len / 2;


        for(int i = 0; i < hash_len; i+=2)
            sscanf(hash + i, "%02hhx", &md5->hash[i / 2]);

        q_insert(q, md5);
    }

    fclose(fp);
}


void sum_file(struct file_md5 *md5) {
    EVP_MD_CTX *mdctx;
    int nbytes;
    FILE *fp;
    char *buf;

    if((fp = fopen(md5->file, "r")) == NULL) {
        printf("Could not open %s\n", md5->file);
        return;
    }

    buf = malloc(BLOCK_SIZE);
    const EVP_MD *md = EVP_get_digestbyname("md5");

    mdctx = EVP_MD_CTX_create();
    EVP_DigestInit_ex(mdctx, md, NULL);

    while((nbytes = fread(buf, 1, BLOCK_SIZE, fp)) >0)
        EVP_DigestUpdate(mdctx, buf, nbytes);

    md5->hash = malloc(EVP_MAX_MD_SIZE);
    EVP_DigestFinal_ex(mdctx, md5->hash, &md5->hash_size);

    EVP_MD_CTX_destroy(mdctx);
    free(buf);
    fclose(fp);
}

void recurse(char *entry, void *arg) {
    queue *q =  (queue *) arg;
    struct stat st;

    stat(entry, &st);

    struct param1 *p=malloc(sizeof(struct param1));
    p->in_q=q;
    p->dir=entry;
    if(S_ISDIR(st.st_mode))
        get_entries(p);
    free(p);
}

void add_files(char *entry, void *arg) {
    queue q = * (queue *) arg;
    struct stat st;

    stat(entry, &st);

    if(S_ISREG(st.st_mode))
        q_insert(q, strdup(entry));
}

void walk_dir(char *dir, void (*action)(char *entry, void *arg), void *arg) {
    DIR *d;
    struct dirent *ent;
    char full_path[MAX_PATH];

    if((d = opendir(dir)) == NULL) {
        printf("Could not open dir %s\n", dir);
        return;
    }

    while((ent = readdir(d)) != NULL) {
        if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") ==0)
            continue;

        snprintf(full_path, MAX_PATH, "%s/%s", dir, ent->d_name);

        action(full_path, arg);
    }

    closedir(d);
}

int get_entries(void *args) {
    struct param1* param=(struct param1*) args;
    walk_dir(param->dir, add_files, param->in_q);
    walk_dir(param->dir, recurse, param->in_q);
    return 0;
}

int fun(void *args){
    struct param1 *parameters=(struct param1*) args;
    get_entries(parameters);
    queue *cola;
    cola=parameters->in_q;
    acabar( *cola);
    return 0;
}

int fun2(void *args){
    struct file_md5 *md5;
    char* ent;
    struct param2 *parametros2=(struct param2*) args;
    while((ent = q_remove(*parametros2->in_q))!=NULL){
        md5=malloc(sizeof(struct file_md5));
        md5->file = ent;
        sum_file(md5);
        q_insert(*parametros2->out_q, md5);
    }
    acabar(*parametros2->out_q);
    return 0;
}

int fun3(void *args){
    struct file_md5 *md5;
    struct param3 *parametros3=(struct param3*) args;
    while((md5 = q_remove(*parametros3->out_q)) != NULL) {
        fprintf(parametros3->out, "%s: ", md5->file + parametros3->dirname_len);

        for(int i = 0; i < md5->hash_size; i++)
            fprintf(parametros3->out, "%02hhx", md5->hash[i]);
        fprintf(parametros3->out, "\n");

        free(md5->file);
        free(md5->hash);
        free(md5);
    }
    acabar2(*parametros3->out_q);
    return 0;
}

int  fun4(void *args){
    struct param4 *parameters=(struct param4*) args;
    read_hash_file(parameters->file, parameters->dir, *parameters->in_q);
    queue *cola;
    cola=parameters->in_q;
    acabar2( *cola);
    return 0;
}

int fun5(void *args){
    struct file_md5 *md5_in;
    struct file_md5 *md5_file= malloc(sizeof (struct file_md5));
    struct param5 *parameters=(struct param5*) args;
    while((md5_in = q_remove(*parameters->in_q))) {
        md5_file->file = md5_in->file;

        sum_file(md5_file);

        if(memcmp(md5_file->hash, md5_in->hash, md5_file->hash_size)!=0) {
            printf("File %s doesn't match.\nFound:    ", md5_file->file);
            print_hash(md5_file);
            printf("\nExpected: ");
            print_hash(md5_in);
            printf("\n");
        }

        free(md5_file->hash);
        free(md5_in->file);
        free(md5_in->hash);
        free(md5_in);
    }
    return 0;
}


void hilos(thrd_t *t, void *args, int size){
    for(int i=0; i<size;i++){
        thrd_create(&t[i], fun2, args);
    }
}

void hilos2(thrd_t *t, void *args, int size){
    thrd_create(t, fun3, args);
    thrd_join(*t, NULL);
}

void check(struct options opt) {
    queue in_q;
    struct param4* parameters4=malloc(sizeof (struct param4));
    struct param5* parameters5=malloc(sizeof (struct param5));
    thrd_t *t= malloc(sizeof(thrd_t));
    thrd_t *t2=malloc(sizeof(thrd_t)*opt.num_threads);

    in_q  = q_create(opt.queue_size, opt.num_threads);
    parameters4->in_q=&in_q;
    parameters4->dir=opt.dir;
    parameters4->file=opt.file;
    parameters5->in_q=&in_q;

    thrd_create(t, fun4,parameters4);

    for(int i=0; i<opt.num_threads; i++){
        thrd_create(&t2[i], fun5, parameters5);
    }
    for(int i=0; i<opt.num_threads; i++){
        thrd_join(t2[i], NULL);
    }
    acabar2(*parameters5->in_q);
    free(t);
    q_destroy(in_q);
}

void sum(struct options opt) {
    queue in_q, out_q;
    FILE *out;
    int dirname_len;

    in_q  = q_create(100, 1);
    out_q = q_create(opt.queue_size, opt.num_threads);


    thrd_t *t = malloc(sizeof(thrd_t));
    thrd_t *t2= malloc(sizeof (thrd_t)*opt.num_threads);
    thrd_t *t3=malloc(sizeof (thrd_t));

    struct param1* parameters=malloc(sizeof (struct param1));
    struct param2* parameters2=malloc(sizeof (struct param2));
    struct param3* parameters3=malloc(sizeof (struct param3));
    parameters->in_q=&in_q;
    parameters->dir=opt.dir;
    parameters2->in_q=&in_q;
    parameters2->out_q=&out_q;
    parameters3->out_q=&out_q;
    thrd_create(t, fun,parameters);

    acabar(out_q);

    if((out = fopen(opt.file, "w")) == NULL) {
        printf("Could not open output file\n");
        exit(0);
    }
    parameters3->out=out;

    hilos(t2, parameters2, opt.num_threads);

    dirname_len = strlen(opt.dir) + 1; // length of dir + /
    parameters3->dirname_len=dirname_len;

    hilos2(t3, parameters3, opt.num_threads);

    fclose(out);
    q_destroy(in_q);
    q_destroy(out_q);
    free(t);
    free(t2);
    free(parameters);
}

int main(int argc, char *argv[]) {
    struct options opt;
    opt.num_threads = 5;
    opt.queue_size  = 1000;
    opt.check       = true;
    opt.file        = NULL;
    opt.dir         = NULL;

    read_options (argc, argv, &opt);

    if(opt.check)
        check(opt);
    else
        sum(opt);
}