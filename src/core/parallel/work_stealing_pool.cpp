/**
 * @file work_stealing_pool.cpp
 * @brief 工作窃取线程池与 parallel_for 实现（FR-PERF-001，07 册 §2.2）。
 * @details 池为 parallel_for 的实现细节（每次调用按需创建销毁，无进程级全局状态）：
 *          per-worker deque（mutex 保护）本地 LIFO 取任务、空则窃取他人队头 FIFO；
 *          块任务自带异常吸收与完成计数，出错后未启动的块快速跳过，计数必然归零
 *          （无死锁）；首个异常经 atomic 择优保留，在汇合点重抛。
 */
#include "work_stealing_pool.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

namespace opensuperdea::parallel
{
namespace
{
/** @brief 工作窃取线程池：worker 数固定，本地队列 LIFO + 窃取队头 FIFO。 */
class WorkStealingPool
{
  public:
    explicit WorkStealingPool(std::size_t thread_count)
        : queues(thread_count), queue_mutexes(thread_count)
    {
        workers.reserve(thread_count);
        for (std::size_t id = 0; id < thread_count; ++id)
        {
            workers.emplace_back(
                [this, id](std::stop_token stop_token) { worker_loop(std::move(stop_token), id); });
        }
    }

    ~WorkStealingPool()
    {
        // 先显式停机并 join 全部 worker，再让成员（cv/mutex/队列）按序销毁——
        // 若依赖成员逆序销毁，wake_cv/wake_mutex 会先于 workers 析构，
        // 轮询中的 worker 访问已销毁的同步原语属 UB（实测挂死）
        for (std::jthread &worker : workers)
        {
            worker.request_stop();
        }
        {
            std::scoped_lock wake_lock(wake_mutex);
            is_stopped = true;
        }
        wake_cv.notify_all();
        for (std::jthread &worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }

    /** @brief 提交一个任务（轮转分配到本地队列，唤醒一个 worker）。 */
    void submit(std::function<void()> task)
    {
        const std::size_t id = next_queue.fetch_add(1, std::memory_order_relaxed) % queues.size();
        {
            std::scoped_lock queue_lock(queue_mutexes[id]);
            queues[id].push_back(std::move(task));
        }
        {
            std::scoped_lock wake_lock(wake_mutex);
            ++queued_count;
        }
        wake_cv.notify_one();
    }

    /**
     * @brief 阻塞直至无排队且无执行中任务（出错时由块任务的快速跳过保证到达）。
     */
    void wait_idle()
    {
        std::unique_lock<std::mutex> wake_lock(wake_mutex);
        wake_cv.wait(wake_lock, [this] { return queued_count == 0 && running_count == 0; });
    }

  private:
    void worker_loop(std::stop_token stop_token, std::size_t id)
    {
        while (!stop_token.stop_requested())
        {
            std::function<void()> task = take_local(id);
            if (task == nullptr)
            {
                task = steal(id);
            }
            if (task == nullptr)
            {
                // 本地与窃取皆空：等待新任务或停止（worker 生命周期仅由析构 stop 控制，
                // 不得以「无任务」退出——池创建与任务提交之间存在窗口，提前退出会
                // 使后续任务无人执行、wait_idle 永眠）
                std::unique_lock<std::mutex> wake_lock(wake_mutex);
                wake_cv.wait_for(wake_lock, std::chrono::milliseconds(2),
                                 [&stop_token] { return stop_token.stop_requested(); });
                if (stop_token.stop_requested())
                {
                    return;
                }
                continue;
            }
            {
                std::scoped_lock wake_lock(wake_mutex);
                --queued_count;
                ++running_count;
            }
            task(); // 块任务自带异常吸收，此处不抛
            {
                std::scoped_lock wake_lock(wake_mutex);
                --running_count;
                if (queued_count == 0 && running_count == 0)
                {
                    wake_cv.notify_all(); // 唤醒 wait_idle 与空闲 worker 退出
                }
            }
        }
    }

    /** @brief 本地队列尾取任务（LIFO，块任务连续性提升缓存局部性）。 */
    std::function<void()> take_local(std::size_t id)
    {
        std::scoped_lock queue_lock(queue_mutexes[id]);
        if (queues[id].empty())
        {
            return nullptr;
        }
        std::function<void()> task = std::move(queues[id].back());
        queues[id].pop_back();
        return task;
    }

    /** @brief 窃取他人队列头（FIFO，与本地 LIFO 错端操作降低锁竞争）。 */
    std::function<void()> steal(std::size_t id)
    {
        for (std::size_t offset = 1; offset <= queues.size(); ++offset)
        {
            const std::size_t victim = (id + offset) % queues.size();
            std::scoped_lock queue_lock(queue_mutexes[victim]);
            if (!queues[victim].empty())
            {
                std::function<void()> task = std::move(queues[victim].front());
                queues[victim].pop_front();
                return task;
            }
        }
        return nullptr;
    }

    std::vector<std::deque<std::function<void()>>> queues;
    std::vector<std::mutex> queue_mutexes;
    std::vector<std::jthread> workers;
    std::mutex wake_mutex;
    std::condition_variable wake_cv;
    std::size_t queued_count = 0;  // 受 wake_mutex 保护
    std::size_t running_count = 0; // 受 wake_mutex 保护
    std::atomic<std::size_t> next_queue{0};
    bool is_stopped = false;
};
} // namespace

void parallel_for(std::size_t count, unsigned int thread_count,
                  const std::function<void(std::size_t)> &body)
{
    if (count == 0)
    {
        return;
    }
    unsigned int hardware = std::thread::hardware_concurrency();
    if (hardware == 0)
    {
        hardware = 1;
    }
    const unsigned int effective = thread_count == 0 ? hardware : thread_count;
    if (effective <= 1U || count == 1)
    {
        // 串行退化（PR-09 对照路径）：与原 for 循环行为一致
        for (std::size_t i = 0; i < count; ++i)
        {
            body(i);
        }
        return;
    }

    // 块划分（07 册 §2.2）：块大小 = max(1, count / (threads*16))，尾部对齐
    const auto worker_count = static_cast<std::size_t>(effective);
    const std::size_t block_size = std::max<std::size_t>(1, count / (worker_count * 16));
    const std::size_t block_count = (count + block_size - 1) / block_size;

    std::atomic<bool> has_error{false};
    std::mutex error_mutex; // exception_ptr 非平凡可拷贝，不能 atomic
    std::exception_ptr first_error = nullptr;

    WorkStealingPool pool(worker_count);
    for (std::size_t block = 0; block < block_count; ++block)
    {
        const std::size_t lower = block * block_size;
        const std::size_t upper = std::min(count, lower + block_size);
        pool.submit([&body, &has_error, &error_mutex, &first_error, lower, upper] {
            if (has_error.load(std::memory_order_relaxed))
            {
                return; // 已出错：未启动的块快速跳过（完成计数不受影响）
            }
            try
            {
                for (std::size_t i = lower; i < upper; ++i)
                {
                    if (has_error.load(std::memory_order_relaxed))
                    {
                        return;
                    }
                    body(i);
                }
            }
            catch (...)
            {
                // 首个异常择优保留（07 册 §2.4 语义），其余吞掉
                std::scoped_lock error_lock(error_mutex);
                if (first_error == nullptr)
                {
                    first_error = std::current_exception();
                }
                has_error.store(true, std::memory_order_relaxed);
            }
        });
    }
    pool.wait_idle();

    {
        std::scoped_lock error_lock(error_mutex);
        if (first_error != nullptr)
        {
            std::rethrow_exception(first_error); // 归并点重抛（EngineException 语义透传）
        }
    }
}
} // namespace opensuperdea::parallel
