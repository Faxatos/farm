#include <sor_list.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>

/**
 * @file sor_list.c
 * @brief File di implementazione dell'interfaccia per la lista ordinata
 */

/* ------------------- interfaccia della lista ------------------ */

SList *initSList(size_t max_str_len){
    SList *l = (SList *)calloc(sizeof(SList), 1);
    if (!l)
    {
        perror("calloc");
        return NULL;
    }

    l->head = NULL;
    l->lsize = 0;
    l->str_len = max_str_len;

    return l;
}

void deleteSList(SList *l){
    if (!l)
    {
        errno = EINVAL;
        return;
    }

    SNode *current_node = l->head;
    SNode *previous_node = NULL;

    while (current_node != NULL){
        previous_node = current_node;
        current_node = current_node->next;
        free(previous_node->string);
        free(previous_node);
    }

    free(l);
}

int addNode(SList *l, char *string, long index){
    SNode *new_node = malloc(sizeof(SNode));
    if (!new_node)
    {
        perror("malloc");
        return -1;
    }
    new_node->string = (char *)calloc(l->str_len, sizeof(char));
    if (!new_node->string)
    {
        perror("calloc");
        return -1;
    }
    strncpy(new_node->string, string, l->str_len);
    new_node->index = index;

    SNode *current_node = l->head;
    SNode *previous_node = NULL;

    //itero puntatore fino alla posizione ordinata per index
    while(current_node != NULL && index > current_node->index){
        previous_node = current_node;
        current_node = current_node->next;
    }

    if(previous_node == NULL){
        new_node->next = l->head;
        l->head = new_node;
    }
    else{
        previous_node->next = new_node;
        new_node->next = current_node;
    }

    return 0;
}

void printSList(SList *l)
{
    SNode *head = l->head;

    while (head != NULL) {
        printf("%ld %s\n", head->index, head->string);
        head = head->next;
    }
}