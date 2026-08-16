我会先快速读取项目里与主流程相关的源文件（如 `main`、`pid`、`uart`、`servo`），梳理完整的数据流路径，然后按“数据来源→处理→去向”给你做逐段解释。先去定位并打开这些关键文件。这一轮我会先用快速检索找出核心函数和调用关系，目标是产出一份可追踪的数据流清单，然后再逐文件精读。

Search complete: 定位核心数据流

Searched for files matching `**/*.{c,h}`, 143 matches

Searched for files matching `**/main.c`, no matches

Searched for files matching `**/pid*.{c,h}`, 1 match

Searched for files matching `**/uart*.{c,h}`, no matches

Searched for files matching `**/servo*.{c,h}`, no matches

Searched for files matching `mdk` · `*`, 200 matches

Searched for regex `main.c|pid_ctrl.c|uart|servo|GroupName|FileName|FilePath` (`**/stm32f103c8.uvprojx`), 136 results

我已经确认真实源码在 mdk 目录的上一级（如 app、driver），下一步直接读取这些源码文件内容来还原完整数据流，而不是只看工程配置。现在去列目录并打开核心 C 文件。

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid)

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid/app)

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid/driver)

我已找到主程序与驱动源码，接下来会逐个精读 app/main.c、driver/uart.c、driver/pid.c、driver/servo.c，并提炼“变量级”数据流。

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid/app/main.c)

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid/driver/uart.c)

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid/driver/pid.c)

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid/driver/servo.c)

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid/driver/pid.h)

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid/driver/uart.h)

