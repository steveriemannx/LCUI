# Cairo Example

(**English**/[中文](./README.zh-cn.md))

An example of drawing with PandaGL and Cairo graphics libraries. It creates a canvas using PandaGL, then creates a Surface based on the canvas for Cairo to draw complex shapes, fills simple shapes with PandaGL, and outputs the results as a PNG image file using Cairo.

```bash
# Configure the repository with examples enabled
cmake -S ../../../.. -B ../../../../build -DLCUI_BUILD_EXAMPLES=ON

# Build and run the Cairo example
cmake --build ../../../../build --target pandagl-cairo-example
../../../../build/examples/pandagl-cairo-example/pandagl-cairo-example
```

Open the output.png file and you'll see something like this:

![output](./output.png)
