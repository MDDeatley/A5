//By Ryan Corbin and  Michael Deatley

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#define FALSE 0
#define TRUE 1


#define PAGE_TABLE_SIZE 256
#define BUFFER_SIZE 256
#define PHYS_MEM_SIZE 256
#define TLB_SIZE 16


FILE *Addresses;
FILE *BackStore;


struct TLB {
	unsigned char TLBpage[16];
	unsigned char TLBframe[16];
	int ind;
};


//prototypes
int findPage(int, char*, struct TLB*, char*, int*, int*, int*);
int readBackingStore(int, char*, int*);


int main (int argc, char* argv[]) {   
    //open files
    Addresses = fopen("addresses.txt", "r");
    if (Addresses == NULL) {
        printf("Error opening a file %s: %s\n", "addresses.txt", strerror(errno));
        exit(EXIT_FAILURE);
    }
    BackStore = fopen("BACKING_STORE.bin", "rb");
    if (BackStore == NULL) {
        printf("Error opening a file %s: %s\n", "BACKING_STORE.bin", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    //initialize
    int val;
    int openFrame = 0;
    int pageFaults = 0;
    int TLBhits = 0;
    int inputCounter = 0;
    
    unsigned char PageTable[PAGE_TABLE_SIZE];
    memset(PageTable, -1, sizeof(PageTable));
    
    struct TLB tlb;
    memset(tlb.TLBpage, -1, sizeof(tlb.TLBpage));
    memset(tlb.TLBframe, -1, sizeof(tlb.TLBframe));
    tlb.ind = 0;
    
    char PhyMem[PHYS_MEM_SIZE][PHYS_MEM_SIZE];
    
    //scan down list, find pages
    while (fscanf(Addresses, "%d", &val)==1) {
        findPage(val, PageTable, &tlb, (char*)PhyMem, &openFrame, &pageFaults, &TLBhits);
        inputCounter++;
    }
    
    //print fault and tlbhit rates
    float pageFaultRate = (float)pageFaults / (float)inputCounter * 100;
    float TLBHitRate = (float)TLBhits / (float)inputCounter * 100;
    printf("Page Fault Rate = %.2f%% \nTLB hit rate= %.2f%% \n",pageFaultRate, TLBHitRate);
    //close files
    fclose(Addresses);
    fclose(BackStore);
    return 0;
}


int findPage(int logicalAddr, char* PT, struct TLB* tlb, char* physmem, int* offsets, int* pageFaults, int* TLBhits) {
    char TLBfound = FALSE;
    int frame = 0;
    
    printf("Virtual address: %-5d\t", logicalAddr);
    unsigned char pagePostion = (logicalAddr >> 8) & 0xFF;
    unsigned char offset = logicalAddr & 0xFF;
    //Check if in TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        if(tlb->TLBpage[i] == pagePostion) {
            frame = tlb->TLBframe[i];
            TLBfound = TRUE;
            (*TLBhits)++;
        }
    }
    //Check if in PageTable
    if (TLBfound == FALSE) {
        //if not in either read from disk
	if (PT[pagePostion] == -1) {
            int newFrame = readBackingStore(pagePostion, physmem, offsets);
            PT[pagePostion] = newFrame;
            (*pageFaults)++;
        }
        frame = PT[pagePostion];
        tlb->TLBpage[tlb->ind] = pagePostion;
        tlb->TLBframe[tlb->ind] = PT[pagePostion];
        tlb->ind = (tlb->ind + 1)%TLB_SIZE;
    }
    int index = ((unsigned char)frame*PHYS_MEM_SIZE)+offset;
    int value = *(physmem+index);
    printf("Physical address: %-5d\t Value: %d\n",index, value);
    return 0;
}


int readBackingStore(int pagePostion, char* physmem, int* offsets) {          
    //initialization
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    //data cannot be found error
    if (fseek(BackStore, pagePostion * PHYS_MEM_SIZE, SEEK_SET)!=0) {
        printf("Error seeking data in %s: %s\n", "BACKING_STORE.bin", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    //data cannot be read error
    if (fread(buffer, sizeof(char), PHYS_MEM_SIZE, BackStore)==0) {
        printf("Error reading data in %s: %s\n", "BACKING_STORE.bin", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    for(int i = 0; i < PHYS_MEM_SIZE; i++) {
        *((physmem+(*offsets)*PHYS_MEM_SIZE)+i) = buffer[i];
    }
    
    (*offsets)++;
    return (*offsets)-1;
}