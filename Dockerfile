# Use Ubuntu as the base image
FROM ubuntu:latest AS build-stage

# Install build dependencies. git isn't needed: only src/ is copied in, so
# the version comes from the VERSION build arg instead of git describe.
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    libc6-dev \
    make \
    && rm -rf /var/lib/apt/lists/*

ARG VERSION
WORKDIR /build
COPY src /build
RUN make pactester VERSION=$VERSION

# Final stage
FROM ubuntu:latest

WORKDIR /app
COPY --from=build-stage /build/pactester /app/

# Metadata params
ARG BUILD_DATE
ARG VERSION
ARG VCS_REF
# Metadata
LABEL org.label-schema.build-date=$BUILD_DATE \
      org.label-schema.name="Pactester" \
      org.label-schema.vcs-url="https://github.com/manugarg/pacparser" \
      org.label-schema.vcs-ref=$VCS_REF \
      org.label-schema.version=$VERSION \
      com.microscaling.license="Apache-2.0"

ENTRYPOINT ["/app/pactester"]