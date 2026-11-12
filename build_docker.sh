#!/bin/bash
# Alternativa local ao GitHub Actions: precisa de Docker (Docker Desktop no Windows 11) com suporte a arm64.
set -e
docker run --rm --platform linux/arm64 -v "$PWD":/w -w /w debian:bullseye bash -c \
  "apt-get update && apt-get install -y build-essential libsdl2-dev zip && make test && make package"
echo "Pronto: dist/paciencia.zip"
