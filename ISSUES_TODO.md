# Issues & TODO

> 记录潜在问题、待讨论事项和待办事项

---

## 待讨论 / 潜在问题

### SocketBuffer: 边缘触发模式循环可能占用较长时间

**描述：**
在 `SocketBuffer::ReadFromSock()` 中，当 `is_et=true`（边缘触发模式）时，代码会循环读取直到返回 `EAGAIN`。如果客户端持续发送大量数据，循环可能执行很多次，占用 CPU 时间较长。

**代码位置：**
`network/src/socket_buffer.cpp:10-35`

```cpp
do {
  // ...
  int n = read(sock, GetWritePtr(), Writable());
  if (n > 0) {
    res += n;
    Written(n);
  }
  // ...
} while (err_interr || is_et);  // ← 边缘触发时会持续循环
```

**场景分析：**

| 数据量 | 循环次数 | 耗时估算 |
|-------|---------|---------|
| 1 KB | ~4次 | 微秒级 |
| 1 MB | ~4000次 | 毫秒级 |
| 100 MB | ~400,000次 | 可能秒级 |

**当前缓冲区大小：** `kMinBufSize = 256` 字节

**潜在影响：**
- 大数据量时占用 CPU 时间较长
- 线程可能被占用较久，影响其他连接处理

**状态：** 🟡 待讨论
- 目前不确定是否为实际问题
- 需要根据实际使用场景判断

**可能的解决方案：**
1. 保持现状（如果数据量通常不大）
2. 限制单次循环最大次数（如 1000 次）
3. 增大缓冲区大小（如 8KB）
4. 组合方案：增大缓冲区 + 限制循环次数

---

## 已确认但暂不处理

### RingBuffer 线程安全

**描述：**
`RingBuffer` 本身不是线程安全的，但在 `EchoBiz` 中，同一个 `EchoBizClnt` 的 `recv_buf_` 和 `send_buf_` 可能被多个线程同时访问。

**状态：** ⏸️ 暂不处理
- 开发者表示目前没有打算让 RingBuffer 应用到多线程
- 如有需要后续再讨论

**相关：**
- `container/include/ring_buffer.h`
- `business/src/echo_biz.cpp`

---

## 遗留技术债（不紧急）

- [ ] 检查 Epoller 内存管理（析构函数已有 delete[]，需验证）
- [ ] 确认所有错误路径是否有资源泄漏
- [ ] 统一命名约定（`is_et_` vs `m_` prefix vs snake_case）
- [ ] 移除硬编码路径
- [ ] 实现服务器入口点（main.cpp）
- [ ] 添加信号处理和优雅关闭

---

## 更新记录

- 2026-03-09: 添加 SocketBuffer 边缘触发循环问题
