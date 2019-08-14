/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "errno.h"
#define MIN_BLOCK_SIZE 32

/*
 * This function gets the size of block to be allocated.
 *
 * @param size The number of bytes requested to be allocated.
 *
 * @return It returns multiple of 16 where requested size + size of header is less than it, starting from min size 32.
 * If the requested size + size of header is greater than page size it returns 0.
 */
size_t get_size(size_t size);

/*
 * This function searches for the appropriate freelist node in list of freelists
 * such that it can accomodate the allocated size.
 *
 * @param allocated_size The number of bytes that needs to be allocated.
 *
 * @return It returns the free list node that can accomodate allocated size.
 */
void* search_free_lists(size_t allocated_size );

/*
 * This function first checks if the unused size is less than 32 bytes, if so if sets the fields in
 * header and returns the same ptr, if not it splits the block and places the block which is not needed in freelist.
 *
 * @param temp is the ptr to a block that needs to be split.
 * @param alloc_size The number of bytes that needs to be allocated after including sizeof header info.
 * @param requested_size The number of bytes that is actually requested by the user.
 *
 * @return It returns the ptr to the block that is allocated.
 */

sf_header* split_block(sf_header *temp, size_t alloc_size,size_t requested_size);

/*
 * This function searches for the appropriate freelist node in list of freelists
 * and returns a free list node pointing to block preceding  the block that can accomodate the allocated size.
 *
 * @param allocated_size The number of bytes that needs to be allocated.
 *
 * @return It returns a free list node pointing to block preceding  the block that can accomodate the allocated size.
 */

sf_free_list_node * get_preceding_free_list(size_t given_size);

/*
 * This function coaelesces two blocks into one and places that block into freelist.
 *
 * @param sf_prev_head is a ptr to a header of first block(previous).
 * @param sf_new_head is a ptr to a header of the second block(new).
 *
 * @return It coaelesces the two blocks and places the coaelesced block into the free list and returns nothing.
 */
void coaelesce(sf_header* sf_prev_head,sf_header* sf_new_head);

/*
 * This function searches for the appropriate freelist node in list of freelists
 * and returns adds a block in LIFO order of the free list.
 *
 * @param mem_head is a ptr to the block which needs to be added to appropriate free list.
 *
 * @return It just adds the given block into free list and returns nothing.
 */
void add_block_to_free_list(sf_header* mem_head);

/*
 * This function searches for the appropriate freelist node in list of freelists
 * and returns a free list node pointing to block whose size is equal to the allocated size.
 *
 * @param given_size The size of the block for which we need to get the block whose size is equal to the given size.
 *
 * @return It returns a free list node pointing to block whose size is equal to the allocated size.
 */
sf_free_list_node* get_equal_free_list(size_t given_size);

/*
 * This function searches for the given block in the given list
 * and returns the block if found else returns NULL.
 *
 * @param list list in which you have to search for the given block.
 * @param block the block which needs to be searched in the given list.
 *
 * @return It returns the block if found in the given list else returns NULL.
 */

sf_header* get_block_from_list(sf_free_list_node* list,sf_header* block);

/*
 * This function searches for the given block in the given list, removes it from the list
 * and returns the removed block.
 *
 * @param list list in which you have to search for the given block and remove it.
 * @param block the block which needs to be removed from the given list.
 *
 * @return It returns the removed block's header.
 */
sf_header* remove_block_from_list(sf_free_list_node*list,sf_header* block);

sf_header* last;
sf_header* initial;

/*
 * This is your implementation of sf_malloc. It acquires uninitialized memory that
 * is aligned and padded properly for the underlying system.
 *
 * @param size The number of bytes requested to be allocated.
 *
 * @return If size is 0, then NULL is returned without setting sf_errno.
 * If size is nonzero, then if the allocation is successful a pointer to a valid region of
 * memory of the requested size is returned.  If the allocation is not successful, then
 * NULL is returned and sf_errno is set to ENOMEM.
 */
