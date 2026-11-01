# core_msg 消息代码生成

## 文件

| 文件 | 说明 |
|------|------|
| `messages.json` | 消息源数据（344 条，替代原 `Messages.xlsx`） |
| `create_messages.py` | 生成 `TVPMessages.decl.h` |

## 生成产物

构建时写入 **构建目录**（非源码树）：

```
${CMAKE_CURRENT_BINARY_DIR}/gen/TVPMessages.decl.h
```

与 `tjs2/script/create_world_map.py` 相同模式：`execute_process` 在 configure 时生成，`add_custom_command` 在 `messages.json` 变更时重新生成。

## 编辑消息

1. 修改 `messages.json` 中对应项的 `id` / `text`（`category` 仅作文档分类：`core` / `platform`）
2. 重新配置或构建 `core_msg`

```bash
python3 create_messages.py /path/to/build/gen/TVPMessages.decl.h
```

## JSON 格式

```json
{
  "version": 1,
  "messages": [
    { "id": "TVPCannotOpenStorage", "text": "Cannot open storage %1", "category": "core" }
  ]
}
```

- `id`：全局符号名，同时作为 TJS 消息表键名
- `text`：默认英文，支持 `%1` `%2` `%%`
