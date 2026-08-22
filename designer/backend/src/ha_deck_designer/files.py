import tempfile
from hashlib import sha256
from pathlib import Path


class InvalidConfigPathError(ValueError):
    pass


def resolve_config_path(config_root: Path, relative_path: str, *, must_exist: bool = True) -> Path:
    candidate = (config_root / relative_path).resolve(strict=must_exist)
    try:
        candidate.relative_to(config_root)
    except ValueError as exc:
        raise InvalidConfigPathError("Configuration path escapes the configured root") from exc
    if candidate.suffix.lower() not in {".yaml", ".yml"}:
        raise InvalidConfigPathError("Configuration must be a YAML file")
    if must_exist and not candidate.is_file():
        raise InvalidConfigPathError("Configuration path is not a file")
    return candidate


def calculate_revision(source: str) -> str:
    return f"sha256:{sha256(source.encode('utf-8')).hexdigest()}"


def atomic_write_text(path: Path, source: str) -> None:
    mode = path.stat().st_mode
    temporary_path: Path | None = None
    with tempfile.NamedTemporaryFile(
        "w",
        encoding="utf-8",
        newline="",
        dir=path.parent,
        prefix=f".{path.name}.",
        suffix=".tmp",
        delete=False,
    ) as temporary:
        temporary.write(source)
        temporary.flush()
        temporary_path = Path(temporary.name)
    try:
        temporary_path.chmod(mode)
        temporary_path.replace(path)
    finally:
        if temporary_path.exists():
            temporary_path.unlink()
