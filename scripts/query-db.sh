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

sql=${1:-"SELECT 1 AS ok;"}

docker compose exec -T mariadb \
  mariadb -u"${MARIADB_USER:-actions_stub}" -p"${MARIADB_PASSWORD:?set MARIADB_PASSWORD in .env}" \
  "${MARIADB_DATABASE:-actions_stub}" -e "$sql"