void *sf_malloc(size_t size) {

    if(sf_mem_end()-sf_mem_start()== 0){// malloc is called for the first time
        last=sf_mem_grow();
        if(last==NULL){
            sf_errno = ENOMEM;
            return NULL;
        }
        ((sf_prologue*)sf_mem_start())->padding = 0;
        ((sf_prologue*)sf_mem_start())->header.info.block_size=PAGE_SZ>>4;
        ((sf_prologue*)sf_mem_start())->header.info.allocated=0;
        ((sf_prologue*)sf_mem_start())->header.info.prev_allocated=0;
        ((sf_prologue*)sf_mem_start())->header.info.requested_size=0;
        ((sf_prologue*)sf_mem_start())->footer.info.allocated=0;
        ((sf_prologue*)sf_mem_start())->footer.info.prev_allocated=0;
        ((sf_prologue*)sf_mem_start())->footer.info.requested_size=0;
        ((sf_prologue*)sf_mem_start())->footer.info.block_size=PAGE_SZ>>4;

        sf_epilogue* epilogue=sf_mem_end() - sizeof(sf_epilogue);
        epilogue->footer.info.allocated = 0;
        epilogue->footer.info.prev_allocated = 0;
        epilogue->footer.info.requested_size=0;
        epilogue->footer.info.block_size=PAGE_SZ >> 4;

        initial = sf_mem_start() + sizeof(sf_prologue);
        initial->info.allocated=0;
        initial->info.prev_allocated=0;
        initial->info.block_size=((PAGE_SZ)-sizeof(sf_prologue)-sizeof(sf_epilogue))>>4;
        initial->info.requested_size=0;
        sf_footer* initial_foot;
        initial_foot = (sf_footer*)((uintptr_t)initial+(initial->info.block_size<<4)-sizeof(sf_footer));
        initial_foot->info.allocated=0;
        initial_foot->info.prev_allocated=0;
        initial_foot->info.block_size=((PAGE_SZ)-sizeof(sf_prologue)-sizeof(sf_epilogue))>>4;
        initial_foot->info.requested_size=0;

        if(sf_add_free_list(initial_foot->info.block_size<<4,sf_free_list_head.next)==NULL){//Add initial block into the free list
            sf_errno=ENOMEM;
            return NULL;
        }
        sf_free_list_node *new_list_head=sf_free_list_head.next;
        new_list_head->head.links.next= initial;
        initial->links.next= &new_list_head->head;
        initial->links.prev= &new_list_head->head;
        sf_free_list_head.next->head.links.prev = initial;

    }
    if(size==0)//requested size is 0
        return NULL;
    size_t alloc_size = get_size(size);
    sf_free_list_node *free_list;
    sf_header *allocated_block;
    int i=1;
    while((free_list=search_free_lists(alloc_size))==NULL){
        void* mem_new;
        if((mem_new = sf_mem_grow())==NULL){
            sf_errno = ENOMEM;
            return NULL;
        }
        ((sf_prologue*)sf_mem_start())->padding = 0;
        ((sf_prologue*)sf_mem_start())->header.info.block_size+=PAGE_SZ>>4;
        ((sf_prologue*)sf_mem_start())->header.info.allocated=0;
        ((sf_prologue*)sf_mem_start())->header.info.prev_allocated=0;
        ((sf_prologue*)sf_mem_start())->header.info.requested_size=0;
        ((sf_prologue*)sf_mem_start())->footer.info.allocated=0;
        ((sf_prologue*)sf_mem_start())->footer.info.prev_allocated=0;
        ((sf_prologue*)sf_mem_start())->footer.info.requested_size=0;
        ((sf_prologue*)sf_mem_start())->footer.info.block_size+=PAGE_SZ>>4;
        sf_epilogue* epilogue=sf_mem_end() - sizeof(sf_epilogue);
        epilogue->footer.info.allocated = 0;
        epilogue->footer.info.prev_allocated = 0;
        epilogue->footer.info.requested_size=0;
        epilogue->footer.info.block_size=((sf_prologue*)sf_mem_start())->footer.info.block_size;
        sf_header* mem_head=mem_new-sizeof(sf_epilogue);
        sf_footer* mem_foot=(sf_footer*)((uintptr_t)epilogue -sizeof(sf_footer));
        mem_head->info.block_size=(PAGE_SZ>>4);
        mem_head->info.prev_allocated = ((sf_footer*)((uintptr_t)mem_head -sizeof(sf_footer)))->info.allocated;
        mem_foot->info.block_size = mem_head->info.block_size;
        mem_foot->info.prev_allocated=mem_head->info.prev_allocated;
        if(mem_head->info.prev_allocated==0){//check prev block is free or not
            sf_footer* sf_prev_foot = (sf_footer*)((uintptr_t)mem_head -sizeof(sf_footer));
            sf_header* sf_prev_head = (sf_header*)((uintptr_t)mem_head - (sf_prev_foot->info.block_size<<4));
            if(sf_prev_head >= (sf_header*)(sf_mem_start()+sizeof(sf_prologue)) && sf_prev_head < (sf_header*)(sf_mem_end()-sizeof(sf_epilogue)))
                coaelesce(sf_prev_head, mem_head);
            i++;

        }
        else{
            add_block_to_free_list(mem_head);
        }
    }
    sf_header *ptr=&free_list->head;
    allocated_block = ptr->links.next;
    ptr->links.next=allocated_block->links.next;
    allocated_block->links.next->links.prev=ptr;
    allocated_block=split_block(allocated_block,alloc_size,size);
    return &allocated_block->payload;
}


