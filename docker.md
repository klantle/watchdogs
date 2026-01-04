## Docker

#### Prerequisites

- Docker installed and running
- User added to docker group

#### Setup Commands

```yaml
# Downloading - apt << Ubuntu >>
sudo apt install docker.io
```

```yaml
# Add user to docker group (Linux)
sudo usermod -aG docker $USER
newgrp docker

# Start Docker service
sudo systemctl enable docker
sudo systemctl start docker
```

#### Run Ubuntu

```yaml
docker run -it ubuntu
```

### Saving image

```yaml
# Specific session name
docker run -it --name session_name ubuntu
# Sample
docker run -it --name my_ubuntu ubuntu
# And you can running again
docker start my_ubuntu
docker exec -it my_ubuntu /bin/bash
# Exec with specific user
sudo docker exec -it --user system my_ubuntu sh
```

#### Common Docker Commands

```yaml
docker ps -a                               # List all containers
docker start <container-name>              # Start the container
docker exec -it <container-name> /bin/bash # Enter the running container
docker stop <container-name>               # Stop the container
docker rm -f <container-name>              # Force-remove the container
```
