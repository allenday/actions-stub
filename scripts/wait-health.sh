#!/usr/bin/env sh
set -eu

timeout_seconds=${WAIT_HEALTH_TIMEOUT:-180}
services=${*:-app nginx mariadb}
deadline=$(( $(date +%s) + timeout_seconds ))

while :; do
  all_healthy=1
  for service in $services; do
    container_id=$(docker compose ps -q "$service")
    if [ -z "$container_id" ]; then
      all_healthy=0
      printf '%s: missing container\n' "$service"
      continue
    fi
    status=$(docker inspect --format '{{if .State.Health}}{{.State.Health.Status}}{{else}}{{.State.Status}}{{end}}' "$container_id")
    printf '%s: %s\n' "$service" "$status"
    if [ "$status" != "healthy" ] && [ "$status" != "running" ]; then
      all_healthy=0
    fi
  done

  if [ "$all_healthy" -eq 1 ]; then
    exit 0
  fi

  if [ "$(date +%s)" -ge "$deadline" ]; then
    docker compose ps
    exit 1
  fi
  sleep 3
done
