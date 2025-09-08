# spider2-build container
It is a container that can be used to build a project. It is based on the `spider2-base` container and contains the necessary tools to build a project.

## Local build
```bash 
    docker compose -f containers/spider2-build/docker-compose.yml build spider2-build
```
## Local build with push
```bash 
    docker compose -f containers/spider2-build/docker-compose.yml build --push spider2-build
```

## Build tests
```bash 
    docker compose -f containers/spider2-tests/docker-compose.yml build
```

## Run tests
```bash 
    docker compose -f containers/spider2-tests/docker-compose.yml run spider2-tests
```