#include "ssd_util.h"

/* Coperd: src: s->bs->filename {/home/huaicheng/images/u14s_old.raw} -> u14s_old */
char *truncate_filename(char *src)
{
    const char *delim = "/.";
    char *token = NULL, *last_token = NULL, *llast_token = NULL;
    while ((token = strtok(src, delim)) != NULL) {
        llast_token = last_token;
        last_token = token;
        src = NULL;
    }

    //printf("Last token is %s\n", last_token);
    return llast_token;
}

/* Coperd: set ssd->name */
void set_ssd_name(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    ssd->name = (char *)malloc(sizeof(char)*1024);
    strcpy(ssd->name, s->bs->filename);
    ssd->name = truncate_filename(ssd->name);
}

char *get_ssd_name(IDEState *s)
{
    SSDState *ssd = &(s->ssd);
    return ssd->name;
}

/* Coperd: src -> bst_filename, struct_name -> "_block_state_table.dat" */
void set_ssd_struct_filename(IDEState *s, char *src, const char *ssd_struct_name)
{
    strcpy(src, "./data/");

    char *tmp = (char *)malloc(sizeof(char)*1024);
    strcpy(tmp, s->bs->filename); /* Coperd: we don't want to change s->bs->filename */
    tmp = truncate_filename(tmp);

    strcat(src, tmp);

    strcat(src, ssd_struct_name);

    //free(tmp);
}
