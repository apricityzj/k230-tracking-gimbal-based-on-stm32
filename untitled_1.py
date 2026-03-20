# Untitled - By: Lenovo - Thu Mar 19 2026
import os
import time
import sys

from media.sensor import *
from media.display import *
from media.media import *

sensor = None

try:
    print("camera")
    #硬件初始化
    sensor = Sensor(width=1024,height=768)
    sensor.reset()
    sensor.set_framesize(width=1024,height=768)
    sensor.set_pixformat(Sensor.RGB565)
    #显示与内存池配置
    Display.init(Display.LT9611,to_ide=True)
    MediaManager.init()
    #启动sensor
    sensor.run()

    clock = time.clock()


    while True:
        clock.tick()
        os.exitpoint()
        img = sensor.snapshot(chn=CAM_CHN_ID_0)  #获取图像



        # img_rect = img.to_grayscale(copy=True)  #转为灰度图,img_rect为灰度图，来进行处理
        # img_rect = img_rect.binary([(134, 231)])  #二值化
        # rects = img_rect.find_rects(threshold=5000)  #查找矩形
        # for rect in rects:
        #     corner = rect.corners()  #获取矩形的四个角点坐标(列表形式)
        #     [[x1,y1],[x2,y2],[x3,y3],[x4,y4]] = corner
        #     img.draw_line(corner[0][0],corner[0][1],corner[1][0],corner[1][1],color=(0,255,0),thickness=2)  #画线
        #     img.draw_line(corner[1][0],corner[1][1],corner[2][0],corner[2][1],color=(0,255,0),thickness=2)  #画线
        #     img.draw_line(corner[2][0],corner[2][1],corner[3][0],corner[3][1],color=(0,255,0),thickness=2)  #画线
        #     img.draw_line(corner[3][0],corner[3][1],corner[0][0],corner[0][1],color=(0,255,0),thickness=2)  #画线
        blobs = img.find_blobs([(19, 38, -29, -10, 13, 33)], False,(0,0,1024,768),\
                               x_stride=5,y_stride=5,\
                               pixels_threshold=3000,margin=True)  #查找色块
        for blob in blobs:
            img.draw_rectangle(blob[0],blob[1],blob[2],blob[3],color=(0,255,0),thickness=2,fill=False)  #画矩形框


        #显示到ide
        img.compress_for_ide()  #自动放大到屏幕
        Display.show_image(img)
        print("{} FPS".format(clock.fps()))


except KeyboardInterrupt as e:
    print("stop",e)
except BaseException as e:
    print("stop",e)
#资源回收
finally:
    if isinstance(sensor,Sensor):
        sensor.stop()
    Display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    MediaManager.deinit()

