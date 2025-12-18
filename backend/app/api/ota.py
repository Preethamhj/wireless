from fastapi import APIRouter, HTTPException
from app.services.firmware_store import OTA_ASSIGNMENTS
from app.models.schemas import OTAAssignRequest
from app.services.firmware_store import FIRMWARE_BUILDS
from fastapi.responses import FileResponse

router = APIRouter()

@router.post("/assign")
def assign_ota(req: OTAAssignRequest):
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
    """
    ESP32 calls this endpoint to download the compiled firmware (.bin)
    """

    # 1️⃣ Check if build_id exists
    if build_id not in FIRMWARE_BUILDS:
        raise HTTPException(
            status_code=404,
            detail="Firmware not found for this build_id"
        )

    firmware_path = FIRMWARE_BUILDS[build_id]

    # 2️⃣ Check file exists on disk
    if not firmware_path.exists():
        raise HTTPException(
            status_code=404,
            detail="Firmware file missing on disk"
        )

    # 3️⃣ Send firmware binary
    return FileResponse(
        path=firmware_path,
        media_type="application/octet-stream",
        filename=firmware_path.name
    )