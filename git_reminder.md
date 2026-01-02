# Git提交信息撰写指南

## 📌 核心原则
每个提交信息都应清晰传达**做了什么**和**为什么做**。

## 📝 基本格式
```
<类型>[可选范围]: <简短描述>

[可选正文]

[可选脚注]
```

## 🏷️ 提交类型（Type）
| 类型 | 说明 | 示例 |
|------|------|------|
| `feat` | 新功能 | `feat: 添加用户注册功能` |
| `fix` | 修复bug | `fix: 修复登录页面的CSS溢出问题` |
| `docs` | 文档更新 | `docs: 更新API使用说明` |
| `style` | 代码格式调整 | `style: 调整缩进为2个空格` |
| `refactor` | 代码重构 | `refactor: 简化用户验证逻辑` |
| `test` | 测试相关 | `test: 添加登录功能单元测试` |
| `chore` | 构建/工具变更 | `chore: 更新webpack至v5` |
| `perf` | 性能优化 | `perf: 优化图片懒加载性能` |
| `ci` | CI配置 | `ci: 添加GitHub Actions工作流` |

## ✨ 撰写技巧

### 1. 标题行（第一行）
- **长度**：不超过50字符
- **时态**：使用祈使句现在时（"添加"而非"添加了"）
- **格式**：`类型: 描述`
- **示例**：`fix: 修复首页加载缓慢问题`

### 2. 正文内容
- **说明原因**：解释"为什么"要这样修改
- **行长度**：每行不超过72字符
- **使用要点**：
  ```markdown
  重构用户模块，因为：
  - 原有代码耦合度过高
  - 新结构便于单元测试
  - 性能提升约40%
  ```

### 3. 脚注部分
- 关联Issue：`Closes #123`、`Fixes JIRA-456`
- 破坏性变更：`BREAKING CHANGE: 旧API已废弃`
- 关联提交：`See also: abc1234`

## 📋 优秀示例

### 简单修复
```git
fix: 修复移动端菜单点击无效问题

修复事件委托绑定错误，确保移动端菜单可正常展开/收起。

Closes #89
```

### 功能添加
```git
feat(auth): 添加Google OAuth登录支持

- 新增Google OAuth2.0集成
- 添加用户同意页面
- 更新登录界面按钮样式

相关文档已同步更新至/docs/auth.md
```

### 重构示例
```git
refactor(utils): 重构日期处理函数

原有日期库过于臃肿，新实现：
- 减少依赖项3个
- 代码量减少60%
- 支持更多时区格式

BREAKING CHANGE: formatDate()函数参数顺序变更
```

## 🔧 工具推荐
1. **Commitizen** - 交互式提交工具
   ```bash
   npm install -g commitizen
   cz
   ```

2. **Husky + commitlint** - 提交校验
   ```bash
   # 安装
   npm install husky @commitlint/cli @commitlint/config-conventional
   ```

3. **Git模板** - 统一格式
   ```bash
   git config commit.template ~/.gitmessage
   ```

## ❌ 常见错误
| 错误示例 | 问题 | 改进 |
|----------|------|------|
| `update code` | 过于模糊 | `fix: 修复用户数据同步逻辑` |
| `bug fix` | 未说明具体问题 | `fix: 修复订单状态更新延迟` |
| `添加了新功能` | 时态不一致 | `feat: 添加导出PDF功能` |
| `修改了很多东西...` | 描述过长 | 拆分多个提交，每个提交专注一件事 |

## 💡 最佳实践
1. **一次提交只做一件事** - 保持提交原子性
2. **先写提交信息再编码** - 明确目标再行动
3. **定期整理提交历史** - 使用`rebase`保持清晰
4. **团队统一规范** - 建立团队内部的提交约定

## 📚 扩展阅读
- [Conventional Commits规范](https://www.conventionalcommits.org)
- [Angular提交规范](https://github.com/angular/angular/blob/main/CONTRIBUTING.md)

---

**记忆口诀**：类型明确，描述清晰，正文解释，关联问题。

> 好的提交信息是项目的活文档，让六个月后的你也能快速理解当时的变更意图。

---

*最后更新：2026-01-02*  
*将此文档分享给团队成员，确保统一规范*