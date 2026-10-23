#include "parallel.h"
#include <float.h>
#include <assert.h>
#include <glib-2.0/glib.h>
#include <math.h>

double get_trace_mini_distance(afl_state_t *afl, u8 *trace1, u8 *trace2) {

  u32 hamming_distance = 0;
  // u32 same_len = 0;
  // u32 len_a = 0;
  // u32 len_b = 0;

  // u8 到 u64 >>3
  u32 len = (afl->fsrv.map_size >> 6);  // u64 长度

  u64 *a = (u64 *)trace1;
  u64 *b = (u64 *)trace2;

  for (u32 i = 0; i < len; i++) {

    u64 xor = a[i] ^ b[i];
    // u64 and = a[i] & b[i];

    if (xor) hamming_distance += __builtin_popcountll(xor);
    // if (and) same_len += __builtin_popcountll(and);
    // if (a[i]) len_a += __builtin_popcountll(a[i]);
    // if (b[i]) len_b += __builtin_popcountll(b[i]);

  }

  // return (hamming_distance - (fmax(len_a, len_b) - fmin(len_a, len_b))) /
  //        (double)same_len;

  // return (double)hamming_distance * (1 - same_len / fmin(len_a, len_b));

  // if ((double)hamming_distance > afl->max_d)
  //   afl->max_d = (double)hamming_distance;

  return (double)hamming_distance;
  // return hamming_distance / sqrt((double)same_len);

}

/* 确保覆盖dst（trace_bits）不会对后续造成影响 */
void decompress_trace_mini(u8 *dst, u8 *src, u32 map_size) {

  u32 i = 0;
  u32 bit_offset;
  while (i < map_size) {

    bit_offset = i & 7;          // 计算当前位在字节中的位置
    uint8_t byte = src[i >> 3];  // 获取字节
    dst[i] = (byte >> bit_offset) & 0X1;  // 提取对应的 bit
    ++i;

  }

}

double get_distance_from_matrix(afl_state_t *afl, u32 i, u32 j) {

  double ret_dist = 0.0;

  if (i == j)
    ret_dist = 0.0;
  else if (i < j)
    ret_dist = afl->distance_matrix_cache[i][j - i - 1];
  else
    ret_dist = afl->distance_matrix_cache[j][i - j - 1];

  return ret_dist;

}

void update_edge_freq(afl_state_t *afl, u32 *freq_map) {

  u64 *current = (u64 *)afl->fsrv.trace_bits;

  u32 i = ((afl->fsrv.real_map_size + 7) >> 3);

  for (u32 j = 0; j < i; j++) {

    if (unlikely(*current)) {

      u8 *cur = (u8 *)current;

      if (cur[0] && freq_map[8 * j + 0] < UINT32_MAX) freq_map[8 * j + 0]++;
      if (cur[1] && freq_map[8 * j + 1] < UINT32_MAX) freq_map[8 * j + 1]++;
      if (cur[2] && freq_map[8 * j + 2] < UINT32_MAX) freq_map[8 * j + 2]++;
      if (cur[3] && freq_map[8 * j + 3] < UINT32_MAX) freq_map[8 * j + 3]++;
      if (cur[4] && freq_map[8 * j + 4] < UINT32_MAX) freq_map[8 * j + 4]++;
      if (cur[5] && freq_map[8 * j + 5] < UINT32_MAX) freq_map[8 * j + 5]++;
      if (cur[6] && freq_map[8 * j + 6] < UINT32_MAX) freq_map[8 * j + 6]++;
      if (cur[7] && freq_map[8 * j + 7] < UINT32_MAX) freq_map[8 * j + 7]++;

    }

    current++;

  }

  afl->freq_total++;

}

/* for seed priority */

// int partition_CD(struct queue_entry **arr, int low, int high) {

//   double pivot = arr[high]->cluster_deviation;  // 选择最后一个元素作为枢轴
//   int    i = low - 1;                           //
//   i指向较大元素的最后一个位置

//   for (int j = low; j < high; j++) {

//     if (arr[j]->WCSD_dist < pivot) {

//       i++;
//       // 交换 arr[i] 和 arr[j]
//       struct queue_entry *temp = arr[i];
//       arr[i] = arr[j];
//       arr[j] = temp;
//     }
//   }

//   // 将基准元素放到正确位置
//   struct queue_entry *temp = arr[i + 1];
//   arr[i + 1] = arr[high];
//   arr[high] = temp;

//   return i + 1;  // 返回基准元素的索引
// }

// void quickSort_CD(struct queue_entry **arr, int low, int high) {

//   if (low < high) {

//     int pivot = partition_CD(arr, low, high);
//     quickSort_CD(arr, low, pivot - 1);
//     quickSort_CD(arr, pivot + 1, high);
//   }
// }

/**
 * @description: 就近分点
 * 1. 已经固定的簇不再分点。
 * 2. 已经固定的点不再变化
 * @return {*}
 */
void divide_seed_to_subdomain(afl_state_t *afl, u32 seed_num,
                              subdomain *subdomain_arr, u32 subdomain_num) {

  // 负载均衡方案1：划分前调整，跳过已经成熟子域的种子

  // // 恢复要调整的簇种子数量
  // for (u32 k = 0; k < subdomain_num; k++) {

  //   subdomain *c = &subdomain_arr[k];
  //   if (!c->fixed) { c->cluster_size = 0; }
  // }

  // 划分已有距离种子，计算距离
  for (u32 i = 0; i < seed_num; i++) {

    struct queue_entry *q = afl->dist_queue_buf[i];

    if (q->disabled || q->is_center) continue;

    double min_d = DBL_MAX;
    u32    k_assign = UINT32_MAX;

    for (u32 k = 0; k < subdomain_num; k++) {

      double d =
          get_distance_from_matrix(afl, q->id, subdomain_arr[k].center->id);

      if (d < min_d) {

        min_d = d;
        k_assign = k;

      }

    }

    assert(k_assign != UINT32_MAX);

    // seed q -> subdomain k_assign, dist is min_d
    q->subdomain_id = k_assign;
    // q->center_deviation = min_d;
    u32                 *size = &subdomain_arr[k_assign].size;
    struct queue_entry **arr =
        afl_realloc((void **)&subdomain_arr[k_assign].items,
                    sizeof(struct queue_entrt *) * (*size + 1));
    arr[*size] = q;
    (*size)++;

  }

}

/**
 * @description: 从seed_buf中使用ART方法获取质心
 * @param {afl_state_t} *afl
 * @param {queue_entry} *
 * @param {u32} seed_num
 * @param {Cluster} *cluster_arr.medoid 存放返回结果
 * @param {u32} get_num
 * @return {*}
 */
void get_domain_center(afl_state_t *afl, struct queue_entry **seed_buf,
                       u32 seed_num, subdomain *subdomain_arr,
                       u32 subdomain_num) {

  assert(seed_num > 2 && subdomain_num >= 2);

  double max_d = -DBL_MAX;
  u32    select_1 = UINT32_MAX;
  u32    select_2 = UINT32_MAX;

  for (u32 j = 1; j < seed_num; j++) {

    for (u32 i = 0; i < j; i++) {

      double d = get_distance_from_matrix(afl, i, j);
      if (d > max_d) {

        max_d = d;
        select_1 = i;
        select_2 = j;

      }

    }

  }

  assert(select_1 != UINT32_MAX && select_2 != UINT32_MAX);

  // 最大化选前两个
  subdomain_arr[0].center = seed_buf[select_1];
  subdomain_arr[0].center->is_center = 1;
  subdomain_arr[0].center->subdomain_id = 0;
  subdomain_arr[0].id = 0;
  subdomain_arr[1].center = seed_buf[select_2];
  subdomain_arr[1].center->is_center = 1;
  subdomain_arr[1].center->subdomain_id = 1;
  subdomain_arr[1].id = 1;

  // 选后续的
  for (u32 i = 2; i < subdomain_num; i++) {  // 质心数目

    double              max = -DBL_MAX;
    struct queue_entry *m = NULL;

    for (u32 j = 0; j < seed_num; j++) {

      if (seed_buf[j]->is_center) continue;
      if (seed_buf[j]->disabled) continue;

      struct queue_entry *m_min = NULL;
      double              min = DBL_MAX;

      for (u32 k = 0; k < i; k++) {  // 所有质心求最小

        double temp = get_distance_from_matrix(afl, seed_buf[j]->id,
                                               subdomain_arr[k].center->id);

        if (temp < min) {  // 与所有质心最小的
          min = temp;
          m_min = seed_buf[j];

        }

      }

      if (min > max) {

        max = min;
        m = m_min;

      }

    }

    if (!m) FATAL("not select one seed to init medoids!");

    // 选定质心
    subdomain_arr[i].center = m;
    subdomain_arr[i].center->is_center = 1;
    subdomain_arr[i].center->subdomain_id = i;
    subdomain_arr[i].id = i;

  }

}

