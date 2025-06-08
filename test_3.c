#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_MEM_SIZE 640 // 初始内存大小 640KB
#define MAX_JOBS     10 // 最大作业数量

// 定义内存分区结构体
typedef struct Partition {
    int start_address; // 分区起始地址
    int size;          // 分区大小
    bool is_free;      // 是否空闲
    char job_name[20]; // 如果非空闲，记录作业名
    struct Partition *next; // 指向下一个分区
} Partition;

// 空闲分区链表头指针 (按地址排序)
Partition *free_partitions_head = NULL;
// 已分配分区链表头指针
Partition *allocated_partitions_head = NULL;

// --- 辅助函数 ---

// 创建新的分区节点
Partition* create_partition(int start, int size, bool is_free, const char* job_name) {
    Partition *new_node = (Partition*)malloc(sizeof(Partition));
    if (new_node == NULL) {
        perror("Failed to allocate memory for partition");
        exit(EXIT_FAILURE);
    }
    new_node->start_address = start;
    new_node->size = size;
    new_node->is_free = is_free;
    strcpy(new_node->job_name, job_name);
    new_node->next = NULL;
    return new_node;
}

// 打印内存状态
void print_memory_status() {
    printf("\n--- 当前内存状态 ---\n");
    printf("已分配分区:\n");
    if (allocated_partitions_head == NULL) {
        printf("  (无)\n");
    } else {
        Partition *current = allocated_partitions_head;
        while (current != NULL) {
            printf("  作业名: %-8s | 起始地址: %4dKB | 大小: %4dKB\n",
                   current->job_name, current->start_address, current->size);
            current = current->next;
        }
    }

    printf("\n空闲分区链表 (按地址排序):\n");
    if (free_partitions_head == NULL) {
        printf("  (无)\n");
    } else {
        Partition *current = free_partitions_head;
        while (current != NULL) {
            printf("  起始地址: %4dKB | 大小: %4dKB\n", current->start_address, current->size);
            current = current->next;
        }
    }
    printf("--------------------\n");
}

// 将新空闲分区插入空闲分区链表 (按地址排序，并进行合并)
void insert_free_partition(Partition *new_free_partition) {
    // 1. 找到插入位置
    Partition *current = free_partitions_head;
    Partition *prev = NULL;

    while (current != NULL && current->start_address < new_free_partition->start_address) {
        prev = current;
        current = current->next;
    }

    // 插入新分区
    if (prev == NULL) { // 插入到链表头
        new_free_partition->next = free_partitions_head;
        free_partitions_head = new_free_partition;
    } else { // 插入到链表中间或尾部
        new_free_partition->next = current;
        prev->next = new_free_partition;
    }

    // 2. 合并空闲分区 (向后合并)
    if (new_free_partition->next != NULL &&
        (new_free_partition->start_address + new_free_partition->size == new_free_partition->next->start_address)) {
        
        Partition *next_free = new_free_partition->next;
        new_free_partition->size += next_free->size;
        new_free_partition->next = next_free->next;
        free(next_free); // 释放被合并的分区节点
    }

    // 3. 合并空闲分区 (向前合并)
    if (prev != NULL &&
        (prev->start_address + prev->size == new_free_partition->start_address)) {
        
        prev->size += new_free_partition->size;
        prev->next = new_free_partition->next;
        free(new_free_partition); // 释放被合并的分区节点
    }
}

// 从已分配分区链表中移除分区
void remove_allocated_partition(const char *job_name) {
    Partition *current = allocated_partitions_head;
    Partition *prev = NULL;

    while (current != NULL && strcmp(current->job_name, job_name) != 0) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("错误: 未找到作业 %s 的已分配分区。\n", job_name);
        return;
    }

    if (prev == NULL) { // 移除头节点
        allocated_partitions_head = current->next;
    } else { // 移除中间或尾部节点
        prev->next = current->next;
    }
    // 注意：这里不free(current)，因为要将其转换为空闲分区
}


// --- 分配算法 ---