/*
 * Marks a dynamically allocated region as no longer in use.
 * Adds the newly freed block to the free list.
 *
 * @param ptr Address of memory returned by the function sf_malloc.
 *
 * If ptr is invalid, the function calls abort() to exit the program.
 */
void sf_free(void *pp) {
    if(pp==NULL)
        abort();
    sf_header* pp_head;
    pp_head = (sf_header*)((char*)pp - sizeof(pp_head->info));
    if(pp_head < (sf_header*)(sf_mem_start()+sizeof(sf_prologue)) || pp_head >= (sf_header*)(sf_mem_end()-sizeof(sf_epilogue)))
        abort();
    if(pp_head->info.allocated ==0)
        abort();
    if((pp_head->info.block_size<<4) % 16 != 0 || (pp_head->info.block_size<<4) < MIN_BLOCK_SIZE)
        abort();
    if(pp_head->info.requested_size + sizeof(pp_head->info)> (pp_head->info.block_size<<4))
        abort();
    if((pp_head->info.prev_allocated)==0){
        sf_footer* prev_foot = (sf_footer*)((char*)pp_head - sizeof(sf_footer));
        sf_header* prev_head = (sf_header*)((uintptr_t)pp_head - (prev_foot->info.block_size<<4));
        if(prev_foot > (sf_footer*)(sf_mem_start() + sizeof(sf_prologue)) && prev_foot < (sf_footer*)(sf_mem_end() - sizeof(sf_epilogue)) && prev_head >=(sf_header*)(sf_mem_start() + sizeof(sf_prologue)) && prev_head <(sf_header*)(sf_mem_end() - sizeof(sf_epilogue))){
            if(prev_foot->info.allocated!=0 || prev_head->info.allocated !=0)
                abort();
        }
    }
    //copy header into footer position of free block and free it
    sf_footer* cur_foot = (sf_footer*)((char*)pp_head + (pp_head->info.block_size<<4) - sizeof(sf_footer));
    cur_foot->info.allocated = pp_head->info.allocated;
    cur_foot->info.prev_allocated = pp_head->info.prev_allocated;
    cur_foot->info.requested_size = pp_head->info.requested_size;
    cur_foot->info.block_size = pp_head->info.block_size;
    pp_head->info.allocated = 0;
    pp_head->info.requested_size = 0;
    cur_foot->info.allocated = 0;
    cur_foot->info.requested_size = 0;
    //get header of next block
    sf_header* next_head= (sf_header*)((char*)cur_foot + sizeof(sf_footer));
    //get prev head
    sf_footer* prev_foot = (sf_footer*)((char*)pp_head - sizeof(sf_footer));
    sf_header* prev_head = (sf_header*)((char*)pp_head - (prev_foot->info.block_size<<4));
    if(prev_head >= (sf_header*)(sf_mem_start()+sizeof(sf_prologue))  && next_head < (sf_header*)(sf_mem_end()-sizeof(sf_epilogue))){

        //Check if prev block and next block are free and coaelesce accordingly
        if(prev_head->info.allocated == 1 && next_head->info.allocated ==1){
            pp_head->info.prev_allocated = 1;
            cur_foot->info.prev_allocated = 1;
            next_head->info.prev_allocated = 0;
            add_block_to_free_list(pp_head);
            return;
        }
        else if(prev_head->info.allocated == 1 && next_head->info.allocated ==0){
            pp_head->info.prev_allocated = 1;
            cur_foot->info.prev_allocated = 1;
            //next_head->info.prev_allocated = 0;
            coaelesce(pp_head, next_head);
        }
        else if(prev_head->info.allocated == 0 && next_head->info.allocated ==1){
            pp_head->info.prev_allocated = 0;
            cur_foot->info.prev_allocated = 0;
            next_head->info.prev_allocated = 0;
            coaelesce(prev_head,pp_head);
        }
        else{
            pp_head->info.prev_allocated = 0;
            cur_foot->info.prev_allocated = 0;
            sf_header* next_head_next = (sf_header*)((char*)next_head + (next_head->info.block_size<<4));
            next_head_next->info.prev_allocated=0;
            coaelesce(prev_head,pp_head);
            coaelesce(prev_head,next_head);

        }
    }
    else if(prev_head < (sf_header*)(sf_mem_start()+sizeof(sf_prologue))){
        if(next_head < (sf_header*)(sf_mem_end()-sizeof(sf_epilogue))){
            if(next_head->info.allocated==0){
                pp_head->info.prev_allocated = 0;
                cur_foot->info.prev_allocated = 0;
                next_head->info.prev_allocated = 0;
                coaelesce(pp_head,next_head);

            }
            else{
                add_block_to_free_list(pp_head);
                next_head->info.prev_allocated = 0;
                return;
            }
        }
    }
    else if(prev_head >= (sf_header*)(sf_mem_start()+sizeof(sf_prologue))){
        if(next_head >= (sf_header*)(sf_mem_end()-sizeof(sf_epilogue))){
            if(prev_head->info.allocated==0){
                pp_head->info.prev_allocated = 0;
                cur_foot->info.prev_allocated = 0;
                next_head->info.prev_allocated = 0;
                coaelesce(prev_head,pp_head);

            }
            else{
                add_block_to_free_list(pp_head);
                next_head->info.prev_allocated = 0;
                return;
            }
        }
    }
    else{

        add_block_to_free_list(pp_head);
        next_head->info.prev_allocated = 0;
    }
    return;
}


