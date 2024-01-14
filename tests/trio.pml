// Setup mmlib
#define PROCSIZE           2    // number of threads
#define VARSIZE            9    // number of variables
#define BUFFSIZE           16   // max queue size for reads
#include "mmlib/library/tso.h"  // use TSO model, and apply the above defines

// Variables indices
#define front_buffer_ptr   0
#define back_buffer_ptr    1
#define middle_buffer_ptr  2

// Only use even numbers for bufferss
// to have least sgnificant bitreserved for dirty flag
#define buffer0            4    
#define buffer1            6    
#define buffer2            8

#define max_number         25   // max number of writes in test

// Atomic swap (emulates std::atomic_exchange)
inline ATOMIC_EXCHANGE(obj, desired) {
    atomic {
        prev = READ(obj);
        WRITE(obj, desired);
        FENCE();
    }
}

// Swaps buffers pointed by middle_buffer pointer and other_buffer pointer
// Adds dirty flag to middle buffer's least segnificant bit
inline SWAP_BUFFERS(middle_buffer, other_buffer, dirty) {
    int prev;
    int other_buffer_ptr = READ(other_buffer);
    ATOMIC_EXCHANGE(middle_buffer, other_buffer_ptr | dirty);
    WRITE(other_buffer, prev & 14); // 14 dec = 1110 bin
}

// Writing numbers 1 to max_number and swapping buffers
// after every write
proctype Producer() {
    int value = 0;
    do
    :: value > max_number -> break
    :: else ->
        value = value + 1;
        int write_buffer = READ(back_buffer_ptr);
        
        // Dispatch those writes into labels so that we
        // we can easily verify mutual exclusivity of reads and writes
        // in never{} clause.
        if
        :: write_buffer == buffer0 -> goto write0;
        :: write_buffer == buffer1 -> goto write1;
        :: write_buffer == buffer2 -> goto write2;
        fi;

write0:
        WRITE(buffer0, value); goto commit;
write1:
        WRITE(buffer1, value); goto commit;
write2:
        WRITE(buffer2, value); goto commit;
commit:
        SWAP_BUFFERS(middle_buffer_ptr, back_buffer_ptr, 1);
    od;
}

// Reading buffer until we get max_number
proctype Consumer() {
    int value = 0, prev_value = 0;
    do
    :: value == max_number -> break
    :: else ->
        if
        :: READ(middle_buffer_ptr) & 1 == 1 ->  // if dirty -> swap
            SWAP_BUFFERS(middle_buffer_ptr, front_buffer_ptr, 0);
        :: else
        fi;

        int read_buffer = READ(front_buffer_ptr);
        if
        :: read_buffer == buffer0 -> goto read0;
        :: read_buffer == buffer1 -> goto read1;
        :: read_buffer == buffer2 -> goto read2;
        fi;
read0:
        value = READ(buffer0); goto check_order;
read1:
        value = READ(buffer1); goto check_order;
read2:
        value = READ(buffer2); goto check_order;
check_order:
        assert(value >= prev_value); // check values arrived in expected order
        prev_value = value;
    od;
}

// Setup producer and consumer and shared buffers
init {
    INIT(front_buffer_ptr,  buffer0);
    INIT(back_buffer_ptr,   buffer1);
    INIT(middle_buffer_ptr, buffer2);
    INIT(buffer0, 0);

    atomic {
        run Producer();
        run Consumer();
    }
}

// Check system-wide invariants that we expect to always hold
never  {
T0_init:
	do
    // We expect producer and consumer to never write and read from the same buffer concurrently
	:: atomic { (Producer@write0 && Consumer@read0) -> assert(!(Producer@write0 && Consumer@read0)) }
	:: atomic { (Producer@write1 && Consumer@read1) -> assert(!(Producer@write1 && Consumer@read1)) }
	:: atomic { (Producer@write2 && Consumer@read2) -> assert(!(Producer@write2 && Consumer@read2)) }

	:: (1) -> goto T0_init
	od;
accept_all:
    skip;
}
