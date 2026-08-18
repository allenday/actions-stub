FROM debian:12-slim AS build

RUN apt-get update \
  && apt-get install -y --no-install-recommends ca-certificates cmake g++ make \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src
COPY tests ./tests

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  && cmake --build build --parallel \
  && ctest --test-dir build --output-on-failure \
  && cmake --install build --prefix /install

FROM debian:12-slim AS runtime

LABEL org.opencontainers.image.source="https://github.com/allenday/actions-stub"
LABEL org.opencontainers.image.description="C++ hello-world service for the actions-stub deployment demo"

RUN useradd --system --uid 10001 --create-home --home-dir /home/actions-stub actions-stub \
  && mkdir -p /config \
  && chown -R actions-stub:actions-stub /config

COPY --from=build /install/bin/actions-stub /usr/local/bin/actions-stub

USER 10001:10001
EXPOSE 8080

ENTRYPOINT ["/usr/local/bin/actions-stub"]
CMD ["serve"]
