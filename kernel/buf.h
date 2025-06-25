struct buf {
  int valid;   // 缓冲数据有效性标志：1=数据已从磁盘读取且有效，0=需读取磁盘
  int disk;    // 磁盘所有权标志：1=磁盘I/O进行中(缓冲正被读写)，0=缓冲可复用
  uint dev;    // 设备号，标识该缓冲区对应的存储设备
  uint blockno; // 块号，标识设备上的具体物理块位置
  struct sleeplock lock; // 睡眠锁：保障缓冲数据访问的互斥性（允许进程睡眠等待）
  uint refcnt;  // 引用计数：统计当前使用该缓冲区的进程/操作数量
  // struct buf *prev; // LRU链表：指向链表中前一个缓冲区（最近最少使用算法）
  struct buf *next; // LRU链表：指向链表中下一个缓冲区
  int las;             // 最后一次使用的时间戳
  uchar data[BSIZE]; // 实际数据存储区（大小=磁盘块尺寸BSIZE）
};