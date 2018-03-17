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
#include<math.h>
#include <time.h>

#include "xuckoon.h"
#define rightmostnbitsn(n, x) (x) & ((1 << (n)) - 1)
// a bucket stores a single key (full=true) or is empty (full=false)
// it also knows how many bits are shared between possible keys, and the first
// table address that references it
typedef struct bucket {
    int id;		// a unique id for this bucket, equal to the first address
				// in the table which points to it
    int depth;	// how many hash value bits are being used by this bucket
    int elements;	// does this bucket contain a key
    int64 * keys;	// the key stored in this bucket
} Bucket;

// an inner table is an extendible hash table with an array of slots pointing
// to buckets holding up to 1 key, along with some information about the number
// of hash value bits to use for addressing
typedef struct inner_table {
    Bucket **buckets;	// array of pointers to buckets
    int size;			// how many entries in the table of pointers (2^depth)
    int depth;			// how many bits of the hash value to use (log2(size))
    int nkeys;
    int bucketsize;// how many keys are being stored in the table
} InnerTable;

// a xuckoo hash table is just two inner tables for storing inserted keys
struct xuckoon_table {
	int time;
    InnerTable *table1;
    InnerTable *table2;
   
};

//************************Function Prototypes.********************************//
void initialize_table(XuckoonHashTable *table,int bucketsize);
int hash_xn(int table_number, int64 key);
int table_depthn(XuckoonHashTable *table,int toggle);	
int rightn_bitsn(XuckoonHashTable *table,int toggle, int64 key);
int key_numb(XuckoonHashTable*table,int toggle,int address);
int better_key_remove(XuckoonHashTable*table,int toggle,int64 key);
void arrange_table(XuckoonHashTable*table,int64 key_rem,
				  int toggle,int64 key_put);
int table_full(XuckoonHashTable*table,int toggle,int64 key);
void insert_keyn(XuckoonHashTable*table,int toggle,int64 key);
InnerTable * free_bucketn(InnerTable *table,Bucket*bucket);
InnerTable*  doublex_table(InnerTable *table);
InnerTable* reinsertn(InnerTable *table,int64 key,int toggle);
Bucket* new_bucketn(InnerTable *table,int new_first_address,int new_depth);
InnerTable* split_tablen(XuckoonHashTable *Table,InnerTable *table,
						int address,int toggle);

//***************************************************************************//



//malloc the space to both the tables with initial size of tables as 1
//and keys per bucket to bucketsize.
void initialize_table(XuckoonHashTable *table,int bucketsize){
	table->time=0;
    table->table1=malloc(sizeof(InnerTable));
    table->table1->size=1;
    table->table1->depth=0;
    table->table1->nkeys=0;
    
    table->table1->buckets=malloc(sizeof(*table->table1->buckets) );
    table->table1->buckets[0]=malloc(sizeof **table->table1->buckets );
    table->table1->bucketsize=bucketsize;
    table->table1->buckets[0]->depth=0;
    table->table1->buckets[0]->keys=malloc(table->table1->bucketsize* 
    								sizeof(table->table1->buckets[0]->keys));
    table->table1->buckets[0]->elements=0;
    table->table1->buckets[0]->id=0;
    table->table2=malloc(sizeof(InnerTable));
    table->table2->size=1;
    table->table2->depth=0;
    table->table2->nkeys=0;
    table->table2->buckets=malloc(sizeof(*table->table2->buckets) );
    table->table2->buckets[0]=malloc(sizeof **table->table2->buckets );
    table->table2->bucketsize=bucketsize;
    table->table2->buckets[0]->depth=0;
    table->table2->buckets[0]->keys=malloc(table->table2->bucketsize*
    								sizeof(table->table2->buckets[0]->keys));
    table->table2->buckets[0]->elements=0;
    table->table2->buckets[0]->id=0;
    
    //return table;
    
}

//it ruturn the hash value of function of key in given table number as parameter
int hash_xn(int table_number, int64 key)
{
    
    if(table_number==1){
        return h1(key);
    }
    return h2(key);    
}


//it returns the depth of given tables toggle number 
int table_depthn(XuckoonHashTable *table,int toggle){
    if(toggle==1){
        return table->table1->depth;
    }
    return table->table2->depth;
}


