from media.sensor import Sensor
from media.display import Display
from media.media import MediaManager
from machine import UART, FPIOA
import image, gc, time

# Yellow LAB threshold: (L_min, L_max, A_min, A_max, B_min, B_max)
# From threshold editor - NOTE: check if invert is needed
YELLOW_LAB_THRESHOLD = (0, 100, -128, 127, 50, 127)
#39
#(40, 100, -29, 24, 70, 127)

WIDTH  = 1280
HEIGHT = 720
DEAD_ZONE = 20

def send_hex_packet(uart_obj, x, y):
    x_hex = int(x) & 0xFFFF
    y_hex = int(y) & 0xFFFF
    packet = bytearray([
        0xAA,
        (x_hex >> 8) & 0xFF,
        x_hex & 0xFF,
        (y_hex >> 8) & 0xFF,
        y_hex & 0xFF,
        0xBB
    ])
    uart_obj.write(packet)

fpioa = FPIOA()
fpioa.set_function(11, FPIOA.UART2_TXD)
fpioa.set_function(12, FPIOA.UART2_RXD)
uart = UART(UART.UART2, baudrate=115200)
print("UART ready")

sensor = Sensor()
sensor.reset()
sensor.set_framesize(width=WIDTH, height=HEIGHT)
sensor.set_pixformat(Sensor.RGB565)

Display.init(Display.VIRT, width=WIDTH, height=HEIGHT, to_ide=True)
MediaManager.init()
sensor.run()

screen_cx = WIDTH  / 2.0
screen_cy = HEIGHT / 2.0

print("Yellow tracking started...")

try:
    last_offset_x, last_offset_y = 0, 0
    last_send_ms = time.ticks_ms()
    SEND_INTERVAL_MS = 10  # send every 10ms (100Hz), independent of frame rate

    while True:
        img = sensor.snapshot()

        # 画面中心十字准星（始终绘制）
        img.draw_cross(int(screen_cx), int(screen_cy),
                       color=(255, 0, 0), size=15, thickness=2)

        blobs = img.find_blobs(
            [YELLOW_LAB_THRESHOLD],
            pixels_threshold=200,
            area_threshold=200,
            merge=True,
            margin=40
        )

        offset_x, offset_y = 0, 0

        if blobs:
            b = max(blobs, key=lambda b: b.pixels())
            cx, cy = b.cx(), b.cy()

            offset_x = int(cx - screen_cx)
            offset_y = int(cy - screen_cy)

            if abs(offset_x) < DEAD_ZONE: offset_x = 0
            if abs(offset_y) < DEAD_ZONE: offset_y = 0

            # 用色块四角顶点画贴合目标的四边形
            try:
                corners = b.corners()  # 4个顶点 [(x0,y0),(x1,y1),(x2,y2),(x3,y3)]
                if corners and len(corners) == 4:
                    for i in range(4):
                        x0, y0 = corners[i]
                        x1, y1 = corners[(i + 1) % 4]
                        img.draw_line(x0, y0, x1, y1,
                                      color=(255, 0, 0), thickness=3)
            except AttributeError:
                # fallback: 旋转矩形
                r = b.rect()
                img.draw_rectangle(r[0], r[1], r[2], r[3],
                                   color=(255, 0, 0), thickness=3)
            img.draw_cross(cx, cy, color=(0, 255, 0), size=20, thickness=2)
            img.draw_string_advanced(10, 10, 28,
                "X:%d Y:%d" % (offset_x, offset_y),
                color=(0, 255, 0))
        else:
            img.draw_string_advanced(10, 10, 28, "Target Lost", color=(255, 0, 0))

        last_offset_x, last_offset_y = offset_x, offset_y

        # send at fixed interval regardless of image processing speed
        now = time.ticks_ms()
        if time.ticks_diff(now, last_send_ms) >= SEND_INTERVAL_MS:
            send_hex_packet(uart, last_offset_x, last_offset_y)
            last_send_ms = now

        Display.show_image(img.to_rgb888())
        gc.collect()

except KeyboardInterrupt:
    print("Stopped by user")
finally:
    sensor.stop()
    Display.deinit()
    MediaManager.deinit()
    print("Done")
