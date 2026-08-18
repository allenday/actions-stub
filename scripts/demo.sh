#!/usr/bin/env sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

if ! command -v curl >/dev/null 2>&1; then
  echo "curl is required for make demo" >&2
  exit 1
fi

./scripts/prepare-demo.sh

docker compose up -d --build
./scripts/wait-health.sh

set -a
. ./.env
set +a

if [ ! -f "$MARIADB_DATA_DIR/README.md" ]; then
  {
    printf '# MariaDB Data Bind Mount\n\n'
    printf 'This directory is intentionally host bind-mounted for the actions-stub demo.\n'
    printf 'It is written after first database initialization so the MariaDB entrypoint can prepare an empty data directory.\n'
  } >"$MARIADB_DATA_DIR/README.md"
fi

echo "checking app health"
curl -fsS "http://127.0.0.1:${APP_PORT:-8080}/health"

echo "checking nginx proxy"
curl -fsS "http://127.0.0.1:${NGINX_HTTP_PORT:-8088}/"

echo "creating persistence row"
./scripts/query-db.sh "CREATE TABLE IF NOT EXISTS demo_persistence (id INT PRIMARY KEY, value INT NOT NULL); INSERT INTO demo_persistence (id, value) VALUES (1, 1) ON DUPLICATE KEY UPDATE value = value + 1; SELECT id, value FROM demo_persistence WHERE id = 1;"

before=$(./scripts/query-db.sh "SELECT value FROM demo_persistence WHERE id = 1;" | awk 'NR == 2 {print $1}')
echo "value before MariaDB restart: $before"

docker compose restart mariadb
./scripts/wait-health.sh mariadb

after=$(./scripts/query-db.sh "SELECT value FROM demo_persistence WHERE id = 1;" | awk 'NR == 2 {print $1}')
echo "value after MariaDB restart: $after"

if [ -z "$after" ] || [ "$after" != "$before" ]; then
  echo "MariaDB persistence check failed" >&2
  exit 1
fi

echo "inspecting bind-mounted README files"
docker compose run --rm app inspect-mounts

echo "demo complete"
