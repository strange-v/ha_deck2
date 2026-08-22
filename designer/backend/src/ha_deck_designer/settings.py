from pathlib import Path

from pydantic import field_validator
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    config_root: Path = Path.cwd()

    model_config = SettingsConfigDict(env_prefix="HA_DECK_", frozen=True)

    @field_validator("config_root")
    @classmethod
    def resolve_config_root(cls, value: Path) -> Path:
        return value.resolve(strict=True)
