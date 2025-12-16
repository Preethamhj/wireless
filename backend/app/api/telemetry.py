from fastapi import APIRouter, HTTPException
from datetime import datetime
from app.services.device_store import DEVICES
from app.models.schemas import TelemetryPayload

router = APIRouter()

@router.post("/{device_id}")
def receive_telemetry(device_id: str, payload: TelemetryPayload):
    if device_id not in DEVICES:
        raise HTTPException(404, "Device not found")

    DEVICES[device_id]["pins"] = payload.pins
    DEVICES[device_id]["last_seen"] = datetime.utcnow()
    DEVICES[device_id]["online"] = True

    return {"status": "telemetry updated"}
