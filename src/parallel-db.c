#include "parallel-db.h"
#include "afl-fuzz.h"
#include <assert.h>

#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

void save_one_seed_to_db(redisContext *connect, struct queue_entry *q,
                         u8 *mem) {

  redisReply *reply = (redisReply *)redisCommand(
      connect,
      "HSET seed_%u content %b passed_det %u trim_done %u subdomain_id %u "
      "was_fuzzed %u fuzz_level %u handicap %llu depth %llu",
      q->id, mem, q->len, q->passed_det, q->trim_done, q->subdomain_id, q->id,
      q->was_fuzzed, q->fuzz_level, q->handicap, q->depth);

  if (!reply) {

    WARNF("Error in save seed_%u: %s", q->id, connect->errstr);

  } else {

    if (reply->type == REDIS_REPLY_ERROR)
      WARNF("Save seed command failed for seed_%u: %s", q->id, reply->str);
    freeReplyObject(reply);

  }

}

DBq *get_one_db_seed(redisContext *connect, char *key) {

  DBq *ret_dbq = NULL;

  redisReply *reply = (redisReply *)redisCommand(connect, "HGETALL %s", key);
  if (!reply) {

    WARNF("Error: %s\n", connect->errstr);
    return NULL;

  } else {

    if (reply->type != REDIS_REPLY_ARRAY) {

      WARNF("reply type error!");
      freeReplyObject(reply);
      return NULL;  // ，HGETALL

    } else {

      /*  */
      // 
      if (reply->elements == 0) {

        WARNF("Key '%s' does not exist.", key);
        freeReplyObject(reply);
        return NULL;  // key

      }

      ret_dbq = (DBq *)ck_alloc(sizeof(DBq));
      if (!ret_dbq) {

        freeReplyObject(reply);
        FATAL("ck_alloc error");

      }

      for (size_t i = 0; i < reply->elements; i += 2) {

        char *key = reply->element[i]->str;
        char *value = reply->element[i + 1]->str;

        if (strcmp(key, "passed_det") == 0) {

          ret_dbq->passed_det = value[0] - '0' == 1 ? 1 : 0;

        } else if (strcmp(key, "trim_done") == 0) {

          ret_dbq->trim_done = value[0] - '0' == 1 ? 1 : 0;

        } else if (strcmp(key, "was_fuzzed") == 0) {

          ret_dbq->was_fuzzed = value[0] - '0' == 1 ? 1 : 0;

        } else if (strcmp(key, "fuzz_level") == 0) {

          ret_dbq->fuzz_level = strtoul(value, NULL, 10);

        } else if (strcmp(key, "subdomain_id") == 0) {

          ret_dbq->subdomain_id = strtoul(value, NULL, 10);

        } else if (strcmp(key, "handicap") == 0) {

          ret_dbq->handicap = strtoull(value, NULL, 10);

        } else if (strcmp(key, "depth") == 0) {

          ret_dbq->depth = strtoull(value, NULL, 10);

        } else if (strcmp(key, "content") == 0) {

          ret_dbq->len = reply->element[i + 1]->len;

          ret_dbq->content = (u8 *)ck_alloc(ret_dbq->len);
          if (!ret_dbq->content) {

            free_dbq(ret_dbq);
            freeReplyObject(reply);
            FATAL("ck_alloc error");

          }

          memcpy(ret_dbq->content, value, ret_dbq->len);

        }

      }

      freeReplyObject(reply);

    }

  }

  return ret_dbq;

}

void free_dbq(DBq *dbq) {

  if (!dbq) return;

  ck_free(dbq->content);
  ck_free(dbq);

}

/**
 * @description: 
 * @param {redisContext} *connect
 * @param {subdomain} *c
 * @param {u32} trace_len 
 * @return {*}
 */
void save_one_subdomain_to_db(redisContext *connect, char *instance_id,
                              subdomain *c, u32 trace_len) {

  redisReply *reply = (redisReply *)redisCommand(
      connect,
      "HSET subdomain_%s subdomain_id %u radius %f trace_mini %b "
      "if_new_subdomain %u",
      instance_id, c->id, c->r, c->center->trace_mini, trace_len, 1);

  if (!reply) {

    FATAL("Error in save subdomain_%u", c->id);

  } else {

    if (reply->type == REDIS_REPLY_ERROR) {

      WARNF("save subdomain_%u error!", c->id);
      freeReplyObject(reply);

    } else {

      DCPF_DEBUG("save subdomain to db, id=%u ", c->id);
      freeReplyObject(reply);

    }

  }

}

