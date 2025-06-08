#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

// For sleep on Windows/Linux (optional, for observation)
// #ifdef _WIN32
// #include <windows.h>
// #else
// #include <unistd.h>
// #endif

#define NUM_RESOURCES 3 // 资源种类 A, B, C
#define NUM_PROCESSES 5 // 进程数量

// 资源名称
char resource_names[NUM_RESOURCES] = {'A', 'B', 'C'};

// 系统可用资源
int available[NUM_RESOURCES] = {10, 15, 12};

// 进程状态枚举
typedef enum {
    WAIT,   // 就绪 (Ready)
    RUN,    // 运行 (Running)
    BLOCK,  // 阻塞 (Blocked)
    FINISH  // 完成 (Finished)
} ProcessState;

// 进程控制块 (PCB)
typedef struct {
    int pid;                            // 进程ID
    int max[NUM_RESOURCES];             // 最大需求
    int allocation[NUM_RESOURCES];      // 已分配资源
    int need[NUM_RESOURCES];            // 还需要资源 (Max - Allocation)
    ProcessState state;                 // 进程状态
} PCB;

PCB processes[NUM_PROCESSES];
PCB *running_process = NULL; // 当前运行进程

// 队列
PCB *ready_queue[NUM_PROCESSES];
int ready_front = -1, ready_rear = -1;

PCB *block_queue[NUM_PROCESSES];
int block_front = -1, block_rear = -1;

PCB *finish_queue[NUM_PROCESSES]; // 完成队列，仅用作记录
int finish_count = 0;             // 完成进程计数

// --- 队列操作 ---
// 将进程加入队列
void enqueue(PCB *queue[], int *front, int *rear, PCB *p) {
    if (*rear == NUM_PROCESSES - 1) {
        // printf("Queue is full.\n"); // 队列已满，理论上不会发生（进程数固定）
        return;
    }
    if (*front == -1) { // 队列为空时，设置front
        *front = 0;
    }
    (*rear)++;
    queue[*rear] = p;
}

// 从队列中取出进程
PCB* dequeue(PCB *queue[], int *front, int *rear) {
    if (*front == -1 || *front > *rear) { // 队列为空
        return NULL;
    }
    PCB *p = queue[*front];
    (*front)++;
    if (*front > *rear) { // 如果取出后队列为空，重置front和rear
        *front = -1;
        *rear = -1;
    }
    return p;
}

// 检查队列是否为空
bool is_queue_empty(int front, int rear) {
    return front == -1 || front > rear;
}

// --- 银行家算法相关函数 ---

// 检查向量a是否小于等于向量b (a <= b)
bool less_equal(int a[], int b[]) {
    for (int i = 0; i < NUM_RESOURCES; i++) {
        if (a[i] > b[i]) {
            return false;
        }
    }
    return true;
}

// 安全性算法：检查当前系统是否处于安全状态
bool is_safe(int current_available[NUM_RESOURCES]) {
    int work[NUM_RESOURCES];
    // 初始化工作向量Work为当前的Available
    for (int i = 0; i < NUM_RESOURCES; i++) {
        work[i] = current_available[i];
    }

    bool finish[NUM_PROCESSES] = {false}; // 记录进程是否已完成
    int safe_sequence[NUM_PROCESSES];     // 记录安全序列
    int count = 0;                        // 已找到的安全进程数量

    // 复制进程状态，确保不修改原始进程数据
    // 在安全性检查中，只考虑未完成（非FINISH状态）的进程
    PCB temp_processes[NUM_PROCESSES];
    for(int i = 0; i < NUM_PROCESSES; i++) {
        temp_processes[i] = processes[i];
    }

    int loop_iterations = 0; // 防止无限循环，最多检查 NUM_PROCESSES * 2 次
    while (count < NUM_PROCESSES && loop_iterations < NUM_PROCESSES * 2) {
        bool found = false; // 标记是否找到可以执行的进程
        for (int i = 0; i < NUM_PROCESSES; i++) {
            // 如果进程i尚未完成安全性检查，且其Need <= Work
            if (finish[i] == false && temp_processes[i].state != FINISH && less_equal(temp_processes[i].need, work)) {
                // 模拟分配资源并释放
                for (int j = 0; j < NUM_RESOURCES; j++) {
                    work[j] += temp_processes[i].allocation[j]; // 加上已分配资源（模拟进程完成并释放）
                }
                finish[i] = true;                 // 标记为已完成
                safe_sequence[count++] = temp_processes[i].pid; // 加入安全序列
                found = true;                     // 找到一个进程
            }
        }
        if (!found) { // 如果一轮遍历后没有找到任何可以满足的进程，则系统不安全
            break;
        }
        loop_iterations++;
    }

    if (count == NUM_PROCESSES) { // 如果找到了所有进程的安全序列
        printf("安全序列: ");
        for (int i = 0; i < NUM_PROCESSES; i++) {
            printf("P%d ", safe_sequence[i]);
        }
        printf("\n");
        return true;
    }
    return false; // 没有找到安全序列
}