/*
 * Resizes the memory pointed to by ptr to size bytes.
 *
 * @param ptr Address of the memory region to resize.
 * @param size The minimum size to resize the memory to.
 *
 * @return If successful, the pointer to a valid region of memory is
 * returned, else NULL is returned and sf_errno is set appropriately.
 *
 * If sf_realloc is called with an invalid pointer sf_errno should be set to EINVAL.
 * If there is no memory available sf_realloc should set sf_errno to ENOMEM.
 *
 * If sf_realloc is called with a valid pointer and a size of 0 it should free
 * the allocated block and return NULL without setting sf_errno.
 */
void *sf_realloc(void *pp, size_t rsize) {
    sf_header* pp_head =(sf_header*)((char*)pp - sizeof(pp_head->info));
    if(pp==NULL)
        abort();
    if(pp_head < (sf_header*)(sf_mem_start()+sizeof(sf_prologue)) || pp_head >= (sf_header*)(sf_mem_end()-sizeof(sf_epilogue))){
        sf_errno=EINVAL;
        abort();
    }
    if(pp_head->info.allocated ==0)
        abort();
    if((pp_head->info.block_size<<4) % 16 != 0 || (pp_head->info.block_size<<4) < MIN_BLOCK_SIZE)
        abort();
    if(pp_head->info.requested_size + sizeof(pp_head->info)> (pp_head->info.block_size<<4))
        abort();
    if((pp_head->info.prev_allocated)==0){
        sf_footer* prev_foot = (sf_footer*)((char*)pp_head - sizeof(sf_footer));
        sf_header* prev_head = (sf_header*)((char*)pp_head - (prev_foot->info.block_size<<4));
        if(prev_head >= (sf_header*)(sf_mem_start()+sizeof(sf_prologue)) && prev_head < (sf_header*)(sf_mem_end()-sizeof(sf_epilogue)) && prev_foot >= (sf_footer*)(sf_mem_start()+sizeof(sf_prologue)) && prev_foot < (sf_footer*)(sf_mem_end()-sizeof(sf_epilogue))){
            if(prev_foot->info.allocated!=0 || prev_head->info.allocated !=0)
                abort();
        }
    }
    if(rsize == 0){
        sf_free(pp);
        return NULL;
    }
    if(rsize > pp_head->info.requested_size){//larger block
        void* ptr = sf_malloc(rsize);
        if(ptr==NULL)
            return NULL;
        memcpy(ptr,pp,pp_head->info.requested_size);
        sf_free(pp);
        return ptr;
    }
    else if(rsize < pp_head->info.requested_size){//smaller block
        size_t alloc_size = get_size(rsize);
        pp_head=split_block(pp_head,alloc_size,rsize);
        return pp;
    }
    else
        return pp;
}