// 利用已有距离的全局种子来评估
void init_subdomain_overlap_score(afl_state_t *afl, subdomain *subdomain_arr,
                                  u32 subdomain_num, u32 seed_num) {

  for (u32 k1 = 0; k1 < subdomain_num; k1++) {

    u32 overlap = 0;

    for (u32 i = 0; i < seed_num; i++) {

      struct queue_entry *q = afl->queue_buf[i];

      if (q->subdomain_id != k1) continue;

      for (u32 k2 = 0; k2 < subdomain_num; k2++) {

        if (k1 == k2) continue;

        subdomain *c = &subdomain_arr[k2];

        double dist = get_distance_from_matrix(afl, q->id, c->center->id);

        // seed in k1, also in k2
        if (dist <= c->r) {

          overlap++;
          break;  // 有一个任务相交就计数

        }

      }

    }

    subdomain_arr[k1].overlap = (double)overlap / subdomain_arr[k1].size;
    DCPF_DEBUG("task overlap in cluster %u is %f", k1,
               subdomain_arr[k1].overlap);

  }

}

// 有一个子域中种子重叠增长超过20%就返回 1
u8 check_subdomain_overlap(afl_state_t *afl, subdomain *subdomain_arr,
                           u32 subdomain_num, u32 seed_num) {

  for (u32 k1 = 0; k1 < subdomain_num; k1++) {

    u32 overlap = 0;

    for (u32 i = 0; i < seed_num; i++) {

      struct queue_entry *q = afl->queue_buf[i];

      if (q->subdomain_id != k1) continue;

      for (u32 k2 = 0; k2 < subdomain_num; k2++) {

        if (k1 == k2) continue;

        subdomain *c = &subdomain_arr[k2];

        double dist = get_distance_from_matrix(afl, q->id, c->center->id);

        // seed in k1, also in k2
        if (dist <= c->r) {

          overlap++;
          break;  // 有一个任务相交就计数

        }

      }

    }

    double subdomain_overlap = (double)overlap / subdomain_arr[k1].size;
    if (subdomain_overlap > 1.2 * subdomain_arr[k1].overlap) {

      DCPF_DEBUG("子域 %u 重叠增长超过0.2倍，%f->%f", k1,
                 subdomain_arr[k1].overlap, subdomain_overlap);
      return 1;

    }

  }

  return 0;

}

int get_instance_num(afl_state_t *afl) {

  DIR           *sd;
  struct dirent *sd_ent;
  u32            entries = 0;

  sd = opendir(afl->sync_dir);
  if (!sd) { PFATAL("Unable to open '%s'", afl->sync_dir); }

  while ((sd_ent = readdir(sd))) {

    /* Skip dot files and our own output directory. */

    if (sd_ent->d_name[0] == '.' || !strcmp(afl->sync_id, sd_ent->d_name)) {

      continue;

    }

    /* Get the full path of the entry. */
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", afl->sync_dir, sd_ent->d_name);

    /* Get the status of the file. */
    struct stat file_stat;
    if (stat(path, &file_stat) == -1) {

      perror("stat failed");
      continue;  // Skip this entry if stat fails

    }

    /* Skip non-directories. */
    if (!S_ISDIR(file_stat.st_mode)) { continue; }

    entries++;

  }

  closedir(sd);

  return entries;

}

// void merge_subdomain(afl_state_t *afl, u8 *dst, u8 *src) {

//   u32  len = (afl->fsrv.map_size >> 6);  // u64 长度
//   u64 *a = (u64 *)src;
//   u64 *b = (u64 *)dst;

//   for (u32 i = 0; i < len; i++) {

//     if (a[i]) b[i] |= a[i];
//   }
// }

// bool if_have_same_hit(afl_state_t *afl, u8 *dst, u8 *src) {

//   u32  len = (afl->fsrv.map_size >> 6);  // u64 长度
//   u64 *a = (u64 *)src;
//   u64 *b = (u64 *)dst;

//   for (u32 i = 0; i < len; i++) {

//     if (a[i] & b[i]) return true;
//   }

//   return false;
// }

// void evaluate_overlap(afl_state_t *afl, Cluster *c_arr, u32 c_arr_len,
//                       u32 seed_num) {

//   for (u32 k = 0; k < c_arr_len; k++) {

//     // 子域 k
//     Cluster *c = &c_arr[k];
//     c->subdomain = ck_alloc(afl->fsrv.map_size >> 3);
//     for (u32 i = 0; i < seed_num; i++) {

//       struct queue_entry *q = afl->queue_buf[i];
//       if (q->cluster_id != k) continue;
//       merge_subdomain(afl, c->subdomain, q->trace_mini);
//     }

//                                                          /* 统计子域 k
//                                                          内桩点数目 */
//     u32  hit_num = 0;
//     u32  len = (afl->fsrv.map_size >> 6);  // u64 长度
//     u64 *a = (u64 *)c->subdomain;
//     for (u32 i = 0; i < len; i++) {

//       if (a[i]) hit_num += __builtin_popcountll(a[i]);
//     }

//     DCPF_LOG("subdomain %u hit_num=%u", k, hit_num);
//   }

//                                                           /*
//                                                           计算子域内独有桩点数目
//                                                           */
//   u8 *other_domain = ck_alloc(afl->fsrv.map_size >> 3);
//   u8 *unique_domain = ck_alloc(afl->fsrv.map_size >> 3);

//   for (u32 k1 = 0; k1 < c_arr_len; k1++) {

//     memset(other_domain, 0, afl->fsrv.map_size >> 3);
//     memset(unique_domain, 0, afl->fsrv.map_size >> 3);

//     for (u32 k2 = 0; k2 < c_arr_len; k2++) {

//       if (k1 == k2) continue;
//       merge_subdomain(afl, other_domain, c_arr[k2].subdomain);
//     }

//     // 统计独有桩点数目
//     u32  unique_hit_num = 0;
//     u32  len = (afl->fsrv.map_size >> 6);  // u64 长度
//     u64 *a = (u64 *)c_arr[k1].subdomain;
//     u64 *b = (u64 *)other_domain;
//     u64 *c = (u64 *)unique_domain;
//     for (u32 i = 0; i < len; i++) {

//       c[i] = a[i] & (~b[i]);
//       if (c[i]) unique_hit_num += __builtin_popcountll(c[i]);
//     }

//     u32 count = 0;
//     u32 hit_count = 0;
//     for (u32 i = 0; i < seed_num; i++) {

//       if (k1 != afl->queue_buf[i]->cluster_id) continue;
//       count++;
//       if (if_have_same_hit(afl, unique_domain,
//       afl->queue_buf[i]->trace_mini))
//         hit_count++;
//     }

//     DCPF_LOG(
//         "subdomain %u unique_hit_num=%u, total %u seeds, %u hit unique_hit",
//         k1, unique_hit_num, count, hit_count);
//   }

//   ck_free(other_domain);

//   for (u32 k = 0; k < c_arr_len; k++) {

//     ck_free(c_arr[k].subdomain);
//   }
// }

