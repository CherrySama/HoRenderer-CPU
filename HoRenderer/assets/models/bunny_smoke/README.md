# 兔子烟雾 VDB

`bunny_smoke.vdb` 是 HoRenderer 当前使用的唯一正式兔子密度资产。Blender 只用于生成 OpenVDB，最终画面由 HoRenderer 的异构介质路径追踪完成。

## 资产约定

- 网格：单个 Float32 `density`，类型为 Fog Volume，背景值为 0。
- SHA-256：`3990bf72acfedde94078a4c21a51a3130de0a9e252a8a7e27fbf3cb6b03bb01e`。
- 有效体素：4,027,226。
- 正密度范围：0.0010000345～1.0。
- 体素间距：0.00069700391 Blender 长度单位。
- 有效索引边界：`(-12, -18, -17)` 到 `(263, 209, 259)`，均包含端点。

完整数值元数据见 `validation.json`。`source/base_density.vdb` 是继续调整边界结构时使用的固定源数据，不是另一个渲染版本。

## 重新生成

`source/build_density.py` 使用固定噪声种子生成新文件，并拒绝覆盖已有文件。应先输出到临时路径，检查后再决定是否替换正式资产：

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  --background --factory-startup --python-exit-code 1 \
  --python source/build_density.py -- \
  --output /private/tmp/bunny_smoke_next.vdb --relief 0.65
```

HoRenderer 通过 `FileManager::getModelPath("bunny_smoke/bunny_smoke.vdb")` 读取正式文件，不依赖当前工作目录或硬编码绝对路径。
