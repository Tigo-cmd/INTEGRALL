#!/bin/sh
# First start: create the dynamic-security database with the backend's admin account.
set -e
DB=/mosquitto/data/dynamic-security.json
if [ ! -f "$DB" ]; then
  : "${MQTT_ADMIN_USER:?set MQTT_ADMIN_USER}"
  : "${MQTT_ADMIN_PASSWORD:?set MQTT_ADMIN_PASSWORD}"
  mosquitto_ctrl dynsec init "$DB" "$MQTT_ADMIN_USER" "$MQTT_ADMIN_PASSWORD"
  chown mosquitto:mosquitto "$DB"
fi
exec /usr/sbin/mosquitto -c /mosquitto/config/mosquitto.conf
