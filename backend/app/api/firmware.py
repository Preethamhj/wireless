from fastapi import APIRouter
from app.models.schemas import CompileRequest
from app.services.compiler_service import compile_code
from app.services.firmware_store import FIRMWARE_BUILDS 


router = APIRouter()

@router.post("/compile")
def compile_firmware(req: CompileRequest):

    return compile_code(req.code)
