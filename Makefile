.PHONY: configure build test docker-build prepare up down logs health inspect-mounts db-query demo clean

configure:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

build: configure
	cmake --build build --parallel

test: build
	ctest --test-dir build --output-on-failure

docker-build:
	docker build --tag actions-stub:local .

prepare:
	./scripts/prepare-demo.sh

up: prepare
	docker compose up -d --build

down:
	docker compose down

logs:
	docker compose logs --tail=100

health:
	./scripts/wait-health.sh

inspect-mounts:
	docker compose run --rm app inspect-mounts

db-query:
	./scripts/query-db.sh

demo:
	./scripts/demo.sh

clean:
	rm -rf build
	docker compose down --remove-orphans
