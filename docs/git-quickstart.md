# Git / GitHub 入门（够用版）

## 一次性设置

```bash
git config --global user.name "你的名字"
git config --global user.email "你的邮箱"
```

## 把本项目推上 GitHub

1. 在 GitHub 网页点 **New repository**，名字例如 `stm32-foc-driver`，**不要**勾选自动生成 README / .gitignore / License（本仓库已经有）。
2. 在本地项目目录执行：

```bash
git init
git add .
git commit -m "初始化仓库骨架"
git branch -M main
git remote add origin https://github.com/<你的用户名>/stm32-foc-driver.git
git push -u origin main
```

> 国内 push 不稳定时，可用 SSH 方式（配置 SSH key）或检查网络代理。

## 日常循环

```bash
git status                 # 看改了什么
git add firmware/foc/      # 选择要提交的文件
git commit -m "feat: 实现 Park 变换"
git push
```

## 提交信息建议

`feat:` 新功能　`fix:` 修复　`docs:` 文档　`hw:` 硬件改动　`test:` 测试

## 分支

```bash
git switch -c feat/current-loop   # 新建并切换分支
git push -u origin feat/current-loop
```
然后在 GitHub 网页发 Pull Request 合并回 `main`。一个人开发也建议这样，便于回顾。

## 发布版本

```bash
git tag v0.1.0
git push --tags
```
然后在 GitHub 的 **Releases** 页面基于该 tag 创建发布，上传 Gerber / BOM / hex。

## 注意

- 不要提交编译产物（`.gitignore` 已处理 Keil、KiCad 的常见垃圾文件）。
- 二进制大文件（Gerber zip、固件 hex）放 Releases，不放仓库。
- CubeMX 的 `.ioc` 文件一定要提交，否则别人无法重新生成工程。
