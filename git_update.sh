#!/usr/bin/env bash

# ===============================================
# GIT 代码更新
# 使用方法：
#   ./git_update.sh commit_description    
# ===============================================

COMMIT_DESCRIPTION=${1:-1}
git add .
git commit -m "$COMMIT_DESCRIPTION"
git pull
git push -u origin main #仅当branch为main