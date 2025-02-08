# Use Ubuntu as the base image
FROM ubuntu:latest AS build-stage

# Install build dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    git \
    g++ \
    bash \
    make

WORKDIR /build
COPY src /build
RUN make pactester

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