# 计算机网络课Project

## 代码结构
* `bt_parse.c ` 封装实现bt_config信息的解析相关函数
* `bt_parse.h` 定义了对等方相应的数据结构
* `chunk.c` 封装实现了chunk.h中的函数
* `chunk.h` 定义了数据块和sha1值转换的函数
* `debug.c` 封装实现了debug相应的函数
* `debug.h` 定义了debug相应的函数
* `debug-text.h` 定义一些数据
* `input_buffer.c` 封装实现了buffer处理相关函数
* `input_buffer.h` 定义了输入buffer相关的数据结构和相关函数
* `packet.c` 封装packet对应函数的C文件, 封装了生成packet\处理packet等函数
* `packet.h` 定义相应packet的数据结构, 以及定义packet相应函数 
* `peer.c` main函数入口, 封装了处理相应数据包的函数并调用 
* `queue.c` 封装了队列相关的函数提供调用
* `queue.h` 定义了队列作为基本数据结构
* `sha.c` sha1实现函数具体C文件
* `sha.h` sha1对应头文件
* `spiffy.c` 封装实现网络传输相应函数
* `spiffy.h` 网络传输相关头文件
* `task.c` 封装实现了处理GET请求相关的函数
* `task.h` 定义了程序处理GET请求的数据结构
* `trans.c` 封装实现数据发送的相应函数
* `trans.h` 定义了数据发送的相应函数

## 数据结构
### queue
```C
typedef struct node {
    void *data;
    struct node *next;
} node;

typedef struct queue {
    node *head;
    node *tail;
    int n;
} queue;
```
用链表实现的队列, 可以储存变长的同一类型数据, 以先进先出的存放和读取数据, 添加新数据到tail节点, 从head节点取出数据, 封装了队列作为基本数据结构, 用于处理程序中不定长度的数据

### conn
```C
typedef struct down_conn_s {
    uint next_ack;
    chunk_t *chunk;
    int pos;
    struct timeval timer;
    bt_peer_t *sender;
} down_conn_t;

typedef struct up_conn_s {
    int last_ack;
    int last_sent;
    int available;
    int duplicate;
    int rwnd;
    data_packet_t **pkts;
    struct timeval timer;
    bt_peer_t *receiver;
} up_conn_t;
```
conn 表示一个正在上传或者下载的连接, 按种类分可具体分为**down_conn**和**up_conn**, 其中up_conn是仿照tcp结构的发送方, down_conn是仿tcp结构的接收方

coon数据结构主要由四部分构成:
1. 为了实现稳定传输, 需要定义相应的变量表示当前窗口状态, 比如up_conn的last_ack, last_sent, available, duplicate, rwnd以及down_conn的next_ack
2. 需要发送或保存的数据, up_conn中是相关data数据包的指针数组, 而down_conn中是需要指向需要被填充chunk的指针
3. 对等方, 即接收或发送数据的一方, 如down_conn中的sender以及up_conn中的receiver
4. 相应的定时器timer, 用于检测当前连接的状态

### pool
```C
typedef struct down_pool_s {
    down_conn_t **conns;
    int conn;
    int max_conn;
} down_pool_t;

typedef struct up_pool_s {
    up_conn_t **conns;
    int conn;
    int max_conn;
} up_pool_t;

```
pool是用于存储相应conn连接的连接池, pool有两种: **down_pool**和**up_pool**, 分别对应**down_conn**和**up_conn**两种conn。 这两种pool的结构是类似的, 都包括:
1. conns 存有conn连接信息的指针数组
2. conn 当前已经建立的连接数
3. max_conn 允许建立的最大连接数

### chunk&&task
```C
typedef struct chunk_s {
    uint8_t sha1[SHA1_HASH_SIZE];
    int flag;
    int inuse;
    int num;
    char *data;
    queue *peers;
} chunk_t;

typedef struct task_s {
    int chunk_num;
    int need_num;
    int max_conn;
    queue *chunks;
    char output_file[BT_FILENAME_LEN];
    struct timeval timer;
} task_t;
```
task和chunk是对用户发起的GET请求的抽象
当用户发起一个GET请求如 `GET A.chunks silly.tar`
那么程序提取出相应**silly.tar**作为目的文件, 即task中的output_file, 程序会最终将数据写入此文件中。然后程序会解析**A.chunks**, 统计需要下载的chunk及其sha1值, 并将封装成数据结构chunk并加入chunks队列中。

