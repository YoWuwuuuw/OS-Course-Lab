#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h> // For INT_MAX
#include <math.h>   // For abs()
#include <time.h>   // For srand()

#define MAX_CYLINDER 199 // 磁盘磁道范围从0到199
#define MIN_CYLINDER 0   // 最小磁道号
#define NUM_REQUESTS 10  // 磁盘请求数量

// 生成随机磁盘请求
void generate_requests(int requests[], int num) {
    printf("生成的磁盘请求: [");
    for (int i = 0; i < num; i++) {
        requests[i] = rand() % (MAX_CYLINDER + 1); // 0到MAX_CYLINDER之间的随机磁道
        printf("%d%s", requests[i], (i == num - 1) ? "" : ", ");
    }
    printf("]\n");
}

// 打印结果
void print_results(const char *algorithm_name, const int served_sequence[], int served_count, int total_movement) {
    printf("\n--- %s 算法结果 ---\n", algorithm_name);
    printf("磁头移动顺序: ");
    for (int i = 0; i < served_count; i++) {
        printf("%d%s", served_sequence[i], (i == served_count - 1) ? "" : " -> ");
    }
    printf("\n");
    printf("总磁头移动磁道数: %d 磁道\n", total_movement);
    // 平均移动磁道数 = 总移动磁道数 / 请求数量
    printf("平均磁头移动磁道数: %.2f 磁道\n", (float)total_movement / NUM_REQUESTS);
}

// FCFS (先来先服务) 算法
void fcfs(int initial_head, int requests[], int num_requests) {
    int current_head = initial_head;
    int total_movement = 0;
    int served_sequence[NUM_REQUESTS + 1]; // +1 用于记录初始磁头位置
    served_sequence[0] = initial_head; // 记录初始位置

    printf("\n--- FCFS (先来先服务) 模拟 ---\n");
    printf("初始磁头位置: %d\n", initial_head);

    for (int i = 0; i < num_requests; i++) {
        int seek_distance = abs(requests[i] - current_head);
        total_movement += seek_distance;
        current_head = requests[i];
        served_sequence[i + 1] = current_head; // 记录服务后的磁头位置
        printf("服务请求 %d (磁道 %d)。磁头从 %d 移动到 %d。寻道距离: %d\n",
               i + 1, requests[i], served_sequence[i], current_head, seek_distance);
    }
    print_results("FCFS", served_sequence, num_requests + 1, total_movement);
}

// SSTF (最短寻道时间优先) 算法
void sstf(int initial_head, int requests[], int num_requests) {
    int current_head = initial_head;
    int total_movement = 0;
    bool visited[NUM_REQUESTS]; // 标记请求是否已被服务
    int served_sequence[NUM_REQUESTS + 1];
    served_sequence[0] = initial_head;
    int served_count = 1; // 已服务序列中的元素数量

    for (int i = 0; i < NUM_REQUESTS; i++) {
        visited[i] = false; // 初始化所有请求为未访问
    }

    printf("\n--- SSTF (最短寻道时间优先) 模拟 ---\n");
    printf("初始磁头位置: %d\n", initial_head);

    for (int i = 0; i < num_requests; i++) { // 循环直到所有请求被服务
        int min_seek = INT_MAX; // 最小寻道距离
        int next_request_index = -1; // 下一个要服务的请求索引

        // 查找距离当前磁头最近的未被服务的请求
        for (int j = 0; j < num_requests; j++) {
            if (!visited[j]) { // 如果请求未被服务
                int seek = abs(requests[j] - current_head);
                if (seek < min_seek) {
                    min_seek = seek;
                    next_request_index = j;
                }
            }
        }

        if (next_request_index != -1) { // 如果找到了请求
            total_movement += min_seek;
            current_head = requests[next_request_index];
            visited[next_request_index] = true; // 标记为已访问
            served_sequence[served_count++] = current_head;
            printf("服务最近请求 (磁道 %d)。磁头移动到 %d。寻道距离: %d\n",
                   requests[next_request_index], current_head, min_seek);
        } else {
            // 没有更多未服务的请求，退出循环
            break;
        }
    }
    print_results("SSTF", served_sequence, served_count, total_movement);
}

