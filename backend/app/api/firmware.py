from fastapi import APIRouter
from app.models.schemas import CompileRequest
from app.services.compiler_service import compile_code

router = APIRouter()

@router.post("/compile")
def compile_firmware(req: CompileRequest):
    return compile_code(req.code)