/*
 * This function gets the size of block to be allocated.
 *
 * @param size The number of bytes requested to be allocated.
 *
 * @return It returns multiple of 16 where requested size + size of header is less than it, starting from min size 32.
 */
size_t get_size(size_t size){
    size_t i=MIN_BLOCK_SIZE;
    sf_header* head;
    for(;;i+=16){
        if(size+sizeof(head->info)<=i)
            break;
    }
    return i;
}

/*
 * This function searches for the appropriate freelist node in list of freelists
 * such that it can accomodate the allocated size.
 *
 * @param allocated_size The number of bytes that needs to be allocated.
 *
 * @return It returns the free list node that can accomodate allocated size.
 */
void* search_free_lists(size_t allocated_size){
    sf_free_list_node *temp=(sf_free_list_node *)&sf_free_list_head;
        do{
            temp=temp->next;
            if(temp->size >= allocated_size && temp->head.links.next!=&temp->head){
                return (temp);
            }
        }while(temp!=(sf_free_list_node *)&sf_free_list_head && temp->next!=temp->prev);
    return NULL;
}


/*
 * This function first checks if the unused size is less than 32 bytes, if so if sets the fields in
 * header and returns the same ptr, if not it splits the block and places the block which is not needed in freelist.
 *
 * @param temp is the ptr to a block that needs to be split.
 * @param alloc_size The number of bytes that needs to be allocated after including sizeof header info.
 * @param requested_size The number of bytes that is actually requested by the user.
 *
 * @return It returns the ptr to the block that is allocated.
 */
