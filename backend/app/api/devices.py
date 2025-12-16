from fastapi import APIRouter, HTTPException
from app.services.device_store import DEVICES

router = APIRouter()

@router.get("/")
def list_devices():
    return DEVICES

@router.get("/{device_id}/status")
def device_status(device_id: str):
    device = DEVICES.get(device_id)
    if not device:
        raise HTTPException(404, "Device not found")
    return device

@router.post("/{device_id}/confirm-network")
def confirm_network(device_id: str):
    if device_id not in DEVICES:
        raise HTTPException(404, "Device not found")
    DEVICES[device_id]["wifi_confirmed"] = True
    return {"status": "confirmed"}
