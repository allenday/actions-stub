#!/usr/bin/env sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

if [ ! -f .env ]; then
  echo "missing .env; run: cp .env.example .env" >&2
  exit 1
fi

set -a
. ./.env
set +a

: "${APP_CONFIG_DIR:?set APP_CONFIG_DIR in .env}"
: "${NGINX_CONFIG_DIR:?set NGINX_CONFIG_DIR in .env}"
: "${NGINX_CONTENT_DIR:?set NGINX_CONTENT_DIR in .env}"
: "${MARIADB_CONFIG_DIR:?set MARIADB_CONFIG_DIR in .env}"
: "${MARIADB_DATA_DIR:?set MARIADB_DATA_DIR in .env}"

mkdir -p "$APP_CONFIG_DIR" "$NGINX_CONFIG_DIR" "$NGINX_CONTENT_DIR" "$MARIADB_CONFIG_DIR" "$MARIADB_DATA_DIR"

copy_if_missing() {
  source_file=$1
  target_file=$2
  if [ ! -f "$target_file" ]; then
    cp "$source_file" "$target_file"
  fi
}

write_readme_if_missing() {
  directory=$1
  title=$2
  if [ ! -f "$directory/README.md" ]; then
    {
      printf '# %s\n\n' "$title"
      printf 'This directory is intentionally host bind-mounted for the actions-stub demo.\n'
      printf 'It must remain outside Docker named volumes so operators can inspect and back it up directly.\n'
    } >"$directory/README.md"
  fi
}

copy_if_missing ops/nginx/default.conf "$NGINX_CONFIG_DIR/default.conf"
copy_if_missing ops/mariadb/conf.d/actions-stub.cnf "$MARIADB_CONFIG_DIR/actions-stub.cnf"

if [ ! -f "$APP_CONFIG_DIR/service.conf" ]; then
  {
    printf 'APP_NAME=%s\n' "${APP_NAME:-actions-stub}"
    printf 'IRC_SERVICE_MODE=stub\n'
  } >"$APP_CONFIG_DIR/service.conf"
fi

if [ ! -f "$NGINX_CONTENT_DIR/index.html" ]; then
  printf '<!doctype html><title>actions-stub</title><p>actions-stub nginx bind content</p>\n' \
    >"$NGINX_CONTENT_DIR/index.html"
fi

write_readme_if_missing "$APP_CONFIG_DIR" "App IRC-Service Config Bind Mount"
write_readme_if_missing "$NGINX_CONFIG_DIR" "Nginx Config Bind Mount"
write_readme_if_missing "$NGINX_CONTENT_DIR" "Nginx Content Bind Mount"
write_readme_if_missing "$MARIADB_CONFIG_DIR" "MariaDB Config Bind Mount"

echo "prepared host bind mounts"
