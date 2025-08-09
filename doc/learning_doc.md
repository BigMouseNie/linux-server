# Linux Server Learning Documents

## 进程时序图

```mermaid
sequenceDiagram
    participant  main as 主进程
    participant log as 日志进程
    participant client as 客户端处理进程
    main->>main: 创建守护进程
    main->>log: 初始化进程
    log->>log: 创建日志服务器
    main->>client: 初始化进程
    client->>client: 创建客户端处理线程池

```



## fd的进程间发送

```c++
// 核心
cmsg->cmsg_level = SOL_SOCKET;
cmsg->cmsg_type = SCM_RIGHTS;
*((int*) CMSG_DATA(cmsg)) = fd;
// 告诉内核要通过 socket 发送一个控制信息，这个控制信息的类型是 SCM_RIGHTS，希望共享当前进程里 fd 表里的这个文件描述符
```



```c++
/*
* 通过管道以及 msghdr 和 cmsghdr 来发送
*/

// read fd
int ReadFromPipe(int& fd) {
    struct msghdr msg;
    struct iovec iov[2];
    char temp[2][10];
    iov[0].iov_len = sizeof(temp[0]);
    iov[0].iov_base = temp[0];
    iov[1].iov_len = sizeof(temp[1]);
    iov[1].iov_base = temp[1];
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;

    char cmsg_buf[CMSG_SPACE(sizeof(int))];
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    if (-1 == recvmsg(pipe_[0], &msg, 0)) {
        printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__, strerror(errno));
        return -1;
    }
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg &&
        cmsg->cmsg_level == SOL_SOCKET &&
        cmsg->cmsg_type == SCM_RIGHTS)
    {
        memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
        return 0;
    }
    printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__, strerror(errno));
    return -1;
}

// send fd
int WriteToPipe(int fd) {
    struct msghdr msg = {};	// note: 必须清零(因为msg_name相关成员没有用到,即没有初始化),sendmsg会Bad address error
    struct iovec iov[2];
    char temp[2][10] = {"wang", "shuo"};
    iov[0].iov_base = temp[0];
    iov[0].iov_len = strlen(temp[0]) + 1;
    iov[1].iov_base = temp[1];
    iov[1].iov_len = strlen(temp[1]) + 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;

    char cmsg_buf[CMSG_SPACE(sizeof(int))];
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;

    void* cdata = CMSG_DATA(cmsg); 
    *static_cast<int*>(cdata) = fd;

    if (-1 == sendmsg(pipe_[1], &msg, 0)) {
        printf("<%s> : <%d> : err(%s)\n", __FUNCTION__, __LINE__, strerror(errno));
        return -1;
    }
    return 0;
}

```



## 守护进程创建

```mermaid
flowchart TD
	start(start)
	fork1("fork()")
	_end1(end)
	setsid("setsid()")
	fork2("fork()")
	_end2(end)
	close(关闭stdio)
	chdir(chdir修改当前目录)
	umask(umask清除文件遮蔽位)
	signal("处理SIGCHID信号(可选)")
	main(主程序运行)
	_end3(end)
	
    start --> fork1
	subgraph 保证不是进程组组长
        fork1 -- 主进程 --> _end1
	end
	
	subgraph 成为会话首进程和组长
    	fork1 -- 子进程 --> setsid
    	setsid --> fork2
    	fork2 -- 主进程 --> _end2
    end
    
    subgraph 不是会话首进程和组长
    	fork2 -- 子进程 --> close --> chdir --> umask --> signal --> main --> _end3
    end

```



```c++
int Daemonize() {
    pid_t pid = fork();
    if (pid == -1) {return -1;}
    if (pid > 0) { exit(0); }

    if (-1 == setsid()) {return -2;}
    pid = fork();
    if (pid == -1) {return -3;}
    if (pid > 0) { exit(0); }
	
    // 忽略 SIGCHLD，防止子进程变僵尸
    signal(SIGCHLD, SIG_IGN);
    
    // 修改工作目录
    chdir("/"); 

    // 重设文件权限掩码
    umask(0);

    // 关闭标准输入输出
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // 重定向到 /dev/null
    open("/dev/null", O_RDONLY);  // stdin
    open("/dev/null", O_WRONLY);  // stdout
    open("/dev/null", O_RDWR);    // stderr

    return 0;
}
```



**`chdir("/")` — 切换工作目录为根目录**

```c++
chdir("/");
```

