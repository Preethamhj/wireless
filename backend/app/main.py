from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI(title="ESP32 Digital Twin Backend")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

from app.api import auth, devices, telemetry, firmware, ota, digital_twin

app.include_router(auth.router, prefix="/auth")
app.include_router(devices.router, prefix="/devices")
app.include_router(telemetry.router, prefix="/telemetry")
app.include_router(firmware.router, prefix="/firmware")
app.include_router(ota.router, prefix="/ota")
app.include_router(digital_twin.router, prefix="/digital-twin")

@app.get("/")
def health():
    return {"status": "running"}
