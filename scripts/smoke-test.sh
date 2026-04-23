#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

COMPOSE_CMD=(docker compose)
API_URL="${API_URL:-http://localhost:8000}"

cleanup() {
  "${COMPOSE_CMD[@]}" down -v --remove-orphans >/dev/null 2>&1 || true
}

require_docker() {
  if ! command -v docker >/dev/null 2>&1; then
    echo "Docker is not installed or not on PATH" >&2
    exit 1
  fi

  if ! docker info >/dev/null 2>&1; then
    echo "Docker daemon is not running. Start Docker Desktop or the Docker service and rerun." >&2
    exit 1
  fi
}

wait_for_postgres() {
  echo "Waiting for PostgreSQL..."

  for _ in $(seq 1 60); do
    if "${COMPOSE_CMD[@]}" exec -T db env PGPASSWORD=password pg_isready -U user -d grid_db >/dev/null 2>&1; then
      echo "PostgreSQL is ready"
      return
    fi

    if [[ "$((SECONDS % 5))" == "0" ]]; then
      echo "PostgreSQL is still starting..."
    fi

    sleep 1
  done

  echo "PostgreSQL did not become ready in time" >&2
  echo "Recent db logs:" >&2
  "${COMPOSE_CMD[@]}" logs --no-color --tail=50 db >&2 || true
  exit 1
}

wait_for_api() {
  echo "Waiting for API..."

  for _ in $(seq 1 60); do
    if curl -fsS "$API_URL/" >/dev/null 2>&1; then
      echo "API is ready"
      return
    fi

    if [[ "$((SECONDS % 5))" == "0" ]]; then
      echo "API is still starting..."
    fi

    sleep 1
  done

  echo "API did not become ready in time" >&2
  exit 1
}

post_json() {
  local path="$1"
  local body="$2"

  curl -fsS -X POST "$API_URL${path}" -H 'Content-Type: application/json' -d "$body"
}

query_db() {
  local sql="$1"

  "${COMPOSE_CMD[@]}" exec -T db env PGPASSWORD=password psql -U user -d grid_db -h 127.0.0.1 -tAc "$sql"
}

trap cleanup EXIT

require_docker

if [[ ! -f .env ]]; then
  cp .env.example .env
  echo "Created .env from .env.example"
fi

cleanup

echo "Starting database..."
"${COMPOSE_CMD[@]}" up -d --build db
wait_for_postgres

echo "Starting API..."
"${COMPOSE_CMD[@]}" up -d --build api
wait_for_api

echo "Creating a test node..."
node_response="$(post_json /nodes/ '{"name":"Smoke Test Node","location":"Lab"}')"
node_id="$(printf '%s' "$node_response" | python3 -c 'import json,sys; print(json.load(sys.stdin)["id"])')"

echo "Posting readings that should trigger incidents..."
post_json /readings/ "$(printf '{"node_id":%s,"voltage":220.0,"load":650.0}' "$node_id")" >/dev/null
post_json /readings/ "$(printf '{"node_id":%s,"voltage":100.0,"load":120.0}' "$node_id")" >/dev/null
post_json /readings/ "$(printf '{"node_id":%s,"voltage":99.0,"load":130.0}' "$node_id")" >/dev/null
post_json /readings/ "$(printf '{"node_id":%s,"voltage":98.0,"load":125.0}' "$node_id")" >/dev/null

readings_count="$(query_db "select count(*) from readings where node_id = ${node_id};" | tr -d '[:space:]')"
incidents_count="$(query_db "select count(*) from incidents where node_id = ${node_id};" | tr -d '[:space:]')"
overload_count="$(query_db "select count(*) from incidents where node_id = ${node_id} and type = 'overload';" | tr -d '[:space:]')"
voltage_drop_count="$(query_db "select count(*) from incidents where node_id = ${node_id} and type = 'voltage_drop';" | tr -d '[:space:]')"

[[ "$readings_count" == "4" ]] || { echo "Expected 4 readings, got $readings_count" >&2; exit 1; }
[[ "$incidents_count" == "2" ]] || { echo "Expected 2 incidents, got $incidents_count" >&2; exit 1; }
[[ "$overload_count" == "1" ]] || { echo "Expected 1 overload incident, got $overload_count" >&2; exit 1; }
[[ "$voltage_drop_count" == "1" ]] || { echo "Expected 1 voltage_drop incident, got $voltage_drop_count" >&2; exit 1; }

echo "Building sensor services..."
"${COMPOSE_CMD[@]}" build cpp_sensor java_sensor

echo "Smoke test passed"