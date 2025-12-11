# log4cxx.properties 配置文件详解

## 配置文件位置

log4cxx 会在以下位置查找配置文件（按优先级）：
1. `log4cxx.properties`（当前目录）
2. `config/log4cxx.properties`
3. 工作目录下的 `log4cxx.properties`

## 配置项详解

### 1. Root Logger 配置

```properties
log4j.rootLogger=INFO, fileAppender, consoleAppender
```

**含义：**
- `log4j.rootLogger`：根日志记录器配置
- `INFO`：日志级别，只记录 INFO、WARN、ERROR 级别的日志
  - 日志级别从低到高：DEBUG < INFO < WARN < ERROR < FATAL
- `fileAppender, consoleAppender`：两个输出目标（Appender）
  - `fileAppender`：文件输出
  - `consoleAppender`：控制台输出

### 2. 文件输出配置（RollingFileAppender）

```properties
log4j.appender.fileAppender=org.apache.log4j.RollingFileAppender
log4j.appender.fileAppender.File=logs/app.log
log4j.appender.fileAppender.MaxFileSize=10MB
log4j.appender.fileAppender.MaxBackupIndex=10
log4j.appender.fileAppender.layout=org.apache.log4j.PatternLayout
log4j.appender.fileAppender.layout.ConversionPattern=%d{yyyy-MM-dd HH:mm:ss} [%p] [%c{1}] %m%n
log4j.appender.fileAppender.ImmediateFlush=true
```

**各项说明：**

- **`org.apache.log4j.RollingFileAppender`**：
  - 滚动文件追加器，当日志文件达到指定大小时自动创建新文件
  - 旧文件会被重命名并保留

- **`File=logs/app.log`**：
  - 日志文件路径：`logs/app.log`
  - 如果 `logs` 目录不存在，log4cxx 会尝试创建（但可能失败）

- **`MaxFileSize=10MB`**：
  - 单个日志文件最大大小：10MB
  - 达到此大小后，会创建新文件（如 `app.log.1`, `app.log.2` 等）

- **`MaxBackupIndex=10`**：
  - 保留的历史日志文件数量：10 个
  - 超过此数量后，最旧的文件会被删除
  - 例如：`app.log`, `app.log.1`, `app.log.2`, ..., `app.log.10`

- **`layout=org.apache.log4j.PatternLayout`**：
  - 使用模式布局，可以自定义日志格式

- **`ConversionPattern=%d{yyyy-MM-dd HH:mm:ss} [%p] [%c{1}] %m%n`**：
  - 日志格式模式：
    - `%d{yyyy-MM-dd HH:mm:ss}`：日期时间（年-月-日 时:分:秒）
    - `[%p]`：日志级别（INFO, WARN, ERROR 等）
    - `[%c{1}]`：日志记录器名称（只显示最后一部分，如 "CADVisBird"）
    - `%m`：日志消息内容
    - `%n`：换行符
  - 示例输出：`2024-01-15 14:30:25 [INFO] [CADVisBird] Application started`

- **`ImmediateFlush=true`**：
  - 立即刷新缓冲区到磁盘
  - 确保日志及时写入，但可能影响性能
  - 建议在调试时设为 `true`，生产环境可设为 `false`

### 3. 控制台输出配置（ConsoleAppender）

```properties
log4j.appender.consoleAppender=org.apache.log4j.ConsoleAppender
log4j.appender.consoleAppender.layout=org.apache.log4j.PatternLayout
log4j.appender.consoleAppender.layout.ConversionPattern=%d{yyyy-MM-dd HH:mm:ss} [%p] [%c{1}] %m%n
```

**各项说明：**

- **`org.apache.log4j.ConsoleAppender`**：
  - 控制台追加器，将日志输出到标准输出（stdout）或标准错误（stderr）

- **`layout` 和 `ConversionPattern`**：
  - 与控制台输出使用相同的格式

### 4. 应用程序特定 Logger 配置

```properties
log4j.logger.CADVisBird=INFO
log4j.additivity.CADVisBird=false
```

**各项说明：**

- **`log4j.logger.CADVisBird=INFO`**：
  - 为名为 "CADVisBird" 的 Logger 设置日志级别为 INFO
  - 代码中使用 `log4cxx::Logger::getLogger("CADVisBird")` 获取此 Logger

