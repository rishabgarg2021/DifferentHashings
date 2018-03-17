/* * * * * * * * *
 * Dynamic hash table using cuckoo hashing, resolving collisions by switching
 * keys between two tables with two separate hash functions
 *
 * created for COMP20007 Design of Algorithms - Assignment 2, 2017
 * by ...
 */

#include <stdio.h>
#include <stdlib.h>
#include<math.h>
#include <assert.h>
#include<time.h>
#include "cuckoo.h"

#define BLANK -1
// an inner table represents one of the two internal tables for a cuckoo
// hash table. it stores two parallel arrays: 'slots' for storing keys and
// 'inuse' for marking which entries are occupied
typedef struct inner_table {
	int64 *slots;	// array of slots holding keys
	bool  *inuse;	// is this slot in use or not?
} InnerTable;

// a cuckoo hash table stores its keys in two inner tables
struct cuckoo_table {
	InnerTable *table1; // first table
	InnerTable *table2; // second table
	int size;// size of each table
    int load1;//load1 carries number of keys in table1.
    int load2;//load2 carries number of keys in table2.
    int time; 
    
    
};


//function Prototypes//
void initialize_tables(CuckooHashTable *table,int size);
int64 hash(int function, int64 key);
int64 get_hash_index(int table_number,int64 key,int size);
void insert_slot_hashtable(CuckooHashTable *table,int table_number,int64 key,
                           int64 hash_index);

void replace_slot_hashtable(CuckooHashTable *table,int table_number,int64 key,
                           int64 hash_index);

int  check_inuse(CuckooHashTable *table,int table_number,int64 hash_index);
int64 get_hash_value(CuckooHashTable *table,int table_number,int index);

void insert_slot_hashtable(CuckooHashTable *table,int table_number,int64 key,
                           int64 hash_index);

int next_prime(int n);
void double_cuckoo_table(CuckooHashTable *table);



//it allocates the space to tabe with given size as numbe of slots and inuse 
//inboth tables.
void initialize_tables(CuckooHashTable *table,int size){
    assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");
    table->table1=malloc(sizeof(InnerTable));
    table->table2=malloc(sizeof(InnerTable));
    table->table1->slots=malloc((sizeof(*table->table1->slots))*size);
    assert(table->table1->slots);
    table->table2->slots=malloc((sizeof(*table->table2->slots))*size);
    assert(table->table2->slots);
    table->table1->inuse=malloc((sizeof *table->table1->inuse)*size);
    assert(table->table1->inuse);
    table->table2->inuse=malloc((sizeof *table->table2->inuse)*size);
    assert(table->table1->inuse);
    int i;
    for (i = 0; i < size; i++) {
        table->table1->inuse[i] = false;
        table->table2->inuse[i]=false;
    }
    

}



// initialise a cuckoo hash table with 'size' slots in each table
CuckooHashTable *new_cuckoo_hash_table(int size) {
    CuckooHashTable *table=malloc(sizeof (*table));
    assert(table);
    table->time=0;
    table->load1=0;
    table->load2=0;
    table->size=size;
    initialize_tables(table,size);
    return table;
}




// free all memory associated with 'table'
void free_cuckoo_hash_table(CuckooHashTable *table) {
	free(table->table1->slots);
	free(table->table1->inuse);
	free(table->table2->slots);
	free(table->table2->inuse);
	free(table->table1);
	free(table->table2);
	free(table);
	
}


//returns the hash value accoring to given function i.e. table number.
int64 hash(int function, int64 key)
{
    
    if(function==1){
      return h1(key);
    }
    return h2(key);
    
}


//gives the hash address according to table number provided.
int64 get_hash_index(int table_number,int64 key,int size){
    return (hash((table_number),key))%size;
}


// insert 'key' into 'table' in table number mentioned and makes it inuse.
void insert_slot_hashtable(CuckooHashTable *table,int table_number,int64 key,
                           int64 hash_index){
    if(table_number==1){
        table->table1->slots[hash_index]=key;
        table->table1->inuse[hash_index]=true;
        table->load1++;
    }
    else if(table_number==2){
        table->table2->slots[hash_index]=key;
        table->table2->inuse[hash_index]=true;
        table->load2++;
        
    }
}



//it just puts the key into given table number without incrementing count.
void replace_slot_hashtable(CuckooHashTable *table,int table_number,int64 key,
                           int64 hash_index){
    if(table_number==1){
        table->table1->slots[hash_index]=key;
        table->table1->inuse[hash_index]=true;
        
    }
    else if(table_number==2){
        table->table2->slots[hash_index]=key;
        table->table2->inuse[hash_index]=true;
        
    }
}


//checks if given table numbers hash_index is empty or not.
//return true if its not empty.
int  check_inuse(CuckooHashTable *table,int table_number,int64 hash_index){
    if(table_number==1){
        return table->table1->inuse[hash_index];
    }
 
    return table->table2->inuse[hash_index];
   
}


//calculate the hash value from index of mentioned table_number.
int64 get_hash_value(CuckooHashTable *table,int table_number,int index){
    if(table_number==1){
        return table->table1->slots[index];
    }
    return table->table2->slots[index];
    
}


//calculates the next prime number after n.
//its useful when i am doubling the table size to next prime of double the 
//size of table.

