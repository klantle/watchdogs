# Podman Equivalent Commands

Podman is a drop-in replacement for Docker that doesn't require a daemon and runs rootless by default.

## Prerequisites & Setup

```bash
# Install Podman (Ubuntu/Debian)
sudo apt install podman podman-docker

# Install Podman (RHEL/Fedora)
sudo dnf install podman podman-docker

# On macOS
brew install podman

# On Windows
choco install podman-desktop
```

## Basic Podman Commands (Docker Syntax Compatible)

```bash
# Podman can use Docker syntax with alias
alias docker=podman  # Optional: create alias for muscle memory

# Or use podman directly with same syntax
podman run -it ubuntu
```

## Your Examples Converted to Podman

### Run Ubuntu Container
```bash
podman run -it ubuntu
```

### Saving image with specific session name
```bash
# Create container with name
podman run -it --name session_name ubuntu

# Sample
podman run -it --name my_ubuntu ubuntu

# Start existing container
podman start my_ubuntu

# Execute command in running container
podman exec -it my_ubuntu /bin/bash

# Exec with specific user
podman exec -it --user system my_ubuntu sh
```

### Common Podman Commands
```bash
podman ps -a                               # List all containers
podman start <container-name>              # Start the container
podman exec -it <container-name> /bin/bash # Enter the running container
podman stop <container-name>               # Stop the container
podman rm -f <container-name>              # Force-remove the container

# Additional Podman-specific commands
podman system prune                        # Clean unused containers/images
podman pod ls                              # List pods (like Kubernetes pods)
```

## Key Differences & Podman Advantages

1. **Rootless by default**: No need for `sudo` or adding users to groups
2. **No daemon required**: Containers run as child processes
3. **Pod support**: Native pod concept (groups of containers)
4. **Systemd integration**: Generate systemd unit files for containers

## Rootless Operation Example
```bash
# No need for sudo or user group modifications
whoami
# Output: yourusername

podman run -it ubuntu
# Works without sudo!

# Run as root if needed (for system containers)
sudo podman run -it ubuntu
```

## Podman-specific Features

```bash
# Create and manage pods
podman pod create --name mypod
podman run --pod mypod -d nginx
podman run --pod mypod -d redis

# Generate systemd service files
podman generate systemd --name my_ubuntu --files

# Rootless port binding (requires additional setup)
podman run -p 8080:80 nginx
```

## Migration Notes

1. **Images**: Podman uses the same OCI images as Docker
2. **Dockerfiles**: Compatible without changes
3. **Volumes**: Same syntax, but paths might differ in rootless mode
4. **Networking**: Similar but with different backend

## Docker Compose Alternative
```bash
# Install podman-compose
pip install podman-compose

# Or use native podman pods
podman play kube docker-compose.yaml  # For Kubernetes YAML
```

Podman maintains excellent compatibility with Docker commands while offering better security and flexibility with its rootless architecture.