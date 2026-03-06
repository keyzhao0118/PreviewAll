## 提交信息风格

- 使用 Conventional Commits 格式：`<type>(<scope>): <描述>`
- type 类型包括：`feat`、`fix`、`docs`、`style`、`refactor`、`perf`、`test`、`build`、`ci`、`chore`
- scope 应反映受影响的模块，例如 `previewimage`、`previewcode`、`previewarchive`、`handler`、`installer`
- 描述部分使用中文，首行不超过 72 个字符
- 如有需要，在空行后添加详细说明，解释*做了什么*以及*为什么*

### 示例

```
feat(previewimage): 在工具栏中添加缩放至适合按钮

fix(previewarchive): 优雅处理损坏的压缩包文件

docs: 更新 README 中的构建说明

build: 升级 Qt 依赖至 6.8
```