- 防止守护进程**阻止文件系统被卸载**。
- 守护进程可能会在后台长时间运行，如果当前目录仍是某个挂载的文件系统（例如 `/mnt/data`），则该挂载点在卸载时会失败。
- `chdir("/")` 将当前目录切换到根目录 `/`，几乎不可能被卸载。




 **`umask(0)` — 清除默认文件权限掩码**

```c++
umask(0);
```

- **umask** 是一个默认的权限掩码，它影响用 `open`、`creat`、`mkdir` 等系统调用新建文件的权限。
- 默认情况下它可能是 `022`，意味着默认去掉了“组”和“其他人”的写权限。
- 守护进程中我们往往希望**完全控制文件权限**，所以将其设置为 `0`，表示不限制任何权限，交由程序显式决定。



**关闭标准输入、输出、错误输出文件描述符**

```c++
close(STDIN_FILENO);   // 关闭标准输入（0）
close(STDOUT_FILENO);  // 关闭标准输出（1）
close(STDERR_FILENO);  // 关闭标准错误（2）
```

- 守护进程通常是**后台运行的，不依赖终端**。
- 如果不关闭它们：
	- 守护进程可能仍然绑定到控制终端；
	- 误操作可能导致输出打到终端上；
	- 如果终端关闭，进程可能接收到 `SIGHUP`（挂起信号）。
- 所以，**必须关闭标准 IO 描述符**（文件描述符 0、1、2）。



**重定向标准输入、输出和错误输出到 `/dev/null`**

```c++
open("/dev/null", O_RDONLY);  // stdin (fd 0)
open("/dev/null", O_WRONLY);  // stdout (fd 1)
open("/dev/null", O_RDWR);    // stderr (fd 2)
```

- 上面三行代码的作用是**重新打开文件描述符 0、1、2**，指向 `/dev/null`，这样程序中若有代码误用 `printf` 或 `read(stdin)`，也不会导致错误或打印到不可预期的位置。
- `/dev/null` 是一个**特殊的设备文件**：
	- 读它总是返回 EOF；
	- 写它会直接被丢弃。



## Container模块

这个模块的类都是比较独立的，其中RingBuffer类的使用见名思意，其中内嵌类CompactRate是用来防止RingBuffer频繁的Compact(紧凑)操作的，超过某个频率后会执行Expand

### RingBuffer

```mermaid
classDiagram

class CompactRate {
    +CompactRate() : compact_cnt(1), total_cnt(kScale)
    +~CompactRate() = default
    +CompactRate(const CompactRate&) = delete
    +CompactRate& operator=(const CompactRate&) = delete
    +bool UpdateStatsAndCheck(bool is_compact)
    +void Reset()
    -int compact_cnt
    -int total_cnt
    -static const int kScale
    -static const int kMinAllowedRate
}

class RingBuffer {
    +RingBuffer()
    +virtual ~RingBuffer()
    +int Read(char* dest, size_t dest_size)
    +int Write(const char* src, size_t src_size)
    +int Read(RingBuffer* dest)
    +int Write(RingBuffer* src)
    +void Clear()
    +int Resize(size_t size)
    +size_t Readable()
    +size_t Writable()
    +size_t UnusedSize()
    +size_t Size()
    +void Expand(size_t size)
    +void EnsureWritableSize(size_t size)
    +const char* GetReadPtr() const
    +char* GetWritePtr()
    +void ReadOut(size_t len)
    +void Written(size_t len)
    -void Compact()
    -void Expand(size_t size, bool fixed)
    -void Shrink(size_t size)
    -CompactRate compact_rate_
    -char* data_
    -char* read_ptr_
    -char* write_ptr_
    -size_t readable_
    -size_t writeable_
    -size_t size_
    -static const size_t kExpandFactor
}

RingBuffer *-- CompactRate : 内嵌

```



### BlockingQueue

该类是一个双端的线程安全的阻塞队列，适用于多生产者多消费者的情景，目前用于线程池，提供了一些方法Pop和Push，下面是锁序，保证不会死锁，更详细需要结合代码，有些判断也是必不可少





```mermaid
graph LR
    Path1 --> PushLock --> Push --> PushUnlock --> NotifyOne

    Path2 --> PopLock --> IsEmpty --N--> Pop --> PopUnlock --> PopEnd
    Path3--> PopLock
    NotifyOne -.wake.-> Wait
    NotifyOne ----> PushEnd
    IsEmpty --Y--> PushLock --unique move--> P3_PushUnlock["PushUnlock"] --enter--> Wait --> P3_PushLock["PushLock"] --> Swap --> Pop

```





## Pool模块

### ThreadPool

