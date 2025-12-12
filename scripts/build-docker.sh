#!/bin/bash
set -e

IMAGE_NAME="ghcr.io/xiantongw/srd-dev"
VERSION="${1:-latest}"
PUSH=false

for arg in "$@"; do
    if [ "$arg" = "--push" ]; then
        PUSH=true
    fi
done

echo "Building multi-architecture Docker image: ${IMAGE_NAME}:${VERSION}"
echo "Platforms: linux/amd64, linux/arm64"

if ! docker buildx version &> /dev/null; then
    echo "Error: docker buildx is not available"
    echo "Please install Docker Desktop or enable buildx"
    exit 1
fi

if ! docker buildx inspect multiarch &> /dev/null; then
    echo "Creating multiarch builder..."
    docker buildx create --name multiarch --use
else
    echo "Using existing multiarch builder..."
    docker buildx use multiarch
fi

docker buildx inspect --bootstrap

BUILD_ARGS=(
    --platform linux/amd64,linux/arm64
    --file Dockerfile
    --tag "${IMAGE_NAME}:${VERSION}"
)


if [ "$VERSION" != "latest" ]; then
    BUILD_ARGS+=(--tag "${IMAGE_NAME}:latest")
fi


if [ "$PUSH" = true ]; then
    BUILD_ARGS+=(--push)
    echo "Building and pushing..."
else
    BUILD_ARGS+=(--load)
    echo "Building locally (not pushing)..."
fi


docker buildx build "${BUILD_ARGS[@]}" .

echo ""
echo "✅ Build completed!"
echo ""

if [ "$PUSH" = true ]; then
    echo "Image pushed to: ${IMAGE_NAME}:${VERSION}"
    if [ "$VERSION" != "latest" ]; then
        echo "               : ${IMAGE_NAME}:latest"
    fi
else
    echo "Image built locally: ${IMAGE_NAME}:${VERSION}"
    echo ""
    echo "To push to registry, run:"
    echo "  $0 $VERSION --push"
fi