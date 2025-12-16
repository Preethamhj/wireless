from fastapi import APIRouter, HTTPException
from app.services.device_store import DEVICES
from app.services.twin_engine import compute_twin

router = APIRouter()

@router.get("/{device_id}")
def digital_twin(device_id: str):
    device = DEVICES.get(device_id)
    if not device:
        raise HTTPException(404, "Device not found")

    return compute_twin(device)