// 辅助函数：对数组进行升序排序 (冒泡排序)
void sort_array(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

// SCAN (扫描/电梯) 算法
void scan(int initial_head, int prev_head, int requests[], int num_requests) {
    int current_head = initial_head;
    int total_movement = 0;
    // 最多 NUM_REQUESTS + 初始位置 + 2个边界（0和MAX_CYLINDER）
    int served_sequence[NUM_REQUESTS + 3];
    served_sequence[0] = initial_head;
    int served_count = 1;
    bool served_status[NUM_REQUESTS]; // 跟踪 sorted_requests 中对应请求是否已服务

    // 复制请求并排序
    int sorted_requests[NUM_REQUESTS];
    for (int i = 0; i < num_requests; i++) {
        sorted_requests[i] = requests[i];
        served_status[i] = false; // 初始化为未服务
    }
    sort_array(sorted_requests, num_requests);

    printf("\n--- SCAN (扫描/电梯) 模拟 ---\n");
    printf("初始磁头位置: %d (来自 %d)\n", initial_head, prev_head);

    // 确定初始扫描方向：如果磁头从80到100，则初始方向是向上
    bool moving_up_initially = (initial_head >= prev_head);

    // 找到在排序数组中，第一个大于等于 initial_head 的请求的索引
    int start_idx_for_first_pass = 0;
    for (int i = 0; i < num_requests; i++) {
        if (sorted_requests[i] >= initial_head) {
            start_idx_for_first_pass = i;
            break;
        }
        // 如果所有请求都小于 initial_head，则从头开始扫描，start_idx_for_first_pass保持0
        if (i == num_requests - 1) { // 遍历完所有请求，都小于 initial_head
            start_idx_for_first_pass = num_requests; // 标记为无此方向上的请求
        }
    }

    if (moving_up_initially) {
        // 第一阶段：向上扫描到 MAX_CYLINDER，服务沿途请求
        for (int i = start_idx_for_first_pass; i < num_requests; i++) {
            total_movement += abs(sorted_requests[i] - current_head);
            current_head = sorted_requests[i];
            served_sequence[served_count++] = current_head;
            served_status[i] = true; // 标记为已服务
            printf("服务请求 (磁道 %d)。磁头移动到 %d。\n", sorted_requests[i], current_head);
        }
        // 移动到 MAX_CYLINDER 边界
        int seek_to_max_boundary = abs(MAX_CYLINDER - current_head);
        total_movement += seek_to_max_boundary;
        current_head = MAX_CYLINDER;
        served_sequence[served_count++] = current_head;
        printf("移动到边界 (磁道 %d)。寻道距离: %d\n", current_head, seek_to_max_boundary);

        // 第二阶段：反向（向下）扫描到 MIN_CYLINDER，服务剩余请求
        for (int i = num_requests - 1; i >= 0; i--) { // 从大到小遍历排序后的请求
            if (!served_status[i]) { // 如果请求尚未被服务
                total_movement += abs(sorted_requests[i] - current_head);
                current_head = sorted_requests[i];
                served_sequence[served_count++] = current_head;
                served_status[i] = true;
                printf("服务请求 (磁道 %d)。磁头移动到 %d。\n", sorted_requests[i], current_head);
            }
        }
        // 移动到 MIN_CYLINDER 边界 (如果还没到)
        if (current_head != MIN_CYLINDER) {
             int seek_to_min_boundary = abs(MIN_CYLINDER - current_head);
             total_movement += seek_to_min_boundary;
             current_head = MIN_CYLINDER;
             served_sequence[served_count++] = current_head;
             printf("移动到边界 (磁道 %d)。寻道距离: %d\n", current_head, seek_to_min_boundary);
        }

    } else { // 初始向下扫描 (本实验要求不涉及此情况，但代码提供完整性)
        // 第一阶段：向下扫描到 MIN_CYLINDER，服务沿途请求
        // 找到在排序数组中，第一个小于等于 initial_head 的请求的索引 (从大到小找)
        start_idx_for_first_pass = num_requests - 1;
        for (int i = num_requests - 1; i >= 0; i--) {
            if (sorted_requests[i] <= initial_head) {
                start_idx_for_first_pass = i;
                break;
            }
            if (i == 0) { // 所有请求都大于 initial_head
                start_idx_for_first_pass = -1; // 标记为无此方向上的请求
            }
        }

        for (int i = start_idx_for_first_pass; i >= 0; i--) {
            total_movement += abs(sorted_requests[i] - current_head);
            current_head = sorted_requests[i];
            served_sequence[served_count++] = current_head;
            served_status[i] = true;
            printf("服务请求 (磁道 %d)。磁头移动到 %d。\n", sorted_requests[i], current_head);
        }
        // 移动到 MIN_CYLINDER 边界
        int seek_to_min_boundary = abs(MIN_CYLINDER - current_head);
        total_movement += seek_to_min_boundary;
        current_head = MIN_CYLINDER;
        served_sequence[served_count++] = current_head;
        printf("移动到边界 (磁道 %d)。寻道距离: %d\n", current_head, seek_to_min_boundary);

        // 第二阶段：反向（向上）扫描到 MAX_CYLINDER，服务剩余请求
        for (int i = 0; i < num_requests; i++) { // 从小到大遍历排序后的请求
            if (!served_status[i]) {
                total_movement += abs(sorted_requests[i] - current_head);
                current_head = sorted_requests[i];
                served_sequence[served_count++] = current_head;
                served_status[i] = true;
                printf("服务请求 (磁道 %d)。磁头移动到 %d。\n", sorted_requests[i], current_head);
            }
        }
        // 移动到 MAX_CYLINDER 边界 (如果还没到)
        if (current_head != MAX_CYLINDER) {
            int seek_to_max_boundary = abs(MAX_CYLINDER - current_head);
            total_movement += seek_to_max_boundary;
            current_head = MAX_CYLINDER;
            served_sequence[served_count++] = current_head;
            printf("移动到边界 (磁道 %d)。寻道距离: %d\n", current_head, seek_to_max_boundary);
        }
    }

    print_results("SCAN", served_sequence, served_count, total_movement);
}


// C-SCAN (循环扫描) 算法
void cscan(int initial_head, int prev_head, int requests[], int num_requests) {
    int current_head = initial_head;
    int total_movement = 0;
    // 最多 NUM_REQUESTS + 初始位置 + 2个边界（MAX_CYLINDER 和 0）
    int served_sequence[NUM_REQUESTS + 3];
    served_sequence[0] = initial_head;
    int served_count = 1;
    bool served_status[NUM_REQUESTS]; // 跟踪 sorted_requests 中对应请求是否已服务

    // 复制请求并排序
    int sorted_requests[NUM_REQUESTS];
    for (int i = 0; i < num_requests; i++) {
        sorted_requests[i] = requests[i];
        served_status[i] = false;
    }
    sort_array(sorted_requests, num_requests);

    printf("\n--- C-SCAN (循环扫描) 模拟 ---\n");
    printf("初始磁头位置: %d (来自 %d)\n", initial_head, prev_head);

    bool moving_up_initially = (initial_head >= prev_head); // 从80到100表示向上

    // 找到在排序数组中，第一个大于等于 initial_head 的请求的索引
    int start_idx_for_first_pass = 0;
    for (int i = 0; i < num_requests; i++) {
        if (sorted_requests[i] >= initial_head) {
            start_idx_for_first_pass = i;
            break;
        }
        if (i == num_requests - 1) {
            start_idx_for_first_pass = num_requests; // 所有请求都小于 initial_head
        }
    }


    if (moving_up_initially) {
        // 第一阶段：向上扫描到 MAX_CYLINDER，服务沿途请求
        for (int i = start_idx_for_first_pass; i < num_requests; i++) {
            total_movement += abs(sorted_requests[i] - current_head);
            current_head = sorted_requests[i];
            served_sequence[served_count++] = current_head;
            served_status[i] = true;
            printf("服务请求 (磁道 %d)。磁头移动到 %d。\n", sorted_requests[i], current_head);
        }
        // 移动到 MAX_CYLINDER 边界 (如果还没到)
        if (current_head != MAX_CYLINDER) {
            int seek_to_boundary = abs(MAX_CYLINDER - current_head);
            total_movement += seek_to_boundary;
            current_head = MAX_CYLINDER;
            served_sequence[served_count++] = current_head;
            printf("移动到边界 (磁道 %d)。寻道距离: %d\n", current_head, seek_to_boundary);
        }

        // 跳过：从 MAX_CYLINDER 迅速跳到 MIN_CYLINDER (不服务请求)
        int jump_movement = abs(MAX_CYLINDER - MIN_CYLINDER);
        total_movement += jump_movement; // 跳跃也算作移动距离
        current_head = MIN_CYLINDER;
        served_sequence[served_count++] = current_head; // 记录跳跃终点
        printf("从 %d 跳跃到 %d (C-SCAN 循环)。寻道距离: %d\n", MAX_CYLINDER, MIN_CYLINDER, jump_movement);

        // 第二阶段：从 MIN_CYLINDER 继续向上扫描，服务剩余请求 (那些最初小于 initial_head 的)
        for (int i = 0; i < num_requests; i++) {
            if (!served_status[i]) { // 如果请求尚未被服务
                total_movement += abs(sorted_requests[i] - current_head);
                current_head = sorted_requests[i];
                served_sequence[served_count++] = current_head;
                served_status[i] = true;
                printf("服务请求 (磁道 %d)。磁头移动到 %d。\n", sorted_requests[i], current_head);
            }
        }
    } else { // 初始向下扫描 (本实验要求不涉及此情况，但代码提供完整性)
        // 找到在排序数组中，第一个小于等于 initial_head 的请求的索引 (从大到小找)
        start_idx_for_first_pass = num_requests - 1;
        for (int i = num_requests - 1; i >= 0; i--) {
            if (sorted_requests[i] <= initial_head) {
                start_idx_for_first_pass = i;
                break;
            }
            if (i == 0) { // 所有请求都大于 initial_head
                start_idx_for_first_pass = -1;
            }
        }

        // 第一阶段：向下扫描到 MIN_CYLINDER，服务沿途请求
        for (int i = start_idx_for_first_pass; i >= 0; i--) {
            total_movement += abs(sorted_requests[i] - current_head);
            current_head = sorted_requests[i];
            served_sequence[served_count++] = current_head;
            served_status[i] = true;
            printf("服务请求 (磁道 %d)。磁头移动到 %d。\n", sorted_requests[i], current_head);
        }
        // 移动到 MIN_CYLINDER 边界
        if (current_head != MIN_CYLINDER) {
            int seek_to_boundary = abs(MIN_CYLINDER - current_head);
            total_movement += seek_to_boundary;
            current_head = MIN_CYLINDER;
            served_sequence[served_count++] = current_head;
            printf("移动到边界 (磁道 %d)。寻道距离: %d\n", current_head, seek_to_boundary);
        }

        // 跳过：从 MIN_CYLINDER 迅速跳到 MAX_CYLINDER
        int jump_movement = abs(MAX_CYLINDER - MIN_CYLINDER);
        total_movement += jump_movement;
        current_head = MAX_CYLINDER;
        served_sequence[served_count++] = current_head;
        printf("从 %d 跳跃到 %d (C-SCAN 循环)。寻道距离: %d\n", MIN_CYLINDER, MAX_CYLINDER, jump_movement);

        // 第二阶段：从 MAX_CYLINDER 继续向下扫描，服务剩余请求
        for (int i = num_requests - 1; i >= 0; i--) {
            if (!served_status[i]) {
                total_movement += abs(sorted_requests[i] - current_head);
                current_head = sorted_requests[i];
                served_sequence[served_count++] = current_head;
                served_status[i] = true;
                printf("服务请求 (磁道 %d)。磁头移动到 %d。\n", sorted_requests[i], current_head);
            }
        }
    }
    print_results("C-SCAN", served_sequence, served_count, total_movement);
}

int main() {
    srand(time(NULL)); // 初始化随机数种子

    int requests[NUM_REQUESTS];
    int initial_head_position = 100;
    int previous_head_position = 80; // 对于 SCAN/C-SCAN，表示磁头从80移动到100，即初始方向为向上

    generate_requests(requests, NUM_REQUESTS); // 生成10个随机请求

    // 为每种算法创建请求数组的副本，确保它们独立模拟
    int fcfs_requests[NUM_REQUESTS];
    int sstf_requests[NUM_REQUESTS];
    int scan_requests[NUM_REQUESTS];
    int cscan_requests[NUM_REQUESTS];

    for (int i = 0; i < NUM_REQUESTS; i++) {
        fcfs_requests[i] = requests[i];
        sstf_requests[i] = requests[i];
        scan_requests[i] = requests[i];
        cscan_requests[i] = requests[i];
    }

    fcfs(initial_head_position, fcfs_requests, NUM_REQUESTS);
    sstf(initial_head_position, sstf_requests, NUM_REQUESTS);
    scan(initial_head_position, previous_head_position, scan_requests, NUM_REQUESTS);
    cscan(initial_head_position, previous_head_position, cscan_requests, NUM_REQUESTS);

    return 0;
}
