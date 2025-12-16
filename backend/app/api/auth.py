from fastapi import APIRouter

router = APIRouter()

@router.post("/login")
def login():
    return {
        "token": "mock-token",
        "user": "demo_user"
    }
