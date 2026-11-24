"""Runtime configuration (environment variables, optionally a .env file)."""
import secrets
import warnings
from functools import lru_cache

from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_file=".env", env_prefix="INTEGRALL_", extra="ignore")

    database_url: str = "postgresql+asyncpg://integrall:integrall@localhost:5432/integrall"

    # Users' JWTs. MUST be set in production, otherwise every restart logs everyone out.
    jwt_secret: str = ""
    jwt_ttl_minutes: int = 60 * 24

    # Broker connection used by the backend bridge (an admin MQTT account).
    mqtt_host: str = "localhost"
    mqtt_port: int = 1883
    mqtt_tls: bool = False
    mqtt_username: str | None = None
    mqtt_password: str | None = None
    mqtt_prefix: str = "integrall"
    # Provision per-device broker logins through Mosquitto's dynamic-security plugin.
    dynsec_enabled: bool = True

    # Commands not acknowledged within this time are marked "expired".
    command_timeout_s: int = 30

    # Dashboard origins allowed to call the API (comma separated).
    cors_origins: str = "http://localhost:5173"

    # Largest telemetry/event payload accepted from a device.
    max_payload_bytes: int = 4096

    # --- OpenRemote (optional UI). Enabled when or_url is set. ---------------
    # A *service user* created in the OpenRemote Manager UI (Users > Service users).
    or_url: str = ""                    # e.g. https://localhost
    or_realm: str = "master"
    or_client_id: str = ""              # service user name
    or_client_secret: str = ""
    or_mqtt_host: str = ""              # default: host of or_url
    or_mqtt_port: int = 8883
    or_mqtt_tls: bool = True
    or_verify_tls: bool = True          # False for the quick-start's self-signed localhost cert
    or_parent_id: str = ""              # optional parent asset for all Integrall devices

    @property
    def or_enabled(self) -> bool:
        return bool(self.or_url and self.or_client_id and self.or_client_secret)

    @property
    def cors_list(self) -> list[str]:
        return [o.strip() for o in self.cors_origins.split(",") if o.strip()]


@lru_cache
def get_settings() -> Settings:
    s = Settings()
    if not s.jwt_secret:
        warnings.warn("INTEGRALL_JWT_SECRET not set: using a random one (sessions reset on restart)")
        s.jwt_secret = secrets.token_urlsafe(48)
    return s
