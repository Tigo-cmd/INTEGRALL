"""device -> OpenRemote asset link

Revision ID: 0002
Revises: 0001
"""
import sqlalchemy as sa
from alembic import op

revision = "0002"
down_revision = "0001"


def upgrade() -> None:
    op.add_column("devices", sa.Column("or_asset_id", sa.String(22), nullable=True))


def downgrade() -> None:
    op.drop_column("devices", "or_asset_id")
