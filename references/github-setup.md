# GitHub 环境配置

## HTTPS + Credential Store（推荐）

GitHub PAT 不支持 git push 密码认证，必须用 credential store：

```bash
# 1. 配置 credential store
git config --global credential.helper store

# 2. 添加 token（替换 TOKEN）
echo "https://user:ghp_XX...XXXX@github.com" > ~/.git-credentials

# 3. 验证 remote
git remote -v
# 应显示: https://github.com/LongTimeDebug/can-dash.git

# 4. 推送
GIT_TERMINAL_PROMPT=0 git push origin main
```

## SSH 方式（稳定方案）

```bash
# 1. 切换到 SSH remote
git remote set-url origin git@github.com:LongTimeDebug/can-dash.git

# 2. 验证 SSH 连接
ssh -T git@github.com
# 应显示: Hi LongTimeDebug! You've successfully authenticated...

# 3. 推送
git push origin main
```

## Token 缺少 workflow scope

workflow 文件 push 被 reject 时：
- 检查 token 是否有 `workflow` scope
- 或临时移除 .github/workflows/ 再推
- 让用户重新生成 token

## GitHub 个人账户改名

只能网页操作：https://github.com/settings/admin

## HTTP 408 推送失败

```bash
# 等待几秒后重试，通常1-3次内成功
GIT_TERMINAL_PROMPT=0 git push origin main
```

## 连接被拒绝（port 443）

网络环境问题，提交到本地，网络恢复后直接 push：
```bash
git add -A && git commit -m "fix: ..."
# 网络恢复后
git push origin main
```
