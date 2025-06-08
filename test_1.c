#include <stdio.h>    // 标准输入输出库
#include <stdlib.h>   // 标准库，用于 rand() 和 srand()
#include <string.h>   // 字符串操作库，用于 memcpy()
#include <time.h>     // 时间库，用于 time()，为 rand() 提供种子

#define MAX_PROCESSES 5   // 最大进程数
#define TIME_SLICE 2      // 时间片轮转算法的时间片大小

// 进程控制块 (PCB) 结构体
typedef struct {
    char name[10];      // 进程名
    int priority;       // 优先级（可用于高响应比计算）
    int arrival_time;   // 到达时间
    int burst_time;     // 需要运行时间（进程长度）
    int remaining_time; // 剩余运行时间
    int start_time;     // 进程开始运行的时间
    int finish_time;    // 进程完成的时间
    int turnaround_time; // 周转时间
    char state;         // 进程状态：'W' (等待/就绪), 'R' (运行), 'F' (完成)
} PCB;

// 函数：用随机数据初始化进程
void initializeProcesses(PCB processes[], int num_processes) {
    srand(time(NULL)); // 使用当前时间作为随机数种子

    processes[0].arrival_time = 0; // 第一个进程在时间 0 到达
    for (int i = 0; i < num_processes; i++) {
        sprintf(processes[i].name, "P%d", i + 1); // 设置进程名，例如 P1, P2
        if (i > 0) {
            processes[i].arrival_time = rand() % 10; // 其他进程随机到达时间（0-9之间）
        }
        processes[i].burst_time = rand() % 15 + 5; // 随机爆发时间（运行时间，5-19之间）
        processes[i].remaining_time = processes[i].burst_time; // 初始剩余时间等于爆发时间
        processes[i].start_time = -1; // 初始开始时间为 -1，表示未开始
        processes[i].finish_time = -1; // 初始完成时间为 -1，表示未完成
        processes[i].turnaround_time = 0; // 初始周转时间为 0
        processes[i].state = 'W'; // 初始状态为等待/就绪
    }
}

// 函数：打印当前模拟状态
void printStatus(int current_time, PCB processes[], int num_processes, int running_process_idx) {
    printf("\n--- 时间: %d ---\n", current_time); // 打印当前时间
    printf("正在运行的进程: ");
    if (running_process_idx != -1) { // 如果有进程正在运行
        printf("%s\n", processes[running_process_idx].name); // 打印其名称
    } else {
        printf("无\n"); // 没有进程运行
    }

    printf("就绪队列: ");
    int ready_count = 0;
    for (int i = 0; i < num_processes; i++) {
        if (processes[i].state == 'W') { // 查找处于等待/就绪状态的进程
            printf("%s ", processes[i].name);
            ready_count++;
        }
    }
    if (ready_count == 0) {
        printf("空"); // 就绪队列为空
    }
    printf("\n");

    printf("已完成进程: ");
    int finished_count = 0;
    for (int i = 0; i < num_processes; i++) {
        if (processes[i].state == 'F') { // 查找已完成的进程
            printf("%s ", processes[i].name);
            finished_count++;
        }
    }
    if (finished_count == 0) {
        printf("无"); // 没有已完成的进程
    }
    printf("\n");

    printf("--- PCB 信息 ---\n");
    // 打印 PCB 表头
    printf("%-8s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\n",
           "进程", "到达时间", "爆发时间", "剩余时间", "开始时间", "完成时间", "周转时间", "状态");
    // 遍历并打印每个进程的 PCB 信息
    for (int i = 0; i < num_processes; i++) {
        printf("%-8s%-12d%-12d%-12d%-12d%-12d%-12d%-12c\n",
               processes[i].name, processes[i].arrival_time, processes[i].burst_time,
               processes[i].remaining_time, processes[i].start_time, processes[i].finish_time,
               processes[i].turnaround_time, processes[i].state);
    }
}

// 函数：计算平均周转时间
double calculateAverageTurnaroundTime(PCB processes[], int num_processes) {
    double total_turnaround_time = 0;
    for (int i = 0; i < num_processes; i++) {
        total_turnaround_time += processes[i].turnaround_time; // 累加所有进程的周转时间
    }
    return total_turnaround_time / num_processes; // 返回平均值
}