chunk对应一个512KB的数据块, 在该数据结构中, 主要有以下值
1. sha1: 相应数据块的sha1值
2. flag: 表示该块是否已经下载完成
3. inuse: 表示该块是否正在被下载
4. data: 该块所对应的512KB数据
5. peers: 可以提供该块下载的对等方队列

## 运行流程
### 数据包解析
程序会首先将收到的数据包进行网络到主机端的字节顺序转换, 转换完成后, 程序会检验数据包的header部分, 如果相应的magic\version\packet type出现错误, 则直接忽略该数据包; 如果解析正常, 则将数据包以及发出该数据包的相应对等方作为参数传入相应handle函数进行处理。

### 处理who_has数据包
当一个peer程序收到一个who_has数据包时, 其会读取数据包中所请求数据块的sha1值, 并与程序已有数据块的sha1值进行比较, 如果存在相应数据块, 则将所有数据块的sha1值封装到一个或多个i_have数据包中发送给请求的对等方。

### 处理i_have数据包
当一个peer程序收到一个i_have数据包时, 其会读取数据包中的sha1值, 并根据对应chunk的状态和对等方的状态来决定是否发送get数据包。如果当前chunk未被下载, 与相应对等方没有建立相应下载连接, 且down_pool未满, 则向该对等方发送get请求并建立相应的down_conn, 然后将down_conn加入down_pool中, 然后启动down_conn中的定时器timer。 

### 处理get数据包
当一个peer程序收到一个get数据包时, 程序首先检查当前建立的up_pool是否已满, 或者当前请求的对等方是否已在up_pool中建立连接, 如果满足以上条件, 则发送denied数据包予以拒绝。如果不满足, 则其首先解析数据包中请求chunk的sha1值, 然后根据sha1值读取数据并生成相应的数据包数组。然后, 程序将数据包以及请求数据的对等方封装入数据结构up_conn中, 并发出当前所有能发的数据包。

### 处理data数据包
当一个peer程序收到一个data数据包时, 程序首先定位该数据包属于哪一个down_conn, 如果不存在与发出数据包的对等方对应的down_conn, 则直接忽略该数据包。然后, 程序会对相应数据包的seq进行校验, 并选择是否发送ack; 如果相应seq值是期待的seq值, 则保存数据包中的相应数据, 如果seq值不是期待的seq值, 则直接向相应对等方发送冗余ack。如果收到预期的所有数据包, 则程序需要完成当前down_conn并将其移出down_pool中。如果当前下载任务已完成, 则需要完成当前下载任务并保存数据。

### 处理ack数据包
当一个peer程序收到一个ack数据包时, 程序首先确定数据包属于哪一个up_conn, 如果不存在与发出ack的对等方对应的up_conn, 则直接忽略该请求; 如果有, 则检查收到的ack值, 如果ack值是预期的ack值, 则扩展发送方窗口, 继续发送可用数据包; 如果收到冗余ack, 在冗余ack到达一定数目后, 需要修改窗口并重新发送正确数据包。如果确认发送完毕, 则从up_pool中移出当前的up_conn。

### 处理用户输入
程序读取用户输入的GET请求, 并将请求解析成task和who_has数据包队列, 然后程序主动向网络中的所有对等方发送who_has数据包。

### 处理超时conn
程序会计算conn中定时器的时间, 当时间已到达一定数值后, 即认为conn已经超时, 此时程序会主动重置conn状态, 并从pool中移出相应conn。如果超时的是下载连接, 则程序会继续寻找一个新的可用下载对等方并请求创建连接。

### 检查任务
当程序完成所有需要chunk的下载时, 程序进入finish过程, 但在程序finish之前, 程序会检查所有下载完成的chunk的sha1值是否正确, 如果所有chunk均正确, 则完成下载任务, 程序将所有data写入目标文件; 如果有chunk下载错误, 则重置该chunk状态, 并重新尝试建立下载连接。

## 过程中遇到的问题
1. 最稀缺优先策略 理论上BT程序需要实现最稀缺优先策略, 程序需要在所有可以下载块中查找最优的块并请求。但在实际实现中, 可以直接忽略最优块选择的过程, 因为本次实现的peer程序较为简单, 对等方较少, 且所需的块往往在最大连接限制以内, 所以采用最稀缺优先策略并不是一个最适合本实验的策略。所以, 本实验采用策略是, 接收到ihave数据包后, 立刻从对等方请求下载可用数据块。
2. 读写问题。本实验需要将请求的文件写入磁盘中, 但采用内存映射时, 测试中会遇到打开文件fd失败的情况, 但在正常情况下正确, 推测为ruby修改了程序的输入流所带来的问题。所以采用另一种输出方式予以输出文件。