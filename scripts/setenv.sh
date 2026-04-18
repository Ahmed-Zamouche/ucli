#!/usr/bin/env bash

# GitLab Registry Configuration
export GH_REGISTRY_SERVER="ghcr.io"
export GH_REGISTRY_PATH="ahmed-zamouche/ucli"
export GH_REGISTRY_IMAGE_URL="$GH_REGISTRY_SERVER/$GH_REGISTRY_PATH:latest"

export GH_REGISTRY_TOKEN=$GH_REGISTRY_PUSH_TOKEN

# Credentials (Leave empty here, or point to where they are stored)
# Devs should set these locally or in a private .env file
export GH_REGISTRY_USER="${GH_REGISTRY_USER:-$USER}"
export GH_REGISTRY_TOKEN="${GH_REGISTRY_TOKEN:-}"

echo "✅ Environment variables loaded for $GH_REGISTRY_PATH"
if [ -z "$GH_REGISTRY_TOKEN" ]; then
    echo "⚠️  Note: GH_REGISTRY_TOKEN is not set. Run 'export GH_REGISTRY_TOKEN=...' to login."
fi

