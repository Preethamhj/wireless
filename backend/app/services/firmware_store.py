import json
from pathlib import Path
from datetime import datetime

STORE_FILE = Path("ota_store.json")

# =====================================================
# LOAD STORE
# =====================================================

def load_store():
    if STORE_FILE.exists():
        raw = json.loads(STORE_FILE.read_text())

        # Convert firmware paths back to Path
        firmware = raw.get("firmware", {})
        for build_id, path in firmware.items():
            firmware[build_id] = Path(path)

        return raw

    return {
        "firmware": {},
        "assignments": {},
        "events": []
    }

# =====================================================
# SAVE STORE
# =====================================================

def save_store(data):
    cleaned = {
        "firmware": {
            build_id: str(path)
            for build_id, path in data["firmware"].items()
        },
        "assignments": data["assignments"],
        "events": data["events"]
    }

    STORE_FILE.write_text(json.dumps(cleaned, indent=2))

    # 🔥 IMPORTANT: restore runtime Paths after saving
    for build_id, path in cleaned["firmware"].items():
        data["firmware"][build_id] = Path(path)

# =====================================================
# MIGRATION (OLD ASSIGNMENTS)
# =====================================================

def migrate_assignments(assignments: dict) -> bool:
    changed = False

    for device_id, value in list(assignments.items()):
        if isinstance(value, str):
            assignments[device_id] = {
                "build_id": value,
                "status": "pending",
                "assigned_at": datetime.utcnow().isoformat()
            }
            changed = True

    return changed

# =====================================================
# INIT STORE
# =====================================================

_STORE = load_store()

if migrate_assignments(_STORE["assignments"]):
    save_store(_STORE)

FIRMWARE_BUILDS = _STORE["firmware"]
OTA_ASSIGNMENTS = _STORE["assignments"]
OTA_EVENTS = _STORE["events"]
