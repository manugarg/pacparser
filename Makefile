# Copyright (C) 2024 Manu Garg.
# Author: Manu Garg <manugarg@gmail.com>
#
# Makefile for pacparser docker image.
-include version.mk

VERSION ?= $(shell git describe --always --tags --candidate=100)

GIT_TAG := $(shell git describe --exact-match --exclude tip --tags HEAD 2>/dev/null || /bin/true)
GIT_COMMIT = $(strip $(shell git rev-parse --short HEAD))

DOCKER_IMAGE ?= manugarg/pactester
ifeq "$(GIT_TAG)" ""
	DOCKER_TAGS := -t $(DOCKER_IMAGE):master -t $(DOCKER_IMAGE):main
else
	DOCKER_TAGS := -t $(DOCKER_IMAGE):$(GIT_TAG) -t $(DOCKER_IMAGE):latest
endif

docker_multiarch: Dockerfile
	docker buildx build --push \
		--build-arg BUILD_DATE=`date -u +"%Y-%m-%dT%H:%M:%SZ"` \
		--build-arg VERSION=$(VERSION) \
		--build-arg VCS_REF=$(GIT_COMMIT) \
		--platform linux/amd64,linux/arm64,linux/arm/v7 \
		$(DOCKER_TAGS) .

