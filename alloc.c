/**
 * Malloc
 * CS 241 - Spring 2020
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
//TODO
typedef struct _metadata{
    //void* user_mem_ptr;
    int size;
    bool free;
    struct _metadata* next;
    struct _metadata* next_free;
}metadata;

typedef struct _b_tag{
    int size;
    bool free;
}b_tag;

//global
static metadata* head = NULL;
static metadata* head_free = NULL;
static void* boundry_left = NULL;
static void* boundry_right = NULL;
//static void* capacity = NULL;
//static void* current_top = NULL;
//static int capacity_size = 0;
static unsigned int current_size = 0;
static int print_debug = 1;
//static int count_sbrk = 0;
static unsigned int amount_to_reserve = 1024 * 1024 * 512; //1G
static unsigned int amount_to_reserve = 100000000; //100M

//functions
metadata* reserve_space(size_t add_amount, size_t user_size);
void split_block(metadata* target, metadata* last_free_entry, size_t size);
void print_lists();
/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    size_t total_size = num * size;
    void* new_user_data = malloc(total_size);
    memset(new_user_data, 0, total_size);
    return new_user_data;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
 
void *malloc(size_t size) {
    // implement malloc!
    fprintf(stderr, "running malloc\n");
    //-get head -> find the first freed -> set free -> update metadata
    //-get head
    //boundry
    if (boundry_left == NULL) {
        boundry_left = sbrk(0);
        fprintf(stderr, "boundry_left: %p\n", boundry_left);
        boundry_right = boundry_left;
    }
    // make size of multiple of 8
    fprintf(stderr, "size: %zu\n", size);
    size = ceil((double)size / 8.0) * 8; 
    fprintf(stderr, "size: %zu\n", size);
    //metadata* curr_head = head;
    metadata* last_free_entry = NULL;
    metadata* curr_head_free = head_free;
    metadata* target = NULL;
    current_size = sizeof(metadata) + size + sizeof(b_tag);
    //-find the first freed : first fit
    print_lists();
    //int count_loop = 0;
    while(curr_head_free) {
        if ( curr_head_free->size >= size) {
           target = curr_head_free;
           break;
        }
        if (count_loop > 1000) break;
        count_loop++;
        last_free_entry = curr_head_free;
        curr_head_free = curr_head_free->next_free;
    }
    //-if not found -> reserve -> continue
    if (target == NULL) {
        unsigned int add_amount = ceil((float)(current_size)/amount_to_reserve) * amount_to_reserve;
        fprintf(stderr, "multiple: %lu\n", ceil((float)(current_size)/amount_to_reserve));
        fprintf(stderr, "add_amount: %lu\n", add_amount);
        target = reserve_space(add_amount, size);
        //boundry_right = sbrk(0);
        boundry_right = (void*)((char*)target + add_amount);
        fprintf(stderr, "reserved some space\n");
    }
    //-update struct -> add b_tag -> check space left -> new struct -> update origninal b_tag
    //-update list: list -> free list
    //-function: start ptr, previous free bloack ptr(NULL?), user size requested
    split_block(target, last_free_entry, size);
    
    print_lists();
    //-return user memory
    return target+1;
}

//add b_tag -> check space left -> new struct -> update origninal b_tag -> update old struct
//update list: list -> free list
//function: start ptr, previous free bloack ptr(NULL?), user size requested
void split_block(metadata* target, metadata* last_free_entry, size_t size) {
    fprintf(stderr, "metadata* target: %p\n", target);
    fprintf(stderr, "metadata* last_free_entry: %p\n", last_free_entry);
    fprintf(stderr, "size_t size: %zu\n", size);
    //free head
    size_t size_threashold = size + sizeof(b_tag) + sizeof(metadata);
    if (target->size >= size_threashold) {
        //add  b_tag
        b_tag* new_tag = (b_tag*)((char*)target + sizeof(metadata) + size);
        new_tag->size = size;
        b_tag* free_space_tag = (b_tag*)((char*)target + sizeof(metadata) + target->size);
        new_tag->free =false;
        //check space left
        int size_left = target->size - size - sizeof(b_tag);
        //new struct
        //treat as struct -> update info
        metadata* free_space_meta = NULL;
        free_space_meta = (metadata*)(new_tag + 1);
        int size_y = size_left - sizeof(metadata);
        free_space_meta->size = size_y;
        free_space_meta->free = true;
        //update list
        free_space_meta->next_free = target->next_free;
        metadata* curr_head_free = head_free;
        metadata* last_free = NULL;
        while(curr_head_free != NULL){
            if (curr_head_free == target) {
                break;
            }
            last_free = curr_head_free;
            curr_head_free = curr_head_free->next_free;
        }
        if (last_free == NULL) {
            //target is head
            head_free = free_space_meta;
        } else{
            //somewhere in the list
            last_free->next_free = free_space_meta;
        }
        
        free_space_meta->next = target->next;
        // update original b tag
        free_space_tag->size = free_space_meta->size;
        free_space_tag->free = true;
        //update old struct
        target->size = size;
        target->free = false;
    } else {
        //not enough space for struct -> change meta size and tag size
        metadata* curr_head_free = head_free;
        metadata* last_free = NULL;
        while(curr_head_free != NULL){
            if (curr_head_free == target) {
                break;
            }
            last_free = curr_head_free;
            curr_head_free = curr_head_free->next_free;
        }
        if (last_free == NULL) {
            //target is head
            head_free = target->next_free;
        } else{
            //somewhere in the list
            last_free->next_free = target->next_free;
        }
        //update original b tag: dead b tag
        free_space_tag->size = old_size;
        free_space_tag->free = false;
        target
    }

    
    //next remains the same if the one splied is dead space
    if (enough_space_for_struct) {
        target->next = free_space_meta;
    }
    target->next_free = NULL;

    fprintf(stderr, "\n");
}





//sbrk -> construct block -> push to list -> push to free list
metadata* reserve_space(size_t add_amount, size_t user_size) {
    fprintf(stderr, "add_amount: %zu\n", add_amount);
    metadata* temp = sbrk(add_amount);
    if (temp == (void*)-1) return NULL;
    //capacity_size += add_this_amount;
    unsigned int size = add_amount -sizeof(metadata) - sizeof(b_tag);
    temp->next = head;
    head = temp;
    temp->size = size;
    temp->free = true;
    temp->next_free = head_free;
    head_free = temp;
    //update b_tag
    b_tag* temp_tag = (b_tag*)((char*)temp + sizeof(metadata) + size);
    temp_tag->size = size;
    if (size > 4000000000) {
        fprintf(stderr, "-----------------wrong--------------------\n");
    }
    temp_tag->free = true;
    //print
    fprintf(stderr, "temp address: %p\n", temp);
    fprintf(stderr, "user_size: %zu\n", user_size);
    fprintf(stderr, "add_amount: %zu\n", add_amount);
    fprintf(stderr, "temp_tag address: %p\n", temp_tag);
    fprintf(stderr, "test pointer: %p\n", (char*)temp + sizeof(metadata) + user_size);
    fprintf(stderr, "\n");
    return temp;
}
/////////////////test/////////////////////
fprintf(stderr, "1. memory is at %p\n", target);
fprintf(stderr, "2. memory is at %p\n", target+4);
sbrk(sizeof(metadata));
fprintf(stderr, "3. memory is at %d\n", sbrk(0));


/////////////////////////////////////////
current_size = sizeof(metadata) + size + sizeof(b_tag);
target = reserve_space(current_size, size);

fprintf(stderr, "size of metadata %lu\n", sizeof(metadata));
fprintf(stderr, "size of b_tag %lu\n", sizeof(b_tag));

return target;




/////////////////////////////////////////
/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    fprintf(stderr, "running free\n");
    print_lists();
    // implement free!
    if (ptr == NULL){
        return;
    }
    //TODO: check for invalid pointer
    // find ptr -> free it
    metadata* this_block = (metadata*)((char*)ptr - sizeof(metadata));
    this_block->free = true;
    b_tag* this_b_tag = (b_tag*)((char*)ptr + this_block->size);
    this_b_tag->free = true;
    //get left & right info
    fprintf(stderr, "check_01\n");
    metadata* right_block = (metadata*)((char*)this_b_tag + sizeof(b_tag));
    b_tag* right_b_tag = (b_tag*)((char*)right_block + sizeof(metadata) + right_block->size);
    b_tag* left_b_tag = (b_tag*)((char*)this_block - sizeof(b_tag));
    metadata* left_block = (metadata*)((char*)left_b_tag - left_b_tag->size - sizeof(metadata));
    fprintf(stderr, "check_02\n");
    fprintf(stderr, "this block: %p\n", this_block);
    fprintf(stderr, "left block: %p\n", left_block);
    fprintf(stderr, "left tag: %p\n", left_b_tag);
    fprintf(stderr, "left tag size: %d\n", (int)left_b_tag->size);
    fprintf(stderr, "right block: %p\n", right_block);
    fprintf(stderr, "right tag: %p\n", right_b_tag);
    fprintf(stderr, "boundry_left: %p\n", boundry_left);
    fprintf(stderr, "boundry_right: %p\n", boundry_right);
    bool left_is_free = false;
    bool right_is_free = false;
    size_t total_size = 0;
    fprintf(stderr, "check_03\n");
    //check boundries
    fprintf(stderr, "boundry_left: %p\n", boundry_left);
    fprintf(stderr, "boundry_right: %p\n", boundry_right);
    if ((void*)left_block < boundry_left || (void*)left_block > boundry_right) {
        fprintf(stderr, "over boundry left\n");
        left_is_free = false;
    } else {
        fprintf(stderr, "not over boundry left\n");
        left_is_free = left_block->free;
    }
    if ((void*)right_b_tag > boundry_right) {
        fprintf(stderr, "over boundry right\n");
        right_is_free = false;
    } else {
        fprintf(stderr, "not over boundry right\n");
        right_is_free = right_block->free;
    }
    //fprintf(stderr, "check_04\n");
    fprintf(stderr, "this block: %p\n", this_block);
    fprintf(stderr, "this tag: %p\n", this_b_tag);
    fprintf(stderr, "left block: %p\n", left_block);
    fprintf(stderr, "left tag: %p\n", left_b_tag);
    fprintf(stderr, "right block: %p\n", right_block);
    fprintf(stderr, "right tag: %p\n", right_b_tag);
    fprintf(stderr, "left is free: %d\n", left_is_free);
    fprintf(stderr, "rght is free: %d\n", right_is_free);
    //only left
    if (left_is_free && !right_is_free) {
        fprintf(stderr, "left block is free\n");
        total_size = this_block->size + left_block->size + sizeof(metadata) + sizeof(b_tag);
        //left block and my tag
        left_block->size = total_size;
        this_b_tag->size = total_size;
        if (total_size > 4000000000) {
            fprintf(stderr, "-----------------wrong--------------------\n");
        }
        //free list
    }
    //only right
    if (right_is_free && !left_is_free) {
        fprintf(stderr, "right block is free\n");
        total_size = this_block->size + right_block->size + sizeof(metadata) + sizeof(b_tag);
        //my block and right tag
        this_block->size = total_size;
        right_b_tag->size = total_size;
        if (total_size > 4000000000) {
            fprintf(stderr, "-----------------wrong--------------------\n");
        }
        //update free list
            metadata* curr_head_free = head_free;
            metadata* last_free_entry = NULL;
            this_block->next_free = right_block->next_free;
            
            while(curr_head_free != NULL){
                if (curr_head_free == right_block) {
                    break;
                }
                last_free_entry = curr_head_free;
                curr_head_free = curr_head_free->next_free;
            }
            if (last_free_entry == NULL) {
                //the right block is head free
                head_free = this_block;
            } else {
                //just somewhere in the linked list
                last_free_entry->next_free = this_block;
                if (curr_head_free == NULL) {
                    this_block->next_free = NULL;
                }
            }
    }
    //left and right
    if (right_is_free && left_is_free) {
        fprintf(stderr, "both blocks are free\n");
        total_size = left_block->size + sizeof(b_tag) + sizeof(metadata) + this_block->size + sizeof(b_tag) + sizeof(metadata) + right_block->size;
        //left block and right tag
        left_block->size = total_size;
        right_b_tag->size = total_size;
        if (total_size > 4000000000) {
            fprintf(stderr, "-----------------wrong--------------------\n");
        }
        //update free list
            metadata* curr_head_free = head_free;
            metadata* last_free_entry = NULL;
            this_block->next_free = right_block->next_free;
            while(curr_head_free != NULL){
                if (curr_head_free == right_block) {
                    break;
                }
                last_free_entry = curr_head_free;
                curr_head_free = curr_head_free->next_free;
            }
            if (last_free_entry == NULL) {
                //the right block is head free
                head_free = right_block->next_free;
            } else {
                //just somewhere in the linked list
                last_free_entry->next_free = right_block->next_free;
                if (curr_head_free == NULL) {
                        this_block->next_free = NULL;
                }
            }

    }
    //only self
    if (!right_is_free && !left_is_free) {
        fprintf(stderr, "only this block is free\n");
        this_block->next_free = head_free;
        head_free = this_block;
    }
    //////////////////print lists///////////////////////
    print_lists();
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    print_lists();
    // implement realloc!
    if(ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    //realloc
    //get original size
    fprintf(stderr, "after size: %zu\n", size);
    metadata* old_block = (metadata*)((char*)ptr - sizeof(metadata));
    //b_tag* old_tag = (b_tag*)((char*)ptr + old_block->size);
    size_t old_size = old_block->size;
    if (old_size == size) {
        return ptr;
    }
    //*/
    //print_lists();
    // if shrink block: new b_tag -> update old block size
    //TODO: check remainging block: might be reusable
    fprintf(stderr, "old_size: %zu\n", old_size);
    fprintf(stderr, "size: %zu\n", size);
    if (old_size > size) {
        //fprintf(stderr, "shrink block\n");
        size_t size_threashold = size + sizeof(b_tag) + sizeof(metadata);
        if (old_size >= size_threashold) {
            b_tag* new_b_tag = (b_tag*)((char*)ptr + size);
            //fprintf(stderr, "new_b_tag %p\n", new_b_tag);p
            new_b_tag->free = false;
            new_b_tag->size = size;
            //update old block
            old_block->size = size;
            //check remaining block:check size -> new size -> new struct -> old tag
            int size_x = old_size - size - sizeof(b_tag);
            //fprintf(stderr, "size_x: %d\n", size_x);
            //print_lists();
            //fprintf(stderr, "second block oin the right\n");
            //new struct
            metadata* new_struct = (metadata*)((char*)old_block + sizeof(metadata) + new_b_tag->size + sizeof(b_tag));
            //fprintf(stderr, "new_struct: %p\n", new_struct);
            size_t size_y = size_x - sizeof(metadata);
            print_lists();
            new_struct->size = size_y;
            print_lists();
            //fprintf(stderr, "size_y %zu\n", size_y);
            new_struct->free = true;
            new_struct->next = head;
            //print_lists();
            head = new_struct;
            //new_struct->next_free = head_free;
            //head_free = new_struct;
            //update old tag
            b_tag* old_b_tag = (b_tag*)((char*)new_struct + sizeof(metadata) + size_y);
            old_b_tag->free = true;
            old_b_tag->size = size_y;
            //fprintf(stderr, "right_block %p\n", right_block);
            //combine with memory on the right
            //if right free -> get right size -> change this size -> update right b tag
            metadata* right_block = (metadata*)((char*)old_b_tag + sizeof(b_tag));
            fprintf(stderr, "right_block %p\n", right_block);
            //update free list
                fprintf(stderr, "update free list\n");
                int count_loop = 0;
                metadata* curr_head_free = head_free;
                metadata* last_free_entry = NULL;
                new_struct->next_free = right_block->next_free;
                fprintf(stderr, "check-11\n");
                while(curr_head_free != NULL){
                    fprintf(stderr, "free address: %p\n", curr_head_free);
                    if (curr_head_free == right_block) {
                        break;
                    }
                    if (count_loop > 1000) right_block->free =false;
                    count_loop++;
                    last_free_entry = curr_head_free;
                    curr_head_free = curr_head_free->next_free;
                }
            if (right_block->free && (int)right_block->size != 0) {
                fprintf(stderr, "right is free\n");
                //get right size
                size_t right_size = right_block->size;
                fprintf(stderr, "right_size %u\n", right_size);
                print_lists();
                //change this size
                new_struct->size += sizeof(b_tag) + sizeof(metadata) + right_size;
                //update right tag
                b_tag* right_tag = (b_tag*)((char*)right_block + sizeof(metadata) + right_size);
                right_tag->size = new_struct->size;
              
                fprintf(stderr, "check-12\n");
                if (last_free_entry == NULL) {
                    //the right block is head free
                    head_free = new_struct;
                } else {
                    //just somewhere in the linked list
                    last_free_entry->next_free = new_struct;
                    if (curr_head_free == NULL) {
                        this_block->next_free = NULL;
                    }
                }
            } else {
                //update free list
                fprintf(stderr, "update free without combining\n");
                new_struct->next_free = head_free;
                head_free = new_struct;
            }
        }
        return ptr;
    }
    //if expand size: 
    fprintf(stderr, "------------------expend size------------\n");
    //check around -> check total size -> memmove -> struct -> b tag
    metadata* this_block = old_block;
    b_tag* this_b_tag = (b_tag*)((char*)ptr + old_size);
    //get left & right info
    metadata* right_block = (metadata*)((char*)this_b_tag + sizeof(b_tag));
    b_tag* right_b_tag = (b_tag*)((char*)right_block + sizeof(metadata) + right_block->size);
    b_tag* left_b_tag = (b_tag*)((char*)this_block - sizeof(b_tag));
    metadata* left_block = (metadata*)((char*)left_b_tag - left_b_tag->size - sizeof(metadata));
    // bool left_is_free = left_block->free;
    // bool right_is_free = right_block->free;
    // size_t total_size = 0;
    metadata* new_block = NULL;
    b_tag* tag_block_space_left = NULL;
     fprintf(stderr, "this block: %p\n", this_block);
    fprintf(stderr, "left block: %p\n", left_block);
    fprintf(stderr, "left tag: %p\n", left_b_tag);
    fprintf(stderr, "left tag size: %d\n", (int)left_b_tag->size);
    fprintf(stderr, "right block: %p\n", right_block);
    fprintf(stderr, "right tag: %p\n", right_b_tag);
    fprintf(stderr, "boundry_left: %p\n", boundry_left);
    fprintf(stderr, "boundry_right: %p\n", boundry_right);
    bool left_is_free = false;
    bool right_is_free = false;
    size_t total_size = 0;
    //check boundries ////
    if ((void*)left_block < boundry_left || (void*)left_block > boundry_right) {
        fprintf(stderr, "over boundry left\n");
        left_is_free = false;
    } else {
        fprintf(stderr, "not over boundry left\n");
        left_is_free = left_block->free;
    }
    if ((void*)right_b_tag > boundry_right) {
        fprintf(stderr, "over boundry right\n");
        right_is_free = false;
    } else {
        fprintf(stderr, "not over boundry right\n");
        right_is_free = right_block->free;
    }
    //calcualte maximum possible size at current position
    //get new location based on left and right being free or not 
    if (!left_is_free && !right_is_free) {
        fprintf(stderr, "neither is free\n");
        total_size = old_size;
        new_block = old_block;
        tag_block_space_left = this_b_tag;
    } else if(left_is_free && !right_is_free) {
        //only left
        fprintf(stderr, "only left is free\n");
        total_size = left_block->size + sizeof(b_tag) + sizeof(metadata) + old_size;
        new_block = left_block;
        tag_block_space_left = this_b_tag;
    } else if(!left_is_free && right_is_free) {
        //only right
        fprintf(stderr, "only right is free\n");
        total_size = old_size + sizeof(b_tag) + sizeof(metadata) +right_block->size;
        new_block = old_block;
        tag_block_space_left = right_b_tag;
    } else if(left_is_free && right_is_free) {
        //both are free
        fprintf(stderr, "both left and right are free\n");
        total_size = left_block->size + sizeof(b_tag) + sizeof(metadata) + old_size + sizeof(b_tag) + sizeof(metadata) +right_block->size;
        new_block = left_block;
        tag_block_space_left = right_b_tag;
    }
    fprintf(stderr, "old block address: %p\n", old_block);
    fprintf(stderr, "old block size: %zu\n", old_block->size);
    //check if have to call malloc 
    //size_t total_size_requested = size + sizeof(metadata) + sizeof(b_tag);
    if (total_size >= size) {
        fprintf(stderr, "no need for malloc\n");
        //stay and move
        void* new_user_data = (void*)((char*)new_block + sizeof(metadata));
        memmove(new_user_data, (void*)(old_block+1), old_size);
        fprintf(stderr, "check-point-1\n");
        //update struct
        new_block->size = size;
        new_block->free = false;
        //remove new block from free list
        if(left_is_free) {
            // left or both
            metadata* curr_head_free = head_free;
            metadata* last_free_entry = NULL;
            fprintf(stderr, "check-point-1.2\n");
            fprintf(stderr, "left_block: %p\n", left_block);
            print_lists(); //seg fault: maybe left_block not reachable

            fprintf(stderr, "check-point-1.25\n");
            while(curr_head_free != NULL){
                if (curr_head_free == left_block) {
                    break;
                }
                last_free_entry = curr_head_free;
                curr_head_free = curr_head_free->next_free;
            }
            fprintf(stderr, "check-point-1.3\n");
            if (last_free_entry == NULL) {
                //the left block is head free
                head_free = left_block->next_free;
            } else {
                //just somewhere in the linked list
                last_free_entry->next_free = left_block->next_free;
            }
        } 
        fprintf(stderr, "check-point-1.5\n");
        if(right_is_free) {
            // right or both
            metadata* curr_head_free = head_free;
            metadata* last_free_entry = NULL;
            while(curr_head_free != NULL){
                if (curr_head_free == right_block) {
                    break;
                }
                last_free_entry = curr_head_free;
                curr_head_free = curr_head_free->next_free;
            }
            if (last_free_entry == NULL) {
                //the right block is head free
                head_free = right_block->next_free;
            } else {
                //just somewhere in the linked list
                last_free_entry->next_free = right_block->next_free;
            }
        } 
        fprintf(stderr, "check-point-2\n");
        //update b tag
        b_tag* new_b_tag = (b_tag*)((char*)new_block + sizeof(metadata) + size);
        new_b_tag->size = size;
        new_b_tag->free = false;
        //space left
        size_t size_threashold = size + sizeof(b_tag) + sizeof(metadata);
        //if space left > struct: -> newstruct -> free list
        int size_x = total_size - size - sizeof(b_tag);
        if (total_size >= size_threashold) {
            metadata* space_left_block = (metadata*)((char*)new_b_tag + sizeof(b_tag));
            size_t size_y = size_x - sizeof(metadata);
            space_left_block->size = size_y;
            space_left_block->free = true;
            //update free list
            space_left_block->next_free = head_free;
            head_free = space_left_block; 
            tag_block_space_left->free = true;
            tag_block_space_left->size = size_y;
        } else {
            //garbage space -> taken by new block
            new_block->size = total_size;
            tag_block_space_left->free = false;
            tag_block_space_left->size = total_size;
        }
        print_lists();
        //returns
        return (void*)((char*)new_block + sizeof(metadata));
    } else {
        //move to a new location
        fprintf(stderr, "move to a new location\n");
        print_lists();
        void* new_location = malloc(size);
        memmove(new_location, (void*)(old_block+1), old_size);
        fprintf(stderr, "checkcheck-01\n");
        //new struct
        new_block = (metadata*)((char*)new_location - sizeof(metadata));
        new_block->size = size;
        new_block->free = false;
        new_block->next = head;
        head = new_block;
        fprintf(stderr, "checkcheck-02\n");
        //new b tag
        b_tag* new_b_tag = (b_tag*)((char*)new_location + size);
        new_b_tag->size = size;
        new_b_tag->free = false;
        //free old block
        fprintf(stderr, "old block address: %p\n", old_block);
        fprintf(stderr, "old block size: %zu\n", old_block->size);
        print_lists();
        free(old_block+1);
        return new_location;
    }
}


void print_lists() {
    //print all list
    metadata* curr_head_print = head;
    while(curr_head_print != NULL){
        fprintf(stderr, "node address: %p\n", curr_head_print);
        curr_head_print = curr_head_print->next;
    }
    //prtin free list
    int count = 0;
    metadata* curr_head_print_free = head_free;
    while(curr_head_print_free != NULL){
        fprintf(stderr, "free size: %zu\n", curr_head_print_free->size);
        fprintf(stderr, "free address: %p\n", curr_head_print_free);
        if (count > 50) {
            fprintf(stderr, "----over 50 -------\n");
            break;
        }
        count++;
        curr_head_print_free = curr_head_print_free->next_free;
    }
    fprintf(stderr, "\n");
}