Read [](file:///c%3A/Users/Lenovo/Desktop/stm_all/project/cloud_standard_mid/driver/servo.h)

核心代码已经读完，我现在补充抓取关键语句的精确行号，确保你看到的每一步数据流都能直接点到对应代码位置。随后我会给出“从哪来、怎么处理、到哪去”的完整链路图。

Searched for regex `int main\(|g_offset_update_flag|PID_PD_Calc|SERVO_SetPulseUs|UART_Init|apply_deadband_i16|clamp_step_i32` (`**/app/main.c`), no results

Searched for regex `g_offset_x|g_offset_y|UART_ParseByte|USART1_IRQHandler|UART_PKT_HEADER|UART_PKT_TAIL|UART_OFFSET_MIN` (`**/driver/uart.c`), no results

Searched for regex `PID_PD_Init|PID_PD_Calc|PID_Limit|pre_err|err = setpoint` (`**/driver/pid.c`), no results

Searched for regex `SERVO_Init|SERVO_SetPulseUs|SERVO_ClampPulseUs|TIM_TimeBaseInit|TIM_SetCompare1|TIM_SetCompare2` (`**/driver/servo.c`), no results

Searched for regex `UART_PKT_HEADER|UART_PKT_TAIL|UART_OFFSET_MIN|UART_OFFSET_MAX|extern volatile` (`**/driver/uart.h`), no results

Searched for regex `SERVO_CHANNEL_X|SERVO_CHANNEL_Y|SERVO_PULSE_MIN_US|SERVO_PULSE_MAX_US|SERVO_PULSE_MID_US` (`**/driver/servo.h`), no results

检索工具受工作区索引范围限制，我改用终端直接给源码加行号，这样你能看到每个数据流节点的精确位置。下一步会抓取四个核心文件的带行号内容。

Ran terminal command: $files = @('..\app\main.c','..\driver\uart.c','..\driver\pid.c','..\driver\servo.c','..\driver\uart.h','..\driver\servo.h'); foreach ($f in $files) { Write-Output "===== $f ====="; $i=0; Get-Content $f | ForEach-Object { $i++; "{0,4}: {1}" -f $i, $_ } }

我已经拿到带行号的完整输出文件，马上读取并提取关键行位，之后直接给你一份可点击定位的数据流解读。

Read [](file:///c%3A/Users/Lenovo/AppData/Roaming/Code/User/workspaceStorage/855180b5747aff671fc8fe4d806c522b/GitHub.copilot-chat/chat-session-resources/17bf86ef-3a61-4af3-b276-03c5e83d1f2a/call_r9KGdnVep247JsWmi26GNXvp__vscode-1777905682534/content.txt#1-1), lines 1 to 320

Read [](file:///c%3A/Users/Lenovo/AppData/Roaming/Code/User/workspaceStorage/855180b5747aff671fc8fe4d806c522b/GitHub.copilot-chat/chat-session-resources/17bf86ef-3a61-4af3-b276-03c5e83d1f2a/call_r9KGdnVep247JsWmi26GNXvp__vscode-1777905682534/content.txt#321-321), lines 321 to 452

**整体结论**
这套代码是一个典型闭环链路：上位机发送目标偏差帧 → STM32 串口中断解析成 x/y 偏差 → 主循环做抗抖与 PD 计算 → 结果转换成舵机脉宽增量 → 写入 TIM3 的 PWM 比较寄存器驱动两路舵机。

主入口在 main.c。

**1) 数据从哪来**
- 外部来源是 USART1 接收的字节流，初始化在 uart.c。
- 串口协议格式定义在 uart.h：
  - 帧头 0x2C
  - 数据 4 字节（x 高、x 低、y 高、y 低）
  - 帧尾 0x5B
  - 合法范围是 [-1280, 1280]，见 uart.h。
- 每收到 1 字节会进入中断 uart.c，读取 RX 数据后交给状态机 uart.c。
- 解析成功后把数据写到全局变量：
  - g_offset_x: uart.c
  - g_offset_y: uart.c
  - g_offset_update_flag: uart.c

**2) 怎么处理**
处理在主循环 if 分支内，入口判断在 main.c。

- 原子读取与清标志
  - 关中断读取 g_offset_x/g_offset_y 并清 g_offset_update_flag，防止主循环与中断竞争，见 main.c。
- 抗抖死区
  - 偏差绝对值小于 25 直接置 0，函数在 main.c，调用在 main.c。
- 坐标符号修正
  - 通过 OFFSET_SIGN_X/Y 做正负方向修正，见 main.c 与 main.c。
- PD 计算
  - 调用在 main.c。
  - 控制器初始化参数在 main.c，当前是 kp=0.85, kd=0.26，输出限幅 ±15。
  - 核心公式在 pid.c 与 pid.c：
    - 误差：$e = setpoint - feedback$
    - 微分：$\Delta e = e - pre\_err$
    - 输出：$u = kp \cdot e + kd \cdot \Delta e$
  - PD 输出再经一次限幅 PID_Limit，见 pid.c 与 pid.c。
- 输出转脉宽增量
  - 把 PD 输出转成 step_x/step_y，见 main.c。
  - 每次控制步进限制在 ±18us，函数在 main.c，调用在 main.c。
- 积分式累加到当前脉宽
  - pwm_x += step_x, pwm_y += step_y，见 main.c。
- 最终脉宽安全限幅
  - 限制到 [500, 2500]us，调用在 main.c，实现在 servo.c。
  - 这个范围定义在 servo.h。

**3) 到哪去**
- 最终写入舵机 PWM 通道函数 servo.c。
- 通道映射：
  - X 轴通道 1，Y 轴通道 2，定义在 servo.h。
  - 主程序映射宏在 main.c。
- 硬件落点是 TIM3：
  - X 写 TIM3 CH1 比较寄存器，见 servo.c。
  - Y 写 TIM3 CH2 比较寄存器，见 servo.c。
- TIM3 PWM 基础配置在 servo.c：
  - 72MHz / 72 = 1MHz 计数频率（1 计数=1us）
  - 周期 20000us（20ms，50Hz）
  - 所以比较值就是舵机脉宽微秒值。

**4) 一条完整数据样例**
- 上位机发一帧：帧头 + x/y 偏差 + 帧尾。
- 中断解析后得到例如 x=120, y=-80，写入全局并置更新标志。
- 主循环读到后：
  - 先过死区（25）和符号修正；
  - 以目标 0 做 PD，得到 out_x/out_y；
  - 转成 step，限制单次最多 18us；
  - 累加到当前 pwm_x/pwm_y；
  - 限幅到 500~2500us；
  - 写入 TIM3 CH1/CH2，舵机动作。

**5) 你最关心的“数据流地图”（一句话版）**
USART1 RX 字节流 → UART 状态机组帧并验范围 → g_offset_x/y + 更新标志 → 主循环原子取数 → 死区/方向修正 → PD 输出 → 步进限幅与脉宽限幅 → TIM3 CCR1/CCR2 → 舵机机械运动。

