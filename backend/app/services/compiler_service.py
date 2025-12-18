import subprocess
import uuid
from pathlib import Path

from app.services.firmware_store import FIRMWARE_BUILDS

ARDUINO = "arduino-cli"
BUILD_DIR = Path("build/temp")
BIN_DIR = Path("firmware_bins")

LIBRARIES_PATH = "E:/ardinoide/libraries"  # ArduinoJson path


def compile_code(source_code: str):
    build_id = str(uuid.uuid4())

    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    BIN_DIR.mkdir(parents=True, exist_ok=True)

    sketch_dir = BUILD_DIR / build_id
    sketch_dir.mkdir(parents=True, exist_ok=True)

    ino_path = sketch_dir / f"{build_id}.ino"
    ino_path.write_text(source_code, encoding="utf-8")

    cmd = [
        ARDUINO,
        "compile",
        "--fqbn", "esp32:esp32:esp32",
        "--libraries", LIBRARIES_PATH,
        "--output-dir", str(BIN_DIR),
        str(sketch_dir)
    ]

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=1800
        )
    except subprocess.TimeoutExpired:
        return {"success": False, "logs": "Compilation timed out"}

    if result.returncode != 0:
        return {"success": False, "logs": result.stderr}

    # ==================================================
    # 🔑 CORRECT OTA FIRMWARE SELECTION
    # ==================================================

    app_bins = [
        f for f in BIN_DIR.glob("*.ino.bin")
        if not any(x in f.name for x in ["bootloader", "partitions", "merged"])
    ]

    if not app_bins:
        return {
            "success": False,
            "logs": "Application firmware (.ino.bin) not found for OTA"
        }

    firmware_bin = max(app_bins, key=lambda f: f.stat().st_size)

    # Register firmware for OTA
    FIRMWARE_BUILDS[build_id] = firmware_bin

    print("[COMPILE] Firmware registered")
    print("  build_id :", build_id)
    print("  bin path :", firmware_bin)
    print("  size     :", firmware_bin.stat().st_size, "bytes")

    return {
        "success": True,
        "build_id": build_id,
        "logs": result.stdout
    }
