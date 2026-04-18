#!/usr/bin/env bash

# dx - Development eXecution wrapper
# Usage: ./scripts/dx.sh <action> [args]

# 1. Ensure docker group permissions without requiring logout/login
if [ "$(id -gn)" != "docker" ] && getent group docker | grep -qw "$USER"; then
    exec newgrp docker << 'INNER_EOF'
        $0 "$@"
INNER_EOF
fi

# 2. Source the environment configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -z "$GH_REGISTRY_SERVER" ]; then
    source "$SCRIPT_DIR/setenv.sh"
fi

# 3. Resolve Project Directory
# If running from inside the project, use current folder name
PROJECT_DIR=$(basename "$(pwd)")
ACTION="$1"
shift 1

case "$ACTION" in
    login)
        if [ -z "$GH_REGISTRY_TOKEN" ]; then
            echo "❌ Error: GH_REGISTRY_TOKEN environment variable is not set."
            exit 1
        fi
        echo "Logging into $GH_REGISTRY_SERVER..."
        echo "$GH_REGISTRY_TOKEN" | docker login "$GH_REGISTRY_SERVER" -u "$GH_REGISTRY_USER" --password-stdin
        ;;

    start)
        echo "📥 Pulling remote image..."
        docker compose pull dev
        docker compose up -d dev
        ;;

    build)
        echo "🔨 Building image locally..."
        docker compose build dev
        ;;

    up)
        echo "⬆️  Starting local container (no pull)..."
        docker compose up -d dev
        ;;

    push)
        echo "🚀 Pushing image to registry: $GH_REGISTRY_IMAGE_URL"
        docker compose push dev
        ;;

    stop)
        echo "🛑 Stopping container..."
        docker compose down
        ;;

    attach)
        docker compose exec dev bash
        ;;

    run)
        docker compose exec dev "$@"
        ;;

    *)
        echo "Usage: dx <action> [cmd...]"
        echo ""
        echo "Actions:"
        echo "  login    - Authenticate with the GitLab Registry"
        echo "  start    - Pull the latest remote image and start"
        echo "  build    - Build the image locally (safety: does not push)"
        echo "  up       - Start the container using the local image"
        echo "  push     - Upload your local build to the GitLab Registry"
        echo "  stop     - Shut down the container"
        echo "  attach   - Open a bash shell inside the container"
        echo "  run      - Run a specific command inside the container"
        ;;
esac
