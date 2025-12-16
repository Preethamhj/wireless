def compute_twin(device):
    pins = device.get("pins", {})
    warnings = []

    for pin, value in pins.items():
        if value not in [0, 1]:
            warnings.append(f"Pin {pin} abnormal value")

    return {
        "pins": pins,
        "warnings": warnings
    }
