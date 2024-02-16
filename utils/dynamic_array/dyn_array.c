#include <dyn_array.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

/* ------------------- interfaccia dell'array ------------------ */

DArray *initDArray(size_t init_size, size_t strings_size){
    DArray *d = (DArray *)calloc(1, sizeof(DArray));
    if (!d)
    {
        perror("calloc");
        return NULL;
    }

    d->data = (char **)calloc(init_size, sizeof(char *));
    if (!d->data)
    {
        perror("calloc data");
        free(d);
        return NULL;
    }

    d->asize = init_size;
    d->aused = 0;
    d->strings_size = strings_size;

    for (int i = 0; i < init_size; i++)
    {
        d->data[i] = (char *)calloc(strings_size+1, sizeof(char));
        if (!d->data[i])
        {
            perror("calloc buf");
            for (int j = i-1; j >= 0; j++){
                free(d->data[j]);
            }
            free(d->data);
            free(d);
            return NULL;
        }
    }

    return d;
}

void deleteDArray(DArray *d){
    if (!d)
    {
        errno = EINVAL;
        return;
    }
    if (d->data){
        for (int i = 0; i < d->asize; i++)
            free(d->data[i]);
        free(d->data);
    }
    free(d);
}

int getDUsed(DArray *d){
    if (!d)
    {
        errno = EINVAL;
        return -1;
    }
    return d->aused;
}

int addDData(DArray *d, char *data){
    if (!d || !data)
    {
        errno = EINVAL;
        return -2;
    }

    if(d->asize == d->aused){ //reallocation
        int oldsize = d->asize;
        d->asize *= 2;

        char **new_data = realloc(d->data, d->asize * sizeof(void *));
        if(d->data == new_data){ //realloc error
            perror("realloc");
            for (int i = 0; i < d->asize; i++)
                free(d->data[i]);
            free(d->data);
            free(d);
            return -1;
        }
        else{ //reallocation
            d->data = new_data;
        }
        
        for (int i = oldsize; i < d->asize; i++)
        {
            d->data[i] = (char *)calloc(d->strings_size+1, sizeof(char));
            if (d->data[i] == NULL)
            {
                perror("calloc");
                for (int j = i-1; j >= 0; j++)
                    free(d->data[j]);
                free(d->data);
                free(d);
                return -1;
            }
        }
    }

    strncpy(d->data[d->aused], data, d->strings_size);
    d->aused++;
    return 0;
}

char *getDData(DArray *d){
    if (!d)
    {
        errno = EINVAL;
        return NULL;
    }

    if(d->aused == 0) //array vuoto
        return NULL;
        
    d->aused--;
    char *ret_data = (char *)calloc(d->strings_size, sizeof(char));
    if(!ret_data){
        perror("calloc");
        return NULL;
    }
    strncpy(ret_data, d->data[d->aused], d->strings_size);
    memset(d->data[d->aused], 0, d->strings_size);

    return ret_data;
}