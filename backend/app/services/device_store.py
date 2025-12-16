from datetime import datetime

DEVICES = {
    "esp32_01": {
        "online": True,
        "last_seen": datetime.utcnow(),
        "wifi_confirmed": False,
        "pins": {}
    }
}
