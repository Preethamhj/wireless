from fastapi import APIRouter, HTTPException
from fastapi.responses import StreamingResponse
from pathlib import Path

from app.services.firmware_store import OTA_ASSIGNMENTS, FIRMWARE_BUILDS

router = APIRouter()
from datetime import datetime
from app.services.firmware_store import OTA_EVENTS

@router.post("/fallback-event")
def ota_fallback_event(payload: dict):
    """
    ESP32 notifies backend that fallback OTA was triggered
    """

    event = {
        "device_id": payload.get("device_id"),
        "build_id": payload.get("build_id"),
        "reason": payload.get("reason"),
        "status": payload.get("status"),  # started / success / failed
        "timestamp": datetime.utcnow().isoformat()
    }

    OTA_EVENTS.append(event)

    print(
        f"[OTA-FALLBACK] device={event['device_id']} "
        f"build={event['build_id']} "
        f"status={event['status']} "
        f"reason={event['reason']}"
    )

    return {"status": "logged"}


# =====================================================
# EXISTING ENDPOINTS (UNCHANGED)
# =====================================================

@router.post("/assign")
def assign_ota(req):
    OTA_ASSIGNMENTS[req.device_id] = req.build_id
    return {"status": "assigned"}

@router.get("/check")
def check_ota(device_id: str):
    if device_id not in OTA_ASSIGNMENTS:
        return {"update": False}

    return {
        "update": True,
        "build_id": OTA_ASSIGNMENTS[device_id]
    }

@router.get("/download/{build_id}")
def download_firmware(build_id: str):
    if build_id not in FIRMWARE_BUILDS:
        raise HTTPException(status_code=404, detail="Firmware not found")

    firmware_path = FIRMWARE_BUILDS[build_id]

    if not firmware_path.exists():
        raise HTTPException(status_code=404, detail="Firmware file missing")

    return FileResponse(
        path=firmware_path,
        media_type="application/octet-stream",
        filename=firmware_path.name
    )

# =====================================================
# 🆕 PUSH OTA ENDPOINT (FALLBACK MODE)
# =====================================================

@router.post("/push/{device_id}")
def push_firmware(device_id: str):
    """
    ESP32 calls this endpoint when normal OTA download fails.
    Backend responds by STREAMING the firmware binary.
    """

    # 1️⃣ Check if device has OTA assigned
    if device_id not in OTA_ASSIGNMENTS:
        raise HTTPException(
            status_code=404,
            detail="No OTA assigned for this device"
        )

    build_id = OTA_ASSIGNMENTS[device_id]

    # 2️⃣ Validate build_id
    if build_id not in FIRMWARE_BUILDS:
        raise HTTPException(
            status_code=404,
            detail="Firmware build not found"
        )

    firmware_path: Path = FIRMWARE_BUILDS[build_id]

    # 3️⃣ Ensure file exists
    if not firmware_path.exists():
        raise HTTPException(
            status_code=404,
            detail="Firmware file missing on disk"
        )

    # 4️⃣ Generator to stream binary in chunks
    def firmware_stream():
        with open(firmware_path, "rb") as f:
            while chunk := f.read(4096):
                yield chunk

    # 5️⃣ Stream firmware as raw binary
    return StreamingResponse(
        firmware_stream(),
        media_type="application/octet-stream",
        headers={
            "Content-Length": str(firmware_path.stat().st_size),
            "X-Firmware-Build": build_id
        }
    )
@router.get("/events")
def get_ota_events():
    return OTA_EVENTS[-50:]  # last 50 events