sf_header* split_block(sf_header *ptr, size_t alloc_size, size_t size){

    size_t unused =(ptr->info.block_size<<4)-alloc_size;
    if(unused <= MIN_BLOCK_SIZE){//check if unused is less than MIN_BLOCK_SIZE
        ptr->info.allocated=1;
        sf_footer* prev_foot = ((sf_footer*)((uintptr_t)ptr -sizeof(sf_footer)));
        ptr->info.prev_allocated=prev_foot->info.allocated;
        ptr->info.requested_size=size;
        sf_footer* ptr_foot = ((sf_footer*)((uintptr_t)ptr + (ptr->info.block_size<<4) -sizeof(sf_footer)));
        ptr_foot->info.allocated=1;
        ptr_foot->info.prev_allocated=ptr->info.prev_allocated;
        ptr_foot->info.requested_size=ptr->info.requested_size;
        return ptr;
    }
    sf_header *split;
    if(unused > MIN_BLOCK_SIZE){//splitting the block
        split=(sf_header*)((uintptr_t)ptr + alloc_size);
        split->info.allocated=0;
        split->info.block_size=(unused)>>4;
        split->info.requested_size=0;
        split->info.prev_allocated=1;
        sf_footer *foot=(sf_footer*)((uintptr_t)split + unused - sizeof(sf_footer));
        foot->info.allocated=0;
        foot->info.prev_allocated=1;
        foot->info.block_size=unused>>4;
        foot->info.requested_size=0;
        //Changing the original header
        ptr->info.allocated=1;
        ptr->info.block_size=alloc_size>>4;
        ptr->info.requested_size=size;
        sf_footer* prev_foot = ((sf_footer*)((uintptr_t)ptr -sizeof(sf_footer)));
        ptr->info.prev_allocated=prev_foot->info.allocated;
        sf_footer* ptr_foot = ((sf_footer*)((uintptr_t)ptr + (ptr->info.block_size<<4) -sizeof(sf_footer)));
        ptr_foot->info.allocated=1;
        ptr_foot->info.block_size=ptr->info.block_size;
        ptr_foot->info.prev_allocated=ptr->info.prev_allocated;
        ptr_foot->info.requested_size=ptr->info.requested_size;
    }
    //Adding split block into free list
    sf_footer* split_foot = (sf_footer*)((char*)split + (split->info.block_size<<4) - sizeof(sf_footer));
    //get header of next block
    sf_header* next_head= (sf_header*)((char*)split_foot + sizeof(sf_footer));
    if(split >= (sf_header*)(sf_mem_start()+sizeof(sf_prologue)) && next_head < (sf_header*)(sf_mem_end()-sizeof(sf_epilogue))){
        //check if the split block can be coaelesced further
        if(next_head->info.allocated ==0){
            coaelesce(split,next_head);
            return ptr;
        }
        else{
            add_block_to_free_list(split);
            return ptr;
        }

    }
    add_block_to_free_list(split);
    return ptr;
}

/*
 * This function searches for the appropriate freelist node in list of freelists
 * and returns a free list node pointing to block preceding  the block that can accomodate the allocated size.
 *
 * @param allocated_size The number of bytes that needs to be allocated.
 *
 * @return It returns a free list node pointing to block preceding  the block that can accomodate the allocated size.
 */
sf_free_list_node* get_preceding_free_list(size_t given_size){
    sf_free_list_node *temp=(sf_free_list_node *)&sf_free_list_head;
    do{
        temp=temp->next;
        if(temp->size > given_size){
            return (temp);
        }
    }while(temp!=(sf_free_list_node *)&sf_free_list_head && temp->next!=temp->prev);
    return &sf_free_list_head;
}


/*
 * This function searches for the appropriate freelist node in list of freelists
 * and returns a free list node pointing to block whose size is equal to the allocated size.
 *
 * @param given_size The size of the block for which we need to get the block whose size is equal to the given size.
 *
 * @return It returns a free list node pointing to block whose size is equal to the allocated size.
 */
sf_free_list_node* get_equal_free_list(size_t given_size){
    sf_free_list_node *temp=(sf_free_list_node *)&sf_free_list_head;
    do{
        temp=temp->next;
        if(temp->size == given_size){
            return (temp);
        }
    }while(temp!=(sf_free_list_node *)&sf_free_list_head && temp->next!=temp->prev);
    return NULL;
}

/*
 * This function coaelesces two blocks into one and places that block into freelist.
 *
 * @param sf_prev_head is a ptr to a header of first block(previous).
 * @param sf_new_head is a ptr to a header of the second block(new).
 *
 * @return It coaelesces the two blocks and places the coaelesced block into the free list and returns nothing.
 */
