#include "cachelab.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

//comment here is testing how vim works

int check_cache(long address, int set_bits, int assoc, int offset_bits);
void add_block_queue(int set);
void move_head_to_rear(int set);
int sets;
int assoc;
int bytes; //bytes per block
int eviction_count = 0;

typedef struct Block{
    long tag;
    int valid_bit;
    struct Block *next;
    struct Block *prev;
} block;

typedef struct Block_Queue {
    block *start;
    block *rear;
} block_queue;

typedef struct Set{
    unsigned int index;
    block_queue *my_queue;
} set;

set *my_cache;

//intialize the cache
void create_cache(int set_bits, int assoc, int offset_bits){
    sets = 1 << set_bits; //number of sets in the cache = 2 ^ set_bits
    assoc = assoc; //number of lines in each set = assosciativity
    bytes = 1 << offset_bits; // bytes per cache block is equal to 2^offset_bits

    printf("my_cache: %p\n", my_cache);
    my_cache = (set *) malloc(sets * sizeof(set)); //need space equal to the size of the set times the number of sets
    printf("my_cache: %p\n", my_cache);

    //for each set that we have created we need to create the block_queue and then the blocks for each queue
    int i, j;
    for(i = 0; i < sets; i++){
        my_cache[i].index = i;
        my_cache[i].my_queue = (block_queue *) malloc(sizeof(block_queue)); //create a block queue
        my_cache[i].my_queue->start = (block *) malloc(sizeof(block)); //each block queue needs one initial block
        my_cache[i].my_queue->rear = my_cache[i].my_queue->start; //start/rear point to the same initial block
        // initialize the attributes for the single block in the linked list
        my_cache[i].my_queue->start->prev = NULL; 
        my_cache[i].my_queue->start->next = NULL;
        my_cache[i].my_queue->start->valid_bit = -1;
        my_cache[i].my_queue->start->tag = -1;
        // add blocks for the correct number of lines per set
        for( j = 0; j < assoc - 1; j++){
            add_block_queue(i); //function to add blocks to queue
        }
    }
}

//this function takes a set index and adds a new block to the end of the queue
void add_block_queue(int set){
    block *last = my_cache[set].my_queue->rear;
    block *new_rear = (block *) malloc(sizeof(block)); //adding this block
    new_rear->next = NULL; //new rear will point to NULL
    new_rear->prev = last; //prev will point to the previous rear
    new_rear->valid_bit = -1; 
    new_rear->tag = -1;
    last->next = new_rear; //previous rear will point to the new rear
    my_cache[set].my_queue->rear = new_rear; //adjust the queue to point to new rear
}

