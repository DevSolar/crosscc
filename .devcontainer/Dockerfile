FROM mcr.microsoft.com/devcontainers/base:ubuntu

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install -y --no-install-recommends \
        clang \
        clangd \
        clang-format \
        lldb \
        lld \
        make \
        mingw-w64 \
    && rm -rf /var/lib/apt/lists/*

# Provide x86_64-w64-mingw32-clang expected by Makefile's check-win-api target
RUN printf '#!/bin/sh\nexec clang --target=x86_64-w64-mingw32 "$@"\n' > /usr/local/bin/x86_64-w64-mingw32-clang \
    && chmod +x /usr/local/bin/x86_64-w64-mingw32-clang
