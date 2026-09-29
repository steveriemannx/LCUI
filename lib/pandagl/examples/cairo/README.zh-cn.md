# Cairo Example

([English](./README.md)/**中文**)

PandaGL 和 Cairo 图形库混用的例子。大致流程是先用 PandaGL 创建画布，然后基于画布创建一个 Surface 供 Cairo 绘制复杂图形，之后用 PandaGL 填充简单的图形，最后用 Cairo 将绘制结果输出为 png 图片文件。

```bash
# 开启示例选项并配置项目
cmake -S ../../../.. -B ../../../../build -DLCUI_BUILD_EXAMPLES=ON

# 构建并运行 Cairo 示例
cmake --build ../../../../build --target pandagl-cairo-example
../../../../build/examples/pandagl-cairo-example/pandagl-cairo-example
```

输出的 output.png 图片内容如下：

![output](./output.png)
