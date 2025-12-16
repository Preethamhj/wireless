const BASE = import.meta.env.VITE_API_BASE;

export async function getDeviceStatus(deviceId) {
  const res = await fetch(`${BASE}/devices/${deviceId}/status`);
  return res.json();
}

export async function compileFirmware(code) {
  const res = await fetch(`${BASE}/firmware/compile`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ code })
  });
  return res.json();
}

export async function assignOTA(deviceId, buildId) {
  const res = await fetch(`${BASE}/ota/assign`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ device_id: deviceId, build_id: buildId })
  });
  return res.json();
}
    