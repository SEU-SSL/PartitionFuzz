# PartitionFuzz

Implemented based on AFL++ 4.21

## **Key Features**

**Domain Partition.** PartitionFuzz introduces domain partitioning to enhance parallel fuzzing efficiency. By applying the *Partitioning Around Medoids* (PAM) algorithm to executed seeds, it clusters the input domain into disjoint subdomains based on *Hamming distance*. Each subdomain, represented by its *medoid seed*, is assigned to a dedicated fuzzing instance, which enables focused and non-overlapping exploration of diverse program behaviors.

**Subdomain Exploration.** Within each subdomain, PartitionFuzz strengthens the fuzzing process by incorporating seed distance information into *Adaptive Random Testing* (ART). This integration prioritizes stray seeds and promotes evenly distributed test case generation across the subdomain. As a result, the exploration avoids redundant efforts, improves coverage density, and increases the likelihood of discovering hard-to-reach program states.

**Collaborative Scheduling** To achieve global optimization, PartitionFuzz introduces *domain-aware* collaborative scheduling mechanisms. *Selective synchronization* shares only seeds that fall outside a local subdomain, effectively reducing synchronization overhead. In addition, *delegate fuzzing* dynamically redistributes seeds among slave instances to balance workloads, ensuring efficient utilization of parallel resources while maintaining subdomain integrity.

[Supplementary Results of Experiments are also avaiable](experiment/ExperimentalResults.pdf)
