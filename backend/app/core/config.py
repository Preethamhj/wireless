import os
from dotenv import load_dotenv

load_dotenv()

APP_NAME = os.getenv("APP_NAME", "ESP32 Digital Twin Backend")
ENV = os.getenv("APP_ENV", "development")