/* 划分任务到，子域 */
void input_domain_division(afl_state_t *afl) {

  u64 start_ms = get_cur_time();
  u32 worker_num = get_instance_num(afl);
  u32 seed_num = afl->last_dist_queue_buf_len;
  // 避免在种子添加过程中调用show_stats带来的问题
  u32 total_seed_num = afl->queued_items - 1;

  u8 time_tmp[64];
  u_simplestring_time_diff(time_tmp, afl->prev_run_time + get_cur_time(),
                           afl->start_time);
  DCPF_LOG("total %u seeds, divide task on %u seeds(have distance) at %s",
           total_seed_num, seed_num, time_tmp);

  if (worker_num <= 1) {

    DCPF_DEBUG("NOT divide! worker_num=%u", worker_num);
    return;

  }

  /* Reset the seed & subdomain state related to clustering  */
  for (u32 i = 0; i < seed_num; i++) {

    struct queue_entry *q = afl->dist_queue_buf[i];
    q->subdomain_id = UINT32_MAX;
    q->is_center = 0;

  }

  /* 重置质心内容, 第一次是0 */
  for (u32 k = 0; k < afl->subdomain_num; k++) {

    subdomain *c = &afl->subdomain_array[k];

    // c->items
    c->size = 0;
    c->center = NULL;
    c->id = UINT32_MAX;
    c->r = 0;
    // c->subdomain

  }

  u32        c_arr_len = worker_num;  // subdomain num
  subdomain *c_arr = afl_realloc((void **)&afl->subdomain_array,
                                 sizeof(subdomain) * c_arr_len);

  get_domain_center(afl, afl->dist_queue_buf, seed_num, c_arr, c_arr_len);

  DCPF_DEBUG("[sundomain center info]");
  for (u32 i = 0; i < c_arr_len; i++) {

    DCPF_DEBUG("  subdomain %u, queue_id=%u, bitmap_size=%u, exec_us=%llu ", i,
               c_arr[i].center->id, c_arr[i].center->bitmap_size,
               c_arr[i].center->exec_us);
    for (u32 j = 0; j < c_arr_len; j++)
      DCPF_DEBUG("     distance with subdomain %u center = %f", j,
                 get_distance_from_matrix(afl, c_arr[i].center->id,
                                          c_arr[j].center->id));

  }

  /* 就近分点, 所有种子 */
  divide_seed_to_subdomain(afl, seed_num, c_arr, c_arr_len);

  fflush(stdout);

  /* 计算任务半径 */

  for (u32 k = 0; k < c_arr_len; k++) {

    double max = 0;  // 簇内没有种子半径就是0

    for (u32 i = 0; i < c_arr[k].size; i++) {

      struct queue_entry *q = c_arr[k].items[i];
      double dist = get_distance_from_matrix(afl, q->id, c_arr[k].center->id);

      if (dist > max) { max = dist; }

    }

    // if (max == -DBL_MAX) max = 1000.0;

    c_arr[k].r = max;

    DCPF_DEBUG("[result] cluster %u has %u seeds, r= %f", k, c_arr[k].size,
               c_arr[k].r);

  }

  // 存储种子簇到数据库中,用于worker同步master种子
  for (u32 i = 0; i < total_seed_num; i++) {

    struct queue_entry *q = afl->queue_buf[i];

    if (q->not_sync) q->subdomain_id = UINT32_MAX;

    if (i < seed_num)
      update_seed_subdomain_to_db(afl->redis_connect, afl->dist_queue_buf[i]);
    else {

      // 没有计算完成，重置成默认的max，避免上一次分布的簇分配影响
      afl->queue_buf[i]->subdomain_id = UINT32_MAX;
      update_seed_subdomain_to_db(afl->redis_connect, afl->queue_buf[i]);

    }

  }

  /* 存储任务trace, 子域信息到数据库 */

  u32            k = 0;
  DIR           *sd;
  struct dirent *sd_ent;
  u8             tmp_path[PATH_MAX];

  sd = opendir(afl->sync_dir);
  if (!sd) { PFATAL("Unable to open '%s'", afl->sync_dir); }

  afl->stage_max = afl->stage_cur = 0;
  afl->cur_depth = 0;

  while ((sd_ent = readdir(sd))) {

    /* Skip dot files and our own output directory. */

    if (sd_ent->d_name[0] == '.' || !strcmp(afl->sync_id, sd_ent->d_name)) {

      continue;

    }

    /* skil files */
    snprintf(tmp_path, PATH_MAX, "%s/%s", afl->sync_dir, sd_ent->d_name);
    struct stat st;
    if (lstat(tmp_path, &st) < 0) {

      perror("lstat");
      continue;

    }

    if (!S_ISDIR(st.st_mode)) { continue; }

    char *instance_name = sd_ent->d_name;

    save_one_subdomain_to_db(afl->redis_connect, instance_name, &c_arr[k],
                             afl->fsrv.map_size >> 3);

    k++;

  }

  assert(k == worker_num);
  closedir(sd);

  u64 new_cur_ms = get_cur_time();

  DCPF_DEBUG("divide task use %.2fs", (new_cur_ms - start_ms) / 1000.0);

  afl->last_divide_time = new_cur_ms;

  afl->subdomain_num = c_arr_len;

}

/**
 * @description:  helper fun for k_medoids.
 * 将数据点分配到最近的质心,disabled的种子不分配
 * @param {afl_state_t} *afl
 * @param {Cluster} *cluster_array
 * @param {u32} cluster_num
 * @return {*}dms->dist_queue_buf[i]->cluster_id, -1 是disbale未分配
 */
void assign_points_to_clusters(afl_state_t *afl, subdomain *cluster_array,
                               u32 cluster_num, u32 seed_num) {

  for (u32 i = 0; i < seed_num; i++) {  // 分配每个种子

    if (afl->dist_queue_buf[i]->is_center) continue;
    if (afl->dist_queue_buf[i]->disabled) continue;

    u32 old_cluster = afl->dist_queue_buf[i]->subdomain_id;

    double min_dist = DBL_MAX;
    u32    new_cluster = UINT32_MAX;

    for (u32 k = 0; k < cluster_num; k++) {  // 找到距离当前点最近的质心

      double dist =
          get_distance_from_matrix(afl, i, cluster_array[k].center->id);

      if (dist < min_dist) {

        min_dist = dist;
        new_cluster = k;

      }

    }

    assert(new_cluster != UINT32_MAX);

    // 记录每个点所属的簇
    if (new_cluster != old_cluster) {

      afl->dist_queue_buf[i]->subdomain_id = new_cluster;
      // 种子所属簇发生变化，两个簇都要更新质心
      if (old_cluster != UINT32_MAX)
        cluster_array[old_cluster].need_update_medoid = 1;
      cluster_array[new_cluster].need_update_medoid = 1;

    }

  }

}

/**
 * @description: helper fun for k_medoids.
 * 选择一个点作为簇的中心，使得其到簇内所有点的总距离最小
 * @param {afl_state_t} *afl
 * @param {Cluster} *cluster_array
 * @param {u32} cluster_num
 * @return {*}返回 1 算法收敛。
 */
u8 update_medoids(afl_state_t *afl, subdomain *cluster_array, u32 cluster_num,
                  u32 seed_num) {

  u8 all_converged = 1;  // 是否所有的簇都已经收敛

  for (u32 k = 0; k < cluster_num; k++) {  // 遍历每个簇

    // 收敛且簇内节点没有变化，跳过该质心
    if (!cluster_array[k].need_update_medoid) {

      DCPF_DEBUG("SKIP UPDATE medoid %u", k);
      continue;

    }

    u32 old_medoid = cluster_array[k].center->id;

    double min_total_dist = DBL_MAX;
    u32    new_medoid = UINT32_MAX;

    for (u32 i = 0; i < seed_num; i++) {  // 遍历簇内的所有点

      if (afl->dist_queue_buf[i]->subdomain_id != k) continue;
      if (afl->dist_queue_buf[i]->disabled) continue;

      // 计算簇内点 i 作为质心时的簇内的总距离
      double total_dist = 0;
      for (u32 j = 0; j < seed_num; j++) {

        if (afl->dist_queue_buf[j]->subdomain_id != k) continue;
        if (afl->dist_queue_buf[j]->disabled) continue;

        total_dist += get_distance_from_matrix(afl, i, j);

      }

      // 选择该簇最中心的点
      if (total_dist < min_total_dist) {

        min_total_dist = total_dist;
        new_medoid = i;

      }

    }

    assert(new_medoid != UINT32_MAX);

    if (new_medoid == old_medoid) {  // 第 k 个簇质心没变

      // 簇收敛，若后续簇内节点发生变化则继续更新
      cluster_array[k].need_update_medoid = 0;

    } else {  // 更新质心

      all_converged = 0;
      cluster_array[k].center->is_center = 0;                     // 辞旧
      cluster_array[k].center = afl->dist_queue_buf[new_medoid];  // 迎新
      cluster_array[k].center->is_center = 1;

    }

  }

  return all_converged;

}