// --- 短进程优先 (SJF) 调度算法 ---
void SJF_scheduling(PCB processes[], int num_processes) {
    printf("\n\n=== 短进程优先 (SJF) 调度 ===\n");
    // 复制原始进程数据，避免影响其他算法
    PCB sjf_processes[MAX_PROCESSES];
    memcpy(sjf_processes, processes, sizeof(PCB) * num_processes);

    int current_time = 0; // 当前时间
    int completed_processes = 0; // 已完成进程数

    while (completed_processes < num_processes) { // 循环直到所有进程完成
        int shortest_job_idx = -1; // 最短作业的索引
        int min_burst_time = 9999; // 最小爆发时间（初始化为一个大值）

        // 查找已到达且处于就绪状态的最短作业
        for (int i = 0; i < num_processes; i++) {
            if (sjf_processes[i].state == 'W' && sjf_processes[i].arrival_time <= current_time) {
                if (sjf_processes[i].burst_time < min_burst_time) {
                    min_burst_time = sjf_processes[i].burst_time;
                    shortest_job_idx = i;
                }
            }
        }

        if (shortest_job_idx != -1) { // 如果找到了最短作业
            sjf_processes[shortest_job_idx].state = 'R'; // 设置为运行状态
            if (sjf_processes[shortest_job_idx].start_time == -1) {
                sjf_processes[shortest_job_idx].start_time = current_time; // 记录开始时间
            }

            printStatus(current_time, sjf_processes, num_processes, shortest_job_idx); // 打印当前状态

            // 进程运行到完成（SJF是不可抢占的）
            current_time += sjf_processes[shortest_job_idx].remaining_time;
            sjf_processes[shortest_job_idx].remaining_time = 0; // 剩余时间归零
            sjf_processes[shortest_job_idx].state = 'F'; // 设置为完成状态
            sjf_processes[shortest_job_idx].finish_time = current_time; // 记录完成时间
            sjf_processes[shortest_job_idx].turnaround_time =
                sjf_processes[shortest_job_idx].finish_time - sjf_processes[shortest_job_idx].arrival_time; // 计算周转时间

            completed_processes++; // 完成进程数加一
        } else {
            current_time++; // 如果没有进程就绪，时间前进
        }
    }
    printStatus(current_time, sjf_processes, num_processes, -1); // 打印最终状态
    printf("\nSJF 平均周转时间: %.2f\n", calculateAverageTurnaroundTime(sjf_processes, num_processes));
}

// --- 时间片轮转 (RR) 调度算法 ---
void RR_scheduling(PCB processes[], int num_processes) {
    printf("\n\n=== 时间片轮转 (RR) 调度 (时间片: %d) ===\n", TIME_SLICE);
    // 复制原始进程数据
    PCB rr_processes[MAX_PROCESSES];
    memcpy(rr_processes, processes, sizeof(PCB) * num_processes);

    int current_time = 0; // 当前时间
    int completed_processes = 0; // 已完成进程数
    int queue[MAX_PROCESSES]; // RR 的就绪队列
    int front = 0, rear = -1; // 队列的头和尾指针

    // 将时间 0 到达的进程加入队列
    for (int i = 0; i < num_processes; i++) {
        if (rr_processes[i].arrival_time == 0) {
            rear++;
            queue[rear] = i;
        }
    }

    while (completed_processes < num_processes) { // 循环直到所有进程完成
        int running_process_idx = -1;
        if (front <= rear) { // 如果队列不为空
            running_process_idx = queue[front]; // 取出队列头部的进程
            front++; // 队列头部指针后移
        }

        if (running_process_idx != -1) { // 如果有进程可以运行
            rr_processes[running_process_idx].state = 'R'; // 设置为运行状态
            if (rr_processes[running_process_idx].start_time == -1) {
                rr_processes[running_process_idx].start_time = current_time; // 记录开始时间
            }

            printStatus(current_time, rr_processes, num_processes, running_process_idx); // 打印当前状态

            // 计算本次运行的时间
            int time_to_run = (rr_processes[running_process_idx].remaining_time > TIME_SLICE) ?
                              TIME_SLICE : rr_processes[running_process_idx].remaining_time;

            current_time += time_to_run; // 更新当前时间
            rr_processes[running_process_idx].remaining_time -= time_to_run; // 更新剩余运行时间

            // 将在此时间片内到达的新进程添加到就绪队列
            for (int i = 0; i < num_processes; i++) {
                if (rr_processes[i].state == 'W' && rr_processes[i].arrival_time > (current_time - time_to_run) && rr_processes[i].arrival_time <= current_time) {
                    // 检查是否已经在队列中，避免重复添加
                    int already_in_queue = 0;
                    for(int q_idx = front; q_idx <= rear; q_idx++) {
                        if (queue[q_idx] == i) {
                            already_in_queue = 1;
                            break;
                        }
                    }
                    if (!already_in_queue) {
                         rear++;
                         queue[rear] = i;
                    }
                }
            }


            if (rr_processes[running_process_idx].remaining_time == 0) { // 如果进程完成
                rr_processes[running_process_idx].state = 'F'; // 设置为完成状态
                rr_processes[running_process_idx].finish_time = current_time; // 记录完成时间
                rr_processes[running_process_idx].turnaround_time =
                    rr_processes[running_process_idx].finish_time - rr_processes[running_process_idx].arrival_time; // 计算周转时间
                completed_processes++; // 完成进程数加一
            } else { // 如果进程未完成，则放回就绪队列尾部
                rr_processes[running_process_idx].state = 'W'; // 设置为等待/就绪状态
                rear++; // 队列尾部指针后移
                queue[rear] = running_process_idx; // 将进程索引放入队列尾部
            }
        } else {
             // 如果就绪队列为空，查找下一个将要到达的进程，并快进时间
            int next_arrival_time = 99999; // 初始化为一个很大的时间
            for (int i = 0; i < num_processes; i++) {
                if (rr_processes[i].state == 'W' && rr_processes[i].arrival_time > current_time && rr_processes[i].arrival_time < next_arrival_time) {
                    next_arrival_time = rr_processes[i].arrival_time;
                }
            }
            if (next_arrival_time != 99999) { // 如果找到了下一个到达的进程
                current_time = next_arrival_time; // 时间跳到该进程到达时间
                 // 将所有在当前时间到达的进程加入队列
                 for (int i = 0; i < num_processes; i++) {
                    if (rr_processes[i].state == 'W' && rr_processes[i].arrival_time == current_time) {
                        rear++;
                        queue[rear] = i;
                    }
                }
            } else {
                current_time++; // 没有新进程，时间只前进一单位
            }
        }
    }
    printStatus(current_time, rr_processes, num_processes, -1); // 打印最终状态
    printf("\nRR 平均周转时间: %.2f\n", calculateAverageTurnaroundTime(rr_processes, num_processes));
}

