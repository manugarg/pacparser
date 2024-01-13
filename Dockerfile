# Use Alpine Linux as the base image
FROM alpine:latest as build-stage

# Install build dependencies
RUN apk update && apk add --no-cache \
    gcc \
    git \
    g++ \
    bash \
    make

WORKDIR /build
COPY src /build
RUN make pactester

# Final stage
FROM alpine:latest

WORKDIR /app
COPY --from=build-stage /build/pactester /app/

CMD /app/pactester