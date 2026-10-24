# =========================================================================
# Distroless or Minimal Alpine Layer for Winnow Production Runtime
# =========================================================================
FROM alpine:3.20

# Install core system dependencies for modern local compilation execution
RUN apk add --no-cache \
    libgcc \
    libstdc++ \
    make \
    gcc \
    g++ \
    openssl \
    ca-certificates

WORKDIR /workspace

# Create XDG cache compliance path for the tiny 1.5B/3B GGUF weights
RUN mkdir -p /home/winnow/.cache/winnow/models && \
    addgroup -g 1000 winnow && \
    adduser -u 1000 -G winnow -s /bin/sh -D winnow && \
    chown -R winnow:winnow /home/winnow

# The GitHub Actions workflow injects the compiled static binary here
COPY dist/winnow-linux-amd64 /usr/local/bin/winnow
RUN chmod +x /usr/local/bin/winnow

USER winnow:winnow

# Expose model cache as a mount point for persistent zero-network CI runs
VOLUME ["/home/winnow/.cache/winnow"]

ENTRYPOINT ["winnow"]
CMD ["--help"]
