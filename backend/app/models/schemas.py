from pydantic import BaseModel
from typing import Dict

class TelemetryPayload(BaseModel):
    pins: Dict[int, int]

class CompileRequest(BaseModel):
    code: str

class OTAAssignRequest(BaseModel):
    device_id: str
    build_id: str