void input_domain_division_k_medoids(afl_state_t *afl) {

  u64 start_ms = get_cur_time();
  u32 worker_num = get_instance_num(afl);
  u32 seed_num = afl->last_dist_queue_buf_len;  // 截取已经计算好距离的种子

  // 避免在种子添加过程中调用show_stats带来的问题
  u32 total_seed_num = MAX(seed_num, afl->queued_items - 1);

  u8 time_tmp[64];
  u_simplestring_time_diff(time_tmp, afl->prev_run_time + get_cur_time(),
                           afl->start_time);
  DCPF_LOG("total %u seeds, divide task on %u seeds(have distance) at %s",
           total_seed_num, seed_num, time_tmp);

  if (worker_num <= 1) {

    DCPF_DEBUG("NOT divide! worker_num=%u", worker_num);
    return;

  }

  /* Reset the seed & subdomain state related to clustering  */
  for (u32 i = 0; i < seed_num; i++) {

    struct queue_entry *q = afl->dist_queue_buf[i];
    q->subdomain_id = UINT32_MAX;
    q->is_center = 0;

  }

  /* 重置质心内容, 第一次是0 */
  afl->max_contribute = 0;
  afl->min_contribute = UINT32_MAX;
  for (u32 k = 0; k < afl->subdomain_num; k++) {

    subdomain *c = &afl->subdomain_array[k];

    // c->items
    c->size = 0;
    c->center = NULL;
    c->id = UINT32_MAX;
    c->r = 0;
    // c->subdomain
    c->need_update_medoid = 0;
    c->contribute = 1;  // 重置贡献，初始为1，避免比例为0
    c->LB_contribute = 0;
    c->cursor = 0;

  }

  u32        c_arr_len = worker_num;  // subdomain num
  subdomain *c_arr = afl_realloc((void **)&afl->subdomain_array,
                                 sizeof(subdomain) * c_arr_len);

  /* 步骤 1. maximum minimum distance 确定初始质心 */

  get_domain_center(afl, afl->dist_queue_buf, seed_num, c_arr, c_arr_len);
  DCPF_DEBUG("[sundomain center info]");
  for (u32 i = 0; i < c_arr_len; i++) {

    DCPF_DEBUG("  subdomain %u, queue_id=%u, bitmap_size=%u, exec_us=%llu ", i,
               c_arr[i].center->id, c_arr[i].center->bitmap_size,
               c_arr[i].center->exec_us);
    for (u32 j = 0; j < c_arr_len; j++)
      DCPF_DEBUG("     distance with subdomain %u center = %f", j,
                 get_distance_from_matrix(afl, c_arr[i].center->id,
                                          c_arr[j].center->id));

  }

  /* 步骤 2. 迭代 K-Medoids 算法，直到收敛或达到最大迭代次数 */

  u8  is_converged = 0;
  u64 iter_start_time = get_cur_time();

  for (int iter = 0; iter < 100; iter++) {

    /* 步骤 3: 分配点到簇 */
    assign_points_to_clusters(afl, c_arr, c_arr_len, seed_num);

    /* debug */
    // u32 assign_num = 0;
    // for (u32 k = 0; k < cluster_num; k++) {

    //   for (u32 i = 0; i < dms->dist_queue_buf_len; i++) {

    //     if (dms->dist_queue_buf[i]->cluster_id == k) {

    //       assign_num++;
    //       DCPF_DEBUG(
    //           "assin seed %u to cluster %u, dist =%f, map_size=%u, "
    //           "exec_us=%llu",
    //           i, k,
    //           get_distance_from_matrix(afl, i,
    //                                    dms->cluster_array[k].medoid->id),
    //           dms->dist_queue_buf[i]->bitmap_size,
    //           dms->dist_queue_buf[i]->exec_us);
    //     }
    //   }
    // }

    // DCPF_DEBUG("NOT assined seed num = %u",
    //            dms->dist_queue_buf_len - assign_num);
    /* debug end */

    /* 步骤 4: 更新质心 */
    is_converged = update_medoids(afl, c_arr, c_arr_len, seed_num);

    // 如果质心没有变化，则算法收敛，退出循环
    if (is_converged) {

      DCPF_LOG("K-Medoids converged after %d iterations.", iter + 1);
      break;

    }

  }

  if (!is_converged) WARNF("K-Medoids not converged, iter %d times", 100 + 1);

  u64 iter_end_time = get_cur_time();
  DCPF_LOG("iter use %llu seconds", (iter_end_time - iter_start_time) / 1000);

  /* 步骤 5: 计算簇半径 */
  // 划分已有距离种子，计算距离
  for (u32 i = 0; i < seed_num; i++) {

    struct queue_entry *q = afl->dist_queue_buf[i];

    if (q->disabled || q->is_center) continue;

    if (q->subdomain_id == UINT32_MAX) { continue; }

    u32                 *size = &c_arr[q->subdomain_id].size;
    struct queue_entry **arr =
        afl_realloc((void **)&c_arr[q->subdomain_id].items,
                    sizeof(struct queue_entrt *) * (*size + 1));
    arr[*size] = q;
    (*size)++;

  }

  for (u32 k = 0; k < c_arr_len; k++) {

    double max = 0.0;  // 簇内没有种子半径就是0

    for (u32 i = 0; i < c_arr[k].size; i++) {

      struct queue_entry *q = c_arr[k].items[i];
      double dist = get_distance_from_matrix(afl, q->id, c_arr[k].center->id);

      if (dist > max) { max = dist; }

    }

    // if (max == -DBL_MAX) max = 1000.0;

    c_arr[k].r = max;

    DCPF_DEBUG("[result] cluster %u has %u seeds, r= %f", k, c_arr[k].size,
               c_arr[k].r);

  }

  // 存储种子簇到数据库中,用于worker同步master种子
  for (u32 i = 0; i < total_seed_num; i++) {

    struct queue_entry *q = afl->queue_buf[i];

    if (q->not_sync) q->subdomain_id = UINT32_MAX;

    if (i < seed_num)
      update_seed_subdomain_to_db(afl->redis_connect, afl->dist_queue_buf[i]);
    else {

      // 没有计算完成，重置成默认的max，避免上一次分布的簇分配影响
      afl->queue_buf[i]->subdomain_id = UINT32_MAX;
      update_seed_subdomain_to_db(afl->redis_connect, afl->queue_buf[i]);

    }

  }

  /* 存储任务trace, 子域信息到数据库 */

  u32            k = 0;
  DIR           *sd;
  struct dirent *sd_ent;
  u8             tmp_path[PATH_MAX];

  sd = opendir(afl->sync_dir);
  if (!sd) { PFATAL("Unable to open '%s'", afl->sync_dir); }

  afl->stage_max = afl->stage_cur = 0;
  afl->cur_depth = 0;

  while ((sd_ent = readdir(sd))) {

    /* Skip dot files and our own output directory. */

    if (sd_ent->d_name[0] == '.' || !strcmp(afl->sync_id, sd_ent->d_name)) {

      continue;

    }

    /* skil files */
    snprintf(tmp_path, PATH_MAX, "%s/%s", afl->sync_dir, sd_ent->d_name);
    struct stat st;
    if (lstat(tmp_path, &st) < 0) {

      perror("lstat");
      continue;

    }

    if (!S_ISDIR(st.st_mode)) { continue; }

    char *instance_name = sd_ent->d_name;

    // 绑定实例名称到子域
    // GINT_TO_POINTER(0) == NULL, 故加一存，读出来减一
    g_hash_table_insert(afl->Iname_Sid_map, g_strdup(instance_name),
                        GINT_TO_POINTER(k + 1));

    save_one_subdomain_to_db(afl->redis_connect, instance_name, &c_arr[k],
                             afl->fsrv.map_size >> 3);

    k++;

  }

  assert(k == worker_num);
  closedir(sd);

  u64 new_cur_ms = get_cur_time();

  DCPF_DEBUG("divide task use %.2fs", (new_cur_ms - start_ms) / 1000.0);

  afl->last_divide_time = new_cur_ms;

  afl->subdomain_num = c_arr_len;

}

enum distance_update_phase {

  DISTANCE_UPDATE_IDLE = 0,
  DISTANCE_UPDATE_SNAPSHOT,
  DISTANCE_UPDATE_RESIZE,
  DISTANCE_UPDATE_REPAIR,
  DISTANCE_UPDATE_NEW_ROWS

};

enum distance_service_result {

  DISTANCE_SERVICE_COMPLETE = 0,
  DISTANCE_SERVICE_BUDGET_EXHAUSTED,
  DISTANCE_SERVICE_STOPPED

};

static u64 distance_monotonic_time_us(void) {

  struct timespec ts;
  if (unlikely(clock_gettime(CLOCK_MONOTONIC, &ts))) {

    PFATAL("clock_gettime(CLOCK_MONOTONIC)");

  }

  return (u64)ts.tv_sec * 1000000ULL + (u64)ts.tv_nsec / 1000ULL;

}

void start_distance_statistics(afl_state_t *afl) {

  afl->dist_worker_start_us = distance_monotonic_time_us();

}

static inline u8 distance_budget_exhausted(u64 deadline_us) {

  return distance_monotonic_time_us() >= deadline_us;

}

static void begin_distance_matrix_update(afl_state_t *afl) {

  if (unlikely(afl->last_dist_queue_buf_len > afl->dist_queue_buf_len ||
               afl->queued_items <= afl->dist_queue_buf_len)) {

    FATAL("Invalid distance snapshot state: ready=%u snapshot=%u queue=%u",
          afl->last_dist_queue_buf_len, afl->dist_queue_buf_len,
          afl->queued_items);

  }

  afl->dist_target_queue_buf_len = afl->queued_items;
  afl->dist_snapshot_cursor = afl->dist_queue_buf_len;
  afl->dist_prepare_row = 0;
  afl->dist_repair_row = 1;
  afl->dist_repair_col = 0;
  afl->dist_new_row = afl->last_dist_queue_buf_len;
  afl->dist_new_col = 0;

  afl->dist_queue_buf = (struct queue_entry **)afl_realloc(
      (void **)&afl->dist_queue_buf,
      afl->dist_target_queue_buf_len * sizeof(struct queue_entry *));
  if (unlikely(!afl->dist_queue_buf)) { PFATAL("alloc"); }

  afl->distance_matrix_cache =
      (double **)afl_realloc((void **)&afl->distance_matrix_cache,
                             afl->dist_target_queue_buf_len * sizeof(double *));
  if (unlikely(!afl->distance_matrix_cache)) { PFATAL("alloc"); }

  /* The newly allocated pointer slots are zeroed by afl_realloc, so they are
     safe to release even if this preparation is interrupted. */
  afl->matrix_row_to_free = afl->dist_target_queue_buf_len;
  afl->dist_update_phase = DISTANCE_UPDATE_SNAPSHOT;

}

