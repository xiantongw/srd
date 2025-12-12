#!/bin/bash
set -e

IMAGE_NAME="ghcr.io/xiantongw/srd-dev"
VERSION="${1:-latest}"

echo "Building Docker image: ${IMAGE_NAME}:${VERSION}"

docker build -t "${IMAGE_NAME}:${VERSION}" -f Dockerfile .
if [ "$VERSION" != "latest" ]; then
    docker tag "${IMAGE_NAME}:${VERSION}" "${IMAGE_NAME}:latest"
fi

echo ""
echo "Build completed!"
echo ""
echo "To push to GitHub Container Registry:"
echo "  docker push ${IMAGE_NAME}:${VERSION}"
if [ "$VERSION" != "latest" ]; then
    echo "  docker push ${IMAGE_NAME}:latest"
fi
echo ""
echo "Or run this script with --push flag:"
echo "  $0 $VERSION --push"

if [ "$2" = "--push" ] || [ "$1" = "--push" ]; then
    echo ""
    echo "Pushing to registry..."
    docker push "${IMAGE_NAME}:${VERSION}"
    if [ "$VERSION" != "latest" ]; then
        docker push "${IMAGE_NAME}:latest"
    fi
    echo "Push completed!"
fi
