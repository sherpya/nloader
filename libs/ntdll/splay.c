/*
 * Copyright (c) 2010, Sherpya <sherpya@netfarm.it>, aCaB <acab@0xacab.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ntdll.h"

typedef enum _RTL_GENERIC_COMPARE_RESULTS {
  GenericLessThan,
  GenericGreaterThan,
  GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

typedef ULONG CLONG;

struct _RTL_GENERIC_TABLE;

typedef RTL_GENERIC_COMPARE_RESULTS (NTAPI *PRTL_GENERIC_COMPARE_ROUTINE)(struct _RTL_GENERIC_TABLE *Table, PVOID FirstStruct, PVOID SecondStruct);
typedef PVOID (NTAPI *PRTL_GENERIC_ALLOCATE_ROUTINE)(struct _RTL_GENERIC_TABLE *Table, CLONG ByteSize);
typedef VOID (NTAPI *PRTL_GENERIC_FREE_ROUTINE)(struct _RTL_GENERIC_TABLE *Table, PVOID Buffer);

typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;


typedef struct _RTL_GENERIC_TABLE {
    PRTL_SPLAY_LINKS              TableRoot;
    LIST_ENTRY                    InsertOrderList;
    PLIST_ENTRY                   OrderedPointer;
    ULONG                         WhichOrderedElement;
    ULONG                         NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE  CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE     FreeRoutine;
    PVOID                         TableContext;
} RTL_GENERIC_TABLE, *PRTL_GENERIC_TABLE;


/* Utility struct for offset calculation */
struct item_header {
    RTL_SPLAY_LINKS _tree;
    LIST_ENTRY _entry;
};

VOID NTAPI RtlInitializeGenericTable(PRTL_GENERIC_TABLE Table, PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine, PRTL_GENERIC_FREE_ROUTINE FreeRoutine, PVOID TableContext)
{
    InitializeListHead(&Table->InsertOrderList);
    Table->TableRoot = NULL;
    Table->NumberGenericTableElements = 0;
    Table->WhichOrderedElement = 0;
    Table->OrderedPointer = &Table->InsertOrderList;
    Table->CompareRoutine = CompareRoutine;
    Table->AllocateRoutine = AllocateRoutine;
    Table->FreeRoutine = FreeRoutine;
    Table->TableContext = TableContext;
}


static inline void *treeitem_to_itemdata(void *tree_item) {
    return ((char *)tree_item) + sizeof(struct item_header);
}

static inline LIST_ENTRY *treeitem_to_entry(void *tree_item) {
    struct item_header *item = (struct item_header *)tree_item;
    return &item->_entry;
}

static inline void *treeentry_to_itemdata(LIST_ENTRY *tree_entry) {
    return (void *)(tree_entry+1);
}


int splay(PRTL_GENERIC_TABLE table, PVOID item) {
    RTL_SPLAY_LINKS *root = table->TableRoot, next = { NULL, NULL, NULL }, *right = &next, *left = &next, *temp;
    int ret = 0;

    if (!root)
	return 0;

    for(;;) { /* parse the tree top down */
	RTL_GENERIC_COMPARE_RESULTS comp_res = table->CompareRoutine(table, item, treeitem_to_itemdata(root));

	if(comp_res == GenericLessThan) { /* left branch */
	    if(!root->LeftChild)
		break;

	    if(table->CompareRoutine(table, item, treeitem_to_itemdata(root->LeftChild)) == GenericLessThan) {
		temp = root->LeftChild;
                root->LeftChild = temp->RightChild;

		if(temp->RightChild)
		    temp->RightChild->Parent = root;

                temp->RightChild = root;
		root->Parent = temp;
                root = temp;

                if(!root->LeftChild)
		    break;
	    }

            right->LeftChild = root;
	    root->Parent = right;
            right = root;
            root = root->LeftChild;

	} else if(comp_res == GenericGreaterThan) { /* right branch */
	    if(!root->RightChild)
		break;

	    if(table->CompareRoutine(table, item, treeitem_to_itemdata(root->RightChild)) == GenericGreaterThan) {
		temp = root->RightChild;
                root->RightChild = temp->LeftChild;

		if(temp->LeftChild)
		    temp->LeftChild->Parent = root;

                temp->LeftChild = root;
		root->Parent = temp;
                root = temp;

		if(!root->RightChild)
		    break;
	    }

	    left->RightChild = root;
	    root->Parent = left;
            left = root;
            root = root->RightChild;

	} else if(comp_res == GenericEqual) {
	    /* item in tree */
	    ret = 1;
	    break;
	} else {
	    /* epic fail */
	    printf("Compare routine returned %d for table %p\n", comp_res, table);
	    abort();
	}
    }


    left->RightChild = root->LeftChild;
    if(root->LeftChild)
	root->LeftChild->Parent = left;
    right->LeftChild = root->RightChild;
    if(root->RightChild)
	root->RightChild->Parent = right;
    root->LeftChild = next.RightChild;
    if(next.RightChild)
	next.RightChild->Parent = root;
    root->RightChild = next.LeftChild;
    if(next.LeftChild)
	next.LeftChild->Parent = root;
    root->Parent = NULL;
    table->TableRoot = root;


    return ret;
}