//it return the rightmost tables toggle depth bits of hash key value.
int rightn_bitsn(XuckoonHashTable *table,int toggle, int64 key){
    int depth= table_depthn(table,toggle);
    int hash_value=hash_xn(toggle,key);
    return rightmostnbitsn(depth,hash_value);
}


// initialise an extendible cuckoon hash table
XuckoonHashTable *new_xuckoon_hash_table(int bucketsize) {
    XuckoonHashTable *table=malloc(sizeof (*table));
    assert(table);
    initialize_table(table,bucketsize);
    
    return table;
}



//it  returns the number of elements in particular bucket.
int key_numb(XuckoonHashTable*table,int toggle,int address){
    if(toggle==1){
        return table->table1->buckets[address]->elements;
    }
    return table->table2->buckets[address]->elements;
    
}



//it returns the key from table by hashing all values to different table
//and return the key which has least number of keys in hash value of that 
//bucket of another table
int better_key_remove(XuckoonHashTable*table,int toggle,int64 key){
    
    if(toggle==1){
        int min_key;
        int address1;
        int min_element;
        int address2;
        int curr_element;
        int i;
        int best_key;
        best_key=key;
        address1=rightmostnbitsn(table->table1->depth, h1(key));
        min_key=rightmostnbitsn(table->table1->depth, 
        		h1(table->table1->buckets[address1]->keys[0]));
        
        address2=rightmostnbitsn(table->table2->depth, h2(min_key));
        if(table->table2->buckets[address2]!=NULL && 
        table->table2->buckets[address2]->keys!=NULL)
        	{
            min_element=table->table2->buckets[address2]->elements;
            
            for(i=0;i<table->table1->buckets[address1]->elements;i++){
                min_key=rightmostnbitsn(table->table1->depth,
                		h1(table->table1->buckets[address1]->keys[i]));
                address2=rightmostnbitsn(table->table2->depth, h2(min_key));
                if(table->table2->buckets[address2]!=NULL &&
                	table->table2->buckets[address2]->keys!=NULL)
                	{
                    curr_element=table->table2->buckets[address2]->elements;
                    if(min_element>=curr_element)
                    	{
                        min_element=curr_element;
                        best_key=table->table1->buckets[address1]->keys[i];
                    }
                }
                
                
            }
        }
        return best_key;
        
    }
    else{
        
        int min_key;
        int address1;
        int min_element;
        int address2;
        int curr_element;
        int i;
        int best_key;
        address2=rightmostnbitsn(table->table2->depth, h2(key));
        best_key=key;
        min_key=rightmostnbitsn(table->table2->depth, 
        		h1(table->table2->buckets[address2]->keys[0]));
        address1=rightmostnbitsn(table->table1->depth, h1(min_key));
        if(table->table2->buckets[address2]!=NULL && 
        	table->table2->buckets[address2]->keys!=NULL){
            min_element=table->table1->buckets[address1]->elements;
            //gives the key with least number of keys in table2 after hashing 
            //all values to table2 from bucket of table 1.
            for(i=0;i<table->table2->buckets[address2]->elements;i++){
                min_key=rightmostnbitsn(table->table2->depth, 
                		h2(table->table2->buckets[address2]->keys[i]));
                address1=rightmostnbitsn(table->table1->depth, h1(min_key));
                if(table->table1->buckets[address1]!=NULL && 
					table->table1->buckets[address1]->keys!=NULL)
					{
                    curr_element=table->table1->buckets[address1]->elements;
                    if(min_element>=curr_element){
                        min_element=curr_element;
                        best_key=table->table2->buckets[address2]->keys[i];
                    }
                    
                    
                }
            }
        }
        return best_key;
    }
}



//after removing the best key it replaces that key with key_put in that same
//table from which it was removed at same place.
void arrange_table(XuckoonHashTable*table,int64 key_rem,
				  int toggle,int64 key_put){
    if(toggle==1){
        int address1=rightmostnbitsn(table->table1->depth, h1(key_rem));
        int size=table->table1->buckets[address1]->elements;
        int i;
        for(i=0;i<size;i++){
            if(table->table1->buckets[address1]->keys[i]==key_rem){
                
                table->table1->buckets[address1]->keys[i]=key_put;
            }
        }
    }
    else{
        
        int address2=rightmostnbitsn(table->table2->depth, h2(key_rem));
        int size=table->table2->buckets[address2]->elements;
        int i;
        for(i=0;i<size;i++){
            if(table->table2->buckets[address2]->keys[i]==key_rem){
                
                table->table2->buckets[address2]->keys[i]=key_put;
            }
        }
        
    }
    
}