static enum distance_service_result update_distance_matrix_until(
    afl_state_t *afl, u64 deadline_us) {

  while (likely(!afl->stop_soon)) {

    switch (afl->dist_update_phase) {

      case DISTANCE_UPDATE_SNAPSHOT:

        while (afl->dist_snapshot_cursor < afl->dist_target_queue_buf_len) {

          u32 i = afl->dist_snapshot_cursor++;
          afl->dist_queue_buf[i] = afl->queue_buf[i];
          if (distance_budget_exhausted(deadline_us))
            return DISTANCE_SERVICE_BUDGET_EXHAUSTED;

        }

        afl->dist_update_phase = DISTANCE_UPDATE_RESIZE;
        break;

      case DISTANCE_UPDATE_RESIZE:

        while (afl->dist_prepare_row < afl->dist_target_queue_buf_len) {

          u32 i = afl->dist_prepare_row++;
          afl->distance_matrix_cache[i] = (double *)afl_realloc(
              (void **)&afl->distance_matrix_cache[i],
              (afl->dist_target_queue_buf_len - i - 1) * sizeof(double));
          if (unlikely(!afl->distance_matrix_cache[i])) { PFATAL("alloc"); }
          if (distance_budget_exhausted(deadline_us))
            return DISTANCE_SERVICE_BUDGET_EXHAUSTED;

        }

        /* Publish the expanded snapshot only after every matrix row is ready.
         */
        afl->dist_queue_buf_len = afl->dist_target_queue_buf_len;
        afl->dist_update_phase = DISTANCE_UPDATE_REPAIR;
        break;

      case DISTANCE_UPDATE_REPAIR:

        while (afl->dist_repair_row < afl->dist_new_row) {

          u32 j = afl->dist_repair_row;

          if (afl->dist_queue_buf[j]->disabled) {

            afl->dist_repair_row++;
            afl->dist_repair_col = 0;
            if (distance_budget_exhausted(deadline_us))
              return DISTANCE_SERVICE_BUDGET_EXHAUSTED;
            continue;

          }

          while (afl->dist_repair_col < j) {

            u32 i = afl->dist_repair_col++;
            if (!afl->dist_queue_buf[i]->disabled &&
                afl->distance_matrix_cache[i][j - i - 1] < 0) {

              double distance = get_trace_mini_distance(
                  afl, afl->dist_queue_buf[i]->trace_mini,
                  afl->dist_queue_buf[j]->trace_mini);
              afl->distance_matrix_cache[i][j - i - 1] = distance;
              afl->dist_pair_count++;

            }

            if (distance_budget_exhausted(deadline_us))
              return DISTANCE_SERVICE_BUDGET_EXHAUSTED;

          }

          afl->dist_repair_row++;
          afl->dist_repair_col = 0;

        }

        afl->dist_update_phase = DISTANCE_UPDATE_NEW_ROWS;
        break;

      case DISTANCE_UPDATE_NEW_ROWS:

        while (afl->dist_new_row < afl->dist_queue_buf_len) {

          u32 j = afl->dist_new_row;
          while (afl->dist_new_col < j) {

            u32 i = afl->dist_new_col++;
            if (afl->dist_queue_buf[i]->disabled ||
                afl->dist_queue_buf[j]->disabled) {

              afl->distance_matrix_cache[i][j - i - 1] = -1.0;

            } else {

              double distance = get_trace_mini_distance(
                  afl, afl->dist_queue_buf[i]->trace_mini,
                  afl->dist_queue_buf[j]->trace_mini);
              afl->distance_matrix_cache[i][j - i - 1] = distance;
              afl->dist_pair_count++;

            }

            if (afl->dist_new_col == j) break;
            if (distance_budget_exhausted(deadline_us))
              return DISTANCE_SERVICE_BUDGET_EXHAUSTED;

          }

          /* A row becomes visible to schedulers only after all of its pairs
             have been computed. */
          afl->last_dist_queue_buf_len = j + 1;
          afl->dist_completed_row_count++;
          afl->dist_new_row++;
          afl->dist_new_col = 0;

          if (afl->dist_new_row == afl->dist_queue_buf_len) {

            afl->dist_update_phase = DISTANCE_UPDATE_IDLE;
            return DISTANCE_SERVICE_COMPLETE;

          }

          if (distance_budget_exhausted(deadline_us))
            return DISTANCE_SERVICE_BUDGET_EXHAUSTED;

        }

        afl->dist_update_phase = DISTANCE_UPDATE_IDLE;
        return DISTANCE_SERVICE_COMPLETE;

      case DISTANCE_UPDATE_IDLE:
        return DISTANCE_SERVICE_COMPLETE;

      default:
        FATAL("Invalid distance update phase %u", afl->dist_update_phase);

    }

    if (afl->dist_update_phase != DISTANCE_UPDATE_IDLE &&
        distance_budget_exhausted(deadline_us))
      return DISTANCE_SERVICE_BUDGET_EXHAUSTED;

  }

  return DISTANCE_SERVICE_STOPPED;

}

void maybe_service_distance_matrix(afl_state_t *afl) {

  if (unlikely(afl->last_dist_queue_buf_len > afl->dist_queue_buf_len)) {

    FATAL("Invalid distance prefix: ready=%u snapshot=%u",
          afl->last_dist_queue_buf_len, afl->dist_queue_buf_len);

  }

  if (afl->dist_update_phase == DISTANCE_UPDATE_IDLE &&
      afl->queued_items <= afl->dist_queue_buf_len)
    return;

  u64 start_us = distance_monotonic_time_us();
  if (afl->dist_throttled && start_us < afl->next_dist_time_us) return;

  u8 periodic_trigger = afl->dist_throttled;
  u64 pairs_before = afl->dist_pair_count;
  u64 rows_before = afl->dist_completed_row_count;

  afl->dist_invocation_count++;
  if (periodic_trigger)
    afl->dist_periodic_invocation_count++;
  else
    afl->dist_immediate_invocation_count++;

  if (afl->dist_update_phase == DISTANCE_UPDATE_IDLE)
    begin_distance_matrix_update(afl);

  u64 deadline_us = start_us + afl->dist_time_limit_us;
  enum distance_service_result result =
      update_distance_matrix_until(afl, deadline_us);
  u64 end_us = distance_monotonic_time_us();
  u64 elapsed_us = end_us - start_us;

  afl->dist_total_time_us += elapsed_us;
  if (elapsed_us > afl->dist_max_round_time_us)
    afl->dist_max_round_time_us = elapsed_us;

  if (result == DISTANCE_SERVICE_BUDGET_EXHAUSTED) {

    afl->dist_budget_exhausted_count++;
    afl->dist_throttled = 1;
    afl->next_dist_time_us = end_us + afl->dist_period_us;

  } else if (result == DISTANCE_SERVICE_COMPLETE) {

    afl->dist_throttled = 0;
    afl->next_dist_time_us = 0;

  }

  const char *result_name =
      result == DISTANCE_SERVICE_COMPLETE
          ? "complete"
          : (result == DISTANCE_SERVICE_BUDGET_EXHAUSTED ? "budget_exhausted"
                                                         : "stopped");

  DCPF_LOG(
      "DISTANCE_ROUND invocation=%llu trigger=%s result=%s elapsed_us=%llu "
      "total_us=%llu pairs=%llu rows=%llu ready=%u snapshot=%u queued=%u",
      afl->dist_invocation_count, periodic_trigger ? "periodic" : "immediate",
      result_name, elapsed_us, afl->dist_total_time_us,
      afl->dist_pair_count - pairs_before,
      afl->dist_completed_row_count - rows_before,
      afl->last_dist_queue_buf_len, afl->dist_queue_buf_len,
      afl->queued_items);

}

void log_distance_summary(afl_state_t *afl) {

  u64 end_us = distance_monotonic_time_us();
  u64 elapsed_us = afl->dist_worker_start_us
                       ? end_us - afl->dist_worker_start_us
                       : 0;

  double average_invocation_us =
      afl->dist_invocation_count
          ? (double)afl->dist_total_time_us / afl->dist_invocation_count
          : 0.0;
  double average_pair_us =
      afl->dist_pair_count
          ? (double)afl->dist_total_time_us / afl->dist_pair_count
          : 0.0;
  double overhead_percent =
      elapsed_us ? (double)afl->dist_total_time_us * 100.0 / elapsed_us : 0.0;

  u8 pending = afl->dist_update_phase != DISTANCE_UPDATE_IDLE ||
               afl->queued_items > afl->dist_queue_buf_len;

  DCPF_LOG(
      "DISTANCE_SUMMARY invocations=%llu immediate=%llu periodic=%llu "
      "pairs=%llu rows=%llu total_us=%llu average_invocation_us=%.3f "
      "average_pair_us=%.6f max_round_us=%llu budget_exhausted=%llu "
      "worker_elapsed_us=%llu overhead_percent=%.6f pending=%u ready=%u "
      "snapshot=%u queued=%u limit_us=%llu period_ms=%llu",
      afl->dist_invocation_count, afl->dist_immediate_invocation_count,
      afl->dist_periodic_invocation_count, afl->dist_pair_count,
      afl->dist_completed_row_count, afl->dist_total_time_us,
      average_invocation_us, average_pair_us, afl->dist_max_round_time_us,
      afl->dist_budget_exhausted_count, elapsed_us, overhead_percent,
      (u32)pending,
      afl->last_dist_queue_buf_len, afl->dist_queue_buf_len,
      afl->queued_items, afl->dist_time_limit_us,
      afl->dist_period_us / 1000ULL);

}

/* redis op */

