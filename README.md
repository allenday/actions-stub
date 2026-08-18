# actions-stub

A small Linux deployment demo for a C++20 hello-world service, official nginx,
and official MariaDB. The app image is the only image this repository builds or
publishes.

## Quick Start

Prerequisites: Docker Engine, Docker Compose, GNU Make, and curl.

```bash
cp .env.example .env
make demo
```

The demo:

- prepares host bind-mount directories under `./local/`;
- builds and starts the app, nginx, and MariaDB services;
- waits for all health checks;
- calls the app and nginx proxy;
- queries MariaDB;
- prints `README.md` from every configured bind mount;
- restarts MariaDB and verifies a persisted database row is still present.

## Bind Mounts

All persistent and configuration state is stored in host bind mounts. There are
no Docker named volumes in `compose.yaml`.

| Purpose | `.env` variable | Container path |
| --- | --- | --- |
| app/IRC-service config | `APP_CONFIG_DIR` | `/config` |
| nginx config | `NGINX_CONFIG_DIR` | `/etc/nginx/conf.d` |
| nginx content/data | `NGINX_CONTENT_DIR` | `/usr/share/nginx/html` |
| MariaDB config | `MARIADB_CONFIG_DIR` | `/etc/mysql/conf.d` |
| MariaDB data | `MARIADB_DATA_DIR` | `/var/lib/mysql` |

The app also receives read-only inspection mounts for nginx and MariaDB paths.
Run this after `make up`:

```bash
make inspect-mounts
```

## Development

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
docker build --tag actions-stub:local .
```

Useful Make targets:

```bash
make test
make docker-build
make up
make health
make db-query
make down
```

## GitHub Actions And GHCR

Pull requests compile the C++ service, run tests, and build the app Docker
image without pushing.

Pushes to `main` and version tags matching `v*` publish only:

```text
ghcr.io/allenday/actions-stub
```

The publish workflow uses `GITHUB_TOKEN` with `contents: read` and
`packages: write`, then runs ordinary `docker build`, `docker tag`, and
`docker push`. It does not use buildx. GHCR pushes use bounded retry/backoff so
transient secondary rate limits fail only after repeated push failures.

GitHub notes that container packages published from a workflow with
`GITHUB_TOKEN` are linked to the workflow repository automatically, while new
packages may be private by default and visibility/access can be configured in
the package settings. See GitHub's official Container Registry documentation:
https://docs.github.com/packages/working-with-a-github-packages-registry/working-with-the-container-registry

## Troubleshooting

If `make demo` says `.env` is missing, run:

```bash
cp .env.example .env
```

If a port is already in use, edit `APP_PORT`, `NGINX_HTTP_PORT`, or
`MARIADB_PORT` in `.env`.

If MariaDB does not initialize, stop the stack and inspect the host data mount:

```bash
make down
ls -la ./local/mariadb/data
docker compose logs mariadb
```

If Docker Hub or GHCR is slow, retry the Docker command. The C++ tests can run
without Docker when CMake is installed.

## Baseline Exception

This repository was empty when the project started. An empty root commit was
pushed to `main` only to give GitHub a base branch for the initial service PR
(#1). The service, Compose demo, workflow, and documentation files were then
introduced through that review branch before normal `main`-based follow-up
work began.