```mermaid
classDiagram

class BlockingQueue {
	+Push()
	+Pop()
	+Realse()
}

class ThreadPool {
    +ThreadPool() = default
    +~ThreadPool()

    +ThreadPool(const ThreadPool&) = delete
    +ThreadPool& operator=(const ThreadPool&) = delete
    +int Start(int thrd_num)
    +void AddTask(Func&& func, Args&&... args)
 
    -void Work()
    -BlockingQueue<function> que_
    -std::vector<std::thread> workers_
}

ThreadPool *-- BlockingQueue
```





## Run模块

```mermaid
classDiagram

class RunBase {
    +RunBase() : state_(RunState::kInvalid)
    +virtual ~RunBase() = default;
    +int Init(Func&& func, Args&&... args)
    +virtual int Run(void* arg = nullptr)
    +virtual int Stop(void* arg = nullptr)
    +virtual int PauseOrUnpaused(void* arg = nullptr)
    #RunBase(const RunBase&) = delete
    #RunBase& operator=(const RunBase&) = delete
    #std::function entry_
    #RunState state_
}

class ThreadWrapper {
    +ThreadWrapper()
    +~ThreadWrapper()
    +SetThreadAttr(int flag)
    +virtual int Run(void* arg = nullptr)
    +virtual int Stop(void* arg = nullptr)
    -void ClearAttr()
    -static void* Entry(void* arg)
    -static void Sigaction(int signo, siginfo_t* info, void* context)
    -pthread_t pthread_
    -pthread_attr_t attr_
}

class ProcessWrapper{
    +ProcessWrapper() = default
    +~ProcessWrapper() = default
    +virtual int Run(void* arg = nullptr)
    +int ReadFdFromPipe(int& fd)
    +int WriteFdToPipe(int fd)
    +static int Daemonize()
    -pid_t pid_
    -int pipe_[2]
}

ThreadWrapper --|> RunBase
ProcessWrapper --|> RunBase

```



基类RunBase实现了多参数的bind,可以调用Init进行绑定，继承RunBase的类都有一个状态RunState,表示当前的状态的切换总体为下面所示，当然这是理想情况，比如RunBase实例化的Run就是当前线程一个函数的执行，就无法Pause,Stop了，当前只有ThreadWrapper实现了Stop根据Sigaction回调，Pause可以根据Sigaction回调来实现

```mermaid
stateDiagram-v2
    [*] --> Invalid
    Invalid --> Initialized : Init()
    Initialized --> Running : Run()
    Running --> Paused: PauseOrUnpaused()
    Paused --> Running : PauseOrUnpaused()
    Paused --> Stopped : stop()
    Running --> Stopped : stop()
    Stopped --> [*]
```









## Network模块

