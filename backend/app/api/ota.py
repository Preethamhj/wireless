from fastapi import APIRouter, HTTPException
from app.services.firmware_store import OTA_ASSIGNMENTS
from app.models.schemas import OTAAssignRequest

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
