#include <memory.h>
#include "secmem.h"

typedef struct list_entry{
	struct list_entry *_before;
	void *_ptr;
	struct list_entry *_next;
} list_entry;

static list_entry *le_memory_handles_first = 0;
static list_entry *le_memory_handles_last = 0;

void sec_cleanup() {
	list_entry *le_iterate = 0;
	// be sure to rewind
	while(le_memory_handles_first->_before)
		le_memory_handles_first = le_memory_handles_first->_before;
	// clean up all registered pointers
	while(le_memory_handles_first){
		le_iterate = le_memory_handles_first->_next;
		free(le_memory_handles_first->_ptr);
		free(le_memory_handles_first);
		le_memory_handles_first = le_iterate;
	}
	le_memory_handles_first = 0;
	le_memory_handles_last = 0;
}

void sec_addnewentry(){
	// on init, initialiase first occourences
	if(!le_memory_handles_first){
		le_memory_handles_first = malloc(sizeof(list_entry));
		le_memory_handles_last = le_memory_handles_first;
		le_memory_handles_first->_before = 0;
	}
	else{
		//forward to end
		while(le_memory_handles_last->_next)
			le_memory_handles_last = le_memory_handles_last->_next;
		le_memory_handles_last->_next = malloc(sizeof(list_entry));
		le_memory_handles_last->_next->_before = le_memory_handles_last;
		le_memory_handles_last = le_memory_handles_last->_next;
	}
	le_memory_handles_last->_next = 0;
}


void *sec_malloc(size_t size){
	void *ptr;
	ptr = malloc(size);
	memset(ptr, 0, size);
	sec_addnewentry();
	le_memory_handles_last->_ptr = ptr;
	return le_memory_handles_last->_ptr;
}

void *sec_calloc(size_t nmemb, size_t size){
	void *ptr;
	ptr = calloc(nmemb, size);
	memset(ptr, 0, nmemb * size);
	sec_addnewentry();
	le_memory_handles_last->_ptr = ptr;
	return le_memory_handles_last->_ptr;
}

void *sec_realloc(void *ptr, size_t size){
	list_entry *le = 0;
	if(!ptr)
		return sec_malloc(size);
	le = sec_find_element(ptr);
	if(!le)
		return 0;
	if(!size){
		free(ptr);
		if(le->_before)
			le->_before->_next = le->_next;
		if(le->_next)
			le->_next->_before = le->_before;
		if(le == le_memory_handles_first)
			le_memory_handles_first = le->_next;
		if(le == le_memory_handles_last)
			le_memory_handles_last = le->_before;
		free(le);
		return 0;
	}
	else{
		le->_ptr = realloc(ptr, size);
	}
	return le->_ptr;
}

void *sec_find_element(void *ptr){list_entry *le = 0;
while(le_memory_handles_first->_before)
	le_memory_handles_first = le_memory_handles_first->_before;
while(le){
	if(le->_ptr == ptr)
		break;
	le = le->_next;
}
return le;}

//TODO: remove this function, if not needed anymore
void sec_test(){
	char *ptr;
	int i;

for (i = 0; i<500; i++){
	sec_malloc(i);
	sec_calloc(i*3, i);
}
ptr = sec_malloc(30);

for (i=0; i<100;++i)
	ptr=sec_realloc(ptr, i);

ptr = sec_malloc(300000);
for(i=0; i<3000; ++i)
	ptr = sec_realloc(ptr, i);
ptr = sec_realloc(0, 222);
ptr = sec_realloc(ptr, 0);

sec_cleanup();
}
