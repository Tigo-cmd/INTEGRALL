"""initial schema

Revision ID: 0001
Revises:
"""
import sqlalchemy as sa
from alembic import op
from sqlalchemy.dialects.postgresql import JSONB

revision = "0001"
down_revision = None

Json = sa.JSON().with_variant(JSONB(), "postgresql")


def upgrade() -> None:
    op.create_table(
        "users",
        sa.Column("id", sa.Integer, primary_key=True),
        sa.Column("email", sa.String(255), nullable=False),
        sa.Column("password_hash", sa.String(100), nullable=False),
        sa.Column("created_at", sa.DateTime(timezone=True), nullable=False),
    )
    op.create_index("ix_users_email", "users", ["email"], unique=True)
    op.create_table(
        "devices",
        sa.Column("id", sa.String(32), primary_key=True),
        sa.Column("owner_id", sa.Integer, sa.ForeignKey("users.id", ondelete="CASCADE"), nullable=False),
        sa.Column("name", sa.String(80), nullable=False),
        sa.Column("online", sa.Boolean, nullable=False),
        sa.Column("last_seen", sa.DateTime(timezone=True)),
        sa.Column("meta", Json),
        sa.Column("health", Json),
        sa.Column("last_telemetry", Json),
        sa.Column("created_at", sa.DateTime(timezone=True), nullable=False),
    )
    op.create_index("ix_devices_owner_id", "devices", ["owner_id"])
    op.create_table(
        "telemetry",
        sa.Column("id", sa.Integer, primary_key=True),
        sa.Column("device_id", sa.String(32), sa.ForeignKey("devices.id", ondelete="CASCADE"), nullable=False),
        sa.Column("ts", sa.DateTime(timezone=True), nullable=False),
        sa.Column("data", Json, nullable=False),
    )
    op.create_index("ix_telemetry_device_ts", "telemetry", ["device_id", "ts"])
    op.create_table(
        "events",
        sa.Column("id", sa.Integer, primary_key=True),
        sa.Column("device_id", sa.String(32), sa.ForeignKey("devices.id", ondelete="CASCADE"), nullable=False),
        sa.Column("ts", sa.DateTime(timezone=True), nullable=False),
        sa.Column("type", sa.String(64), nullable=False),
        sa.Column("detail", sa.Text, nullable=False),
    )
    op.create_index("ix_events_device_ts", "events", ["device_id", "ts"])
    op.create_table(
        "commands",
        sa.Column("id", sa.String(36), primary_key=True),
        sa.Column("device_id", sa.String(32), sa.ForeignKey("devices.id", ondelete="CASCADE"), nullable=False),
        sa.Column("action", sa.String(64), nullable=False),
        sa.Column("args", Json, nullable=False),
        sa.Column("status", sa.String(16), nullable=False),
        sa.Column("message", sa.Text),
        sa.Column("result", Json),
        sa.Column("created_at", sa.DateTime(timezone=True), nullable=False),
        sa.Column("completed_at", sa.DateTime(timezone=True)),
    )
    op.create_index("ix_commands_device_id", "commands", ["device_id"])


def downgrade() -> None:
    for t in ("commands", "events", "telemetry", "devices", "users"):
        op.drop_table(t)
