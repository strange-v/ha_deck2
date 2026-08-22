FROM node:24-alpine AS frontend
WORKDIR /build/designer/frontend
COPY designer/frontend/package.json designer/frontend/package-lock.json ./
RUN npm ci --no-audit --no-fund
COPY designer/frontend/ ./
RUN npm run build

FROM ghcr.io/astral-sh/uv:0.12.5 AS uv

FROM python:3.12-slim AS runtime
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    HA_DECK_CONFIG_ROOT=/config
WORKDIR /app/designer/backend
COPY --from=uv /uv /uvx /bin/
COPY designer/backend/pyproject.toml designer/backend/uv.lock designer/backend/.python-version ./
COPY designer/backend/src/ ./src/
RUN uv sync --frozen --no-dev
COPY --from=frontend /build/designer/frontend/dist /app/designer/frontend/dist
EXPOSE 8099
VOLUME ["/config"]
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD [".venv/bin/python", "-c", "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8099/api/health', timeout=2)"]
CMD [".venv/bin/python", "-m", "uvicorn", "ha_deck_designer.main:app", "--host", "0.0.0.0", "--port", "8099"]