void get_one_subdomain_from_db(redisContext *connect, subdomain *c,
                               const char *sync_id) {

  redisReply *reply = NULL;

  reply = (redisReply *)redisCommand(connect, "HGETALL subdomain_%s", sync_id);

  if (!reply) { FATAL("Error in get all fields of subdomain_%s", sync_id); }

  if (reply->type != REDIS_REPLY_ARRAY) {

    freeReplyObject(reply);
    FATAL("reply type error!");
    return;  // ，HGETALL

  }

  if (reply->type == REDIS_REPLY_ARRAY) {

    if (reply->elements == 0) {  // key
      WARNF("Key subdomain_%s does not exist.", sync_id);
      freeReplyObject(reply);
      return;

    }

    for (size_t i = 0; i < reply->elements; i += 2) {

      char *field = reply->element[i]->str;
      char *value = reply->element[i + 1]->str;

      if (strcmp(field, "radius") == 0) {

        c->r = atof(value);

      } else if (strcmp(field, "trace_mini") == 0) {

        size_t db_trace_len = reply->element[i + 1]->len;
        // if (db_trace_len != trace_len) FATAL("read trace_mini error");
        // 
        if (!c->center)
          c->center =
              (struct queue_entry *)ck_alloc(sizeof(struct queue_entry));

        if (!c->center->trace_mini)
          c->center->trace_mini = (u8 *)ck_alloc(db_trace_len);
        memcpy(c->center->trace_mini, (u8 *)value, db_trace_len);

      } else if (strcmp(field, "subdomain_id") == 0) {

        c->id = strtoul(value, NULL, 10);

      }

    }

    DCPF_LOG("  get subdomain_%s from db, id=%u, r=%f", sync_id, c->id, c->r);

  } else {

    WARNF("Error get data for subdomain_%s", sync_id);

  }

  freeReplyObject(reply);

}

/**
 * @description: helper function for set_worker_task.
 * @param {redisContext} *connect
 * @param {subdomain} *
 * @param {u32} trace_len
 * @param {u32} *subdomain_num
 * @return {*}
 */
void get_all_subdomain_from_db(redisContext *connect,
                               subdomain   **subdomain_array,
                               u32          *subdomain_num) {

  redisReply *reply = NULL;
  u32         cursor = 0;  // SCAN
  u32         count = 0;   // subdomain

  // // subdomain_num
  // *subdomain_num = 0;

  // SCANsubdomain_*
  do {

    // DCPF_DEBUG("SCAN %u MATCH subdomain_* COUNT 1000", cursor);
    reply = (redisReply *)redisCommand(
        connect, "SCAN %u MATCH subdomain_* COUNT 1000", cursor);
    if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements != 2) {

      FATAL("Error in SCAN command or invalid reply format");

    }

    // 
    cursor = strtoul(reply->element[0]->str, NULL, 10);

    // 
    redisReply *keys = reply->element[1];
    for (size_t i = 0; i < keys->elements; i++) {

      const char *key = keys->element[i]->str;
      const char *instance_name = key + strlen("subdomain_");

      // subdomain_array
      *subdomain_array = (subdomain *)afl_realloc(
          (void **)subdomain_array, (count + 1) * sizeof(subdomain));
      if (!*subdomain_array) FATAL("afl_realloc error");

      // HGETALLsubdomain
      get_one_subdomain_from_db(connect, &(*subdomain_array)[count],
                                instance_name);

      count++;  // subdomain

    }

    freeReplyObject(reply);

  } while (cursor != 0);  // 0

  // subdomain_num
  *subdomain_num = count;
  DCPF_DEBUG("TOTAL get %u subdomain", count);

}

bool if_coopqueue_empty(redisContext *connect, u32 subdomain_id) {

  bool ret = true;

  //  LLEN 
  redisReply *reply =
      (redisReply *)redisCommand(connect, "LLEN coop_queue_%u", subdomain_id);

  if (reply == NULL) {

    DCPF_DEBUG("LLEN Error: %s\n", connect->errstr);
    return true;

  }

  // 
  if (reply->integer == 0) {

    ret = true;

  } else {

    ret = false;

  }

  freeReplyObject(reply);
  return ret;

}

/**
 * @description: 
 * @param {redisContext} *connect
 * @param {u32} subdomain_id
 * @return {*}
 */