PVOID NTAPI RtlInsertElementGenericTable(PRTL_GENERIC_TABLE Table, PVOID Buffer, CLONG BufferSize, PBOOLEAN NewElement)
{
    struct item_header *new_item;
    RTL_SPLAY_LINKS *tree_item, *root_item;
    LIST_ENTRY *item_entry, *root_entry;
    void *item_data;

    RTL_GENERIC_COMPARE_RESULTS comp_res;


    if(splay(Table, Buffer)) {
	if(NewElement)
	    *NewElement = FALSE;
	return treeitem_to_itemdata(Table->TableRoot);
    }

    new_item = Table->AllocateRoutine(Table, BufferSize + sizeof(*new_item));
    if(!new_item)
	return (PVOID)(FALSE); /* sic! */

    if(NewElement)
	*NewElement = TRUE;

    root_item = Table->TableRoot;
    tree_item = &new_item->_tree;
    root_entry = &Table->InsertOrderList;
    item_entry = &new_item->_entry;
    item_data = treeitem_to_itemdata(new_item);
    memcpy(item_data, Buffer, BufferSize);

    if(root_item) {
	comp_res = Table->CompareRoutine(Table, item_data, treeitem_to_itemdata(root_item));

	if(comp_res == GenericLessThan) {
	    tree_item->LeftChild = root_item->LeftChild;
	    tree_item->RightChild = root_item;
	    root_item->LeftChild = NULL;
	    tree_item->Parent = NULL;
	    root_item->Parent = tree_item;
	} else if(comp_res == GenericGreaterThan) {
	    tree_item->RightChild = root_item->RightChild;
	    tree_item->LeftChild = root_item;
	    root_item->RightChild = NULL;
	    tree_item->Parent = NULL;
	    root_item->Parent = tree_item;
	} else {
	    printf("This is not possible!\n");
	    abort();
	}
    } else /* Empty tree */
	tree_item->LeftChild = tree_item->RightChild = NULL;

    Table->TableRoot = tree_item;
    if(root_entry->Blink == root_entry) {
	if(Table->NumberGenericTableElements) {
	    printf("Found self chained entry in table %p which has got %d elements\n", Table, Table->NumberGenericTableElements);
	    abort();
	}
	root_entry->Flink = root_entry->Blink = item_entry;
	item_entry->Flink = item_entry->Blink = root_entry;
    } else {
	if(!Table->NumberGenericTableElements) {
	    printf("Found non self chained entry in table %p which has got no elements\n", Table);
	    abort();
	}
	item_entry->Blink = root_entry->Blink; /* item to prev */
	item_entry->Blink->Flink = item_entry; /* prev to item */
	root_entry->Blink = item_entry;  /* root to item */
	item_entry->Flink = root_entry; /* item to root */
    }

    Table->NumberGenericTableElements++;

    /* reset enum data */
    Table->WhichOrderedElement = 0;
    Table->OrderedPointer = &Table->InsertOrderList;

    return item_data;
}

PVOID NTAPI RtlLookupElementGenericTable(PRTL_GENERIC_TABLE Table, PVOID Buffer)
{
    if(splay(Table, Buffer))
	return treeitem_to_itemdata(Table->TableRoot); /* always the same as Buffer ? */
    else
	return NULL;
}

BOOLEAN NTAPI RtlDeleteElementGenericTable(PRTL_GENERIC_TABLE Table, PVOID Buffer)
{
    RTL_SPLAY_LINKS *del_item;

    if(!splay(Table, Buffer))
	return FALSE;

    del_item = Table->TableRoot;
    if(!del_item->LeftChild) {
	Table->TableRoot = del_item->RightChild;
	Table->TableRoot->Parent = NULL;
    } else {
	Table->TableRoot = del_item->LeftChild;
	if(splay(Table, treeitem_to_itemdata(del_item))) {
	    printf("Duplicate item in tree\n");
	    abort();
	}
	Table->TableRoot->RightChild = del_item->RightChild;
	del_item->RightChild->Parent = Table->TableRoot;
    }

    Table->NumberGenericTableElements--;
    if(!Table->NumberGenericTableElements) {
	/* reset list entries */
	InitializeListHead(&Table->InsertOrderList);
    } else {
	LIST_ENTRY *del_entry = treeitem_to_entry(del_item);
	del_entry->Blink->Flink = del_entry->Flink; /* prev to next */
	del_entry->Flink->Blink = del_entry->Blink; /* next to prev */
    }

    Table->FreeRoutine(Table, (PVOID)del_item);

    /* reset enum data */
    Table->WhichOrderedElement = 0;
    Table->OrderedPointer = &Table->InsertOrderList;

    return TRUE;
}

PVOID NTAPI RtlEnumerateGenericTableWithoutSplaying(PRTL_GENERIC_TABLE Table, PVOID *RestartKey) {
    LIST_ENTRY *entry;

    if(!*RestartKey) /* starting enum */
	entry = &Table->InsertOrderList;
    else /* resuming enum */
	entry = (LIST_ENTRY *)*RestartKey;

    entry = entry->Flink; /* move to next */
    if(entry == &Table->InsertOrderList)
	return NULL; /* back at the start */

    *RestartKey = (PVOID)entry;
    return treeentry_to_itemdata(entry);
}
