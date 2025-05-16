# Cursor Code Rules

This directory contains custom rules for the Cursor code editor to enforce certain coding standards in the project.

## Rules

### No Chinese in Comments
- Ensures that all comments in the code are written in English
- Detects both single-line (`//`) and multi-line (`/* */`) comments containing Chinese characters
- Unicode range `\u4e00-\u9fa5` covers common Chinese characters

### No Chinese in Logs
- Ensures that all log messages are written in English
- Detects calls to logging functions (`LOG_INF`, `LOG_DBG`, `LOG_WAR`, `LOG_ERR`) containing Chinese characters

## How It Works

The rules use regular expressions to identify patterns in the code that match Chinese characters in specific contexts. When Cursor's linting system detects these patterns, it will flag them as errors according to the configured severity level.

## Implementation Details

- The rules work by detecting Unicode code points in the range commonly used for Chinese characters
- The pattern matching is applied to all supported file types in the project
- Rules are enforced at edit-time through Cursor's built-in linting system 