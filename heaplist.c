//
// Created by hyprayzen on 3/30/26.
//
#include "heaplist.h"
#include <stdlib.h>

HeapList * MakeEmptyList()
{
    HeapList * List = (HeapList * ) malloc (sizeof(HeapList));
    if (List == NULL)
    {
        free(List);
        return NULL;
    }
    return List;
}

HeapList * CreateList ()
{
    HeapList * List = MakeEmptyList();
    if (List == NULL) {
        free(List);
        return NULL;
    }
    HeapNode * dummy = (HeapNode *) malloc (sizeof(HeapNode));
    if (dummy == NULL)
    {
        free(dummy); free(List);
        return NULL;
    }
    dummy->data = NULL;
    dummy->next = NULL;
    List->head = dummy; List->tail = dummy; List->size = 0;
    return List;
}

void InsertToLast (HeapList * List, void * data) {
    HeapNode * newNode = (HeapNode *) malloc(sizeof(HeapNode));
    if (newNode == NULL) {
        free(newNode);
        return;
    }
    newNode->data = data; newNode->next = NULL;
    List->tail->next = newNode;
    List->tail = newNode;
    List->size++;
}

void cleanUp (HeapList * List) {
    HeapNode * current = List->head;
    HeapNode * next = NULL;
    while (current != NULL) {
        next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    free(List);
}