//return true if tables bucket is full calculated by hashing the address of key.
int table_full(XuckoonHashTable*table,int toggle,int64 key){
    if(toggle==1){
        int address1=rightmostnbitsn(table->table1->depth, h1(key));
        if(table->table1->buckets[address1]->elements<
        	table->table1->bucketsize){
            return false;
        }
        
        
    }
    else{
        int address2=rightmostnbitsn(table->table2->depth, h2(key));
        if(table->table2->buckets[address2]->elements<
        	table->table2->bucketsize){
            return false;
        }
    }
    return true;
}



//it insert the key in particular table by increasing the number of elements in
//that table and increasing the total number of keys inserted in table.
void insert_keyn(XuckoonHashTable*table,int toggle,int64 key){
    if(toggle==1){
        int address1 = rightmostnbitsn(table->table1->depth, h1(key));
        table->table1->buckets[address1]->keys[key_numb(table,1,address1)]=key;
        table->table1->buckets[address1]->elements++;
        table->table1->nkeys++;
    }
    
    else{
        int address2 = rightmostnbitsn(table->table2->depth, h2(key));
        table->table2->buckets[address2]->keys[key_numb(table,2,address2)]=key;
        table->table2->buckets[address2]->elements++;
        table->table2->nkeys++;
    }
    
    
}


//it free the bucket in which rehashing of all values needs to be done again
//after splitting or doubling, thus making number of elements to 0 and 
//keys are again malloc so that previous keys copyright is not done.
InnerTable * free_bucketn(InnerTable *table,Bucket*bucket){
    bucket->elements=0;
    Bucket *newbucket;
    newbucket=bucket;
    
    bucket->keys=NULL;
    
    
    bucket->keys=malloc(sizeof(newbucket->keys)* table->bucketsize);
    return table;
}



//double the extendible cuckoon table.
InnerTable*  doublex_table(InnerTable *table){
    int size = table->size*2;
    assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");
    table->buckets = realloc(table->buckets, (sizeof *table->buckets) * size);
    assert(table->buckets);
    int i;
    int j;
    
    //copies all the pointers one by one after doubling the table by 
    //same nnumber of bits to previous half to table.
    for (j = 0; j < table->size; j++) {
        Bucket *bucket = malloc(sizeof *bucket);
        table->buckets[j+table->size]=bucket;
        table->buckets[j+table->size]->keys=malloc(
        								sizeof(bucket->keys)*table->bucketsize);
    }
    for (i = 0; i < table->size; i++) {
        table->buckets[table->size + i] = table->buckets[i];
        
        
    }
    table->size = size;
    table->depth++;
    return table;
}


//it inserts the the key in table according to toggle number and increase the 
//number of elements in it.
InnerTable* reinsertn(InnerTable *table,int64 key,int toggle){
    if (toggle==1){
        int address = rightmostnbitsn(table->depth, h1(key));
        table->buckets[address]->keys[table->buckets[address]->elements]=key;
        table->buckets[address]->elements++;
        return table;
    }
    else{
        int address = rightmostnbitsn(table->depth, h2(key));
        table->buckets[address]->keys[table->buckets[address]->elements]=key;
        table->buckets[address]->elements++;
        return table;
    }
}



//creates a single bucket space with keys as bucketsize.
Bucket* new_bucketn(InnerTable *table,int new_first_address,int new_depth){
    Bucket *bucket = malloc(sizeof *bucket);
    bucket->id=new_first_address;
    bucket->depth=new_depth;
    bucket->elements=0;
    bucket->keys=malloc(sizeof(bucket->keys)*table->bucketsize);
    return bucket;
}