int is_prime (int n){
	int i = 0;
	for (i = 2; i < n; ++i){
		if (n % i == 0){
			return 0;
		}
	}
	return 1;
}
int next_prime(int n){
	int i;
	 for(i=n+1;;i++)
    {
        if (is_prime (i)){
        	return i;
        }
    }
     assert( 2*i < MAX_TABLE_SIZE && "error: table has grown too large!");
	
}




void double_cuckoo_table(CuckooHashTable *table){
	//both tables inuse and slots are stored to reinsert it when we increase the 
	//table size.
    int64 *old_slots1;
    old_slots1=table->table1->slots;
    bool *old_inuse1;
    old_inuse1=table->table1->inuse;
    int64 *old_slots2;
    old_slots2=table->table2->slots;
    bool* old_inuse2;
    old_inuse2=table->table2->inuse;
    int old_size=table->size;
    //have made size of table to next prime of double size of table for less
    //collisions.
    table->size=next_prime((table->size)*2);
    assert(table->size < MAX_TABLE_SIZE && "error: table has grown too large!");
    table->load1=0;
    table->load2=0;
    //increase the table size to new size 
    initialize_tables(table,table->size);
    int i;
    int64 key;
    //reinsert all the values stored from both the tables before increasing 
   // the size.
    for(i=0;i<old_size;i++){
        if(old_inuse1[i]){
            key=old_slots1[i];
            cuckoo_hash_table_insert(table,key);
        }
    }
    int j;
    for(j=0;j<old_size;j++){
        if(old_inuse2[j]){
            key=old_slots2[j];
            cuckoo_hash_table_insert(table,key);
        }
    }
}

 


bool cuckoo_hash_table_insert(CuckooHashTable *table, int64 key) {
    int toggle=1;
    int size=table->size;
    int64 hash_index=get_hash_index(toggle,key,size);
    int64 value=key;
    int total=0;
    int BREAK_CYCLE =2*table->size-1;//it is checking maximum number of 
    //probing possible to avoid cycle checking.
    int start_time = clock(); // start timing
    if(cuckoo_hash_table_lookup(table, key)){
    	table->time += clock() - start_time;
        return false;
    }
    
    //if key is not present in table 1 put it else put the new key
    //in that slot  and insert the removed key in table2.
    //if still key is present there , put the removed key from table1 and try to 
    //the removed key from table2 to table1. Repeat the steps until we break the 
    //cycle or insert it into any of table.
    if(!table->table1->inuse[hash_index]){
        insert_slot_hashtable(table,toggle,key ,get_hash_index(toggle,key,size));
        return true;
    }
    while (1==1 ){
       
        if ( total > BREAK_CYCLE
        	||(table->load1+table->load2) +1 >= 2*table->size){
      
            
            break;
        }
        if(!check_inuse(table,toggle,get_hash_index(toggle,key,size))){
            insert_slot_hashtable(table,toggle,key 
            					  ,get_hash_index(toggle,key,size));
            table->time += clock() - start_time;
            return true;
        }
        else{
            value=get_hash_value(table,toggle,
            					get_hash_index(toggle,key,size));
            
            replace_slot_hashtable(table,toggle,key,
            						get_hash_index(toggle,key,size));
            
            total++;
        }
        key=value;
        
        //changes the toggle as it now try to insert the key removed from
       	// any of the table to other table 
        if(toggle==1){
            toggle=2;
        }
        else if(toggle==2){
            toggle=1;
        }
        
        }
    double_cuckoo_table(table);
   
    return cuckoo_hash_table_insert(table,key);

}




bool cuckoo_hash_table_lookup(CuckooHashTable *table, int64 key) {
    int size;
    size=table->size;
    int64 hash_value1,hash_value2;
    hash_value1=(hash(1,key))%size;
    hash_value2=(hash(2,key))%size;
    //return true if we found the lookup is successful of that key
    if(table->table1->inuse[hash_value1] && table->table1->slots[hash_value1]==key){
        return true;
    }
    else if(table->table1->inuse[hash_value1] && table->table2->slots[hash_value2]==key){
        return true;
    }
	return false;
}




// print the contents of 'table' to stdout
void cuckoo_hash_table_print(CuckooHashTable *table) {
	assert(table);
	printf("--- table size: %d\n", table->size);

	// print header
	printf("                    table one         table two\n");
	printf("                  key | address     address | key\n");
	
	// print rows of each table
	int i;
	for (i = 0; i < table->size; i++) {

		// table 1 key
		if (table->table1->inuse[i]) {
			printf(" %20llu ", table->table1->slots[i]);
		} else {
			printf(" %20s ", "-");
		}

		// addresses
		printf("| %-9d %9d |", i, i);

		// table 2 key
		if (table->table2->inuse[i]) {
			printf(" %llu\n", table->table2->slots[i]);
		} else {
			printf(" %s\n",  "-");
		}
	}

	// done!
	printf("--- end table ---\n");
}




// print some statistics about 'table' to stdout
void cuckoo_hash_table_stats(CuckooHashTable *table) {
    assert(table != NULL);
    printf("--- table stats ---\n");
    printf("current size: %d slots\n", table->size);
    printf("current load for table 1: %d items\n", table->load1);
    printf("current load for table 2: %d items\n", table->load2);
    //load factor is %age of keys present in total.
    printf(" load factor: %.3f%%\n", (table->load1+table->load2) 
    								  * 100.0 / (2*table->size));
    float seconds = table->time * 1.0 / CLOCKS_PER_SEC;
	printf("CPU time spent: %.6f sec\n", seconds);

    printf("--- end stats ---\n");
}

