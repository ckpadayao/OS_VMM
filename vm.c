/**
 * Demonstration C program illustrating how to perform file I/O for vm assignment.
 *
 * Input file contains logical addresses
 * 
 * Backing Store represents the file being read from disk (the backing store.)
 *
 * We need to perform random input from the backing store using fseek() and fread()
 *
 * This program performs nothing of meaning, rather it is intended to illustrate the basics
 * of I/O for the vm assignment. Using this I/O, you will need to make the necessary adjustments
 * that implement the virtual memory manager.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// number of characters to read for each line from input file
#define BUFFER_SIZE         10

// number of bytes to read
// #define CHUNK               99
#define CHUNK               256

#define MEMORY_SIZE  256 * 256

#define NUM_OF_PAGES 256

#define NUM_OF_TLB_ENTRIES 16

#define REDUCED_BUFFER_SIZE 128

FILE    *address_file;
FILE    *backing_store;

// how we store reads from input file
char    address[BUFFER_SIZE];

int page_table[NUM_OF_PAGES];

int tlb[NUM_OF_TLB_ENTRIES][2];

int     logical_address;

// both page number and offset are 8 bits
const int MASK = 0xFF;

// the buffer containing reads from backing store
signed char     buffer[REDUCED_BUFFER_SIZE][NUM_OF_PAGES];

// the value of the byte (signed char) in memory
signed char     value;

int offset; 
int page_num;
int frame;
int tlb_count;
int check = -1;;
int buffer_check;

int main(int argc, char *argv[])
{
    int i = 0;

    // set all values in page table to -1
    for (int p = 0; p < NUM_OF_PAGES; p++) {
        page_table[p] = -1;
    }

    // set all values in tlb to -1
    for (int t = 0; t < NUM_OF_TLB_ENTRIES; t++) {
        tlb[t][0] = -1;
    }

    // perform basic error checking
    if (argc != 3) {
        fprintf(stderr,"Usage: ./vm [backing store] [input file]\n");
        return -1;
    }

    // open the file containing the backing store
    backing_store = fopen(argv[1], "rb");
    
    if (backing_store == NULL) { 
        fprintf(stderr, "Error opening %s\n",argv[1]);
        return -1;
    }

    // open the file containing the logical addresses
    address_file = fopen(argv[2], "r");

    if (address_file == NULL) {
        fprintf(stderr, "Error opening %s\n",argv[2]);
        return -1;
    }

    int total_addresses = 0;
    int page_fault = 0;
    int tlb_hits = 0;
    // read through the input file and output each logical address
    while ( fgets(address, BUFFER_SIZE, address_file) != NULL) {
        // increment total number of addresses
        total_addresses++;
        logical_address = atoi(address);
        // get the offset & page number
        offset = logical_address & MASK;
        page_num = logical_address >> 8;
        page_num = page_num & MASK;

        // reset check
        check = -1;

        // check tlb first
        for (int c = 0; c < NUM_OF_TLB_ENTRIES; c++) {
            // check if the current page number is in the tlb
            // if it is, get the frame 
            // [c][0] == [row][page_num]
            // [c][1] == [row][frame]
            if(tlb[c][0] == page_num) {
                frame = tlb[c][1];
                check = 1;
                // increment tlb hits
                tlb_hits++;
                break;
            }
        }

        // page number is not in tlb
        if(check != 1) {
            if(page_table[page_num] == -1) {
                page_fault++;
                // first seek to byte CHUNK in the backing store
                // SEEK_SET in fseek() seeks from the beginning of the file
                if (fseek(backing_store, CHUNK * page_num, SEEK_SET) != 0) {
                    fprintf(stderr, "Error seeking in backing store\n");
                    return -1;
                }

                // now read CHUNK bytes from the backing store to the buffer
                if (fread(buffer[i], sizeof(signed char), CHUNK, backing_store) == 0) {
                    fprintf(stderr, "Error reading from backing store\n");
                    return -1;
                }
                else {
                    // above 128, because physical memory can only have 128 values
                    if(buffer_check > REDUCED_BUFFER_SIZE) {
                        int tlb_page = 0; 
                         // update page table
                        for (int p = 0; p < NUM_OF_PAGES; p++) {
                            // find former page & invalidate it
                            if(page_table[p] == i) {
                                page_table[p] = -1;
                                tlb_page = p;
                            }
                        }
                        
                         // update tlb 
                         for(int t = 0; t < NUM_OF_TLB_ENTRIES; t++) {
                             if(tlb[t][0] == tlb_page) {
                                 // frame & page num are free
                                tlb[t][0] = -1;
                                tlb[t][1] = -1;
                             }
                         }
                    }

                    page_table[page_num] = i;
                    buffer_check++;
                    // FIFO the physical memory
                    i = (i+1) % REDUCED_BUFFER_SIZE;
                }
            }
            // update tlb using newly acquired frame
            frame = page_table[page_num];
            tlb[tlb_count][0] = page_num;
            tlb[tlb_count][1] = frame;
            tlb_count = (tlb_count + 1) % NUM_OF_TLB_ENTRIES;
        }
        // we get from physical memory since page number is not stored in page table
        value = buffer[frame][offset];
        // printf("%d \t %d \t %d %d\t \n",logical_address, offset, page_num, value);
        // printf("%d\n", value);
        printf("Virtual address: %d Physical Address: %d Value: %d\n", logical_address, (frame << 8) + offset, value);
    }

    // print page-fault rate & TLB hit rate
    printf("Page Faults = %d\n", page_fault);
    printf("Page Fault Rate = %.3f\n", (page_fault / (1. * total_addresses)));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate: %.3f\n", (tlb_hits/ (1. * total_addresses)));
    fclose(address_file);
    fclose(backing_store);

    return 0;
}

