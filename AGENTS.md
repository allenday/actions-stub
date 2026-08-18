# AGENTS.md

This repository contains a public Linux deployment demo for a C++ app, nginx,
and MariaDB. Keep changes small, reviewable, and free of real credentials or
private host facts.

## Commands

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
docker build --tag actions-stub:local .
cp .env.example .env
make demo
```

## Boundaries

- Do not commit `.env`, `local/`, `.beads/`, `.agents/`, or generated build
  output.
- Do not add Docker named volumes. All persistent/config paths must remain host
  bind mounts.
- Publish only the app image. nginx and MariaDB must remain official pinned
  image references in Compose.
- Do not use buildx in GitHub Actions for this project.
- Use Conventional Commits and keep service, Compose/demo, workflow, and docs
  changes logically separable.