如果你愿意，我下一步可以给你再画一版“变量级时序图”（按每个变量何时读写）或“调参入口清单”（改哪几个宏会影响稳态抖动、响应速度、超调）。





我会聚焦串口状态机这几行代码，按“每个状态做了什么、数据什么时候落到全局变量、主循环如何消费并继续分析”给你拆开讲，并配一个真实字节流示例。状态机核心在 uart.c，它做了两件事：

1. 把“连续字节流”切成“完整一帧”  
2. 把帧里的 4 字节解析成 x/y 偏差，并写入共享变量给主循环用

下面按你问的“怎么写入数据、怎么分析数据”来讲。

**状态机怎么写入数据**
状态定义在 uart.c，有 3 个状态：

1. UART_RX_WAIT_HEADER  
2. UART_RX_GET_DATA  
3. UART_RX_WAIT_TAIL  

接收中断入口在 uart.c，每来 1 字节就调用解析函数 uart.c。

具体流程：

1. 等帧头阶段  
在 uart.c 只接受帧头字节。帧头定义是 0x2C，见 uart.h。  
收到帧头后，清 s_data_idx=0，并切到 GET_DATA，见 uart.c。

2. 收数据阶段  
在 uart.c 执行：
- s_data_buf[s_data_idx++] = data  
这句就是“写入数据缓冲区”的关键。  
收满 4 字节后切到 WAIT_TAIL，见 uart.c。

3. 等帧尾并提交阶段  
在 uart.c 判断是否是帧尾。帧尾定义 0x5B，见 uart.h。  
如果帧尾正确，就把 4 字节组装成两个 int16：
- x = (buf[0] << 8) | buf[1]，见 uart.c  
- y = (buf[2] << 8) | buf[3]，见 uart.c

然后做范围过滤（-1280~1280，定义见 uart.h）。  
通过过滤后，真正写入全局共享数据：
- g_offset_x = x，见 uart.c  
- g_offset_y = y，见 uart.c  
- g_offset_update_flag = 1，见 uart.c

最后无论成功与否都回到 WAIT_HEADER，重新同步下一帧，见 uart.c。

**这就是“写入数据”的完整定义**
- 中断里先写临时缓冲 s_data_buf  
- 帧完整且合法后，才写 g_offset_x/g_offset_y，并置更新标志  
- 主循环只看更新标志，不会读半包数据

**怎么分析数据（协议层 + 控制层）**

先是协议层分析（在 UART 状态机里）：
1. 帧结构分析：头-数据-尾，防止串口噪声导致错位  
2. 字节拼接分析：高字节在前，恢复为 int16  
3. 合法性分析：范围校验，过滤异常值  
4. 新鲜度分析：用 g_offset_update_flag 表示“有新数据”

再是控制层分析（在主循环里）：
主循环消费逻辑在 main.c。

1. 原子读取  
在 main.c 关中断，读 g_offset_x/g_offset_y 并清标志，避免与 ISR 抢占冲突。

2. 去抖分析  
在 main.c 把小偏差压成 0（死区 25），减少中心抖动。

3. 误差分析并计算控制量  
在 main.c 调用 PD，目标是 0 偏差。  
也就是“让收到的偏差趋近 0”。

4. 输出分析与约束  
先把控制量转成脉宽步进，再限每次最大步长，见 main.c 和 main.c。  
再对总脉宽做安全限幅，见 main.c。

5. 执行到硬件  
最终写入 PWM 比较寄存器由 servo.c 完成。

**用一个字节流例子看懂状态机**
假设收到序列：

0x2C 0x00 0x64 0xFF 0x9C 0x5B

1. 0x2C 命中帧头，进入 GET_DATA  
2. 收 4 字节进缓冲：00 64 FF 9C  
3. 收到 0x5B 命中帧尾，开始解析  
4. x = 0x0064 = 100  
5. y = 0xFF9C = -100（int16 补码）  
6. 范围合法，写 g_offset_x=100, g_offset_y=-100, flag=1  
7. 主循环下一次看到 flag=1 就拿去做 PD 计算并驱动舵机

如果你想，我可以下一步专门给你画一张“状态机时序图（按每个字节到达顺序）”，以及“出错帧/丢包/噪声下会发生什么”的排错表。