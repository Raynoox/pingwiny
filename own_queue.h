#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

TAILQ_HEAD(tailhead, entry) head;

struct entry {
	int ships;
	int scalar_clock;
	int rank;
  TAILQ_ENTRY(entry) entries;
};

int add_to_queue(int proc_clock,int proc_rank, int proc_ships) {
  int place = 1;
  struct entry *elem, *elemIterator;
  elem = malloc(sizeof(struct entry));
  if (elem) {
    	elem->ships = proc_ships;
	elem->rank = proc_rank;
	elem->scalar_clock = proc_clock;
  }
  
  elemIterator = TAILQ_FIRST(&head);
	do{
		if(elemIterator == NULL){
			TAILQ_INSERT_TAIL(&head, elem, entries);
			return place;
		}
		else{

			if((elem->scalar_clock < elemIterator->scalar_clock) || ((elem->scalar_clock == elemIterator->scalar_clock) && (elem->rank < elemIterator->rank))){
			
				TAILQ_INSERT_BEFORE(elemIterator,elem,entries);
				return place;
			}

			elemIterator = TAILQ_NEXT(elemIterator,entries);
			place++;

		}
	}while(1);	
}

void remove_rank(int proc_rank){
	
	struct entry *elemIterator;
	elemIterator = TAILQ_FIRST(&head);
	do{
		if(elemIterator->rank == proc_rank){
			TAILQ_REMOVE(&head,elemIterator,entries);
			free(elemIterator);
			break;
		}
		elemIterator = TAILQ_NEXT(elemIterator,entries);	
	}while(1);
}

int how_many_ships(int proc_rank){
	
	struct entry *elemIteratortmp;
	elemIteratortmp = head.tqh_first;
	int ship_count =0;
	do{

		ship_count = ship_count + elemIteratortmp->ships;
		if(elemIteratortmp->rank == proc_rank){
			return ship_count;
		}
		elemIteratortmp = TAILQ_NEXT(elemIteratortmp,entries);	
	}while(1);
}
void print_queue(){
 
    struct entry *elemIterator;
    elemIterator = head.tqh_first;
    do{
        if(elemIterator == NULL){          
            break;
        }
        printf("%i", elemIterator->rank);
        elemIterator = TAILQ_NEXT(elemIterator,entries);   
 
    }while(1);
    printf("\n");
}