long long get_LBqueue_size(redisContext *connect, u32 subdomain_id) {

  long long ret = -1;

  //  LLEN 
  redisReply *reply = (redisReply *)redisCommand(
      connect, "LLEN load_balance_queue_%u", subdomain_id);

  if (reply == NULL) {

    DCPF_DEBUG("LLEN Error: %s\n", connect->errstr);
    return -1;

  }

  ret = reply->integer;

  freeReplyObject(reply);
  return ret;

}

void update_seed_subdomain_to_db(redisContext *connect, struct queue_entry *q) {

  redisReply *reply = (redisReply *)redisCommand(
      connect, "HSET seed_%u subdomain_id %u", q->id, q->subdomain_id);
  if (!reply) {

    FATAL("Error in save seed_%u", q->id);

  } else {

    if (reply->type == REDIS_REPLY_ERROR) {

      WARNF("command error at update seed subdomain to db!");

    }

    freeReplyObject(reply);

  }

}

bool if_in_subdomain(redisContext *connect, u32 subdomain_id, u8 *key,
                     bool *key_exist) {

  // assert(subdomain_id != UINT32_MAX);

  u32 db_subdomain_id = UINT32_MAX;

  // Redis subdomain_id
  redisReply *reply = redisCommand(connect, "HGET %s subdomain_id", key);
  if (!reply) {

    WARNF("Error retrieving subdomain_id from Redis\n");
    return false;

  }

  if (reply->type == REDIS_REPLY_NIL) {  // key
    *key_exist = false;
    freeReplyObject(reply);
    return false;

  }

  // 
  if (reply->type == REDIS_REPLY_STRING) {

    // 10，
    db_subdomain_id = strtoul(reply->str, NULL, 10);

  } else {

    WARNF("Unexpected reply type: %d", reply->type);
    freeReplyObject(reply);
    return false;

  }

  freeReplyObject(reply);

  return db_subdomain_id == subdomain_id;

}

u8 if_need_update_subdomain(redisContext *connect, char *sync_id) {

  redisReply *reply = (redisReply *)redisCommand(
      connect, "HGET subdomain_%s if_new_subdomain", sync_id);

  if (!reply) {

    FATAL("Error in HGET subdomain_%s if_new_subdomain", sync_id);

  } else {

    if (reply->type == REDIS_REPLY_ERROR) {

      WARNF("HGET subdomain_%s if_new_subdomain error!", sync_id);

    } else if (reply->type == REDIS_REPLY_NIL) {  // KEY 

      DCPF_DEBUG("subdomain_%s not exist", sync_id);
      freeReplyObject(reply);
      return 0;

    } else if (reply->type == REDIS_REPLY_STRING) {

      u8 ret = reply->str[0] - '0' == 1 ? 1 : 0;
      freeReplyObject(reply);
      return ret;

    } else {

      WARNF("UNCATCHED reply type");

    }

    freeReplyObject(reply);
    return 0;

  }

}

void change_subdomain_state(redisContext *connect, char *sync_id,
                            u8 if_new_subdomain) {

  redisReply *reply = (redisReply *)redisCommand(
      connect, "HSET subdomain_%s if_new_subdomain %u", sync_id,
      if_new_subdomain);

  if (!reply) {

    FATAL("Error in HSET subdomain_%s if_new_subdomain", sync_id);

  } else {

    if (reply->type == REDIS_REPLY_ERROR) {

      WARNF("HSET subdomain_%s if_new_subdomain error!", sync_id);

    }

    freeReplyObject(reply);

  }

}

void set_subdomain_radius(redisContext *connect, char *sync_id, double r) {

  redisReply *reply = (redisReply *)redisCommand(
      connect, "HSET subdomain_%s radius %u", sync_id, r);

  if (!reply) {

    FATAL("Error in HSET subdomain_%s radius", sync_id);

  } else {

    if (reply->type == REDIS_REPLY_ERROR) {

      WARNF("HSET subdomain_%s radius error!", sync_id);

    }

    freeReplyObject(reply);

  }

}

double get_subdomain_radius(redisContext *connect, char *sync_id) {

  double      ret = 0.0;
  redisReply *reply =
      (redisReply *)redisCommand(connect, "HGET subdomain_%s radius", sync_id);

  if (!reply) {

    FATAL("Error in HGET subdomain_%s radius", sync_id);

  } else {

    if (reply->type == REDIS_REPLY_ERROR) {

      WARNF("HSET subdomain_%s radius error!", sync_id);

    } else if (reply->type == REDIS_REPLY_STRING) {

      ret = atof(reply->str);

    } else {

      WARNF("UNEXPected typein HGET subdomain_%s radius", sync_id);

    }

    freeReplyObject(reply);

  }

  return ret;

}