// 资源请求算法：尝试为进程P分配资源Request
bool request_resources(PCB *p, int request[NUM_RESOURCES]) {
    // 1. 检查请求资源是否超过其最大需求 (Need)
    if (!less_equal(request, p->need)) {
        printf("P%d 请求资源 (%d,%d,%d) 超过其剩余需求 (%d,%d,%d)。请求非法！\n",
               p->pid, request[0], request[1], request[2], p->need[0], p->need[1], p->need[2]);
        return false;
    }

    // 2. 检查请求资源是否超过当前可用资源 (Available)
    if (!less_equal(request, available)) {
        printf("P%d 请求资源 (%d,%d,%d) 暂时无法满足 (Available: %d,%d,%d)，进入阻塞队列。\n",
               p->pid, request[0], request[1], request[2], available[0], available[1], available[2]);
        p->state = BLOCK;
        enqueue(block_queue, &block_front, &block_rear, p);
        return false;
    }

    // 3. 尝试分配资源，并进行安全性检查
    // 试分配：假设资源已分配
    for (int i = 0; i < NUM_RESOURCES; i++) {
        available[i] -= request[i];
        p->allocation[i] += request[i];
        p->need[i] -= request[i];
    }

    if (is_safe(available)) { // 如果试分配后系统仍安全
        printf("P%d 请求资源 (%d,%d,%d) 成功分配！\n",
               p->pid, request[0], request[1], request[2]);
        return true;
    } else { // 如果试分配后系统不安全，则回滚分配
        printf("P%d 请求资源 (%d,%d,%d) 会导致系统进入不安全状态，拒绝分配。进入阻塞队列。\n",
               p->pid, request[0], request[1], request[2]);
        for (int i = 0; i < NUM_RESOURCES; i++) {
            available[i] += request[i];       // 归还资源
            p->allocation[i] -= request[i];   // 撤销分配
            p->need[i] += request[i];         // 撤销需求更新
        }
        p->state = BLOCK;
        enqueue(block_queue, &block_front, &block_rear, p);
        return false;
    }
}

// 释放资源：进程完成后释放所有已分配资源
void release_resources(PCB *p) {
    for (int i = 0; i < NUM_RESOURCES; i++) {
        available[i] += p->allocation[i]; // 将已分配资源归还给Available
        p->allocation[i] = 0;             // 清零进程的已分配资源
        p->need[i] = 0;                   // 进程已完成，需求清零
    }
    printf("P%d 完成并释放所有资源。当前Available: (%d,%d,%d)\n",
           p->pid, available[0], available[1], available[2]);
}

// --- 初始化和打印函数 ---