// 首次适应算法
Partition* first_fit(int request_size) {
    Partition *current = free_partitions_head;
    Partition *prev = NULL;

    while (current != NULL) {
        if (current->size >= request_size) { // 找到第一个足够大的空闲分区
            // 从空闲分区链表中移除
            if (prev == NULL) {
                free_partitions_head = current->next;
            } else {
                prev->next = current->next;
            }

            // 判断是否需要分裂
            if (current->size - request_size > 0) { // 需要分裂
                Partition *new_free_part = create_partition(current->start_address + request_size,
                                                            current->size - request_size,
                                                            true, "");
                insert_free_partition(new_free_part); // 将剩余部分重新插入空闲链表
                current->size = request_size; // 更新当前分配分区的大小
            }
            current->is_free = false;
            return current; // 返回找到的分区
        }
        prev = current;
        current = current->next;
    }
    return NULL; // 未找到合适分区
}

// 最佳适应算法
Partition* best_fit(int request_size) {
    Partition *current = free_partitions_head;
    Partition *best_fit_part = NULL;
    Partition *best_fit_prev = NULL;
    Partition *prev = NULL;
    int min_fragment = MAX_MEM_SIZE + 1; // 记录最小碎片大小

    while (current != NULL) {
        if (current->size >= request_size) {
            int fragment = current->size - request_size;
            if (fragment < min_fragment) {
                min_fragment = fragment;
                best_fit_part = current;
                best_fit_prev = prev; // 记录最佳适应分区的前一个节点
            }
        }
        prev = current;
        current = current->next;
    }

    if (best_fit_part != NULL) { // 找到最佳适应分区
        // 从空闲分区链表中移除最佳适应分区
        if (best_fit_prev == NULL) {
            free_partitions_head = best_fit_part->next;
        } else {
            best_fit_prev->next = best_fit_part->next;
        }

        // 判断是否需要分裂
        if (best_fit_part->size - request_size > 0) { // 需要分裂
            Partition *new_free_part = create_partition(best_fit_part->start_address + request_size,
                                                        best_fit_part->size - request_size,
                                                        true, "");
            insert_free_partition(new_free_part); // 将剩余部分重新插入空闲链表
            best_fit_part->size = request_size; // 更新当前分配分区的大小
        }
        best_fit_part->is_free = false;
        return best_fit_part; // 返回找到的分区
    }
    return NULL; // 未找到合适分区
}


// --- 内存管理操作 ---

// 内存分配
void allocate_memory(const char *job_name, int request_size, int algorithm_choice) {
    printf("\n--- 申请内存 ---\n");
    printf("作业名: %s, 申请大小: %dKB\n", job_name, request_size);

    // 检查作业是否已存在
    Partition *temp_alloc = allocated_partitions_head;
    while(temp_alloc != NULL){
        if(strcmp(temp_alloc->job_name, job_name) == 0){
            printf("错误: 作业 %s 已经分配了内存。请勿重复分配。\n", job_name);
            return;
        }
        temp_alloc = temp_alloc->next;
    }

    Partition *allocated_part = NULL;
    if (algorithm_choice == 1) { // 首次适应
        printf("使用首次适应算法...\n");
        allocated_part = first_fit(request_size);
    } else if (algorithm_choice == 2) { // 最佳适应
        printf("使用最佳适应算法...\n");
        allocated_part = best_fit(request_size);
    } else {
        printf("无效的算法选择。\n");
        return;
    }

    if (allocated_part != NULL) {
        strcpy(allocated_part->job_name, job_name);
        allocated_part->is_free = false;

        // 将新分配的分区加入已分配链表 (可以按地址排序，也可以直接头插/尾插)
        // 这里采用头插法，简化操作
        allocated_part->next = allocated_partitions_head;
        allocated_partitions_head = allocated_part;
        printf("成功为作业 %s 分配 %dKB 内存，起始地址: %dKB。\n",
               job_name, request_size, allocated_part->start_address);
    } else {
        printf("内存不足！无法为作业 %s 分配 %dKB 内存。\n", job_name, request_size);
    }
    print_memory_status();
}