// --- 高响应比优先 (HRRF) 调度算法 ---
void HRRF_scheduling(PCB processes[], int num_processes) {
    printf("\n\n=== 高响应比优先 (HRRF) 调度 ===\n");
    // 复制原始进程数据
    PCB hrrf_processes[MAX_PROCESSES];
    memcpy(hrrf_processes, processes, sizeof(PCB) * num_processes);

    int current_time = 0; // 当前时间
    int completed_processes = 0; // 已完成进程数

    while (completed_processes < num_processes) { // 循环直到所有进程完成
        int hrrf_process_idx = -1; // 具有最高响应比的进程索引
        double max_response_ratio = -1.0; // 最高响应比（初始化为一个小值）

        // 查找已到达且处于就绪状态的进程中响应比最高的
        for (int i = 0; i < num_processes; i++) {
            if (hrrf_processes[i].state == 'W' && hrrf_processes[i].arrival_time <= current_time) {
                double waiting_time = current_time - hrrf_processes[i].arrival_time; // 等待时间
                // 响应比 = (等待时间 + 运行时间) / 运行时间
                double response_ratio = (waiting_time + hrrf_processes[i].burst_time) / hrrf_processes[i].burst_time;

                if (response_ratio > max_response_ratio) { // 如果找到更高的响应比
                    max_response_ratio = response_ratio;
                    hrrf_process_idx = i;
                }
            }
        }

        if (hrrf_process_idx != -1) { // 如果找到了高响应比进程
            hrrf_processes[hrrf_process_idx].state = 'R'; // 设置为运行状态
            if (hrrf_processes[hrrf_process_idx].start_time == -1) {
                hrrf_processes[hrrf_process_idx].start_time = current_time; // 记录开始时间
            }

            printStatus(current_time, hrrf_processes, num_processes, hrrf_process_idx); // 打印当前状态

            // 进程运行到完成（HRRF是不可抢占的）
            current_time += hrrf_processes[hrrf_process_idx].remaining_time;
            hrrf_processes[hrrf_process_idx].remaining_time = 0; // 剩余时间归零
            hrrf_processes[hrrf_process_idx].state = 'F'; // 设置为完成状态
            hrrf_processes[hrrf_process_idx].finish_time = current_time; // 记录完成时间
            hrrf_processes[hrrf_process_idx].turnaround_time =
                hrrf_processes[hrrf_process_idx].finish_time - hrrf_processes[hrrf_process_idx].arrival_time; // 计算周转时间

            completed_processes++; // 完成进程数加一
        } else {
            current_time++; // 如果没有进程就绪，时间前进
        }
    }
    printStatus(current_time, hrrf_processes, num_processes, -1); // 打印最终状态
    printf("\nHRRF 平均周转时间: %.2f\n", calculateAverageTurnaroundTime(hrrf_processes, num_processes));
}


int main() {
    PCB processes[MAX_PROCESSES]; // 定义进程数组

    // 运行 SJF 调度
    initializeProcesses(processes, MAX_PROCESSES); // 初始化进程
    SJF_scheduling(processes, MAX_PROCESSES);      // 执行 SJF 调度

    // 运行 RR 调度
    initializeProcesses(processes, MAX_PROCESSES); // 重新初始化进程，确保各算法使用相同初始数据
    RR_scheduling(processes, MAX_PROCESSES);       // 执行 RR 调度

    // 运行 HRRF 调度
    initializeProcesses(processes, MAX_PROCESSES); // 再次重新初始化进程
    HRRF_scheduling(processes, MAX_PROCESSES);     // 执行 HRRF 调度

    return 0; // 程序正常结束
}