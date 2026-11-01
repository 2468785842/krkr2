# launch.json 与配置

[← 索引](README.md)

---

## 1. 三种配置的分工

| 存储 | 格式 | 生命周期 | 谁写 | 谁读 |
|------|------|----------|------|------|
| **launch.json** | JSON | 单次启动 / launcher 生成 | 外部 UI、脚本、RN | `LaunchOptionsAdapter` |
| **GlobalConfig** | XML | 持久全局 | 设置 UI / 用户 | `GlobalConfigManager` |
| **IndividualConfig** | XML | 持久 per-game | 设置 UI | `IndividualConfigManager` |

```text
launch.json  = 「这次怎么启动」
GlobalConfig = 「模拟器默认偏好」
IndividualConfig = 「某个游戏的存档/偏好」
```

**Phase 1 不要求** 将 GlobalConfig XML 改为 JSON。

---

## 2. launch.json Schema（草案）

```json
{
  "$schema": "https://krkr2.dev/schemas/launch-v1.json",
  "xp3Path": "/absolute/or/relative/path/to/game.xp3",
  "engineArgs": [
    "-debug=yes",
    "-contfreq=60"
  ],
  "globalOverrides": {
    "locale": "ja",
    "fullscreen": true
  },
  "individualOverrides": {
    "soundVolume": 80
  }
}
```

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `xp3Path` | string | 否* | 启动目标；与 CLI `--xp3` 等价 |
| `engineArgs` | string[] | 否 | 传给 `applyEngineArg` |
| `globalOverrides` | object | 否 | 写入 GlobalConfig（内存 overlay，可选持久化） |
| `individualOverrides` | object | 否 | 启动后对该游戏 IndividualConfig 生效 |

\* `--config` 使用时通常必填 `xp3Path`，除非引擎参数中已含可启动路径（少见）。

---

## 3. 优先级

从高到低：

```text
1. CLI 显式参数（--xp3、-- 后 engineArgs）
2. launch.json 内字段
3. GlobalConfig / IndividualConfig XML 已存值
4. 代码内默认值
```

同一 engine 参数名：**CLI > JSON > 已有 TVPProgramArguments**。

---

## 4. globalOverrides 字段映射（示例）

与 `GlobalConfigManager` / 设置页对齐（实现时按 `AllConfig` key 补全）：

| launch.json | GlobalConfig key | 类型 |
|-------------|------------------|------|
| `locale` | `locale` | string |
| `fullscreen` | `fullscreen` | bool |
| `recentGames` | — | 建议不由 launch.json 写 |

`applyConfigFile()` 伪代码：

```cpp
void LaunchOptionsAdapter::applyGlobalOverrides(const json& j) {
    auto* mgr = GlobalConfigManager::GetInstance();
    if (j.contains("locale"))
        mgr->SetValue("locale", j["locale"].get<std::string>());
    if (j.contains("fullscreen"))
        mgr->SetValueInt("fullscreen", j["fullscreen"].get<bool>() ? 1 : 0);
}
```

是否 **SaveToFile** 由策略决定：

| 模式 | 行为 |
|------|------|
| 临时启动 | 仅内存 overlay，不写 XML |
| 「保存为默认」launcher | overlay 后 `SaveToFile()` |

默认：**临时 overlay**，不写 XML。

---

## 5. Desktop 使用方式

```bash
# launcher / RN / 脚本 生成
cat > /tmp/launch.json <<'EOF'
{
  "xp3Path": "/games/foo.xp3",
  "engineArgs": ["-debug=yes"]
}
EOF

krkr2 --config /tmp/launch.json
```

或合并 CLI：

```bash
krkr2 --config ~/.config/krkr2/last.json -- -contfreq=120
```

---

## 6. Mobile 使用方式

Settings Activity 写入与 **同一 schema** 等价的 JSON 或拆成 Intent extras：

```kotlin
intent.putExtra("xp3Path", path)
intent.putExtra("engineArgs", arrayOf("-debug=yes"))
// 或 putExtra("launchConfigJson", jsonString)
```

Native 侧：

```cpp
LaunchOptions opts = parseFromIntent(intent);
LaunchOptionsAdapter::apply(opts);
```

---

## 7. 与 GlobalConfig XML 结构对照

现有 XML（`GlobalConfigManager`）：

```xml
<Config>
  <Item key="locale" value="ja"/>
  <Custom key="..." value="..."/>
  <KeyMap key="..." value="..."/>
</Config>
```

| launch.json | XML 节点 |
|-------------|----------|
| `globalOverrides.*` | `<Item key="..." value="..."/>` |
| — | `<Custom>` 仍只由设置 UI 管理 |
| — | `<KeyMap>` 仍只由设置 UI 管理 |

**CustomArguments / KeyMap** 不建议放进 launch.json Phase 1。

---

## 8. 版本与兼容

| 字段 | 说明 |
|------|------|
| `$schema` / `version` | 可选；解析器忽略未知字段 |
| `launch-v1` | 首版；后续 additive 扩展 |

---

## 9. 示例文件位置（建议）

| 平台 | 路径 |
|------|------|
| Linux | `~/.config/krkr2/launch.json` |
| macOS | `~/Library/Application Support/krkr2/launch.json` |
| Windows | `%APPDATA%/krkr2/launch.json` |
| Android | app private storage 或 Intent 直传 |

---

## 10. 实现清单

- [ ] `LaunchConfigJson.cpp`：parse / validate（可用 nlohmann/json 或已有 JSON 库）  
- [ ] schema 文档与 CI 样例 `docs/launch/examples/launch.min.json`（可选）  
- [ ] RN / launcher 开发指南链接到本文  

---

## 11. 相关文档

- [adapter.md](adapter.md) — `applyConfigFile`  
- [cli-parser.md](cli-parser.md) — `--config`  
- [UI 层](../UI_LAYER.md) — 设置 UI 与 launch.json 分工  