// 内存回收
void free_memory(const char *job_name) {
    printf("\n--- 回收内存 ---\n");
    printf("作业名: %s\n", job_name);

    Partition *current_alloc = allocated_partitions_head;
    Partition *prev_alloc = NULL;
    Partition *recycled_part = NULL;

    // 找到要回收的分区
    while (current_alloc != NULL && strcmp(current_alloc->job_name, job_name) != 0) {
        prev_alloc = current_alloc;
        current_alloc = current_alloc->next;
    }

    if (current_alloc == NULL) {
        printf("错误: 未找到作业 %s 的已分配分区，无法回收。\n", job_name);
        return;
    }

    recycled_part = current_alloc;

    // 从已分配链表中移除该分区
    if (prev_alloc == NULL) { // 移除头节点
        allocated_partitions_head = current_alloc->next;
    } else { // 移除中间或尾部节点
        prev_alloc->next = current_alloc->next;
    }

    // 将回收的分区转换为空闲分区并插入空闲链表
    recycled_part->is_free = true;
    strcpy(recycled_part->job_name, ""); // 清空作业名
    insert_free_partition(recycled_part); // 插入并尝试合并

    printf("成功回收作业 %s 的 %dKB 内存，起始地址: %dKB。\n",
           job_name, recycled_part->size, recycled_part->start_address);
    print_memory_status();
}

// 清理所有内存
void cleanup_memory() {
    Partition *current = free_partitions_head;
    while (current != NULL) {
        Partition *temp = current;
        current = current->next;
        free(temp);
    }
    free_partitions_head = NULL;

    current = allocated_partitions_head;
    while (current != NULL) {
        Partition *temp = current;
        current = current->next;
        free(temp);
    }
    allocated_partitions_head = NULL;
    printf("\n所有内存已清理。\n");
}

// --- 主函数 ---
int main() {
    // 初始状态：整个内存作为一个大空闲分区
    free_partitions_head = create_partition(0, MAX_MEM_SIZE, true, "");
    printf("初始内存状态 (总大小: %dKB):\n", MAX_MEM_SIZE);
    print_memory_status();

    // 模拟请求序列
    typedef struct {
        char job_name[20];
        char operation_type[10]; // "申请" or "释放"
        int size;                // 申请/释放的大小
    } Request;

    Request requests[] = {
        {"作业1", "申请", 130},
        {"作业2", "申请", 60},
        {"作业3", "申请", 100},
        {"作业2", "释放", 60}, // 释放作业2
        {"作业4", "申请", 200},
        {"作业3", "释放", 100}, // 释放作业3
        {"作业1", "释放", 130}, // 释放作业1
        {"作业5", "申请", 140},
        {"作业6", "申请", 60},
        {"作业7", "申请", 50},
        {"作业8", "申请", 60}
    };
    int num_requests = sizeof(requests) / sizeof(requests[0]);

    int choice;
    printf("请选择内存分配算法:\n");
    printf("1. 首次适应算法 (First Fit)\n");
    printf("2. 最佳适应算法 (Best Fit)\n");
    printf("请输入数字 (1或2): ");
    scanf("%d", &choice);

    if (choice != 1 && choice != 2) {
        printf("无效的选择。程序将退出。\n");
        return 1;
    }

    for (int i = 0; i < num_requests; ++i) {
        printf("\n\n=============== 执行请求 %d: %s %s %dKB ===============\n",
               i + 1, requests[i].job_name, requests[i].operation_type, requests[i].size);

        if (strcmp(requests[i].operation_type, "申请") == 0) {
            allocate_memory(requests[i].job_name, requests[i].size, choice);
        } else if (strcmp(requests[i].operation_type, "释放") == 0) {
            free_memory(requests[i].job_name);
        } else {
            printf("未知操作类型: %s\n", requests[i].operation_type);
        }
    }

    cleanup_memory(); // 清理所有动态分配的内存

    return 0;
}