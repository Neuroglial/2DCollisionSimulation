# 2DCollisionSimulation

![img](https://github.com/Neuroglial/2DCollisionSimulation/blob/main/res/Brand.png)

本项目为一个2D小球碰撞模拟器，创建该项目的初衷在于通过对该模拟器的不断迭代，学习和应用优化算法及并行计算技术。模拟器将展示多个小球在二维平面内的运动和碰撞过程，提供实时的物理仿真效果。

初始暴力求解最多能够实时模拟368个小球，经过一系列优化最终超过600,000个，性能提升超1,650倍。

该模拟器会一直发射小球，直到帧数低于75帧为止，通过比对最终小球数量判断算法优劣。

暴力求解（368个）：
![gif](https://github.com/Neuroglial/2DCollisionSimulation/blob/main/res/Violent%20solution.gif)

网格划分（1,922个）提升522%：
![gif](https://github.com/Neuroglial/2DCollisionSimulation/blob/main/res/GridDivision.gif)

四叉树（1,158个）提升315%：
![gif](https://github.com/Neuroglial/2DCollisionSimulation/blob/main/res/Quadtree.gif)

多线程和细节优化（超过260,000个）提升超过70,600%：
![gif](https://github.com/Neuroglial/2DCollisionSimulation/blob/main/res/MultiThread.gif)

优化解算任务提交顺序，使算时不产生内存冲突，整个结算过程零同步（超过600,000个）提升超过165,000%：
![gif](https://github.com/Neuroglial/2DCollisionSimulation/blob/main/res/MultiThread_1.gif)