```mermaid
classDiagram

class RingBuffer {
	+RingBuffer();
	-~RingBuffer();
}

class SocketBuffer {
    +SocketBuffer() = default
    +SocketBuffer(size_t size)
    +~SocketBuffer() = default
    +int ReadFromSock(int sock, bool is_et, int* saved_errno)
    +int WriteToSock(int sock, bool is_et, int* saved_errno)
    -static const size_t kMinBufSize
}

class SocketCfg {
    +SocketCfg()
    +~SocketCfg()
    +SocketCfg(const SocketCfg& other)
    +SocketCfg& operator=(const SocketCfg& other)
    +SocketCfg(SocketCfg&& other)
    +SocketCfg& operator=(SocketCfg&& other)
    +void SetPort(uint16_t port)
    +void SetSockAttr(int sock_attr)
    +void SetBacklog(int backlog)
    +int SetAddrOrPath(const char* addr_or_path)
    +int GetSockAttr() const
    +const char* GetAddress() const
    -uint16_t port_
    -int sock_attr_
    -int backlog_
    -size_t address_size_
    -char* address_
}

class SocketCreator {
    +static SocketCreator& Instance()
    +int Create(const SocketCfg& sock_cfg, int* attr = nullptr)
    -SocketCreator() = default
    -~SocketCreator() = default
    -void Close(int fd)
    -int CreateSocket(const SocketCfg& sock_cfg)
    -int CreateAddress(int fd, struct sockaddr_storage*, const SocketCfg&)
    -int Bind(int fd, struct sockaddr_storage* addr, const SocketCfg& sock_cfg)
    -int Listen(int fd, const SocketCfg& sock_cfg)
    -int Connect(int fd, struct sockaddr_storage* addr, const SocketCfg& sock_cfg)
    -int SetNonBlock(int fd)
}

class SocketWrapper {
    +SocketWrapper(bool manual_mgnt = true)
    +~SocketWrapper()
    +SocketWrapper(SocketWrapper&& other)
    +SocketWrapper& operator=(SocketWrapper&& other)
    +int Create(const SocketCfg& sock_cfg)
    +void Close()
    +int GetSocket()
    +void SetManualMgnt(bool manual)
    +bool IsValid()
    +int Recv(SocketBuffer& buf)
    +int Send(SocketBuffer& buf)
    -int attr_
    -int sockfd_
    -bool manual_mgnt_
}


class AcceptCallBack {
    <<type alias>>
    +void operator()(int conn_fd, struct sockaddr* addr, int addr_len)
}

class Acceptor {
    +Acceptor()
    +~Acceptor() = default
    +virtual int Create(AcceptCallBack cb, bool is_et)
    +int DealConnFromSock(int sock)
    #AcceptCallBack accept_cb_;
    #bool is_et_;
}

class ReactorAcc {
    +ReactorAcc() = default
    +~ReactorAcc() = default
    +virtual int Create(AcceptCallBack cb, int listenfd_num, bool is_et)
    +int SetListenSock(SocketWrapper&& sock)
    +int Accept(int timeout_ms)
    -void Remove(int listnefd)
    -Epoller epoller_
    -std::unordered_map<int, SocketWrapper> listenfd_map_
}

class EventsCallBack {
    <<type alias>>
    +void operator()(epoll_event* evs, size_t size)
}

class Epoller {
  +Epoller()
  +~Epoller()
  +int Create(EventsCallBack cb, size_t event_arr_size, bool is_et)
  +int Add(int fd, uint32_t events)
  +int Modify(int fd, uint32_t events)
  +int Del(int fd)
  +int Wait(int timeout_ms)

  -int SetNonBlock(int fd)
  -int epollfd_
  -bool is_et_
  -size_t event_arr_size_
  -struct epoll_event* event_arr_
  -EventsCallBack ev_cb_
}

Epoller *-- EventsCallBack
Acceptor *-- AcceptCallBack
SocketBuffer --|> RingBuffer
ReactorAcc --|> Acceptor
SocketCreator ..> SocketCfg : friend
SocketWrapper ..> SocketCreator
ReactorAcc *-- Epoller
SocketWrapper ..> SocketBuffer : use

```

Eplloer的创建需要给予一个回调函数，在Wait完毕后进行处理。Acceptor的创建也需要一个回调函数，ReactorAcc继承自Acceptor,它需要Epoller的参与才可以创建，它适合Reactor模式的监听，SocketBuffer继承自RingBuffer,增加了对Socket读写操作，SocketCreator是一个单例，提供一个Create函数，配合SocketCfg可以创建多种Socket(支持本地、ipv4、ipv6、tcp、udp、listen、connect),SocketCfg是SocketCreator友元类，SocketWrapper是对套接字本身的封装



## 日志服务器

### 创建运行时序图

```mermaid
sequenceDiagram

main->>+LogProc: Run()
LogProc->>+SubLogProc: fork()
LogProc->>-main: ret
SubLogProc->>+LogServ: Instance().Create()
alt Cfged!=true
	LogServ->>LogServ: Config()
end
LogServ->>Acceptor: Create(ACallBack)
LogServ->>Epoller: Create(ECallBack)
LogServ->>Worker: Run()
LogServ->>-SubLogProc: ret
SubLogProc->>SubLogProc: wait
loop running_ = true
    Worker->>+Epoller: Wait()
    Epoller->>Epoller: epoll_wait
    alt fd==servsock
    	Epoller->>+Acceptor: DealConnFromSock()
    	Acceptor->>Acceptor: ACallBack
    	Acceptor->>-Epoller: ret
    else fd!=servsock
    	Epoller->>Epoller: ECallBack
    end
    Epoller->>-Worker: ret
end

```



### 日志发送时序图

```mermaid
sequenceDiagram

other->>+Logger: Instance().Log()
alt Cfged!=true
Logger->>Logger: Config()
end
alt clnt_sock.IsValid()
	Logger->>+ClntSock: Create()
	ClntSock->>+SocketCreator: Create()
	SocketCreator->>+LogServ: connect()
	LogServ->>-SocketCreator: ret
	SocketCreator->>-ClntSock: ret
	ClntSock->>-Logger: ret
end
Logger->>Logger: FromtLogStr
Logger->>+ClntSock: Send()
ClntSock->>LogServ: send()
ClntSock->>-Logger: ret
LogServ->>File: fwrite()
Logger->>-other: ret

```