//split the table ot double it to inseert the key.
InnerTable* split_tablen(XuckoonHashTable *Table,InnerTable *table,
						int address,int toggle){
    // create a new bucket and update both buckets' depth
    if(table->buckets[address]->depth==table->depth){
        table=doublex_table(table);
    }
    Bucket* bucket=table->buckets[address];
    int depth =bucket->depth;
    int first_address = bucket->id;
    int new_depth=depth+1;
    bucket->depth=new_depth;
    int new_first_address = 1 << depth | first_address;
    Bucket *newbucket = new_bucketn(table,new_first_address, new_depth);
    int bit_address = rightmostnbitsn(depth, first_address);
    // THIRD,
    // redirect every second address pointing to this bucket to the new bucket
    // construct addresses by joining a bit 'prefix' and a bit 'suffix'
    // (defined below)
    
    // suffix: a 1 bit followed by the previous bucket bit address
    
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
    
    //temp array stores all the values from bucket to rehash it again.
    int64 *temp_array;
    temp_array=bucket->keys;
    
    int total_elements=bucket->elements;
    table=free_bucketn(table,bucket);
    int i;
    
    //elements are reinserted back 
    for(i=0;i<total_elements;i++){
        reinsertn(table,temp_array[i],toggle);
    }
    if(toggle==1){
        Table->table1=table;
        return table;
    }
    Table->table2=table;
    return table;
}





// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool xuckoon_hash_table_insert(XuckoonHashTable *table, int64 key) {
    //fprintf(stderr, "not yet implemented\n");
    int start_time = clock();
    if(xuckoon_hash_table_lookup(table,key)){
        return false;
    }
    int toggle;
    int break_prob=200;//break the cuckoo hashing to avoid cycle.
    int count=0;
    toggle=1;
    int key_rem;
    
    
    //checks if table1 is full or not and tries to remove the better key from 
    //that table if its full and insert the new key into that toggle
    //and it keeps on going till key is inserted or there is break in probing.
    while(count<break_prob){
        
        if(!table_full(table,toggle,key)){
            insert_keyn(table,toggle,key);
            table->time += clock() - start_time;
            return true;
            
        }
        else{
            key_rem=better_key_remove(table,toggle,key);
            
            arrange_table(table,key_rem,toggle,key);
            
            key=key_rem;
            count++;
        }
        if(toggle==1){
            toggle=2;
        }
        else{
            toggle=1;
        }
    }
    
    int hash_index1=rightmostnbitsn( table_depthn(table,1)  ,hash_xn(1,key));
    int hash_index2=rightmostnbitsn( table_depthn(table,2)  ,hash_xn(2,key));
    int difference1=table->table1->depth-
    				table->table1->buckets[hash_index1]->depth;
    int difference2=table->table2->depth-	
    				table->table2->buckets[hash_index2]->depth;
   
    int total=0;
    while(1==1){
        total=total+1;
        
        
        if(!table_full(table,2,key)){
            insert_keyn(table,2,key);
            table->time += clock() - start_time;
            return true;
        }
        if(!table_full(table,1,key)){
            insert_keyn(table,1,key);
            table->time += clock() - start_time;
            return true;
            
        }
         	
	  //difference gives the intuation of splitting the table or not.
	  //if its zero double occurs. if its more than zero splitting occurs till
	  //it difference is set to 0.more difference more split less chance double
	  //if difference is not zero its better to go to that table as double cant 
	  //occur.if diiference is zero double the table with less number of keys.
	  //if if both difference are not zero, split which has more difference.
        if((( ((difference1==0)&&difference2!=0))||((difference1-difference2==0)
        	&& table->table1->size>table->table2->size)	
        	|| difference2>difference1 )) 
        	{
           
            
            while(table_full(table,2,key)){
                table->table2=split_tablen(table,table->table2,hash_index2,2);
                
                hash_index2=rightmostnbitsn( table_depthn(table,2)  
                			,hash_xn(2,key));
                hash_index1=rightmostnbitsn( table_depthn(table,1)  
                			,hash_xn(1,key));
                
                difference1=table->table1->depth-
                			table->table1->buckets[hash_index1]->depth;
                difference2=table->table2->depth-
                			table->table2->buckets[hash_index2]->depth;
                
                			
				//look if you got free space to insert element back at same 
				//address.
                if(!table_full(table,2,key)){
                    insert_keyn(table,2,key);
                    table->time += clock() - start_time;
                    return true;
                }
                
                //again perform the cuckoo to check if we luckily get free slot 
                //after doubling the table by key to insert.
                if(xuckoon_hash_table_insert(table, key)){
                	table->time += clock() - start_time;
					return true;
                }
                
            }
            
        }
      
        if((((table->table1->depth-table->table2->depth<=2
        	|| (difference2==0 && difference1!=0)) || 
			((difference1-difference2==0) && 
			table->table2->size>=table->table1->size) 
			|| difference1>difference2)))
			{
            
            
          
            while(table_full(table,1,key)){
                
                table->table1=split_tablen(table,table->table1,hash_index1,1);
                hash_index2=rightmostnbitsn( table_depthn(table,2)  
                			,hash_xn(2,key));
                hash_index1=rightmostnbitsn( table_depthn(table,1) 
                			,hash_xn(1,key));
                
                difference1=table->table1->depth-
                			table->table1->buckets[hash_index1]->depth;
                difference2=table->table2->depth-
							table->table2->buckets[hash_index2]->depth;
                //look if you got free space to insert element back at same 
				//address.
                if(!table_full(table,1,key)){
                    insert_keyn(table,1,key);
                    table->time += clock() - start_time;
                    return true;
                }
                //again perform the cuckoo to check if we luckily get free slot 
                //after doubling the table by key to insert.
                if(xuckoon_hash_table_insert(table, key)){
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
bool xuckoon_hash_table_lookup(XuckoonHashTable *table, int64 key) {
    //fprintf(stderr, "not yet implemented\n");
    int address1 = rightmostnbitsn(table->table1->depth, h1(key));
    int address2=  rightmostnbitsn(table->table2->depth, h2(key));
    int size1=table->table1->buckets[address1]->elements;
    int size2=table->table2->buckets[address2]->elements;
    int i,j;
    for(i=0;i<size1;i++){
        if(table->table1->buckets[address1]->keys[i]==key){
            return true;
        }
    }
    for(j=0;j<size2;j++){
        if(table->table2->buckets[address2]->keys[j]==key){
            return true;
        }
    }
    return false;
}


// print the contents of 'table' to stdout
void xuckoon_hash_table_print(XuckoonHashTable *table) {
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
            printf(	"%9d | %-9d ", i, innertables[t]->buckets[i]->id);
            
            // if this is the first address at which a bucket occurs, print it
            if (innertables[t]->buckets[i]->id == i) {
                printf("%9d ", innertables[t]->buckets[i]->id);
                
                printf("[");
                for(int j = 0; j < innertables[t]->bucketsize; j++) {
                    if (j < innertables[t]->buckets[i]->elements) {
                        printf(" %llu", innertables[t]->buckets[i]->keys[j]);
                    } else {
                        printf(" -");
                    }
                }
                printf(" ]");
            }
            
            // end the line
            printf("\n");
        }
    }
    printf("--- end table ---\n");
}
void free_xuckoon_hash_table(XuckoonHashTable *table){
	int i;
    for (i = table->table1->size-1; i >= 0; i--) {
    	//free the keys in every bucket and buckets pointer 
        if (table->table1->buckets[i]->id == i) {
            free(table->table1->buckets[i]->keys);
            
        	free(table->table1->buckets[i]);
        }
    }
    
    // free the array of bucket pointers
    free(table->table1->buckets);
    
    // free the table struct itself
    free(table->table1);
    int j;
    for (j= table->table2->size-1; j >= 0; j--) {
        if (table->table2->buckets[j]->id == j) {
            free(table->table2->buckets[j]->keys);
            
        	free(table->table2->buckets[j]);
        }
    }
    
    // free the array of bucket pointers
    free(table->table2->buckets);
    
    // free the table struct itself
    free(table->table2);
    free(table);

}

	
	
	

// print some statistics about 'table' to stdout
void xuckoon_hash_table_stats(XuckoonHashTable *table) {
   printf("Statistic are given below |\n");
	printf("Total keys inserted till now=%d",table->table1->nkeys+	
											 table->table2->nkeys);
    printf("\nTotal keys in table1 = %d",table->table1->nkeys);
    printf("\nTotal keys in table = %d",table->table2->nkeys);
    printf("\nload factor for table1 =%.3f% %",((table->table1->nkeys+0.0)
			 /((table->table1->size+0.0)*table->table1->bucketsize))*100);
    printf("\nload factor for table2 =%.3f% %",((table->table2->nkeys+0.0)
    	  /((table->table2->size+0.0)*table->table2->bucketsize))*100);
	printf("\nDepth of table1 =%d",table->table1->depth);
	printf("\nDepth of table2 =%d",table->table2->depth);
	printf("\nSize of table1 =%d",table->table1->size);
	printf("\nSize of table2=%d",table->table2->size);
	float seconds = table->time * 1.0 / CLOCKS_PER_SEC;
	printf("\nCPU time spent: %.6f sec\n", seconds);
	
    return;
}
