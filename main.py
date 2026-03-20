import time, os, gc, sys
from machine import UART, FPIOA
from media.sensor import *
from media.media import *
import image

# ================= 核心配置 =================
# 统一使用 800x480 分辨率
# 这样不仅 IDE 预览清晰，且完美契合你 (cx-400, -cy+240) 的居中算法
WIDTH = 800
HEIGHT = 480

# 串口引脚映射与初始化
fpioa = FPIOA()
fpioa.set_function(11, FPIOA.UART2_TXD)
fpioa.set_function(12, FPIOA.UART2_RXD)
u1 = UART(UART.UART2, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)

# 颜色阈值 [0:红色, 1:绿色]
thresholds = [
    (0, 100, 21, 127, 0, 127),      # generic_red_thresholds  (code=1)
    (11, 82, -96, -26, -128, 127)   # generic_green_thresholds (code=2)
]

# 状态标志：0=暂停, 1=红色, 2=绿色
flag = 1
sensor = None

try:
    # ================= 硬件初始化 =================
    sensor = Sensor(width=WIDTH, height=HEIGHT)
    sensor.reset()
    sensor.set_vflip(True)
    sensor.set_hmirror(True)
    sensor.set_framesize(width=WIDTH, height=HEIGHT, chn=CAM_CHN_ID_0)
    sensor.set_pixformat(Sensor.RGB565, chn=CAM_CHN_ID_0)

    # 纯净初始化 MediaManager，不加载 Display，避免无屏幕报错
    MediaManager.init()
    sensor.run()

    print("系统启动成功！无物理屏幕模式，请在 IDE 缓冲区预览画面。")

    while True:
        # 【修正】没识别到物体时，默认输出画面中心点 (400, 240)
        # 这样下方的串口发送算出来就是 $0 0$，防止外接设备乱动
        cx, cy = 400, 240

        img = sensor.snapshot()

        # ================= AI 识别逻辑 =================
        if flag > 0:
            # 你的 flag 刚好对应 code 的值 (1或2)
            for blob in img.find_blobs(thresholds, pixels_threshold=3000, area_threshold=3000, merge=True):
                # 提取重复逻辑，精简代码
                if blob.code() == flag and 50 <= blob.w() < 500 and 50 <= blob.h() < 300:
                    cx, cy = blob.cx(), blob.cy()
                    img.draw_rectangle([v for v in blob.rect()], color=(255, 255, 255), thickness=2)
                    break  # 找到符合条件的色块后即跳出，保证只追踪一个

        # ================= 串口发送 =================
        # 根据 800x480 的画面，(400, 240) 是绝对中心
        uart_data = f"${cx-400} {-cy+240}$\r\n"
        print("IDE终端测试:", uart_data) # 加这一句
        u1.write(uart_data.encode('utf-8')) # 加上 encode 更安全
        # print(uart_data, end="") # 如果需要调试串口数据可以解除注释

        # ================= IDE 交互界面绘制 =================
        # 背景白框
        img.draw_rectangle(10, 10, 100, 60, color=(255,255,255), fill=True)
        img.draw_rectangle(690, 10, 100, 60, color=(255,255,255), fill=True)
        img.draw_rectangle(10, 410, 100, 60, color=(255,255,255), fill=True)
        img.draw_rectangle(690, 410, 100, 60, color=(255,255,255), fill=True)

        # 黑色文字
        img.draw_string_advanced(10, 10, 50, "红色", color=(0, 0, 0), scale=4)
        img.draw_string_advanced(690, 10, 50, "绿色", color=(0, 0, 0), scale=4)
        img.draw_string_advanced(10, 410, 50, "暂停", color=(0, 0, 0), scale=4)
        img.draw_string_advanced(690, 410, 50, "退出", color=(0, 0, 0), scale=4)

        # 传回电脑 IDE 显示
        img.compress_for_ide()
        gc.collect()

except KeyboardInterrupt:
    print("用户手动停止程序")
except Exception as e:
    print(f"运行异常: {e}")
finally:
    # ================= 释放资源 =================
    u1.deinit()
    if isinstance(sensor, Sensor):
        sensor.stop()
    MediaManager.deinit()
    print("资源释放完毕")
