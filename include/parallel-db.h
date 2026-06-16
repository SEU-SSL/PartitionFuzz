/*******************************************************************
 *
 *                            DB
 *
 *******************************************************************/

#ifndef PARALLEL_DB_H
#define PARALLEL_DB_H

#include <hiredis/hiredis.h>
#include "types.h"
#include "afl-fuzz.h"

typedef struct db_queue_entry {

  bool passed_det,                      /* Deterministic stages passed?     */
      trim_done,                        /* Trimmed?                         */
      was_fuzzed;

  u32 subdomain_id,                                                     /*  */
      fuzz_level;                       /* Number of fuzzing iterations     */

  u64 handicap,                         /* Number of queue cycles behind    */
      depth;                            /* Path depth                       */

  /*  */
  u32 len;                              /* length of seed                   */
  u8 *content;                          /* binary of seed                   */

} DBq;

void save_one_seed_to_db(redisContext *connect, struct queue_entry *q, u8 *mem);

DBq *get_one_db_seed(redisContext *connect, char *key);

void free_dbq(DBq *dbq);

void save_one_subdomain_to_db(redisContext *connect, char *instance_id,
                              subdomain *c, u32 trace_len);

void get_one_subdomain_from_db(redisContext *connect, subdomain *c,
                               const char *sync_id);

void get_all_subdomain_from_db(redisContext *connect,
                               subdomain **subdomain_array, u32 *subdomain_num);

bool if_in_subdomain(redisContext *connect, u32 subdomain_id, u8 *key,
                     bool *key_exist);

bool if_coopqueue_empty(redisContext *connect, u32 subdomain_id);

long long get_LBqueue_size(redisContext *connect, u32 subdomain_id);

void update_seed_subdomain_to_db(redisContext *connect, struct queue_entry *q);

u8 if_need_update_subdomain(redisContext *connect, char *sync_id);

void change_subdomain_state(redisContext *connect, char *sync_id,
                            u8 if_new_subdomain);

void set_subdomain_radius(redisContext *connect, char *sync_id, double r);

double get_subdomain_radius(redisContext *connect, char *sync_id);

#endif