- **`log4j.additivity.CADVisBird=false`**：
  - **重要**：控制日志是否传播到父 Logger（rootLogger）
  - `false`：日志**不会**传播到 rootLogger 的 Appender
    - 这意味着日志**不会**写入 `fileAppender` 和 `consoleAppender`
    - 需要为 CADVisBird Logger 单独配置 Appender
  - `true`：日志**会**传播到 rootLogger 的 Appender（推荐）
    - 日志会同时写入文件和控制台

## ⚠️ 当前配置的问题

当前配置中 `log4j.additivity.CADVisBird=false` 会导致：
- 日志不会输出到 `logs/app.log` 文件
- 日志不会输出到控制台
- 除非为 CADVisBird Logger 单独配置 Appender

## 推荐的修复方案

### 方案 1：启用日志传播（推荐）

```properties
log4j.additivity.CADVisBird=true
```

这样日志会传播到 rootLogger 的 Appender，同时写入文件和控制台。

### 方案 2：为 CADVisBird Logger 单独配置 Appender

```properties
log4j.logger.CADVisBird=INFO, cadVisBirdFileAppender
log4j.additivity.CADVisBird=false

log4j.appender.cadVisBirdFileAppender=org.apache.log4j.RollingFileAppender
log4j.appender.cadVisBirdFileAppender.File=logs/app.log
log4j.appender.cadVisBirdFileAppender.MaxFileSize=10MB
log4j.appender.cadVisBirdFileAppender.MaxBackupIndex=10
log4j.appender.cadVisBirdFileAppender.layout=org.apache.log4j.PatternLayout
log4j.appender.cadVisBirdFileAppender.layout.ConversionPattern=%d{yyyy-MM-dd HH:mm:ss} [%p] [%c{1}] %m%n
log4j.appender.cadVisBirdFileAppender.ImmediateFlush=true
```

## 日志级别说明

| 级别 | 值 | 说明 | 使用场景 |
|------|-----|------|----------|
| DEBUG | 10000 | 调试信息 | 详细的调试信息，通常只在开发时使用 |
| INFO | 20000 | 一般信息 | 程序运行的一般信息，如启动、配置加载等 |
| WARN | 30000 | 警告信息 | 潜在问题，但不影响程序运行 |
| ERROR | 40000 | 错误信息 | 错误事件，可能影响功能 |
| FATAL | 50000 | 致命错误 | 严重错误，可能导致程序终止 |

**日志级别过滤规则：**
- 如果设置级别为 `INFO`，则只记录 INFO、WARN、ERROR、FATAL 级别的日志
- DEBUG 级别的日志会被忽略

## 其他常用的 Appender 类型

- **`FileAppender`**：简单的文件追加器，不会滚动
- **`DailyRollingFileAppender`**：按日期滚动（每天一个文件）
- **`NTEventLogAppender`**：Windows 事件日志（仅 Windows）
- **`SyslogAppender`**：系统日志（Unix/Linux）

## 示例：完整的推荐配置

```properties
# Root logger configuration
log4j.rootLogger=INFO, fileAppender, consoleAppender

# File appender configuration
log4j.appender.fileAppender=org.apache.log4j.RollingFileAppender
log4j.appender.fileAppender.File=logs/app.log
log4j.appender.fileAppender.MaxFileSize=10MB
log4j.appender.fileAppender.MaxBackupIndex=10
log4j.appender.fileAppender.layout=org.apache.log4j.PatternLayout
log4j.appender.fileAppender.layout.ConversionPattern=%d{yyyy-MM-dd HH:mm:ss} [%p] [%c{1}] %m%n
log4j.appender.fileAppender.ImmediateFlush=true

# Console appender configuration
log4j.appender.consoleAppender=org.apache.log4j.ConsoleAppender
log4j.appender.consoleAppender.layout=org.apache.log4j.PatternLayout
log4j.appender.consoleAppender.layout.ConversionPattern=%d{yyyy-MM-dd HH:mm:ss} [%p] [%c{1}] %m%n

# Application-specific logger
log4j.logger.CADVisBird=INFO
log4j.additivity.CADVisBird=true  # 改为 true，让日志传播到 rootLogger
```

## 调试技巧

1. **检查配置文件是否被加载**：
   - 查看控制台是否有 "log4cxx: Configuration loaded from ..." 消息

2. **检查日志文件是否创建**：
   - 确认 `logs` 目录存在
   - 检查 `logs/app.log` 文件是否有内容

3. **临时降低日志级别**：
   - 将 `INFO` 改为 `DEBUG` 可以看到更多日志

4. **测试日志输出**：
   - 在代码中添加 `LOG_INF_S("Test message")` 验证日志是否正常工作