u8 sync_one_seed(afl_state_t *afl, u8 *mem, u32 len, u8 *syncing_party,
                 u8 force_add) {

  u8 sync_res;

  if (len && len <= MAX_FILE) {

    u8 fault;

    /* See what happens. We rely on save_if_interesting() to catch major
       errors and save the test case. */

    u32 new_len = write_to_testcase(afl, (void **)&mem, len, 1);

    fault = fuzz_run_target(afl, &afl->fsrv, afl->fsrv.exec_tmout);

    afl->syncing_party = syncing_party;
    afl->syncing_case = 0;
    sync_res = save_if_interesting(afl, mem, new_len, fault, force_add);
    afl->queued_imported += sync_res;
    show_stats(afl);
    afl->syncing_party = 0;

    // if (sync_res)
    //   SAYF(cGRA
    //        "    len = %u, map size = %u, exec speed = %llu us, hash = "
    //        "%016llx\n" cRST,
    //        afl->queue_top->len, afl->queue_top->bitmap_size,
    //        afl->queue_top->exec_us, afl->queue_top->exec_cksum);

  } else {

    WARNF("seed len unexpected!");
    sync_res = 0;

  }

  return sync_res;

}

void sync_subdomain_seeds_from_db(afl_state_t *afl, u32 subdomain_id) {

  afl->not_do_cooperation = 1;

  u64 sync_start_us = get_cur_time_us();

  u8  key[20];
  u8  time_tmp[64];
  u32 sync_from_db = 0;

  for (u32 global_id = afl->sync_cursor;; global_id++) {

    if (afl->stop_soon) { break; }

    // 维护一个全局种子已经同步过的哈希表
    u8 *GS_flag = afl_realloc((void **)&afl->global_seed_flag,
                              sizeof(u32) * (global_id + 1));
    if (!GS_flag) FATAL("afl_realloc error");

    if (GS_flag[global_id]) continue;

    snprintf(key, 20, "seed_%u", global_id);
    bool key_exist = true;
    if (!if_in_subdomain(afl->redis_connect, subdomain_id, key, &key_exist)) {

      if (!key_exist) {

        DCPF_DEBUG("get seed_%u from db return NULL, may be at end.",
                   global_id);
        afl->sync_cursor = global_id;
        break;

      }

      continue;

    }

    DBq *dbq = get_one_db_seed(afl->redis_connect, key);
    if (!dbq) { FATAL("GET %s error", key); }

    GS_flag[global_id] = 1;

    ++afl->master_sync_count;

    u8 result = sync_one_seed(afl, dbq->content, dbq->len, key, 0);
    if (result) {

      afl->queue_top->subdomain_id = subdomain_id;
      afl->queue_top->disabled = 0;  // 启用

      // sync seed state
      afl->queue_top->passed_det = dbq->passed_det;
      afl->queue_top->trim_done = dbq->trim_done;
      afl->queue_top->was_fuzzed = dbq->was_fuzzed;
      afl->queue_top->fuzz_level = dbq->fuzz_level;
      // afl->queue_top->handicap = 0;  // 覆盖syn时赋的值
      // afl->queue_top->depth = 0;

      ++sync_from_db;

    }

    free_dbq(dbq);

    update_sync_time(afl, &sync_start_us);

  }

  u_simplestring_time_diff(time_tmp, afl->prev_run_time + get_cur_time(),
                           afl->start_time);
  DCPF_LOG("sync subdomain seeds from db, %u seeds at %s ", sync_from_db,
           time_tmp);
  fflush(stdout);

  update_sync_time(afl, &sync_start_us);

  afl->not_do_cooperation = 0;

}

void update_subdomain(afl_state_t *afl) {

  get_one_subdomain_from_db(afl->redis_connect, &afl->sd, afl->sync_id);

  DCPF_LOG("get subdomain_%s, id=%u, r=%f", afl->sync_id, afl->sd.id,
           afl->sd.r);

  /* 重置任务相关属性 */
  afl->max_reward = -DBL_MAX;
  afl->max_potential = -DBL_MAX;
  afl->min_reward = DBL_MAX;
  afl->min_potential = DBL_MAX;

  for (u32 i = 0; i < afl->queued_items; i++) {

    struct queue_entry *q = afl->queue_buf[i];
    q->aggregation = 0;
    q->in_subdomain = 0;
    q->center_deviation = DBL_MAX;
    // q->potential = 0;
    // q->worth = 0;

  }

  /* 重置种子选择 */
  for (u32 i = 0; i < afl->fsrv.map_size; ++i) {

    if (afl->top_rated[i]) {

      afl->top_rated[i]->tc_ref = 0;
      afl->top_rated[i] = NULL;

    }

  }

  /* 更新所有旧种子的任务偏移 */

  u32 task_seed_count = 0;

  memcpy(afl->store_trace, afl->fsrv.trace_bits, afl->fsrv.map_size);
  u64 sum_size = 0;
  u64 sum_time = 0;
  for (u32 i = 0; i < afl->queued_items; i++) {

    struct queue_entry *q = afl->queue_buf[i];
    q->center_deviation =
        get_trace_mini_distance(afl, afl->sd.center->trace_mini, q->trace_mini);

    if (q->center_deviation <= afl->sd.r) {

      q->in_subdomain = 1;
      task_seed_count++;
      sum_size += q->bitmap_size;
      sum_time += q->exec_us;

    }

    // 重新选择种子, 仅限半径内
    decompress_trace_mini(afl->fsrv.trace_bits, q->trace_mini,
                          afl->fsrv.map_size);
    update_bitmap_score(afl, q);

  }

  memcpy(afl->fsrv.trace_bits, afl->store_trace, afl->fsrv.map_size);

  DCPF_DEBUG("TASK avg map_size=%f, avg exec_us=%f",
             (double)sum_time / task_seed_count,
             (double)sum_size / task_seed_count);

  /* 更新种子选择 */
  cull_queue(afl);
  DCPF_LOG("TASK has %u seeds in task", task_seed_count);
  DCPF_DEBUG("queued_favored=%u, pending_favored=%u, pending_not_fuzzed=%u",
             afl->queued_favored, afl->pending_favored,
             afl->pending_not_fuzzed);

}

void do_cooperation(afl_state_t *afl) {

  afl->not_do_cooperation = 1;  // 协同的种子自己保留，不再协同
  afl->doing_cooperation = 1;

  char   tmp[50];
  u32    coop_num = 0;
  u32    save_num = 0;
  double adjust_r = -DBL_MAX;

  u64 sync_start_us = get_cur_time_us();

  while (1) {

    redisReply *reply =
        redisCommand(afl->redis_connect, "LPOP coop_queue_%u", afl->sd.id);

    if (reply == NULL) {

      DCPF_DEBUG("LPOP Error: %s\n", afl->redis_connect->errstr);
      break;

    } else {

      // 检查是否弹出所有元素
      if (reply->type == REDIS_REPLY_NIL) {

        DCPF_LOG("  total pop %u seeds, %u saved.", coop_num, save_num);
        freeReplyObject(reply);
        break;

      }

      assert(reply->type == REDIS_REPLY_STRING);

      u8 *mem = reply->str;
      u32 len = reply->len;

      snprintf(tmp, 50, "cooperation_%u", afl->total_coop_add++);

      // 非强制添加
      ++afl->peer_sync_count;
      u8 result = sync_one_seed(afl, mem, len, tmp, 0);
      if (result) {

        save_num++;
        // 记录保留协同种子的最大中心偏离
        if (afl->queue_top->center_deviation > adjust_r)
          adjust_r = afl->queue_top->center_deviation;

      }

      fflush(stdout);

      // 释放资源
      freeReplyObject(reply);
      coop_num++;

    }

    update_sync_time(afl, &sync_start_us);

  }

  afl->not_do_cooperation = 0;
  afl->doing_cooperation = 0;  // 协同的种子自己保留，不再协同

  /* 根据半径变化调整子域范围 */

  if (adjust_r > afl->sd.r) {

    // 协同阶段调整半径
    DCPF_LOG("调整任务半径 %f -> %f", afl->sd.r, adjust_r);
    afl->sd.r = adjust_r;
    // 更新后的半径同步到数据库
    set_subdomain_radius(afl->redis_connect, afl->sync_id, afl->sd.r);

    // 子域调整
    u32 count = 0;
    memcpy(afl->store_trace, afl->fsrv.trace_bits, afl->fsrv.map_size);
    for (u32 i = 0; i < afl->queued_items; i++) {

      struct queue_entry *q = afl->queue_buf[i];
      if (!q->in_subdomain && q->center_deviation <= afl->sd.r) {

        q->in_subdomain = 1;
        // DCPF_DEBUG("  seed #%u 移动到任务内", q->id);
        ++count;
        // 更新种子选择
        decompress_trace_mini(afl->fsrv.trace_bits, q->trace_mini,
                              afl->fsrv.map_size);
        update_bitmap_score(afl, q);

      }

    }

    memcpy(afl->fsrv.trace_bits, afl->store_trace, afl->fsrv.map_size);

    DCPF_LOG("移动入任务：%u 种子", count);

  }

}

/**
 * @description: 执行负载均衡任务，消息队列取种子fuzz，结果保存且流转
 * @param {afl_state_t} *afl
 * @return {*} 下一个fuzz的种子ID， UINT32_MAX表示出错。
 */
