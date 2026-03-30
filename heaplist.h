//
// Created by hyprayzen on 3/30/26.
//

#ifndef HTTP_CLIENT_HEAPLIST_H
#define HTTP_CLIENT_HEAPLIST_H

typedef struct HeapNode {
    void * data;
    struct HeapNode * next;
} HeapNode;

typedef struct heaplist {
    HeapNode * head;
    HeapNode * tail;
    int size;
} HeapList;



HeapList * MakeEmptyList();

// With Dummy.
HeapList * CreateList ();
void InsertToLast (HeapList * List, void * data);
void cleanUp (HeapList * List);

#endif //HTTP_CLIENT_HEAPLIST_H
