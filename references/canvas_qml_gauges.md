# Canvas QML 仪表盘绘制技巧

## 三层架构（推荐）

```
Item (root, property value/minValue/maxValue/unit)
├── bgCanvas      — 静态：表盘+刻度+中心帽，Component.onCompleted 一次
├── needleCanvas  — 动态：只画指针，Connections 监听 value 变化
└── QML Text      — 动态：value/unit 叠加层，绑定 root.value
```

## bgCanvas 绘制内容

- 外环发光（RadialGradient）
- 表盘外圈（arc）
- 表盘主体（RadialGradient）
- 内圈
- 刻度线 + 数字（for 循环，major/minor 区分）
- 中心圆帽

## needleCanvas 绘制

```qml
onPaint: {
    var t = Math.max(0, Math.min(1, (value - minValue) / (maxValue - minValue)))
    var needleAngle = startAngleDeg + t * (endAngleDeg - startAngleDeg)
    var rad = needleAngle * Math.PI / 180

    ctx.save()
    ctx.translate(cx, cy)
    ctx.rotate(rad)

    // 三角形指针：顶点朝右（0°），rotate 后指向正确方向
    ctx.beginPath()
    ctx.moveTo(needleLen, 0)
    ctx.lineTo(-8, -5)
    ctx.lineTo(-8,  5)
    ctx.closePath()

    var needleGrad = ctx.createLinearGradient(0, 0, -8, 0)
    needleGrad.addColorStop(0, needleColor)
    needleGrad.addColorStop(1, needleColor + "aa")
    ctx.fillStyle = needleGrad
    ctx.fill()
    ctx.restore()
}
```

## 关键技巧

1. **指针三角形顶点朝右**：`ctx.moveTo(needleLen, 0)` 作为第一个点，rotate 后自动指向正确方向
2. **三层分离避免跳变**：bgCanvas 只画一次，needleCanvas 只画指针，QML Text 做 value display
3. **EMA 平滑**：QML 层用 20ms Timer + alpha=0.22 做指数移动平均
4. **避让指针底部**：`anchors.verticalCenterOffset: root.width * 0.16` 防止文字与指针重叠
5. **Connections 监听 value 变化**：Canvas 本身没有 `onValueChanged` 属性，必须用 Connections

## VMware X11 跳变问题

CAN 帧 50ms 间隔 + `requestPaint()` 异步重绘 + X11 转发延迟 → 指针跳变感。

**终极方案**：`Rotation` 变换 + `SpringAnimation`（动画在渲染线程，不走主线程重绘）。

## 渐变色技巧

```qml
var grad = ctx.createRadialGradient(cx, cy, innerR, cx, cy, outerR)
grad.addColorStop(0, "#111111")
grad.addColorStop(1, "#0a0a0a")
ctx.fillStyle = grad
ctx.beginPath()
ctx.arc(cx, cy, outerR, 0, 2 * Math.PI)
ctx.fill()
```

## 刻度数字角度计算

```qml
for (var i = 0; i <= totalMinorTicks; i++) {
    var t = i / totalMinorTicks
    var angle = startAngle + t * angleRange
    var isMajor = (i % minorTicksPerMajor === 0)

    var cos = Math.cos(angle)
    var sin = Math.sin(angle)

    // 刻度线
    ctx.beginPath()
    ctx.moveTo(cx + tickInner * cos, cy + tickInner * sin)
    ctx.lineTo(cx + tickOuter * cos, cy + tickOuter * sin)
    ctx.stroke()

    // 数字
    if (isMajor) {
        var majorIndex = i / minorTicksPerMajor
        var labelValue = minValue + (majorIndex / (majorTickCount)) * (maxValue - minValue)
        var lx = cx + labelR * cos
        var ly = cy + labelR * sin + 4  // +4 微调垂直对齐
        ctx.fillText(labelValue.toFixed(0), lx, ly)
    }
}
```

## GaugeCanvas 使用示例

```qml
GaugeCanvas {
    id: speedGauge
    x: 60; y: 180
    width: 380; height: 380
    minValue: 0
    maxValue: 260
    value: 0
    unit: "km/h"
    dialColor: "#1a2a1a"
    needleColor: "#00FF88"
    labelColor: "#88FF88"
    majorTickCount: 13
    minorTicksPerMajor: 5
    startAngleDeg: 135
    endAngleDeg: 405
}
```