u32 do_load_balance(afl_state_t *afl) {

  u32 ret_next_fuzz = UINT32_MAX;

  afl->not_do_cooperation = 1;
  afl->not_cal_deviation = 1;

  u32  balance_num = 0;
  char tmp[50];

  redisReply *reply = redisCommand(afl->redis_connect,
                                   "LPOP load_balance_queue_%u", afl->sd.id);

  if (reply == NULL) {

    DCPF_DEBUG("LPOP Error: %s\n", afl->redis_connect->errstr);
    ret_next_fuzz = UINT32_MAX;

  } else {

    assert(reply->type == REDIS_REPLY_STRING);

    u8 *mem = reply->str;
    u32 len = reply->len;

    snprintf(tmp, 50, "LB_%u", afl->total_LB_add++);

    /* force add the LB seed to fuzz */
    u8 result = sync_one_seed(afl, mem, len, tmp, 1);
    if (!result) {

      // may hang or crash??
      WARNF("force add error");

      ret_next_fuzz = UINT32_MAX;

    } else {

      // 种子内容缓存进内存，不持久化
      if (!afl->queue_top->testcase_buf) {

        FATAL("testcase_buf is NULL, len = %u", len);

      }

      ret_next_fuzz = afl->queue_top->id;

      balance_num++;

    }

    // 释放资源
    freeReplyObject(reply);

  }

  afl->not_do_cooperation = 0;
  afl->not_cal_deviation = 0;

  return ret_next_fuzz;

}

// InstrumentPointMap *load_instrument_map(const char *filename, u32 map_size) {

//   if (map_size > UINT32_MAX / sizeof(InstrumentPointMap))
//     FATAL("Too big memory to alloc");

//   InstrumentPointMap *ret = NULL;
//   ret = ck_alloc(sizeof(InstrumentPointMap) * map_size);
//   if (!ret) FATAL("ck_alloc error");

//   FILE *file = fopen(filename, "rb");
//   if (!file) {

//     WARNF("Failed to open file");
//     return NULL;
//   }

//   // 读取文件直到结束
//   while (!feof(file)) {

//     u32 point_id;

//     // 读取 id (4 字节)
//     if (fread(&point_id, sizeof(u32), 1, file) != 1) {

//       DCPF_DEBUG("point_id end");
//       break;  // 到文件结尾
//     }
//     if (point_id >= map_size) {

//       FATAL("Invalid point_id %u (map_size=%u)", point_id, map_size);
//       break;
//     }

//     InstrumentPointMap *cur = &ret[point_id];

//     // 读取 vector 的大小 (size_t 类型，通常为 4 或 8 字节)
//     size_t vec_size;
//     if (fread(&vec_size, sizeof(size_t), 1, file) != 1) {

//       DCPF_DEBUG("vec_size end");
//       break;  // 到文件结尾
//     }

//     // 分配内存来存储 successor points
//     cur->succ_num = vec_size;
//     assert(cur->succ_num < UINT32_MAX / sizeof(u32));
//     if (cur->succ_num > 0) {

//       cur->succ_point = (u32 *)ck_alloc(cur->succ_num * sizeof(u32));
//       if (!cur->succ_point) {

//         perror("Failed to allocate memory for successor points");
//         fclose(file);
//         return NULL;
//       }
//     }

//     // 读取 vector 中的所有元素
//     if (fread(cur->succ_point, sizeof(u32), vec_size, file) != vec_size) {

//       free(cur->succ_point);  // 释放内存
//       FATAL("read succ_point error!");
//       break;  // 到文件结尾
//     }

//     // // 处理 point_map (例如，打印 id 和 succ_point)
//     // printf("%u:", point_id);
//     // for (size_t i = 0; i < cur->succ_num; ++i) {

//     //   printf(" %u", cur->succ_point[i]);
//     // }
//     // printf("\n");

//   }

//   fclose(file);
//   return ret;
// }

// void destroy_instrument_map(InstrumentPointMap *map, u32 map_size) {

//   for (u32 i = 0; i < map_size; i++) {

//     if (!map[i].succ_point) { ck_free(map[i].succ_point); }
//   }
//   ck_free(map);
// }

// void set_bits(uint8_t *bitmap, const int *positions, int arr_len) {

//   for (int i = 0; i < arr_len; i++) {

//     const int pos = positions[i];
//     bitmap[pos >> 3] |= (uint8_t)(1U << (pos & 0x07));
//   }
// }

// // 使用编译器内置指令加速位扫描（GCC/Clang）
// #if defined(__GNUC__) || defined(__clang__)
//   #define USE_BUILTIN_CTZ 1
// #endif

// size_t find_set_bits(const uint8_t *bitmap, size_t bitmap_size,
//                      size_t *output) {

//   size_t count = 0;

//   for (size_t byte_idx = 0; byte_idx < bitmap_size; ++byte_idx) {

//     uint8_t byte = bitmap[byte_idx];
//     if (byte == 0) continue;  // 快速跳过空字节

//     // 位扫描处理非零字节
//     size_t base_pos = byte_idx * 8;

//     while (byte != 0) {

//       // 找到最低有效位的1的位置
// #if USE_BUILTIN_CTZ
//   #ifdef _WIN32
//       unsigned long bit_pos;
//       _BitScanForward(&bit_pos, byte);
//   #else
//       unsigned bit_pos = __builtin_ctz(byte);
//   #endif
// #else
//       // 手动实现CTZ（Count Trailing Zeros）
//       static const uint8_t multiply_de_bruijn_bitpos[32] = {

//           0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
//           31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};
//       unsigned bit_pos =
//           multiply_de_bruijn_bitpos[((uint32_t)(byte & -byte) * 0x077CB531U)
//           >>
//                                     27];
// #endif

//       // 记录位置并清除已处理的bit
//       output[count++] = base_pos + bit_pos;
//       byte &= byte - 1;  // 清除最低有效位的1
//     }
//   }

//   return count;
// }

// void get_seed_area(struct queue_entry *q, InstrumentPointMap *area_map, u32
// map_size) {

//   u32 trace_mini_size = map_size >> 3;
//   u8 *q_area = ck_alloc(trace_mini_size);
//   if (!q_area) FATAL("ck_alloc error");

//   u8 *q_bitmap = q->trace_mini;

//   for (u32 byte_idx = 0; byte_idx < (trace_mini_size >> 3); ++byte_idx) {

//     uint8_t byte = q_bitmap[byte_idx];
//     if (byte == 0) continue;  // 快速跳过空字节

//     // 位扫描处理非零字节
//     u32 base_pos = byte_idx * 8;

//     while (byte != 0) {

//       // 找到最低有效位的1的位置
// #if USE_BUILTIN_CTZ
//   #ifdef _WIN32
//       unsigned long bit_pos;
//       _BitScanForward(&bit_pos, byte);
//   #else
//       unsigned bit_pos = __builtin_ctz(byte);
//   #endif
// #else
//       // 手动实现CTZ（Count Trailing Zeros）
//       static const uint8_t multiply_de_bruijn_bitpos[32] = {

//           0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
//           31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};
//       unsigned bit_pos =
//           multiply_de_bruijn_bitpos[((uint32_t)(byte & -byte) * 0x077CB531U)
//           >>
//                                     27];
// #endif

//       // 记录位置并清除已处理的bit
//       u32 offset = base_pos + bit_pos;
//       q_area[offset >> 3] |= (u8)(1U << (offset & 0x07));
//       for (u32 j = 0; j < area_map[offset].succ_num; ++j) {

//         u32 pos = area_map[offset].succ_point[j];
//         q_area[pos >> 3] |= (u8)(1U << (pos & 0x07));
//       }

//       byte &= byte - 1;  // 清除最低有效位的1
//     }
//   }

//   for (u32 i = 0; i < trace_mini_size;i++)
//   {

//     if(i<trace_mini_size){

//     }
//   }
// }

// 种子选择快排
int partition(struct queue_entry **arr, int low, int high) {

  double pivot = arr[high]->priority;  // 选择最后一个元素作为枢轴
  int    i = low - 1;  // i指向较大元素的最后一个位置

  for (int j = low; j < high; j++) {

    if (arr[j]->priority > pivot) {

      i++;
      // 交换 arr[i] 和 arr[j]
      struct queue_entry *temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;

    }

  }

  // 将基准元素放到正确位置
  struct queue_entry *temp = arr[i + 1];
  arr[i + 1] = arr[high];
  arr[high] = temp;

  return i + 1;  // 返回基准元素的索引

}

// 快排函数
void quickSort(struct queue_entry **arr, int low, int high) {

  if (low < high) {

    int pivot = partition(arr, low, high);
    quickSort(arr, low, pivot - 1);
    quickSort(arr, pivot + 1, high);

  }

}

/* load balance functions */

/**
 * @description: 比较函数（对种子优先级降序排序）
 * @param {void} *a
 * @param {void} *b
 * @return {*}
 */
int compare_seed(const void *a, const void *b) {

  struct queue_entry *sa = *(struct queue_entry **)a;
  struct queue_entry *sb = *(struct queue_entry **)b;
  if (sa->priority < sb->priority) return 1;  // 降序排列
  if (sa->priority > sb->priority) return -1;
  return 0;

}

/**
 * @description: 子域 c 根据其接收窗口大小，取种子压入消息队列
 * @param {afl_state_t} *afl
 * @param {queue_entry} *
 * @param {u32} start
 * @param {subdomain} *c
 * @return {*}
 */