// 初始化所有进程的PCB数据
void init_processes() {
    srand(time(NULL)); // 初始化随机数种子

    printf("--- 初始化进程数据 ---\n");
    for (int i = 0; i < NUM_PROCESSES; i++) {
        processes[i].pid = i;
        processes[i].state = WAIT; // 初始状态为就绪

        // 随机生成Max需求，确保不超过总资源且合理
        for (int j = 0; j < NUM_RESOURCES; j++) {
            // 资源A: 1-5, B: 1-7, C: 1-6 (相对合理范围，不超过总资源的一半)
            processes[i].max[j] = 1 + rand() % (available[j] / 2 + 1);
        }

        // 随机生成初始已分配资源 (Allocation)，保证不超Max且不超Available
        for (int j = 0; j < NUM_RESOURCES; j++) {
            // 已分配资源不能超过其最大需求，也不能超过当前Available
            int max_possible_alloc = (processes[i].max[j] > available[j]) ? available[j] : processes[i].max[j];
            processes[i].allocation[j] = rand() % (max_possible_alloc + 1);
            available[j] -= processes[i].allocation[j]; // 从Available中扣除
        }

        // 计算Need
        for (int j = 0; j < NUM_RESOURCES; j++) {
            processes[i].need[j] = processes[i].max[j] - processes[i].allocation[j];
        }

        // 初始所有进程进入就绪队列
        enqueue(ready_queue, &ready_front, &ready_rear, &processes[i]);

        printf("P%d: Max=(%d,%d,%d), Allocation=(%d,%d,%d), Need=(%d,%d,%d)\n",
               processes[i].pid,
               processes[i].max[0], processes[i].max[1], processes[i].max[2],
               processes[i].allocation[0], processes[i].allocation[1], processes[i].allocation[2],
               processes[i].need[0], processes[i].need[1], processes[i].need[2]);
    }
    printf("初始 Available: (%d,%d,%d)\n", available[0], available[1], available[2]);
    if (!is_safe(available)) {
        printf("警告：初始状态可能不安全！这可能会导致很快有进程阻塞。\n");
    }
    printf("------------------------\n\n");
}

// 打印当前系统状态和所有进程的PCB信息
void print_status() {
    printf("\n--- 当前系统状态 ---\n");
    printf("可用资源 (Available): (%d,%d,%d)\n", available[0], available[1], available[2]);

    printf("正在运行的进程: ");
    if (running_process) {
        printf("P%d\n", running_process->pid);
    } else {
        printf("无\n");
    }

    printf("就绪队列 (Ready Queue): ");
    if (is_queue_empty(ready_front, ready_rear)) {
        printf("空\n");
    } else {
        for (int i = ready_front; i <= ready_rear; i++) {
            printf("P%d ", ready_queue[i]->pid);
        }
        printf("\n");
    }

    printf("阻塞队列 (Block Queue): ");
    if (is_queue_empty(block_front, block_rear)) {
        printf("空\n");
    } else {
        for (int i = block_front; i <= block_rear; i++) {
            printf("P%d ", block_queue[i]->pid);
        }
        printf("\n");
    }

    printf("完成队列 (Finish Queue): ");
    if (finish_count == 0) {
        printf("空\n");
    } else {
        for (int i = 0; i < finish_count; i++) {
            printf("P%d ", finish_queue[i]->pid);
        }
        printf("\n");
    }

    printf("所有进程PCB信息:\n");
    printf("PID | State  | Max (A,B,C) | Alloc (A,B,C) | Need (A,B,C)\n");
    printf("----------------------------------------------------------------\n");
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("P%-2d | %-6s | (%2d,%2d,%2d) | (%2d,%2d,%2d) | (%2d,%2d,%2d)\n",
               processes[i].pid,
               (processes[i].state == WAIT) ? "Wait" :
               (processes[i].state == RUN) ? "Run" :
               (processes[i].state == BLOCK) ? "Block" : "Finish",
               processes[i].max[0], processes[i].max[1], processes[i].max[2],
               processes[i].allocation[0], processes[i].allocation[1], processes[i].allocation[2],
               processes[i].need[0], processes[i].need[1], processes[i].need[2]);
    }
    printf("------------------------\n\n");
}

