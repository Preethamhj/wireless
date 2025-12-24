from fastapi import APIRouter, HTTPException
from fastapi.responses import StreamingResponse, FileResponse
from pathlib import Path
from datetime import datetime

from app.models.schemas import OTAAssignRequest
from app.services.firmware_store import (
    OTA_ASSIGNMENTS,
    FIRMWARE_BUILDS,
    OTA_EVENTS,
    save_store,
    _STORE
)

router = APIRouter()

# =====================================================
# FALLBACK EVENT (ESP32 → Backend)
# =====================================================

@router.post("/fallback-event")
def ota_fallback_event(payload: dict):
    event = {
        "device_id": payload.get("device_id"),
        "build_id": payload.get("build_id"),
        "status": payload.get("status"),      # started / success / failed
        "reason": payload.get("reason"),
        "timestamp": datetime.utcnow().isoformat()
    }

    OTA_EVENTS.append(event)
    save_store(_STORE)

    print(
        f"[OTA-FALLBACK] device={event['device_id']} "
        f"build={event['build_id']} "
        f"status={event['status']} "
        f"reason={event['reason']}"
    )

    return {"status": "logged"}

# =====================================================
# ASSIGN OTA (ADMIN / UI)
# =====================================================

@router.post("/assign")
def assign_ota(req: OTAAssignRequest):
    OTA_ASSIGNMENTS[req.device_id] = {
        "build_id": req.build_id,
        "status": "pending",
        "assigned_at": datetime.utcnow().isoformat()
    }

    save_store(_STORE)

    return {
        "status": "assigned",
        "device_id": req.device_id,
        "build_id": req.build_id
    }

# =====================================================
# OTA CHECK (ESP32 POLLING ENDPOINT)
# =====================================================

@router.get("/check")
def ota_check(device_id: str):
    entry = OTA_ASSIGNMENTS.get(device_id)

    # 🛡️ HARD SAFETY GUARD
    if not entry or not isinstance(entry, dict):
        return {"update": False}

    if entry.get("status") != "pending":
        return {"update": False}

    # 🔑 ONLY RETURN STRING BUILD_ID
    return {
        "update": True,
        "build_id": entry["build_id"]
    }

# =====================================================
# OTA DOWNLOAD (PULL MODE)
# =====================================================

@router.get("/download/{build_id}")
def download_firmware(build_id: str):
    if build_id not in FIRMWARE_BUILDS:
        raise HTTPException(status_code=404, detail="Firmware not found")

    firmware_path: Path = FIRMWARE_BUILDS[build_id]

    if not firmware_path.exists():
        raise HTTPException(status_code=404, detail="Firmware file missing")

    return FileResponse(
        path=firmware_path,
        media_type="application/octet-stream",
        filename=firmware_path.name
    )

# =====================================================
# OTA PUSH (FALLBACK MODE)
# =====================================================

@router.post("/push/{device_id}")
def push_firmware(device_id: str):
    entry = OTA_ASSIGNMENTS.get(device_id)

    if not entry or not isinstance(entry, dict):
        raise HTTPException(status_code=404, detail="No OTA assigned")

    build_id = entry["build_id"]

    if build_id not in FIRMWARE_BUILDS:
        raise HTTPException(status_code=404, detail="Firmware build not found")

    firmware_path: Path = FIRMWARE_BUILDS[build_id]

    if not firmware_path.exists():
        raise HTTPException(status_code=404, detail="Firmware file missing")

    def firmware_stream():
        with open(firmware_path, "rb") as f:
            while True:
                chunk = f.read(4096)
                if not chunk:
                    break
                yield chunk

        # ✅ MARK OTA AS COMPLETED AFTER STREAM FINISH
        OTA_ASSIGNMENTS[device_id]["status"] = "completed"
        OTA_ASSIGNMENTS[device_id]["completed_at"] = datetime.utcnow().isoformat()
        save_store(_STORE)

    return StreamingResponse(
        firmware_stream(),
        media_type="application/octet-stream",
        headers={
            "Content-Length": str(firmware_path.stat().st_size),
            "X-Firmware-Build": build_id
        }
    )

# =====================================================
# OTA EVENTS (DEBUG / DASHBOARD)
# =====================================================

@router.get("/events")
def get_ota_events():
    return OTA_EVENTS[-50:]