void send_seed_to_cluster(afl_state_t *afl, struct queue_entry *q,
                          subdomain *c) {

  s32         fd = -1;
  struct stat st;

  u8 *seed_mem = NULL;

  /* get seed content */
  if (q->testcase_buf) {

    seed_mem = q->testcase_buf;

  } else {

    fd = open((char *)q->fname, O_RDONLY);
    if (fd < 0) { return; }

    if (fstat(fd, &st)) { WARNF("fstat() failed"); }

    /* Ignore zero-sized or oversized files. */

    if (st.st_size && st.st_size <= MAX_FILE) {

      seed_mem = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

      if (seed_mem == MAP_FAILED) {

        PFATAL("Unable to mmap '%s'", (char *)q->fname);

      }

    } else {

      close(fd);
      return;

    }

  }

  redisReply *reply =
      redisCommand(afl->redis_connect, "RPUSH load_balance_queue_%u %b", c->id,
                   seed_mem, q->len);

  if (reply == NULL) {

    WARNF("Redis RPUSH failed for load_balance_queue_%u: %s", c->id,
          afl->redis_connect->errstr);
    if (!q->testcase_buf) {

      munmap(seed_mem, st.st_size);
      close(fd);

    }

    return;

  }

  freeReplyObject(reply);

  if (!q->testcase_buf) {

    munmap(seed_mem, st.st_size);
    close(fd);

  }

}

u32 select_send_domain(afl_state_t *afl) {

  u32    s = rand_below(afl, afl->subdomain_num);
  double p = rand_next_percent(afl);
  return (p < afl->send_probability[s] ? s : afl->send_alias_table[s]);

}

u32 select_recv_domain(afl_state_t *afl) {

  u32    s = rand_below(afl, afl->subdomain_num);
  double p = rand_next_percent(afl);
  return (p < afl->recv_probability[s] ? s : afl->recv_alias_table[s]);

}

/* 创建负载均衡别名表 */
void update_subdomain_alias_table(afl_state_t *afl) {

  u32 n = afl->subdomain_num, nSmall = 0, nLarge = n - 1;

  double *P = (double *)ck_alloc(n * sizeof(double));
  u32    *Small = (int *)ck_alloc(n * sizeof(u32));
  u32    *Large = (int *)ck_alloc(n * sizeof(u32));
  afl->send_alias_table =
      (u32 *)afl_realloc((void **)&afl->send_alias_table, n * sizeof(u32));
  afl->send_probability = (double *)afl_realloc((void **)&afl->send_probability,
                                                n * sizeof(double));
  afl->recv_alias_table =
      (u32 *)afl_realloc((void **)&afl->recv_alias_table, n * sizeof(u32));
  afl->recv_probability = (double *)afl_realloc((void **)&afl->recv_probability,
                                                n * sizeof(double));

  // memset((void *)afl->alias_probability, 0, n * sizeof(double));
  // memset((void *)afl->alias_table, 0, n * sizeof(u32));
  // memset((void *)Small, 0, n * sizeof(u32));
  // memset((void *)Large, 0, n * sizeof(u32));

  double send_weight = 0.0;
  double recv_weight = 0.0;
  for (u32 i = 0; i < n; i++) {

    // 非线性扩大采样频率
    afl->subdomain_array[i].sample_weight =
        pow(afl->subdomain_array[i].contribute, 2);

    send_weight += afl->subdomain_array[i].sample_weight;
    recv_weight += 1.0 / afl->subdomain_array[i].sample_weight;

  }

  /* for send */

  memset(afl->send_probability, 0, n * sizeof(double));
  memset(afl->send_alias_table, 0, n * sizeof(u32));

  for (u32 i = 0; i < n; i++) {

    P[i] = afl->subdomain_array[i].sample_weight / send_weight * n;

  }

  for (s32 j = (s32)(n - 1); j >= 0; j--) {

    if (P[j] < 1) {

      Small[nSmall++] = (u32)j;

    } else {

      Large[nLarge--] = (u32)j;

    }

  }

  while (nSmall && nLarge != n - 1) {

    u32 small = Small[--nSmall];
    u32 large = Large[++nLarge];

    afl->send_probability[small] = P[small];
    afl->send_alias_table[small] = large;

    P[large] = P[large] - (1 - P[small]);

    if (P[large] < 1) {

      Small[nSmall++] = large;

    } else {

      Large[nLarge--] = large;

    }

  }

  while (nSmall) {

    afl->send_probability[Small[--nSmall]] = 1;

  }

  while (nLarge != n - 1) {

    afl->send_probability[Large[++nLarge]] = 1;

  }

  /* for recv */

  nSmall = 0, nLarge = n - 1;
  memset((void *)Small, 0, n * sizeof(u32));
  memset((void *)Large, 0, n * sizeof(u32));
  memset(afl->recv_probability, 0, n * sizeof(double));
  memset(afl->recv_alias_table, 0, n * sizeof(u32));

  for (u32 i = 0; i < n; i++) {

    P[i] = 1.0 / afl->subdomain_array[i].sample_weight / recv_weight * n;

  }

  for (s32 j = (s32)(n - 1); j >= 0; j--) {

    if (P[j] < 1) {

      Small[nSmall++] = (u32)j;

    } else {

      Large[nLarge--] = (u32)j;

    }

  }

  while (nSmall && nLarge != n - 1) {

    u32 small = Small[--nSmall];
    u32 large = Large[++nLarge];

    afl->recv_probability[small] = P[small];
    afl->recv_alias_table[small] = large;

    P[large] = P[large] - (1 - P[small]);

    if (P[large] < 1) {

      Small[nSmall++] = large;

    } else {

      Large[nLarge--] = large;

    }

  }

  while (nSmall) {

    afl->recv_probability[Small[--nSmall]] = 1;

  }

  while (nLarge != n - 1) {

    afl->recv_probability[Large[++nLarge]] = 1;

  }

  // 释放内存
  ck_free(P);
  ck_free(Small);
  ck_free(Large);

}

/* master 在 sync_fuzzer前优先同步 LB 贡献种子，统计贡献 */
void sync_LB_seeds(afl_state_t *afl, redisContext *ctx, char *sync_id) {

  u32  pop_num = 0, save_num = 0;
  char tmp[50];

  while (1) {

    redisReply *reply = redisCommand(ctx, "LPOP %s_LB_report", sync_id);

    if (reply == NULL) {

      DCPF_DEBUG("LPOP %s_LB_report Error: %s\n", sync_id, ctx->errstr);
      break;

    } else {

      // 检查是否弹出所有元素
      if (reply->type == REDIS_REPLY_NIL) {

        DCPF_LOG("  total pop %u seeds, %u saved.", pop_num, save_num);
        if (save_num > 0) {

          // 记录贡献
          int      subdomain_id = -1;
          gpointer value = g_hash_table_lookup(afl->Iname_Sid_map, sync_id);
          if (value != NULL) { subdomain_id = GPOINTER_TO_INT(value) - 1; }
          if (subdomain_id >= 0) {

            afl->subdomain_array[subdomain_id].LB_contribute += save_num;
            afl->subdomain_array[subdomain_id].total_contribute += save_num;

            // DCPF_LOG("cluster %u LB contribute= %u", subdomain_id,
            //          afl->subdomain_array[subdomain_id].contribute);

          }

        }

        freeReplyObject(reply);
        break;

      }

      assert(reply->type == REDIS_REPLY_STRING);

      u8 *mem = reply->str;
      u32 len = reply->len;

      snprintf(tmp, 50, "LB_%u", afl->total_LB_add++);

      // 非强制添加
      u8 result = sync_one_seed(afl, mem, len, tmp, 0);
      if (result) { save_num++; }

      // 释放资源
      freeReplyObject(reply);
      pop_num++;

    }

  }

}

/* 扫描 LB 贡献队列 */
void scan_LB_report(afl_state_t *afl) {

  redisContext *ctx = afl->redis_connect;
  char          cursor[64] = "0";
  redisReply   *reply;

  do {

    reply = redisCommand(ctx, "SCAN %s MATCH *_LB_report COUNT 1000", cursor);
    if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements != 2) {

      if (reply) freeReplyObject(reply);
      DCPF_DEBUG("Error during SCAN");
      return;

    }

    // 更新游标
    snprintf(cursor, sizeof(cursor), "%s", reply->element[0]->str);

    // 遍历匹配的 keys
    redisReply *keys = reply->element[1];
    for (size_t i = 0; i < keys->elements; i++) {

      const char *key = keys->element[i]->str;

      // 提取 sync_id（去掉 "_LB_report" 后缀）
      size_t len = strlen(key);
      if (len <= strlen("_LB_report")) continue;

      char sync_id[256] = {0};
      snprintf(sync_id, sizeof(sync_id), "%.*s",
               (int)(len - strlen("_LB_report")), key);

      // 弹出元素
      // DCPF_DEBUG("LPOP %s_LB_report", sync_id);
      sync_LB_seeds(afl, ctx, sync_id);

    }

    freeReplyObject(reply);

  } while (strcmp(cursor, "0") != 0);

}