// --- 主模拟循环 ---
int main() {
    init_processes();

    int time_slice = 1; // 每个进程运行的时间片 (这里简化为一次调度)
    int turn = 0;       // 调度轮次计数

    while (finish_count < NUM_PROCESSES) { // 当未完成进程数小于总进程数时，继续模拟
        turn++;
        printf("=========== 调度轮次 %d ===========\n", turn);

        // 尝试唤醒阻塞队列中的进程
        // 遍历阻塞队列，尝试分配资源
        if (!is_queue_empty(block_front, block_rear)) {
            printf("尝试唤醒阻塞队列中的进程...\n");
            // 注意：这里需要创建一个临时队列来重新构建阻塞队列，因为可能会有进程被移除
            PCB *temp_block_queue[NUM_PROCESSES];
            int temp_block_front = -1, temp_block_rear = -1;

            while (!is_queue_empty(block_front, block_rear)) {
                PCB *p = dequeue(block_queue, &block_front, &block_rear); // 从原阻塞队列取出
                if (p == NULL) break;

                int request[NUM_RESOURCES];
                bool can_request = true;
                for(int j=0; j<NUM_RESOURCES; j++){
                     // 随机请求 Need 内的一部分资源，但不能超过当前Available
                     request[j] = rand() % (p->need[j] + 1);
                     if (request[j] > available[j]) { // 如果随机请求超过Available，则无法满足
                         can_request = false;
                         break;
                     }
                }

                if (can_request && request_resources(p, request)) { // 尝试请求资源 (会调用银行家算法)
                    printf("P%d 从阻塞队列唤醒，并成功分配资源，进入就绪队列。\n", p->pid);
                    p->state = WAIT;
                    enqueue(ready_queue, &ready_front, &ready_rear, p);
                } else {
                    // 如果仍然无法满足或导致不安全，重新放入阻塞队列
                    printf("P%d 仍在阻塞队列中等待。\n", p->pid);
                    enqueue(temp_block_queue, &temp_block_front, &temp_block_rear, p);
                }
            }
            // 将临时阻塞队列的内容复制回主阻塞队列
            block_front = temp_block_front;
            block_rear = temp_block_rear;
            for(int i = (block_front == -1 ? 0 : block_front); i <= block_rear; i++) {
                block_queue[i] = temp_block_queue[i];
            }
        }


        // 时间片轮转调度：从就绪队列中取出进程执行
        running_process = dequeue(ready_queue, &ready_front, &ready_rear);

        if (running_process) {
            running_process->state = RUN;
            printf("P%d 正在运行。\n", running_process->pid);

            bool process_finished_in_this_turn = false;

            // 检查进程是否已达到最大需求（Need是否为0）
            bool all_needed_allocated = true;
            for(int i=0; i<NUM_RESOURCES; i++){
                if(running_process->need[i] > 0){
                    all_needed_allocated = false;
                    break;
                }
            }

            if(all_needed_allocated){ // 如果已完成
                printf("P%d 已达到最大需求，即将完成。\n", running_process->pid);
                running_process->state = FINISH;
                finish_queue[finish_count++] = running_process; // 直接加入完成队列
                release_resources(running_process);
                process_finished_in_this_turn = true;
            } else { // 否则，随机申请资源
                printf("P%d 随机申请资源...\n", running_process->pid);
                int request[NUM_RESOURCES];
                for (int i = 0; i < NUM_RESOURCES; i++) {
                    request[i] = rand() % (running_process->need[i] + 1); // 随机申请 Need 内的一部分
                }
                printf("P%d 请求: (%d,%d,%d)\n",
                       running_process->pid, request[0], request[1], request[2]);

                if (request_resources(running_process, request)) { // 尝试分配资源
                    // 资源分配成功后，再次检查是否达到最大需求
                    bool finished_after_request = true;
                    for (int i = 0; i < NUM_RESOURCES; i++) {
                        if (running_process->need[i] > 0) {
                            finished_after_request = false;
                            break;
                        }
                    }

                    if (finished_after_request) {
                        printf("P%d 已达到最大需求，即将完成。\n", running_process->pid);
                        running_process->state = FINISH;
                        finish_queue[finish_count++] = running_process; // 直接加入完成队列
                        release_resources(running_process);
                        process_finished_in_this_turn = true;
                    }
                }
            }

            // 如果进程没有完成且没有阻塞，则重新进入就绪队列等待下一轮调度
            if (!process_finished_in_this_turn && running_process->state != BLOCK) {
                running_process->state = WAIT;
                enqueue(ready_queue, &ready_front, &ready_rear, running_process);
            }
        } else {
            printf("就绪队列为空，系统空闲或所有进程都已完成/阻塞。\n");
            // 如果所有进程都阻塞，且无法唤醒（在当前轮次），则可能发生死锁，或者需要更长时间等待
            if (is_queue_empty(ready_front, ready_rear) && !is_queue_empty(block_front, block_rear) && finish_count < NUM_PROCESSES) {
                printf("系统可能进入死锁状态 (所有进程阻塞且无法继续前进)。\n");
                break; // 结束模拟
            }
        }

        print_status();

        // 可选：添加一个小的延迟，便于观察输出
        // #ifdef _WIN32
        // Sleep(500); // 暂停0.5秒 (Windows)
        // #else
        // usleep(500000); // 暂停0.5秒 (Linux/Unix)
        // #endif
    }

    printf("\n--- 所有进程已完成或系统进入死锁状态，模拟结束。---\n");

    return 0;
}
