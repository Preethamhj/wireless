from fastapi import APIRouter
from app.models.schemas import CompileRequest
from app.services.compiler_service import compile_code
from app.services.firmware_store import FIRMWARE_BUILDS 


router = APIRouter()

@router.post("/compile")
def compile_firmware(req: CompileRequest):
    result = compile_code(req.code)
    return {
        "success": result.get("success"),
        "build_id": result.get("build_id"),
        "logs": result.get("logs")
    }