void coaelesce(sf_header* sf_prev_head,sf_header* sf_new_head){

    sf_free_list_node *prev_head_list=get_equal_free_list(sf_prev_head->info.block_size<<4);
    sf_free_list_node *cur_head_list=get_equal_free_list(sf_new_head->info.block_size<<4);
    sf_header *removed_prev;
    if(prev_head_list==NULL){//no free list of this size
        sf_free_list_node *cur_head_list=get_equal_free_list(sf_new_head->info.block_size<<4);
        removed_prev=remove_block_from_list(cur_head_list, sf_new_head);
        //change the block size
        sf_prev_head->info.block_size = ((removed_prev->info.block_size<<4) + (sf_prev_head->info.block_size<<4))>>4;

    }
    else if(prev_head_list!=NULL){//prev free list exists
        //Search for the prev_head in that list
        if(get_block_from_list( prev_head_list,sf_prev_head)!=NULL){
            removed_prev=remove_block_from_list(prev_head_list, sf_prev_head);
            if(cur_head_list!=NULL){
                if(get_block_from_list( cur_head_list,sf_new_head)!=NULL)
                    remove_block_from_list(cur_head_list, sf_new_head);
            }
            //change the block size
            sf_prev_head->info.block_size = ((removed_prev->info.block_size<<4) + (sf_new_head->info.block_size<<4))>>4;
        }
        else{// prev freelist exists but no block in that list
            sf_free_list_node *cur_head_list=get_equal_free_list(sf_new_head->info.block_size<<4);
            removed_prev=remove_block_from_list(cur_head_list, sf_new_head);
            //change the block size
            sf_prev_head->info.block_size = ((removed_prev->info.block_size<<4) + (sf_prev_head->info.block_size<<4))>>4;
        }

    }
    sf_prev_head->info.allocated = 0;
    sf_prev_head->info.requested_size = 0;
    sf_footer* new_footer = (sf_footer*)((uintptr_t)sf_new_head + (sf_new_head->info.block_size<<4) -sizeof(sf_footer));
    new_footer->info.block_size = (sf_prev_head->info.block_size);
    new_footer->info.allocated = 0;
    new_footer->info.prev_allocated = sf_prev_head->info.prev_allocated;
    new_footer->info.requested_size = 0;
    //adding the coelesced block into the freelist
    if(get_equal_free_list(sf_prev_head->info.block_size<<4)==NULL){

        if(sf_add_free_list(sf_prev_head->info.block_size<<4,get_preceding_free_list(sf_prev_head->info.block_size<<4))==NULL){
            sf_errno=ENOMEM;
            abort();
        }
    }
    add_block_to_free_list(sf_prev_head);
}


/*
 * This function searches for the appropriate freelist node in list of freelists
 * and returns adds a block in LIFO order of the free list.
 *
 * @param mem_head is a ptr to the block which needs to be added to appropriate free list.
 *
 * @return It just adds the given block into free list and returns nothing.
 */
void add_block_to_free_list(sf_header* mem_head){
    sf_free_list_node* free_list = get_equal_free_list(mem_head->info.block_size<<4);
    if(free_list==NULL){
        free_list = sf_add_free_list(mem_head->info.block_size<<4, get_preceding_free_list(mem_head->info.block_size<<4));
        if(free_list==NULL){
            sf_errno=ENOMEM;
            abort();
        }
    }
    sf_header* temp = free_list->head.links.next;
    free_list->head.links.next = mem_head;
    mem_head->links.next=temp;
    mem_head->links.prev=&free_list->head;
    temp->links.prev=mem_head;
    return;
}

/*
 * This function searches for the given block in the given list
 * and returns the block if found else returns NULL.
 *
 * @param list list in which you have to search for the given block.
 * @param block the block which needs to be searched in the given list.
 *
 * @return It returns the block if found in the given list else returns NULL.
 */
sf_header* get_block_from_list(sf_free_list_node* list,sf_header* block){
    sf_header head1=list->head;//remove the new block from new freelist
    sf_header  *head=&list->head;
    do{
        head =head->links.next;
        if(head == block){
            return block;
            break;
        }
    }while(head!=&head1 && head1.links.next!=head1.links.prev);
    return NULL;
}

/*
 * This function searches for the given block in the given list, removes it from the list
 * and returns the removed block.
 *
 * @param list list in which you have to search for the given block and remove it.
 * @param block the block which needs to be removed from the given list.
 *
 * @return It returns the removed block's header.
 */
sf_header* remove_block_from_list(sf_free_list_node*list,sf_header* block){
    sf_header* removed;
    sf_header head1=list->head;//remove the new block from new freelist
    sf_header  *head=&list->head;
    do{
        head =head->links.next;
        if(head == block){
            removed=head;
            head->links.prev->links.next=head->links.next;
            head->links.next->links.prev=head->links.prev;
            head->links.next=NULL;
            head->links.prev=NULL;
            break;
        }
    }while(head!=&head1 && head1.links.next!=head1.links.prev);
    return removed;
}