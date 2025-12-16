import subprocess
import uuid
import os
from pathlib import Path
from dotenv import load_dotenv

# Load environment variables
load_dotenv()

# ----------------------------
# Configuration
# ----------------------------
ARDUINO = os.getenv("ARDUINO_CLI_PATH")

BASE_DIR = Path(__file__).resolve().parent.parent.parent
TEMPLATE_DIR = BASE_DIR / "firmware_templates"
BUILD_DIR = BASE_DIR / "build" / "temp"
BIN_DIR = BASE_DIR / "firmware_bins"

# Ensure directories exist
BUILD_DIR.mkdir(parents=True, exist_ok=True)
BIN_DIR.mkdir(parents=True, exist_ok=True)


def compile_code(user_code: str):
    """
    Compiles user Arduino code using Arduino CLI and ESP32 core.
    Returns build_id, logs, and success status.
    """

    # ----------------------------
    # Sanity checks
    # ----------------------------
    if not ARDUINO:
        return {
            "success": False,
            "logs": "ARDUINO_CLI_PATH not set in environment variables"
        }

    if not os.path.exists(ARDUINO):
        return {
            "success": False,
            "logs": f"Arduino CLI not found at: {ARDUINO}"
        }

    # ----------------------------
    # Create build workspace
    # ----------------------------
    build_id = str(uuid.uuid4())
    sketch_dir = BUILD_DIR / build_id
    sketch_dir.mkdir(parents=True, exist_ok=True)

    # ----------------------------
    # Load firmware template
    # ----------------------------
    template_path = TEMPLATE_DIR / "base_template.ino"

    if not template_path.exists():
        return {
            "success": False,
            "logs": "Firmware template not found"
        }

    template = template_path.read_text()

    # Inject user code
    sketch_code = template.replace("{{ USER_CODE }}", user_code)

    # Arduino requires filename == folder name
    sketch_file = sketch_dir / f"{build_id}.ino"
    sketch_file.write_text(sketch_code)

    # ----------------------------
    # Arduino CLI compile command
    # ----------------------------
    cmd = [
        ARDUINO,
        "compile",
        "--fqbn",
        "esp32:esp32:esp32dev",
        "--output-dir",
        str(BIN_DIR),
        str(sketch_dir)
    ]

    # ----------------------------
    # Run compiler
    # ----------------------------
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=60
        )
    except subprocess.TimeoutExpired:
        return {
            "success": False,
            "logs": "Compilation timed out"
        }
    except Exception as e:
        return {
            "success": False,
            "logs": str(e)
        }

    # ----------------------------
    # Handle result
    # ----------------------------
    if result.returncode != 0:
        return {
            "success": False,
            "logs": result.stderr
        }

    return {
        "success": True,
        "build_id": build_id,
        "logs": result.stdout
    }
