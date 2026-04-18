# 🚀 uCLI Development Environment

This repository uses a containerized development environment to ensure all developers use the exact same toolchain (`toolchain`, `cppcheck`, `lcov`, etc.) without needing to install them manually on their host machines.

## 🛠 Prerequisites

Docker & Docker Compose: Ensure they are installed and your user is part of the `docker` group.

GitLab Access: You need a Group Access Token with `read:packages` (for pulling) or `write:packages` (for pushing) permissions.

## 🏗 Setup Instructions

  1. Configure your Shell
  To use the `dx` command from anywhere within the project folders, add the following to your `~/.bashrc` (or `~/.zshrc`):

```bash
# GitLab Registry Credentials
export GH_REGISTRY_USER="your_username"
export GH_REGISTRY_PULL_TOKEN="glpat-your-pull-token"
export GH_REGISTRY_PUSH_TOKEN="glpat-your-push-token" # Optional, for maintainers
export GH_REGISTRY_TOKEN=$GH_REGISTRY_PULL_TOKEN

# The DX Global Wrapper
dx() {
    local dir="$PWD"
    while [ "$dir" != "/" ]; do
        if [ -f "$dir/scripts/dx.sh" ]; then
            "$dir/scripts/dx.sh" "$@"
            return $?
        fi
        dir="$(dirname "$dir")"
    done
    echo "❌ Error: Could not find scripts/dx in this or any parent directory."
    return 1
}
```

Remember to run source `~/.bashrc` after saving.

  2. Login to the Registry

Run this once to authenticate your Docker client with the Prevas GitLab registry:


```bash
dx login
```

## 💻 Daily Workflow

### Start Developing

Pull the latest pre-built image and start the container in the background:


```bash
dx start
```

### Enter the Container

Open an interactive bash session inside the container:


```bash
dx attach
```

### Run Commands Directly

You can run commands inside the container without "attaching" first:

```bash
dx run make
dx run python3 my_script.py
```

### Stop the Environment

```bash
dx stop
```

## 🏗 Maintainer Workflow (Updating the Image)

If you modify the `Dockerfile` or the toolchain, follow these steps to update the shared image for the team:

1. ***Switch to Push Token***: Ensure your `$GH_REGISTRY_TOKEN` is set to your Push Token (Developer role).

```bash
export GH_REGISTRY_TOKEN=$GH_REGISTRY_PUSH_TOKEN
dx login
```

2. ***Build Locally***:

```bash
dx build
```

3. Test Locally: Start your local build to verify changes:

```bash
dx up
dx attach
```

4. Push to GitLab: Once verified, publish the image to the registry:

```bash
dx push
```

## 📂 Project Structure

* `scripts/dx`: The core execution logic.
* `scripts/setenv`: Configuration file defining registry paths and variables.
* `.devcontainer/Dockerfile`: Defines the system packages and toolchains.
* `docker-compose.yml`: Manages volume mounts (USB, Workspace) and image tagging.

## ❓ Troubleshooting

* Permission Denied: If you get Docker permission errors, the `dx` script will attempt to fix this using `newgrp docker`, but ensure your user was added to the group via `sudo usermod -aG docker $USER`.
