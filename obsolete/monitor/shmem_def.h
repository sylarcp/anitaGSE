#define NBUF 40  // Length of shared memory buffer, i.e number of segments
// LOS definitions
#define LOS_MAX_DATA 8192
#define LOS_SHMEM_SEM_FILE "/tmp/los"
#define LOS_SHMEM_KEY 4408L
// High rate SIP definitions
#define SIPHR_MAX_DATA 8192
#define SIPHR_SHMEM_SEM_FILE "/tmp/sip_fast_tdrss"
#define SIPHR_SHMEM_KEY 5512L
// Low rate SIP definitions
#define SIPLR_MAX_DATA 255
#define SIPLR_SHMEM_SEM_FILE "/tmp/sip_slow_tdrss"
#define SIPLR_SHMEM_KEY 5234L
// Iridium SIP definitions
#define SIPIR_MAX_DATA 255
#define SIPIR_SHMEM_SEM_FILE "/tmp/sip_iridium"
#define SIPIR_SHMEM_KEY 5876L


