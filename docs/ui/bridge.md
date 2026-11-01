# 外壳与引擎桥接

[← 索引](README.md)

---

## 1. 修订摘要

| 初版 | **现行** |
|------|----------|
| Electron N-API ↔ `engine.h` | Desktop Launcher **spawn** + **`launch.json`** |
| RN Turbo Module 同进程跑游戏 | RN **仅 Settings**；Game 为 Native Activity |
| 统一 `krkr_engine_*` C ABI | C ABI **可选**；主路径为 **Launch 层 Adapter** |

主文档：[Launch 层 Adapter](../launch/adapter.md)

---

## 2. 桥接模型总览

```text
                    Desktop                          Mobile
                      │                                │
         ┌────────────┴────────────┐      ┌───────────┴───────────┐
         │  launch.json + spawn    │      │  Intent / JSON extras │
         └────────────┬────────────┘      └───────────┬───────────┘
                      │                                │
                      └──────────┬─────────────────────┘
                                 ▼
                    LaunchOptionsAdapter::apply()
                      ├─ LaunchContext::xp3Path
                      ├─ TVPSetCommandLine (engineArgs)
                      └─ GlobalConfig overlay (optional)
                                 ▼
                           krkr2core 引擎
```

**Desktop Launcher 不链接引擎。**  
**RN Settings 不链接引擎。**  
仅 **krkr2 可执行文件 / GameActivity** 链 C++ 核心。

---

## 3. launch.json 契约（跨端主桥）

与 [launch-config.md](../launch/launch-config.md) 一致：

```json
{
  "xp3Path": "/games/foo.xp3",
  "engineArgs": ["-debug=yes"],
  "globalOverrides": { "locale": "ja" }
}
```

| 端 | 写入 | 读取 |
|----|------|------|
| Desktop Launcher | `~/.config/krkr2/launch.json` | `krkr2 --config` |
| RN Settings | Intent extra 或同路径文件 | `LaunchOptionsFromIntent` |
| 脚本 / CI | 任意路径 | `--config` |

TypeScript 类型在 `packages/shared/src/launch/LaunchProfile.ts`。

---

## 4. Desktop：Launcher → 引擎

### 4.1 spawn（推荐）

```typescript
// apps/launcher/electron/main.ts
import { spawn } from 'child_process';
import path from 'path';
import fs from 'fs/promises';

const KRKR2_EXE = process.env.KRKR2_EXE ?? 'krkr2';

export async function spawnEngine(profile: LaunchProfile): Promise<void> {
  const configPath = path.join(app.getPath('userData'), 'last-launch.json');
  await fs.writeFile(configPath, JSON.stringify(profile, null, 2));

  spawn(KRKR2_EXE, ['--config', configPath], {
    detached: true,
    stdio: 'ignore',
  }).unref();
}
```

### 4.2 引擎侧

```cpp
// platforms/*/main.cpp
LaunchOptions opts = parseCommandLine(argc, argv);
LaunchOptionsAdapter::apply(opts);
// LaunchContext::hasStartupTarget() → 跳过 Cocos 文件选择
```

**无需** Electron preload、N-API、BrowserWindow。

---

## 5. Mobile：Settings → Game

### 5.1 RN Turbo Module（薄）

```typescript
// NativeModules.KrkrLauncher
interface KrkrLauncherSpec {
  startGame(profileJson: string): Promise<void>;
  getRecentGames(): Promise<string>; // JSON
}
```

### 5.2 Android Native

```kotlin
// KrkrLauncherModule.kt
@ReactMethod
fun startGame(profileJson: String, promise: Promise) {
    val intent = Intent(reactContext, GameActivity::class.java)
    intent.putExtra("launchProfile", profileJson)
    reactContext.startActivity(intent)
    promise.resolve(null)
}
```

```cpp
// GameActivity.onCreate
LaunchOptions opts = LaunchOptionsFromJson(profileJson);
LaunchOptionsAdapter::apply(opts);
// → GLSurfaceView + engine
```

---

## 6. TVPGetCommandLine 适配（引擎内）

外壳 **不** 直接调 `TVPGetCommandLine`。  
`LaunchOptionsAdapter` 将 `engineArgs` 转为 `TVPSetCommandLine`：

```text
launch.json engineArgs: ["-debug=yes"]
    → TVPSetCommandLine(L"-debug", L"yes")
    → 引擎内 TVPGetCommandLine(L"-debug", &val)  // 不变
```

详见 [adapter.md](../launch/adapter.md)。

---

## 7. 可选 C ABI（`bridge/include/krkr/engine.h`）

当 **GameActivity 内** 或 **未来 in-process 工具** 需要稳定 C 接口时保留；**非 Launcher 必需**。

| API | 用途 |
|-----|------|
| `krkr_engine_launch` | Native 启动 XP3（GameActivity 可选封装） |
| `krkr_engine_stop` | 退出游戏回 Settings |
| `krkr_engine_scan_directory` | Settings 扫描库（也可纯 Java/RN + Storage API） |

Desktop Launcher **不必**调用上述 API。

初版 `engine.h` 草案仍见下文 §8，实现优先级低于 Launch Adapter。

---

## 8. C ABI 草案（可选，低优先级）

```c
// bridge/include/krkr/engine.h — 供 GameActivity / 测试，非 Launcher
KRKR_API KrkrEngineError krkr_engine_launch(const char *xp3_path);
KRKR_API KrkrEngineError krkr_engine_stop(void);
```

与 Rust 插件：`docs/rust/ffi.md` 路径独立。

---

## 9. GlobalConfig 桥接

| 来源 | 行为 |
|------|------|
| `launch.json` → `globalOverrides` | `LaunchOptionsAdapter` 内存 overlay |
| RN Settings 保存 | 写 XML（`GlobalConfigManager::SaveToFile`） |
| Launcher「设为默认」 | overlay + `SaveToFile` |

Settings UI **不** 需要 `krkr_engine_*`；读写 ConfigManager 或 JSON 即可，启动时再灌引擎。

---

## 10. 线程与安全

| 场景 | 约定 |
|------|------|
| spawn 后 Launcher 退出 | 引擎独立进程，无共享状态 |
| Intent 传 profile | JSON 大小限制；路径 UTF-8 |
| GameActivity | 引擎线程 ≠ RN JS 线程 |

---

## 11. 测试

| 测试 | 内容 |
|------|------|
| shared | `LaunchProfile` schema 校验 |
| Launcher E2E | 写 JSON → spawn mock 二进制 |
| Android | Intent → `apply()` → mock xp3 |
| 集成 | `engineArgs` → `TVPGetCommandLine` |

---

## 12. 迁移对照（初版 bridge.md）

| 删除/降级 | 替代 |
|-----------|------|
| Electron `contextBridge` + N-API | spawn + `--config` |
| RN `KrkrGameView` in JS tree | Native `GameActivity` |
| `EngineBridge.launchGame` in Launcher | `spawnEngine(profile)` |
| BrowserWindow 事件 | 无 |
