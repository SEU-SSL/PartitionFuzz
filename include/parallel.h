#ifndef PARALLEL_H
#define PARALLEL_H

#include "afl-fuzz.h"
#include "parallel-db.h"

typedef struct instrument_point_map {

  // u32  id;                          /* Instrument Point offset in bitmap */
  u32 *succ_point;                         /* successor point of this point */
  u32  succ_num;

} InstrumentPointMap;

// InstrumentPointMap *load_instrument_map(const char *filename, u32 map_size);
// void destroy_instrument_map(InstrumentPointMap *map, u32 map_size);

void update_edge_freq(afl_state_t *afl, u32 *freq_map);

double get_distance_from_matrix(afl_state_t *afl, u32 i, u32 j);

double get_trace_mini_distance(afl_state_t *afl, u8 *trace1, u8 *trace2);

void decompress_trace_mini(u8 *dst, u8 *src, u32 map_size);

void input_domain_division(afl_state_t *afl);
void input_domain_division_k_medoids(afl_state_t *afl);

void init_subdomain_overlap_score(afl_state_t *afl, subdomain *subdomain_arr,
                                  u32 subdomain_num, u32 seed_num);

u8 check_subdomain_overlap(afl_state_t *afl, subdomain *subdomain_arr,
                           u32 subdomain_num, u32 seed_num);

void bind_all_cpu(s32 fuzz_cpu, pthread_t tid);

void inform_update_distance_matrix(afl_state_t *afl);

void *update_distance_matrix_thread(void *arg);

void sync_subdomain_seeds_from_db(afl_state_t *afl, u32 subdomain_id);

void update_subdomain(afl_state_t *afl);
void do_cooperation(afl_state_t *afl);
u32  do_load_balance(afl_state_t *afl);

void update_subdomain_alias_table(afl_state_t *afl);

int  compare_seed(const void *a, const void *b);
void send_seed_to_cluster(afl_state_t *afl, struct queue_entry *q,
                          subdomain *c);

// 
void quickSort(struct queue_entry **arr, int low, int high);

u32 select_send_domain(afl_state_t *afl);
u32 select_recv_domain(afl_state_t *afl);

void scan_LB_report(afl_state_t *afl);
#endif