void move_head_to_rear(int set_index) {
    block *start_block = my_cache[set_index].my_queue->start; //current start of the queue
    block *rear_block = my_cache[set_index].my_queue->rear; //current end of the queue
    block *new_header = start_block->next; //this will be the new header

    new_header->prev = NULL; //the header must have prev field point to NULL
    if (new_header->next == NULL){ //if the new header is currently the rear then it needs to point to the current start block
        new_header->next = start_block; 
    }
    //make the header the rear
    start_block->prev = rear_block; // prev field should point to the current rear
    rear_block->next = start_block;
    start_block->next = NULL; //next should point to NULL
    my_cache[set_index].my_queue->rear = start_block; //update the rear in queue
    my_cache[set_index].my_queue->start = new_header; //update the start in queue
}
/* will check the cache for a hit*/
int check_cache( long address, int set_bits, int assoc, int offset_bits) {
    long tag_mask = ~(~0 << (64 - set_bits - offset_bits)); //mask of just the tag bits
    int set_mask = ~(~0 << set_bits); //mask of the set bits
    long tag_bits = (address >> (set_bits + offset_bits)) & tag_mask; //the actual tag bits
    int set_index = (address >> offset_bits) & set_mask; //the actual set bits
    block *start_block = my_cache[set_index].my_queue->start; //current start of the queue
    block *rear_block = my_cache[set_index].my_queue->rear; //end of the queue
    block *trav = start_block; //used to traverse the queue
    
    //traverse the queue
    while (trav != NULL) {
        // if the tag matches and the bit is valid then we have a hit
        if ((trav->tag == tag_bits) && (trav->valid_bit == 1)) {
            // check if the block is the rear block or assoc == 1 and then return
            if ((assoc == 1) || (trav == rear_block)){
                return 1;
            } else {
                //check if there is still an empty line
                if (trav->next->valid_bit != 1){
                    return 1;
                }
                //check if we landed on the start of the queue
                if (trav == start_block) {
                    move_head_to_rear(set_index);
                } else {
                    // we have a node somewhere in the middle of the queue, we need to move the node to
                    // the end of the queue and connect its prev and next nodes together
                    trav->prev->next = trav->next; 
                    trav->next->prev = trav->prev;
                    trav->next = NULL;
                    trav->prev = rear_block;
                    rear_block->next = trav;
                    my_cache[set_index].my_queue->rear = trav;
                } return 1;
            }
        }
        trav = trav->next; //else we move to the next node
    }
    trav = start_block; //reset trav to the start of the queue
    while (trav != NULL) {
        // check if any of the blocks are free
        if (trav->valid_bit != 1){
            trav->tag = tag_bits;
            trav->valid_bit = 1;
            return 0;
        } trav = trav->next;
    }
    //if we get here, we need to evict the front of the queue

    //this is the case of if the queue is 1 node
    if (assoc == 1){
        //adjust tag fields
        start_block->valid_bit = 1;
        start_block->tag = tag_bits;
    } else {
        move_head_to_rear(set_index);
        //adjust tag fields
        start_block->tag = tag_bits; 
        start_block->valid_bit = 1; 
    }
    printf(" eviction ");
    eviction_count ++; //update the eviction count
    return 0;
}



int main(int argc, char **argv)
{
    FILE *fp;
    long mem_add = 0;
    char instruct = 0;
    int size = 0;
    char opt = 0;
    int set_bits = 0;
    int associativity = 0;
    int offset_bits = 0;
    char *trace_file = NULL;
    int hit_count = 0;
    int miss_count= 0;

    
    while(1){
        opt = getopt(argc, argv, "vhs:E:t:b:");
        if(opt == -1) break;
        switch(opt){
            case 's':
                set_bits = atoi(optarg);
                break;
            case 'E':
                associativity = atoi(optarg);
                break; 
            case 'b':
                offset_bits = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            default:
                break;
        }
    }
    
    printf("%d, %d, %d, %s \n", set_bits, associativity, offset_bits, trace_file);
   
    create_cache(set_bits, associativity, offset_bits);

    fp = fopen(trace_file, "r");
    while (fscanf(fp, " %c %lx,%d ", &instruct, &mem_add, &size) > 0){
        
        //printf("%c\n", instruct);
        //printf("%x\n", mem_add);
        //printf("%d\n",size);

        
        switch(instruct){
            case 'L':
                printf("%c %lx,%d ", instruct, mem_add, size);
                if (check_cache(mem_add, set_bits, associativity, offset_bits)){
                    hit_count++;
                    printf("hit\n");
                } else {
                    printf("miss\n");
                    miss_count++;
                }
                break;
            case 'S':
                printf("%c %lx,%d ", instruct, mem_add, size);
                if (check_cache(mem_add, set_bits, associativity, offset_bits)){
                    printf("hit\n");
                    hit_count++;
                } else {
                    printf("miss\n");
                    miss_count++;
                }
                break;
            case 'M':
                printf("%c %lx,%d ", instruct, mem_add, size);
                if (check_cache(mem_add, set_bits, associativity, offset_bits)){
                    printf("hit ");
                    //fflush(stdout);
                    hit_count++;
                } else {
                    printf("miss " );
                    //fflush(stdout);
                    miss_count++;
                }
                if (check_cache(mem_add, set_bits, associativity, offset_bits)){
                    printf("hit\n");
                    hit_count++;
                } else {
                    printf("miss\n");
                    miss_count++;
                }
            default:
                break;
        }
    }
    fclose(fp);

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
  }

