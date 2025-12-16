import { useEffect, useState } from "react";
import { getDeviceStatus } from "../api/mockApi";

export default function DeviceStatus() {
  const [status, setStatus] = useState(null);

  useEffect(() => {
    getDeviceStatus().then(setStatus);
  }, []);

  if (!status) return <div>Loading...</div>;

  return (
    <div className="bg-gray-800 p-5 rounded-lg border border-green-700">
      <h3 className="text-lg text-green-400 mb-3">Device Status</h3>

      <p>Status: {status.online ? "🟢 Online" : "🔴 Offline"}</p>
      <p>Last Seen: {status.lastSeen}</p>

      {!status.wifiConfirmed && (
        <button className="mt-4 px-4 py-2 bg-green-600 rounded">
          Confirm Same Wi-Fi Network
        </button>
      )}
    </div>
  );
}
