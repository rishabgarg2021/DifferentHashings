/* * * * * * * * *
 * Dynamic hash table using a combination of extendible hashing and cuckoo
 * hashing with a single keys per bucket, resolving collisions by switching keys
 * between two tables with two separate hash functions and growing the tables
 * incrementally in response to cycles
 *
 * created for COMP20007 Design of Algorithms - Assignment 2, 2017
 * by ...
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#define rightmostnbits(n, x) (x) & ((1 << (n)) - 1)

#include <math.h>
#include "xuckoo.h"

// a bucket stores a single key (full=true) or is empty (full=false)
// it also knows how many bits are shared between possible keys, and the first
// table address that references it
typedef struct bucket {
    int id;		// a unique id for this bucket, equal to the first address
    int depth;	// how many hash value bits are being used by this bucket
    bool full;	// does this bucket contain a key
    int64 key;	// the key stored in this bucket
} Bucket;

// an inner table is an extendible hash table with an array of slots pointing
// to buckets holding up to 1 key, along with some information about the number
// of hash value bits to use for addressing
typedef struct inner_table {
    Bucket **buckets;	// array of pointers to buckets
    int size;			// how many entries in the table of pointers (2^depth)
    int depth;			// how many bits of the hash value to use (log2(size))
    int nkeys;			// how many keys are being stored in the table
} InnerTable;

// a xuckoo hash table is just two inner tables for storing inserted keys
struct xuckoo_table {
    InnerTable *table1;
    InnerTable *table2;
    int time;
};


//function protypes//
void initialize_table1(XuckooHashTable *table);
void initialize_table2(XuckooHashTable *table);
int  table_t_size(XuckooHashTable  *table,int toggle);
int hash_x(int table_number, int64 key);
int table_depth(XuckooHashTable *table,int toggle);
int table_inuse(XuckooHashTable *table,int toggle,int hash_index);
void insert_key(XuckooHashTable *table,int toggle,int hash_index,int key);
int get_hash_value_x(XuckooHashTable *table,int table_number,int index);
void replace_slot_hashtable_x( XuckooHashTable*table,int table_number,int64 key,
                              int hash_index);
int rightn_bits(XuckooHashTable *table,int toggle, int64 key);
static Bucket *new_bucket(int first_address, int depth);
InnerTable* reinsert_key_x(InnerTable *table, int64 key,int toggle);
static void double_tables(XuckooHashTable *table,int toggle);
InnerTable* split_table(XuckooHashTable *Table,InnerTable *table,
						int address,int toggle);
void insertkey(XuckooHashTable*table,int toggle,int64 key);







//both the initialize function mallocs the both tables with size as 1 
//and one key per bucket.
void initialize_table1(XuckooHashTable *table){
	table->table1=malloc(sizeof(InnerTable));
    table->table1->size=1;
    table->table1->depth=0;
    table->table1->nkeys=0;
    table->table1->buckets=malloc(sizeof(*table->table1->buckets) );
    table->table1->buckets[0]=malloc(sizeof **table->table1->buckets );
    table->table1->buckets[0]->depth=0;
    table->table1->buckets[0]->full=0;
    table->table1->buckets[0]->id=0;
    table->table1->buckets[0]->full=false;

    
    
}
void initialize_table2(XuckooHashTable *table){
    table->table2=malloc(sizeof(InnerTable));
    table->table2->size=1;
    table->table2->depth=0;
    table->table2->nkeys=0;
    table->table2->buckets=malloc(sizeof(*table->table2->buckets) );
    table->table2->buckets[0]=malloc(sizeof **table->table2->buckets );
    table->table2->buckets[0]->depth=0;
    table->table1->buckets[0]->full=0;
    table->table1->buckets[0]->id=0;
    table->table2->buckets[0]->full=false;
    
}
// initialise an extendible cuckoo hash table
XuckooHashTable *new_xuckoo_hash_table() {
    XuckooHashTable *table=malloc(sizeof (*table));
    assert(table);
    table->time=0;
    initialize_table1(table);
    initialize_table2(table);
    
    
    return table;
}


// free all memory associated with 'table'
void free_xuckoo_hash_table(XuckooHashTable *table) {
	  // (if we loop through forwards, we wouldn't know which reference was lst)
    
    int i;
    for (i = table->table1->size-1; i >= 0; i--) {
        if (table->table1->buckets[i]->id == i) {
            free(table->table1->buckets[i]);
        }
    }
    
    // free the array of bucket pointers
    free(table->table1->buckets);
    
    // free the table1 struct itself
    free(table->table1);
    int j;
    for (j= table->table2->size-1; j >= 0; j--) {
        if (table->table2->buckets[j]->id == j) {
            free(table->table2->buckets[j]);
        }
    }
    
    // free the array of bucket pointers
    free(table->table2->buckets);
    
    // free the table struct itself
    free(table->table2);
    free(table);
}

int  table_t_size(XuckooHashTable  *table,int toggle){
    if(toggle==1){
        return table->table1->size;
    }
    return table->table2->size;
    
}
int hash_x(int table_number, int64 key)
{
    
    if(table_number==1){
        return h1(key);
    }
    return h2(key);
}

//calculate the depth of table according to given toggle number.
int table_depth(XuckooHashTable *table,int toggle){
    if(toggle==1){
        return table->table1->depth;
    }
    return table->table2->depth;
}


//return 1 if that hash index of that toggle is filled else 0.
int table_inuse(XuckooHashTable *table,int toggle,int hash_index){
    
    if(toggle==1){
        if(!table->table1->buckets[hash_index]->full){
            return 0;
        }
        
    }
    else if(toggle==2){
        if(!table->table2->buckets[hash_index]->full){
            return 0;
        }
    }
    return 1;
}


//insert key in given toggle number.
void insert_key(XuckooHashTable *table,int toggle,int hash_index,int key){
    
    if(toggle==1){
        table->table1->buckets[hash_index]->key=key;
        table->table1->buckets[hash_index]->full=true;
        table->table1->nkeys++;
        
    }
    
    if(toggle==2){
        table->table2->buckets[hash_index]->key=key;
        table->table2->buckets[hash_index]->full=true;
        table->table2->nkeys++;
        
    }    
}

//calculate the hashvalue existing at that table toggles index.
int get_hash_value_x(XuckooHashTable *table,int table_number,int index){
    if(table_number==1){
        return table->table1->buckets[index]->key;
    }
    return table->table2->buckets[index]->key;
    
}

//just put the key accoring to table number to hash index 
void replace_slot_hashtable_x( XuckooHashTable*table,int table_number,int64 key,
                              int hash_index){
    if(table_number==1){
        table->table1->buckets[hash_index]->key=key;
        table->table1->buckets[hash_index]->full=true;
        
    }
    else if(table_number==2){
        table->table2->buckets[hash_index]->key=key;
        table->table2->buckets[hash_index]->full=true;
        
    }
}


//gives the righmost table toggle depth bits of hash value of given key. 
int rightn_bits(XuckooHashTable *table,int toggle, int64 key){
    int depth=table_depth(table,toggle);
    int hash_value=hash_x(toggle,key);
    return rightmostnbits(depth,hash_value);
}


//creates new bucket with given depth and id as parameter.
static Bucket *new_bucket(int first_address, int depth) {
    Bucket *bucket = malloc(sizeof *bucket);
    assert(bucket);
    
    bucket->id = first_address;
    bucket->depth = depth;
    bucket->full = false;
    
    return bucket;
}


//insert key at table toggle address.
InnerTable* reinsert_key_x(InnerTable *table, int64 key,int toggle) {
    int address=0;
    if(toggle==1){
        address=rightmostnbits(table->depth, h1(key));
    }
    if(toggle==2){
        address = rightmostnbits(table->depth, h2(key));
        
    }
    
    table->buckets[address]->key = key;
    table->buckets[address]->full = true;
    return table;
}



//copies both the pointers of both tables after doubling the size and 
//increment its depth by 1 and size double.
static void double_tables(XuckooHashTable *table,int toggle) {
    int size;
    int i,j;
    if(toggle==1){
        int size=table->table1->size*2;
        assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");
        table->table1->buckets=realloc(table->table1->buckets,
        						(sizeof *table->table1->buckets)*size);
        assert(table->table1->buckets);
        for (i = 0; i < table->table1->size; i++) {
            table->table1->buckets[table->table1->size + i] = 
            										table->table1->buckets[i];
            
        }
        table->table1->size=size;
        table->table1->depth++;
        
    }
    else{
        size=table->table2->size*2;
        assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");
        table->table2->buckets=realloc(table->table2->buckets,
        							(sizeof *table->table2->buckets)*size);
        for (j = 0; j < table->table2->size; j++) {
            table->table2->buckets[table->table2->size + j] 
            										= table->table2->buckets[j];
        }
        table->table2->size=size;
        table->table2->depth++;
    }
    
}



InnerTable* split_table(XuckooHashTable *Table,InnerTable *table,int address
						,int toggle){
    // create a new bucket and update both buckets' depth
    if(table->buckets[address]->depth==table->depth){
        double_tables(Table,toggle);
    }
  //  printf("\ndepth%d\n",table->depth);
    Bucket *bucket=table->buckets[address];
    int depth= bucket->depth;
    int first_address=bucket->id;
    int new_depth = depth + 1;
    bucket->depth = new_depth;
    // new bucket's first address will be a 1 bit plus the old first address
    int new_first_address = 1 << depth | first_address;
    
    Bucket *newbucket = new_bucket(new_first_address, new_depth);
    
    // THIRD,
    // redirect every second address pointing to this bucket to the new bucket
    // construct addresses by joining a bit 'prefix' and a bit 'suffix'
    // (defined below)
    
    // suffix: a 1 bit followed by the previous bucket bit address
    int bit_address = rightmostnbits(depth, first_address);
    int suffix = (1 << depth) | bit_address;
    
    // prefix: all bitstrings of length equal to the difference between the new
    // bucket depth and the table depth
    // use a for loop to enumerate all possible prefixes less than maxprefix:
    int maxprefix = 1 << (table->depth - new_depth);
    
    int prefix;
    for (prefix = 0; prefix < maxprefix; prefix++) {
        
        // construct address by joining this prefix and the suffix
        int a = (prefix << new_depth) | suffix;
        
        // redirect this table entry to point at the new bucket
        table->buckets[a] = newbucket;
    }
    
    int64 key = bucket->key;
    
    bucket->full = false;
    table =reinsert_key_x(table, key,toggle);
    return table;
    
    
    
    
    
    
}


//it inserts the key at table toggle address of that key and increment count of
//nkeys.
void insertkey(XuckooHashTable*table,int toggle,int64 key){
    if(toggle==1){
        int address1 = rightmostnbits(table->table1->depth, h1(key));
        table->table1->buckets[address1]->key=key;
        table->table1->buckets[address1]->full=true;
        table->table1->nkeys++;
    }
    
    else{
        int address2 = rightmostnbits(table->table2->depth, h2(key));
        table->table2->buckets[address2]->key=key;
        table->table2->buckets[address2]->full=true;
        table->table2->nkeys++;
    }
    
    
}
bool xuckoo_hash_table_insert(XuckooHashTable *table, int64 key) {
    // fprintf(stderr, "not yet implemented\n");
 
    int start_time = clock(); // start timing
    if(xuckoo_hash_table_lookup(table, key) ){
    	table->time += clock() - start_time;
        return false;
    }
    int count=0;
    int break_probing=200;//its break point set to avoid cycles.
    int toggle;
    //key is tried to insert in table1 if it has same or less number of keys
    //then table2 else it tries to insert in table2 if its empty/
    if(table->table1->nkeys<=table->table2->nkeys){
        toggle=1;
    }
    else{
        toggle=2;
    }
    
    int hash_index=rightmostnbits( table_depth(table,toggle)
    								,hash_x(toggle,key));
    int value;
    
    
    
    //performs cuckoo with given toogle set according to less number of keys
    //and breaks if it probing occurs more tham 200,(mostly indicating cycle).
    while(count<break_probing ){
    	
    	//inserts if it founds the empty place.
        if(!table_inuse(table,toggle,hash_index)){
            insert_key(table,toggle,hash_index,key);
            table->time += clock() - start_time;
            return true;
        }
        else{
            
            
            value=get_hash_value_x(table,toggle,rightn_bits(table,toggle,key));
            replace_slot_hashtable_x(table,toggle,key,
                                     rightn_bits(table, toggle, key));
            count++;
            
            
        }
        key = value;
        if(toggle==1){
            toggle=2;
            hash_index=rightmostnbits(table_depth(table,toggle),
            							hash_x(toggle,value));
        }
        else if(toggle==2){
            toggle=1;
            hash_index=rightmostnbits(table_depth(table,toggle),
            							hash_x(toggle,value));
        }
        
    }
    int hash_index1=rightmostnbits( table_depth(table,1)  ,hash_x(1,key));
    int hash_index2=rightmostnbits( table_depth(table,2)  ,hash_x(2,key));
    int difference1=table->table1->depth-table->table1->		
    				buckets[hash_index1]->depth;
    int difference2=table->table2->depth-table->table2
    				->buckets[hash_index2]->depth;
  
    while(1==1){
    
    	
	  	if(!table->table2->buckets[hash_index2]->full){
	  		insertkey(table, 2, key);
	  		 table->time += clock() - start_time;
			return true;
	  	}
	  	if(!table->table1->buckets[hash_index1]->full){
	  		insertkey(table, 1, key);
	  		 table->time += clock() - start_time;
			return true;
	  	}
	  	
	  //difference gives the intuation of splitting the table or not.
	  //if its zero double occurs. if its more than zero splitting occurs till
	  //it difference is set to 0.more difference more split less chance double
	  //if difference is not zero its better to go to that table as double cant 
	  //occur.if diiference is zero double the table with less number of keys.
	  //if if both difference are not zero, split which has more difference.
	  	if((((difference1==0)&&difference2!=0)) ||((difference1-difference2==0) 
	  			&&table->table1->size>table->table2->size)
	  			||difference2>difference1) 
	  		{
            
           
            while(table->table2->buckets[hash_index2]->full){
                
                
                table->table2=split_table(table,table->table2,hash_index2,2);
                hash_index2=rightmostnbits( table_depth(table,2)
                							,hash_x(2,key));
                hash_index1=rightmostnbits( table_depth(table,1)
                							,hash_x(1,key));
                
                difference1=table->table1->depth-
                			table->table1->buckets[hash_index1]->depth;
                difference2=table->table2->depth-
                			table->table2->buckets[hash_index2]->depth;
                //try to check if after splitting you get empty slot for that 
                //hash value
                if(!table->table2->buckets[hash_index2]->full ){
                    insertkey(table, 2, key);
                     table->time += clock() - start_time;
                    return true;
                }
                //there might be chance to have empty buckets if you rehash to 
                //same address after doubling the table. perform cuckoo again.
                if(xuckoo_hash_table_insert(table, key)){
                	 table->time += clock() - start_time;
                	 return true;
                }
            }
         }
       if(((difference2==0 && difference1!=0) 
       	   ||((difference1-difference2==0) && 
       	   	   table->table2->size>=table->table1->size) 
       	   || difference1>difference2))
       	{
           while(table->table1->buckets[hash_index1]->full){
                table->table1=split_table(table,table->table1,hash_index1,1);
                hash_index2=rightmostnbits( table_depth(table,2)
                			,hash_x(2,key));
                hash_index1=rightmostnbits( table_depth(table,1)
                			,hash_x(1,key));
                
                difference1=table->table1->depth-
                			table->table1->buckets[hash_index1]->depth;
                difference2=table->table2->depth-
                			table->table2->buckets[hash_index2]->depth;
                
                
                //try to check if after splitting you get empty slot for that 
                //hash value
                if(!table->table1->buckets[hash_index1]->full ){
                    insertkey(table, 1, key);
                     table->time += clock() - start_time;
                    return true;
                }
                  //there might be chance to have empty buckets if you rehash to 
                //same address after doubling the table. perform cuckoo again.
                if(xuckoo_hash_table_insert(table, key)){
                	 table->time += clock() - start_time;
					return true;
                }
            }
        }
    }
    table->time += clock() - start_time;
    return true;
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool xuckoo_hash_table_lookup(XuckooHashTable *table, int64 key) {
    //fprintf(stderr, "not yet implemented\n");
    int address1 = rightmostnbits(table->table1->depth, h1(key));
    int address2=  rightmostnbits(table->table2->depth, h2(key));
    if(table->table1->buckets[address1]->key==key){
        if(table->table1->buckets[address1]->full){
            return true;
        }
    }
    if(table->table2->buckets[address2]->key==key){
        if(table->table2->buckets[address2]->full){
            return true;
        }
    }
    
    return false;
}


// print the contents of 'table' to stdout
void xuckoo_hash_table_print(XuckooHashTable *table) {
    assert(table != NULL);
    
    printf("--- table ---\n");
    
    // loop through the two tables, printing them
    InnerTable *innertables[2] = {table->table1, table->table2};
    int t;
    for (t = 0; t < 2; t++) {
        // print header
        printf("table %d\n", t+1);
        
        printf("  table:               buckets:\n");
        printf("  address | bucketid   bucketid [key]\n");
        
        // print table and buckets
        int i;
        for (i = 0; i < innertables[t]->size; i++) {
            // table entry
            printf("%9d | %-9d ", i, innertables[t]->buckets[i]->id);
            
            // if this is the first address at which a bucket occurs, print it
            if (innertables[t]->buckets[i]->id == i) {
                printf("%9d ", innertables[t]->buckets[i]->id);
                if (innertables[t]->buckets[i]->full) {
                    printf("[%llu]", innertables[t]->buckets[i]->key);
                } else {
                    printf("[ ]");
                }
            }
            
            // end the line
            printf("\n");
        }
    }
    printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
void xuckoo_hash_table_stats(XuckooHashTable *table) {
	printf("Statistic are given below |\n");
	printf("Total keys inserted till now=%d",table->table1->nkeys+
												table->table2->nkeys);
	
    printf("\nTotal keys in table1 = %d",table->table1->nkeys);
    printf("\nload factor for table1 =%.3f% %",((table->table1->nkeys+0.0)
    										/(table->table1->size+0.0))*100);
    printf("\nload factor for table2 =%.3f% %",((table->table2->nkeys+0.0)
    										/(table->table2->size+0.0))*100);
    printf("\nTotal keys in table2 = %d",table->table2->nkeys);
	printf("\nDepth of table1 =%d",table->table1->depth);
	printf("\nDepth of table2 =%d",table->table2->depth);
	printf("\nSize of table1 =%d",table->table1->size);
	printf("\nSize of table2=%d\n",table->table2->size);
	float seconds = table->time * 1.0 / CLOCKS_PER_SEC;
	printf("CPU time spent: %.6f sec\n", seconds);
	
    return;
}
